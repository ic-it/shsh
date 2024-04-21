#include "exec.h"
#include "lexer.h"
#include "log.h"
#include "panic.h"
#include "parser.h"
#include "repl.h"
#include "server.h"
#include "types.h"
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef SHSH_VERSION
#define SHSH_VERSION "~dev"
#endif

typedef struct {
  char *script_file;
  bool show_help;
  bool show_about;
  bool is_server;
  bool is_client;
  int port;
  char *host;
  bool verbose;
  bool is_daemon;
  int connection_timeout;
  char *log_file;
} shshargs;

const char *help_message =
    "Usage: shsh [options] [script]\n"
    "Options:\n"
    "  -h\t\tShow this help message\n"
    "  -s\t\tStart a server\n"
    "  -c\t\tStart a client\n"
    "  -p PORT\tPort number\n"
    "  -i HOST\tHost name\n"
    "  -v\t\tVerbose mode\n"
    "  -d\t\tDaemon mode\n"
    "  -t TIMEOUT\tConnection timeout\n"
    "  -l LOGFILE\tLog file\n"
    "  -a\t\tShow about message\n"
    "\n"
    "If no script is provided, the program will start in REPL mode\n";

const char *about_message = "shsh - a simple non-POSIX shell\n"
                            "Version: " SHSH_VERSION "\n"
                            "Author: Illia Chaban <ic-it@mail.com>\n";

shshargs shsh_parse_args(int argc, char *argv[]) {
  shshargs args = {
      .script_file = NULL,
      .show_help = false,
      .is_server = false,
      .is_client = false,
      .port = 0,
      .host = NULL,
      .verbose = false,
      .is_daemon = false,
      .connection_timeout = 0,
      .log_file = NULL,
  };

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      args.show_help = true;
    } else if (strcmp(argv[i], "-a") == 0) {
      args.show_about = true;
    } else if (strcmp(argv[i], "-s") == 0) {
      args.is_server = true;
    } else if (strcmp(argv[i], "-c") == 0) {
      args.is_client = true;
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      args.port = atoi(argv[i + 1]);
      i++; // skip next argument
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      args.host = argv[i + 1];
      i++; // skip next argument
    } else if (strcmp(argv[i], "-v") == 0) {
      args.verbose = true;
    } else if (strcmp(argv[i], "-d") == 0) {
      args.is_daemon = true;
    } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      args.connection_timeout = atoi(argv[i + 1]);
      i++; // skip next argument
    } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
      args.log_file = argv[i + 1];
      i++; // skip next argument
    } else {
      int fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
        log_error("Failed to open file %s\n", argv[i]);
        continue;
      }
      args.script_file = argv[i];
      close(fd);
    }
  }

  return args;
}

// Simple Console
int main(int argc, char *argv[]) {
  shshargs args = shsh_parse_args(argc, argv);
  if (args.show_help) {
    printf("%s", help_message);
    return 0;
  }

  if (args.show_about) {
    printf("%s", about_message);
    return 0;
  }

  if (!args.is_server && !args.is_client) {
    FILE *file = NULL;
    if (args.script_file != NULL) {
      file = fopen(args.script_file, "r");
    }
    int status = shsh_repl((shsh_repl_ctx){.in = file});
    if (file != NULL) {
      fclose(file);
    }
    return status;
  }

  if (args.is_server) {
    return rshsh_server((rshsh_server_ctx){
        .host = args.host,
        .port = args.port,
    });
  }
}
