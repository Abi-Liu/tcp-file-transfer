#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 20

int get_listener_socket() {
  struct addrinfo hints, *res, *p;
  int listener;
  int yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; // use tcp sockets
  hints.ai_family = AF_UNSPEC;     // use either ipv4 or ipv6
  hints.ai_flags = AI_PASSIVE;     // get and use my ip address

  int ai_status = getaddrinfo(NULL, PORT, &hints, &res);
  if (ai_status != 0) {
    // error has occurred uh oh!
    gai_strerror(ai_status);
    exit(1);
  }

  // successfully obtained addrinfo, now we need to create a listener socket
  // and find a valid port to bind to
  for (p = res; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener == -1) {
      continue;
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if(bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
      // error binding, close the listener socket and continue to next node
      close(listener);
      continue;
    }

    if(listen(listener, BACKLOG) == -1) {
      // error listening, close socket and continue
      close(listener);
      continue;
    }

    // if we make it this far we have successfully bound the listening socket to a port
    break;
  }

  freeaddrinfo(res);

  if(p == NULL) {
    // this means we failed to bind. shut down server
    printf("Failed to bind to a port\nShutting down...");
    exit(2);
  }

  return listener;
}

int main() {
  int listener_fd = get_listener_socket();
}
