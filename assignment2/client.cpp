#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream> 
#include "request.h"

void error(char *msg) {
    perror(msg);
    exit(0);
}

char* sendRequest(int& sockfd, request& req) {
    char response[100];
    int result;
    result = write(sockfd, &req, sizeof(req));
    if (result < 0) { 
        error("ERROR writing to socket");
    }
    bzero(response, 100);
    result = read(sockfd, response, 100);
    if (result < 0) {
         error("ERROR reading from socket");
    }
    return response;
}

request buildRequest(int& requestType, int& acctNum) {
    char buffer[100];
    bzero(buffer, 100);
    double amt = 0;
    request req {requestType, acctNum, amt};
    switch (requestType) {
        case 2:
            while (amt <= 0) {
                printf("Enter amount to deposit: ");
                fgets(buffer, 256, stdin);
                sscanf(buffer, "%lf", &amt);
                req.amt = amt;
                if (amt <= 0) {
                    printf("Enter amount greater than 0\n");
                }
            }
            break;
        case 3:
            while (amt <= 0) {
                printf("Enter amount to withdraw: ");
                fgets(buffer, 256, stdin);
                sscanf(buffer, "%lf", &amt);
                req.amt = amt;
                if (amt <= 0) {
                    printf("Enter amount greater than 0\n");
                }
            }
            break;
    }
    return req;
}

/**
 * Prompts user to login via account number
 * @return Account number if successful login. -1 if unsuccessful login
 */
int login(int& sockfd) {
    bool loginSuccess = false;
    char buffer[256];
    bzero(buffer, 256);
    int acctNum;
    printf("Enter account number: ");
    fgets(buffer, 256, stdin);
    sscanf(buffer, "%d", &acctNum); // convert account number to int
    request loginRequest {0, acctNum, 0};
    char* loginResponse = sendRequest(sockfd, loginRequest);
    printf("%s\n", loginResponse);
    if (strcmp("LOGIN_SUCCESS", loginResponse) != 0) {
        exit(0);
    }
    return acctNum;
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    int acctNum = login(sockfd);
    if (acctNum) {
        int input;
        while (true) {
            bzero(buffer, 256);
            printf("1. View balance\n");
            printf("2. Deposit money\n");
            printf("3. Withdraw money\n");
            printf("4. Disconnect\n");
            printf("Enter option: ");
            fgets(buffer, 255, stdin);
            sscanf(buffer, "%d", &input); // convert input to int
            request req = buildRequest(input, acctNum);
            char* response = sendRequest(sockfd, req);
            if (req.requestType == 4) {
                printf("Disconnecting session\n");
                shutdown(sockfd, SHUT_RDWR);
                exit(0);
            }
            printf("%s\n", response);
        }
    }
    return 0;
}

