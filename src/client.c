#include "client.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024 * 3

int rshsh_client(rshsh_client_ctx ctx) {
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

  // Send data to server
  char message[BUFFER_SIZE];
  printf("Enter message to send: ");
  fgets(message, BUFFER_SIZE, stdin);

  // Send message to server
  if (send(client_socket, message, strlen(message), 0) == -1) {
    perror("Error: Unable to send message to server");
    close(client_socket);
    return -1;
  }

  // Receive response from server
  char buffer[BUFFER_SIZE];

  // Receive response from server
  while (recv(client_socket, buffer, BUFFER_SIZE, 0) > 0) {
    printf("%s", buffer);
    memset(buffer, 0, BUFFER_SIZE);
  }

  // Close socket
  close(client_socket);

  return 0;
}
