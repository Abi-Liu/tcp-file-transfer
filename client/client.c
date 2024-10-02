#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define HEADER_LENGTH 40


int main() {
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int ai_status = getaddrinfo(NULL, PORT, &hints, &res);
	if(ai_status != 0) {
		gai_strerror(ai_status);
		return 1;
	}

	int sockfd;
	for(p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(sockfd == -1) {
			continue;
		}

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			continue;
		}

		break;
	}

	// socket was not created
	if(p == NULL) {
		printf("error connecting to host\n");
		return 2;
	}


}
