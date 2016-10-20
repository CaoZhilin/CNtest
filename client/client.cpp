#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include "mySocket.h"
#include "myStdlib.h"
#include "myCExecute.h"


using namespace std;

int main(int argc, char *argv[]) {
    checkArgument(argc, argv);
	int sockfd, portfd;
	struct sockaddr_in addr, pasvAddr, portAddr;
	char sentence[SENTENCE] = {'\0'};
	int len;
    int mode = GUEST;
    sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		return 1;
	}
    memset(&portAddr, 0, sizeof(portAddr));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(myAtoi(argv[2]));
    
	if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) { // it is a hostname not an ip address
        struct hostent *h;
        fd_set writefd;
        FD_SET(sockfd, &writefd);
        struct timeval tv;
        tv.tv_sec = 5;             /* 5 second timeout */
        tv.tv_usec = 0;
        long arg = fcntl(sockfd, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, arg);
        socklen_t lon;
        int valopt = 0;
        if ((h=gethostbyname(argv[1])) == NULL) {  // get the host info
            herror("gethostbyname");
            return 1;
        }
        char **pp;
        for (pp = h->h_addr_list; *pp != NULL; ++pp) { // get all ip addresses of the host, use the first one that can be connected to
            addr.sin_addr.s_addr = ((struct in_addr *)*pp)->s_addr;
            cout << "Trying " << inet_ntoa(addr.sin_addr) << "...\n";
            if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) != -1) {
                break;
            }
            else {
                if (errno == EINPROGRESS) {
                    FD_ZERO(&writefd);
                    FD_SET(sockfd, &writefd);
                    if (select(sockfd + 1, NULL, &writefd, NULL, &tv) > 0) {
                        lon = sizeof(int);
                        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
                        if (valopt) {
                            fprintf(stderr, "Error in connection() %d - %s\n", valopt, strerror(valopt));
                            return 0;
                        }
                        break;
                    }
                    else {
                        fprintf(stderr, "Timeout or error() %d - %s\n", valopt, strerror(valopt));
                        return 0;
                    }
                }
            }
            cout << "No response." << endl;
        }
        if (*pp == NULL) {
            printf("%s: cannot connect to this host\n", argv[1]);
            return 1;
        }
        arg = fcntl(sockfd, F_GETFL, NULL);
        arg ^= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, arg);
	}
    else {
        cout << "Trying " << inet_ntoa(addr.sin_addr) << "...\n";
        if (Connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            return 1;
        }
    }
    cout << "Success!" << endl;
    Recv(sockfd, sentence, SENTENCE, 0);
    printf("%s", sentence);
    while (1) {
        char cmd[100], parameter[100];
        memset(sentence, 0, SENTENCE);
        fgets(sentence, 4096, stdin);
        len = strlen(sentence);
        if (len <= 1) {
            continue;
        }
        if (sentence[len - 1] == '\n') {
            sentence[--len] = '\0';
        }
        parseCommand(sentence, cmd, parameter);
        strcat(sentence, "\r\n");
        if (strcmp(cmd, "QUIT") == 0) {
            executeCQUIT(sockfd, sentence);
        }
        else if (strcmp(cmd, "PORT") == 0) {
            executeCPORT(sockfd, parameter, sentence, &portAddr, &portfd, &mode);
        }
        else if (strcmp(cmd, "PASV") == 0) {
            executeCPASV(sockfd, sentence, &portfd, &pasvAddr, &mode);
        }
        else if (strcmp(cmd, "RETR") == 0) {
            executeCRETR(sockfd, portfd, sentence, parameter, &mode, &pasvAddr);
        }
        else if (strcmp(cmd, "STOR") == 0) {
            executeCSTOR(sockfd, portfd, sentence, parameter, &mode, &pasvAddr);
        }
        else {
            int len = strlen(sentence);
            if (Send(sockfd, sentence, len, 0) == -1) {
                return 1;
            }
            if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
                return 1;
            }
            cout << sentence;
        }
    }
	close(sockfd);
	return 0;
}
