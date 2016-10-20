#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */

#define MAXBUF 1024*1024

void addCount(char *bufin, char *temp, unsigned count) {
	char num[12];
	char num2[12];
	int i = 1;
	int j = 0;
	unsigned cnt = count;
	num[0] = ' ';
	while (cnt) {
		num[i] = cnt % 10 + '0';
		cnt /= 10;
		++i;
	}
	if (count == 0) {
		num[i++] = '0';
	}
	num2[i--] = '\0';
	while(i >= 0) {
		num2[j] = num[i];
		--i;
		++j;
	}
	strcpy(temp, num2);
	strcat(temp, bufin);
}

void echo(int sd) {
    int count;
    char bufin[MAXBUF];
    char temp[MAXBUF];
    struct sockaddr_in remote;

    /* need to know how big address struct is, len must be set before the
       call to recvfrom!!! */
    socklen_t len = sizeof(remote);
    count = 1;
    while (1) {
      /* read a datagram from the socket (put result in bufin) */
      int n = recvfrom(sd, bufin, MAXBUF, 0, (struct sockaddr *) &remote, &len);
      if (n < 0) {
        perror("Error receiving data");
      } else {
    	addCount(bufin, temp, count);
        /* Got something, just send it back */
        sendto(sd, temp, strlen(temp), 0, (struct sockaddr *)&remote, len);
        ++count;
		memset(bufin, 0, MAXBUF);
		memset(temp, 0, MAXBUF);
	}
    }
}

/* server main routine */

int main() {
  int ld;
  struct sockaddr_in skaddr;
  socklen_t length;
  /* create a socket
     IP protocol family (PF_INET)
     UDP protocol (SOCK_DGRAM)
  */
  if ((ld = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }
  /* establish our address
     address family is AF_INET
     our IP address is INADDR_ANY (any of our IP addresses)
     the port number is 9876
  */
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(9876);
  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
    printf("Problem binding\n");
    exit(0);
  }
  /* find out what port we were assigned and print it out */
  length = sizeof(skaddr);
  if (getsockname(ld, (struct sockaddr *) &skaddr, &length) < 0) {
    printf("Error getsockname\n");
    exit(1);
  }
  /* Go echo every datagram we get */
  echo(ld);
  return 0;
}
