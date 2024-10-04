#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 20
#define HEADER_LENGTH 36
// header length for client
#define HEADER_LENGTH_C 31

int parse_file(char *filename);
int pack_header(char *filename, uint8_t *file, int size);
void send_message(char *message);
void send_full_response(char *filename, char *message, int size);


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

    if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
      // error binding, close the listener socket and continue to next node
      close(listener);
      continue;
    }

    if (listen(listener, BACKLOG) == -1) {
      // error listening, close socket and continue
      close(listener);
      continue;
    }

    // if we make it this far we have successfully bound the listening socket to
    // a port
    break;
  }

  freeaddrinfo(res);

  if (p == NULL) {
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
 * 35-39 total size of the packet
 */

// returns -1 if an error occurs, this signifies that something went wrong and
// close the connection stores the data inside the pointers passed to the
// function
int unpack_header(int fd, char *req_type, char *filename, int *size) {
  char *buf = malloc(sizeof(uint8_t) * HEADER_LENGTH);
  int bytes_read = 0;

  while (bytes_read < HEADER_LENGTH) {
    int len = recv(fd, buf, HEADER_LENGTH - bytes_read, 0);
    if (len <= 0) {
      if (len == -1) {
        printf("error occurred when reading \n");
      } else {
        printf("client closed connection\n");
      }
      return -1;
    }

    bytes_read += len;
  }

  // now that we have read the header into the buffer
  // we can focus on copying the data into the pointers passed in as arguments
  // req type
  for (int i = 0; i < 5; i++) {
    if (buf[i] == '\0') {
      req_type[i] = '\0';
      break;
    }
    req_type[i] = buf[i];
  }

  // filename
  // for now I am just assuming that the filename will be less than 30chars
  // TODO: add error handling for this case in the future
  for (int i = 5, j = 0; i < 35; i++) {
    if (buf[i] == '\0') {
      filename[j] = '\0';
      break;
    }

    filename[j] = buf[i];
  }

  *size = buf[HEADER_LENGTH - 1];

  return 0;
}

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
  while (true) {
    read_set = master_set;
    if (select(numfds + 1, &read_set, NULL, NULL, NULL) == -1) {
      printf("Select error\n");
      return 1;
    }

    for (int i = 0; i < numfds + 1; i++) {
      if (FD_ISSET(i, &read_set)) {
        // found a socket ready to read
        if (i == listener_fd) {
          // there is a connection waiting to be accepted
          newfd =
              accept(i, (struct sockaddr *)&their_addr, &their_addr_size) == 1;
          if (newfd == -1) {
            printf("error accepting connection\n");
            continue;
          }

          FD_SET(newfd, &master_set);
          if (newfd > numfds) {
            numfds = newfd;
          }
          printf("New connection added\n");
        } else {
          // new request from a client!! time to transfer some files
          // first we read the header
          char req_type[5], filename[30];
          int *size;

          if (unpack_header(i, req_type, filename, size) == -1) {
            printf("connection unstable\n");
            close(i);
            FD_CLR(i, &master_set);
            continue;
          }

          if (strcmp(req_type, "get") == 0) {
            // get request, aka server sends file to client

          } else if (strcmp(req_type, "post") == 0) {
            // post request, client uploading file to store in server
          } else {
            printf("unknown request type\n");
            // send a message before closing the connection
            // first we have to write the header packet to the client
            close(i);
            FD_CLR(i, &master_set);
            continue;
          }
        }
      }
    }
  }
}

int parse_file(char *filename) {
  FILE *file = fopen(filename, "rb");

  if (file == NULL) {
    // file does not exist, return -1 to signify error
    // no need to close since file does not exist / was not opened
    return -1;
  }

  // because we don't know the length of the file, we need to dynamically
  // reallocate the buffer as we read the file
  int size = 0;
  int capacity = 10;
  uint8_t *buf = malloc(sizeof(uint8_t) * capacity);

  if (buf == NULL) {
    return -1;
  }

  while (fread(buf, sizeof(uint8_t), 1, file) > 0) {
    size++;
    if (size >= capacity) {
      capacity *= 2;
      uint8_t *tmp = realloc(buf, capacity);

      if (tmp == NULL) {
        printf("error reallocating buffer\n");
        free(buf);
        return -1;
      }

      buf = tmp;
    }
  }

  // reallocate the buffer to be the exact size we need
  uint8_t *tmp = realloc(buf, sizeof(uint8_t) * size);
  if (tmp == NULL) {
    printf("error reallocating buffer\n");
    free(buf);
    return -1;
  }

  buf = tmp;
  fclose(file);
  if (pack_header(filename, buf, size) == -1) {
    return -1;
  }
}

int pack_header(char *filename, uint8_t *file, int size) {}

void send_message(char *message) {}

void send_full_response(char *filename, char *message, int size) {}
