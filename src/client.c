#include "client.h"
#include "log.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024 * 3

bool client_is_running = true;

// Client send message to server thread
void *client_send_message(void *arg) {
  int client_socket = *(int *)arg;
  char message[BUFFER_SIZE];

  while (client_is_running) {
    fgets(message, BUFFER_SIZE, stdin);
    if (strcmp(message, "exit\n") == 0) {
      client_is_running = false;
      break;
    }

    if (strlen(message) == 0) {
      continue;
    }

    if (feof(stdin)) {
      client_is_running = false;
      break;
    }

    // Send message to server
    if (send(client_socket, message, strlen(message), 0) == -1) {
      perror("Error: Unable to send message to server");
      close(client_socket);
      client_is_running = false;
    }
  }

  return NULL;
}

int rshsh_client(rshsh_client_ctx ctx) {
  log_warn("Client started. Press 'exit' to stop. USE telnet instead of this "
           "client.\n",
           NULL);
  // Create socket
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    perror("Error: Unable to create socket");
    return -1;
  }

  // Initialize server address
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(ctx.port);
  if (ctx.host == NULL) {
    server_address.sin_addr.s_addr = INADDR_ANY;
  } else {
    server_address.sin_addr.s_addr = inet_addr(ctx.host);
  }

  // Connect to server
  if (connect(client_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) == -1) {
    perror("Error: Unable to connect to server");
    close(client_socket);
    return -1;
  }

  // Create thread to send message to server
  pthread_t send_message_thread;
  pthread_create(&send_message_thread, NULL, client_send_message,
                 (void *)&client_socket);

  // Receive response from server
  char buffer[BUFFER_SIZE];
  while (client_is_running) {
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
      perror("Error: Unable to receive message from server");
      close(client_socket);
      client_is_running = false;
    } else {
      printf("%s", buffer);
    }
  }

  // Close socket
  close(client_socket);

  return 0;
}
