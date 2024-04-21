#include "server.h"
#include "exec.h"
#include "lexer.h"
#include "log.h"
#include "panic.h"
#include "parser.h"
#include "types.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define SELECT_TIMEOUT 5

void *rshsh_handle_client(void *arg);

Jobs *server_jobs;
bool server_running = true;

int *connections;
int connections_size = 0;
int connections_cap = 0;

void conn_push(int conn) {
  if (connections_size == connections_cap) {
    connections_cap = connections_cap == 0 ? 1 : connections_cap * 2;
    connections = realloc(connections, connections_cap * sizeof(int));
  }
  connections[connections_size++] = conn;
}

void conn_remove(int conn) {
  for (int i = 0; i < connections_size; i++) {
    if (connections[i] == conn) {
      for (int j = i; j < connections_size - 1; j++) {
        connections[j] = connections[j + 1];
      }
      connections_size--;
      break;
    }
  }
}

typedef struct {
  int client_fd;
  int timeout;
} ClientThreadArgs;

static void server_handle_sigchld(int sig __attribute__((unused))) {
  int status;
  pid_t pid;

  while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
    if (pid == -1) {
      log_error("Error: waitpid failed\n", NULL);
      return;
    }
    if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
      log_info("[H] Process with PID %d exited abnormally\n", pid);
      continue;
    }
    log_info("[H] Process with PID %d exited with status %d\n", pid,
             WEXITSTATUS(status));
    remove_pid(server_jobs, pid);
  }
  signal(SIGCHLD, server_handle_sigchld); // WTF? Why do we need this?
}

void server_handle_sigint(int sig __attribute__((unused))) {
  log_info("Received SIGINT\n", NULL);
  server_running = false;
}

int rshsh_server(rshsh_server_ctx ctx) {
  if (signal(SIGCHLD, server_handle_sigchld) == SIG_ERR) {
    panic("Error: Unable to catch SIGCHLD\n");
  }
  if (signal(SIGINT, server_handle_sigint) == SIG_ERR) {
    panic("Error: Unable to catch SIGINT\n");
  }
  log_info("Server mode\n", NULL);

  server_jobs = jobs_new();

  // Create a socket and bind it to the given port and host
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    panic("Error: Unable to create socket\n");
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(ctx.port);
  if (ctx.host == NULL) {
    server_addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    server_addr.sin_addr.s_addr = inet_addr(ctx.host);
  }

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    panicf("Error: Unable to bind socket to %s:%d\n", ctx.host, ctx.port);
  }

  if (listen(server_fd, 10) == -1) {
    panic("Error: Unable to listen on socket\n");
  }

  // get the port number
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(server_fd, (struct sockaddr *)&sin, &len) == -1) {
    panic("Error: Unable to get socket name\n");
  }

  log_info("Listening on %s:%d\n", inet_ntoa(sin.sin_addr),
           ntohs(sin.sin_port));

  while (server_running) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = SELECT_TIMEOUT;
    timeout.tv_usec = 0;

    int result = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result == -1) {
      log_error("Error: select() failed\n", NULL);
      continue;
    } else if (result == 0) {
      continue;
    }

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd == -1) {
      log_error("Error: Unable to accept connection\n", NULL);
      continue;
    }

    conn_push(client_fd);

    log_info("Accepted connection from %s:%d\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    pthread_t thread;
    ClientThreadArgs *cta = malloc(sizeof(ClientThreadArgs));
    cta->client_fd = client_fd;
    cta->timeout = ctx.timeout;
    if (pthread_create(&thread, NULL, rshsh_handle_client, cta) != 0) {
      close(client_fd);
      free(cta);
      conn_remove(client_fd);

      panic("Error: Unable to create thread\n");
    }
  }

  for (int i = 0; i < connections_size; i++) {
    log_info("Closing connection %d\n", connections[i]);
    close(connections[i]);
  }

  log_info("Shutting down server\n", NULL);
  close(server_fd);
  jobs_free(server_jobs);
  return 0;
}

void server_fill_prompt(char *prompt, const char *fmt);

typedef enum {
  SERVER_PHR_QUIT = 1,
  SERVER_PHR_HALT = 2,
  SERVER_PHR_HELP = 3,
} ServerPrehookResult;

int server_prehook(Command cmd) {
  char *cmd_name = slice_to_stack_str(cmd.name);
  if (strcmp(cmd_name, "quit") == 0) {
    log_debug("Client requested exit\n", NULL);
    return SERVER_PHR_QUIT;
  }
  if (strcmp(cmd_name, "halt") == 0) {
    log_debug("Client requested halt\n", NULL);
    return SERVER_PHR_HALT;
  }
  if (strcmp(cmd_name, "help") == 0) {
    log_debug("Client requested help\n", NULL);
    return SERVER_PHR_HELP;
  }
  return 0;
}

void *rshsh_handle_client(void *arg) {
  ClientThreadArgs *cta = (ClientThreadArgs *)arg;
  int client_fd = cta->client_fd;
  int timeout = cta->timeout;
  free(cta);

  int in_fd, out_fd;

  if ((in_fd = dup(client_fd)) == -1) {
    log_error("Error: Unable to duplicate file descriptor\n", NULL);
    close(client_fd);
    return NULL;
  }

  if ((out_fd = dup(client_fd)) == -1) {
    log_error("Error: Unable to duplicate file descriptor\n", NULL);
    close(client_fd);
    close(in_fd);
    return NULL;
  }

  char input[1024 * 3]; // 3KB

  Lexer lexer;
  Parser parser;
  Executor executor = executor_new(NULL, server_jobs);

  char *welcome = "                   #             #\n"
                  "             mmm   # mm    mmm   # mm\n"
                  "            #   \"  #\"  #  #   \"  #\"  #\n"
                  "             \"\"\"m  #   #   \"\"\"m  #   #\n"
                  "Welcome to  \"mmm\"  #   #  \"mmm\"  #   # by ic-it\n";

  send(client_fd, welcome, strlen(welcome), 0);

  const char *prompt_fmt = "[%s@%s:%s]-[%s]$ ";

  bool is_eof = false;
  while (is_eof == false && server_running == true) {
    char prompt[1024];
    server_fill_prompt(prompt, prompt_fmt);

    // send prompt
    send(client_fd, prompt, strlen(prompt), 0);

    memset(input, 0, sizeof(input));

    // select for timeout
    if (timeout > 0) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(client_fd, &read_fds);

      struct timeval tv;
      tv.tv_sec = timeout;
      tv.tv_usec = 0;

      int result = select(client_fd + 1, &read_fds, NULL, NULL, &tv);
      if (result == -1) {
        log_error("Error: select() failed\n", NULL);
        break;
      } else if (result == 0) {
        send(client_fd, "Connection timed out\n", 21, 0);
        log_info("Connection timed out\n", NULL);
        break;
      }
    }

    ssize_t bytes_read = recv(client_fd, input, sizeof(input), 0);

    log_info("Received: %s\n", input);

    lexer = lex_new(input);
    parser = parse_new(&lexer);
    executor.parser = &parser;

    while (1) {
      ExecResult er = exec_next(&executor, in_fd, out_fd, server_prehook);
      if (er.status == EXEC_PARSE_EOF) {
        break;
      }

      if (er.status == EXEC_PREHOOK_BREAK) {
        if (er.prehook_result == SERVER_PHR_QUIT) {
          log_info("Client requested exit\n", NULL);
          is_eof = true;
          break;
        }
        if (er.prehook_result == SERVER_PHR_HALT) {
          log_info("Client requested halt\n", NULL);
          server_running = false;
          break;
        }
        if (er.prehook_result == SERVER_PHR_HELP) {
          log_info("Client requested help\n", NULL);
          send(client_fd, "Help is on the way\n", 18, 0);
          continue;
        }
      }

      switch (er.status) {
      case EXEC_ERROR_FILE_OPEN:
        log_error_fd(out_fd, "Unable to open file\n", NULL);
        break;
      case EXEC_PARSE_ERROR:
        log_error_fd(out_fd, "Invalid Syntax\n", NULL);
        break;
      case EXEC_SEMANTIC_ERROR:
        log_error_fd(out_fd, "Semantic Error: %s\n",
                     get_semantic_reason(er.semantic_reason));
        break;
      case EXEC_PARSE_EOF:
        break;
      case EXEC_SUCCESS:
        break;
      case EXEC_IN_BACKGROUND:
        break;
      case EXEC_PIPELINE:
        break;
      case EXEC_PREHOOK_BREAK:
        break;
      }
    }
  }

  log_info("Closing connection\n", NULL);
  if (close(client_fd) == -1) {
    log_error("Error: Unable to close client socket\n", NULL);
  }
  conn_remove(client_fd);
  return NULL;
}

void server_fill_prompt(char *prompt, const char *prompt_fmt) {
  char hostname[1024];
  gethostname(hostname, sizeof(hostname));
  char username[1024];
  getlogin_r(username, sizeof(username));
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  char time_str[1024];
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
  sprintf(prompt, prompt_fmt, username, hostname, cwd, time_str);
}