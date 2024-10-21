#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define HEADER_LENGTH_S 36 // header length to send packets to server
#define HEADER_LENGTH_C 31 // header length that server sends back to client

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("usage: ./client get/post filename\n");
    return 1;
  }

  if (strlen(argv[2]) > 29 || argv[2] == NULL) {
    printf("invalid filename");
    return 2;
  }

  uint8_t *buf = malloc(sizeof(uint8_t) * HEADER_LENGTH_S);
  int index;
  if (buf == NULL) {
    printf("error allocating memory\n");
    return 3;
  }

  if (strcmp(argv[1], "get") != 0 && strcmp(argv[1], "post") != 0) {
    printf("only accepts get / post. use all lowercase\n");
    return 4;
  }

  for (int i = 0; i < strlen(argv[1]); i++) {
    buf[index] = argv[1][i];
    index++;
  }

  // adding nul padding
  while (index < 5) {
    buf[index] = '\0';
    index++;
  }

  for(int i = 0; i < strlen(argv[2]); i++) {
    buf[index] = argv[2][i];
    index++;
  }

  while(index < 35) {
    buf[index] = '\0';
    index++;
  }

  buf[HEADER_LENGTH_S-1] = 0;

  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int ai_status = getaddrinfo(NULL, PORT, &hints, &res);
  if (ai_status != 0) {
    gai_strerror(ai_status);
    return 1;
  }

  int sockfd;
  for (p = res; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      continue;
    }

    break;
  }

  // socket was not created
  if (p == NULL) {
    printf("error connecting to host\n");
    return 2;
  }

  // send over a mock packet to the server
}
