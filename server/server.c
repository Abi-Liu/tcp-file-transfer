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
#include <sys/time.h>

#define PORT "8080"
#define BACKLOG 20
#define HEADER_LENGTH 40

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

/*
* Header will be broken down like so:
* bytes 0-5: get || post. NUL padded
* 5-35 name of the file including extension
* 35-40 total size of the packet
*/

// char* unpack_header(int fd) {
//   char* buf = malloc(sizeof(char) * HEADER_LENGTH);
//   
// }

int main() {
  int listener_fd = get_listener_socket();
  struct sockaddr_storage their_addr;
  socklen_t their_addr_size;
  int newfd;


  int numfds = listener_fd;
  fd_set master_set, read_set;

  FD_ZERO(&master_set);
  FD_ZERO(&read_set);
  FD_SET(listener_fd, &master_set);


  // for now I will only handle 1 request at a time
  // I plan on expanding this to be multithreaded in the future
  while(true) { 
    read_set = master_set;
    if(select(numfds + 1, &read_set, NULL, NULL, NULL) == -1) {
      printf("Select error\n");
      return 1;
    }

    for(int i = 0; i < numfds + 1; i++) {
      if(FD_ISSET(i, &read_set)) {
        // found a socket ready to read
        if(i == listener_fd) {
          // there is a connection waiting to be accepted
          newfd = accept(i, (struct sockaddr* )&their_addr, &their_addr_size) == 1;
          if (newfd == -1) {
            printf("error accepting connection\n");
            continue;
          }

          FD_SET(newfd, &master_set);
          if(newfd > numfds) {
            numfds = newfd;
          }
          printf("New connection added\n");
        }
      } 
    }
  }

}
