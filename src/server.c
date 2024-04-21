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

    log_info("Accepted connection from %s:%d\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    pthread_t thread;
    if (pthread_create(&thread, NULL, rshsh_handle_client, &client_fd) != 0) {
      log_error("Error: Unable to create thread\n", NULL);
      close(client_fd);
    }
  }

  log_info("Shutting down server\n", NULL);
  close(server_fd);
  jobs_free(server_jobs);
  return 0;
}

void *rshsh_handle_client(void *arg) {
  int client_fd = *(int *)arg;

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

  while (1) {
    memset(input, 0, sizeof(input));
    if (recv(client_fd, input, sizeof(input), 0) == 0) {
      break;
    }

    log_info("Received: %s\n", input);

    lexer = lex_new(input);
    parser = parse_new(&lexer);
    executor.parser = &parser;

    while (1) {
      ExecResult er = exec_next(&executor, STDIN_FILENO, STDOUT_FILENO);
      if (er.status == EXEC_PARSE_EOF) {
        break;
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
      }
    }
  }

  close(client_fd);
  return NULL;
}