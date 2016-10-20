#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include "mySocket.h"

#define GUEST 0
#define PASS 1
#define LOGIN 2
#define PORT 3
#define PASV 4
#define SENTENCE 8192

using namespace std;
void executeCQUIT(int sockfd, char *sentence) {
    int len = strlen(sentence);
    if (Send(sockfd, sentence, len, 0) == -1) {
        return;
    }
    if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
        return;
    }
    cout << sentence;
    cout << "Connection closed by foreign host." << endl;
    close(sockfd);
    exit(0);
}

void executeCPORT(int sockfd, char *parameter, char *sentence, struct sockaddr_in *port, int *portfd, int *mode) {
    parseAddress(parameter, port);
    int len = strlen(sentence);
    if (Send(sockfd, sentence, len, 0) == -1) {
        return;
    }
    if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
        return;
    }
    cout << sentence;
    if (getMessageCode(sentence) == 200) {
        if (*portfd > 0) {
            close(*portfd);
        }
        *mode = PORT;
        *portfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (*portfd == -1) {
            return;
        }
        if (Bind(*portfd, (struct sockaddr*)port, sizeof(*port)) == -1) {
            return;
        }
        if (Listen(*portfd, 1) == -1) {
            return;
        }
    }
}

void executeCPASV(int sockfd, char *sentence, int *portfd, struct sockaddr_in *pasvAddr, int *mode) {
    int len = strlen(sentence);
    if (Send(sockfd, sentence, len, 0) == -1) {
        return;
    }
    if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
        return;
    }
    cout << sentence;
    if (getMessageCode(sentence) == 227) {
        if (*portfd > 0) {
            close(*portfd);
        }
        len = strlen(sentence);
        int left = 0;
        for (int i = 0; i < len; ++i) {
            if (sentence[i] == '(') {
                left = i + 1;
            }
            else if (sentence[i] == ')') {
                sentence[i] = '\0';
            }
        }
        char *str = &sentence[left];
        if (parseAddress(str, pasvAddr) == 0) {
            *mode = PASV;
        }
    }
}

void executeCRETR(int sockfd, int portfd, char *sentence, char *parameter, int *mode, struct sockaddr_in *pasvAddr) {
    int len = strlen(sentence);
    if (*mode == PASV || *mode == PORT) {
        if (Send(sockfd, sentence, len, 0) == -1) {
            return;
        }
        int datafd = -1;
        if (*mode == PORT) {
            datafd = Accept(portfd, NULL, NULL);
            if (datafd == -1) {
                return;
            }
        }
        else {
            datafd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (datafd == -1) {
                return;
            }
            if (Connect(datafd, (struct sockaddr*)pasvAddr, sizeof(*pasvAddr)) == -1) {
                return;
            }
        }
        if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
            return;
        }
        cout << sentence;
        int code = getMessageCode(sentence);
        if (code == 150) {
            char filename[256];
            getFilename(parameter, filename);
            FILE *fp = fopen(filename, "wb");
            char fileBuffer[1024];
            int n = 0;
            do {
                n = Recv(datafd, fileBuffer, sizeof(fileBuffer), 0);
                if (n == -1) {
                    close(datafd);
                    return;
                }
                int m = fwrite(fileBuffer, 1, n, fp);
                if (m < n) {
                    cout << strerror(errno) << endl;
                    close(datafd);
                    return;
                }
            } while(n != 0);
            fclose(fp);
            close(datafd);
            memset(sentence, 0, SENTENCE);
            if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
                return;
            }
            cout << sentence;
            *mode = 0;
        }
        else if (code == 550 || code == 425 || code == 426) {
            *mode = 0;
        }
    }
    else {
        if (Send(sockfd, sentence, len, 0) == -1) {
            return;
        }
        if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
            return;
        }
        cout << sentence;
    }
}

void executeCSTOR(int sockfd, int portfd, char *sentence, char *parameter, int *mode, struct sockaddr_in *pasvAddr) {
    int len = strlen(sentence);
    if (*mode == PASV || *mode == PORT) {
        struct stat fileInfo;
        int n = stat(parameter, &fileInfo);
        if (n == -1) {

        }
        FILE *fp = fopen(parameter, "rb");
        if (!(fp && S_ISREG(fileInfo.st_mode))) {
            cout << "This file cannot be transferred, may be because you don't have permission or it doesn't exist or it is a directory." << endl;
            fclose(fp);
            return;
        }
        if (Send(sockfd, sentence, len, 0) == -1) {
            return;
        }
        int datafd = -1;
        if (*mode == PORT) {
            datafd = Accept(portfd, NULL, NULL);
            if (datafd == -1) {
                fclose(fp);
                return;
            }
        }
        else {
            datafd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (datafd == -1) {
                fclose(fp);
                return;
            }
            if (Connect(datafd, (struct sockaddr*)pasvAddr, sizeof(*pasvAddr)) == -1) {
                fclose(fp);
                return;
            }
        }
        if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
            fclose(fp);
            return;
        }
        cout << sentence;
        int code = getMessageCode(sentence);
        if (code == 150) {
            char fileBuffer[1024];
            int n = 0;
            do {
                n = fread(fileBuffer, 1, sizeof(fileBuffer), fp);
                if (ferror(fp)) {

                }
                if (Send(datafd, fileBuffer, n, 0) == -1) {
                    return;
                }
            } while(n != 0);
            fclose(fp);
            close(datafd);
            memset(sentence, 0, SENTENCE);
            if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
                return;
            }
            cout << sentence;
            *mode = 0;
        }
        else if (code == 550 || code == 425 || code == 426) {
            *mode = 0;
        }
    }
    else {
        if (Send(sockfd, sentence, len, 0) == -1) {
            return;
        }
        if (Recv(sockfd, sentence, SENTENCE, 0) == -1) {
            return;
        }
        cout << sentence;
    }
}

void checkArgument(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: client ip port\n");
        exit(0);
    }
    /*struct in_addr a;
    if (inet_aton(argv[1], &a) == 0) {
        printf("the ip address is wrong\n");
        exit(0);
    }*/
    int len = strlen(argv[2]);
    int i;
    for(i = 0; i < len; ++i) {
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            printf("the port is wrong\n");
            exit(0);
        }
    }
    int n = myAtoi(argv[2]);
    if (n > 65535 || n < 1) {
        printf("the port should between 1 and 65535\n");
        exit(0);
    }
}
