#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "request.h"

void error(char *msg) {
    perror(msg);
    exit(1);
}

std::unordered_map<std::string, double> readAccounts(std::string filename) {
    std::ifstream infile(filename);
    if (infile.fail())
        throw std::invalid_argument("Could not open file - invalid file name");
    std::string record;
    std::unordered_map<std::string, double> accounts;
    while (getline(infile, record)) {
        int firstSpace = record.find(" ");
        int secondSpace = record.find(" ", firstSpace + 1);
        std::string acctNum = record.substr(0, firstSpace);
        std::string acctBal = record.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        accounts[acctNum] = std::stod(acctBal);
    }
    infile.close();
    return accounts;
}

char* processRequest(std::unordered_map<std::string, double>& accounts, request& req) {
    char response[100];
    std::string acctNum = std::to_string(req.acctNum);
    double oldBal;
    double newBal;
    switch (req.requestType) {
        case 0:
            if (accounts.find(acctNum) != accounts.end()) {
                printf("Connecting account %d\n", req.acctNum);
                sprintf(response, "LOGIN_SUCCESS");
            } else {
                sprintf(response, "LOGIN_FAILED");
            }
            break;
        case 1:
            sprintf(response, "Account number: %s Balance: $%f", acctNum.c_str(), accounts[acctNum]);
            break;
        case 2:
            oldBal = accounts[acctNum];
            newBal = accounts[acctNum] + req.amt;
            accounts[acctNum] = newBal;
            sprintf(response, "Old Balance: $%f New Balance: $%f", oldBal, newBal);
            break;
        case 3:
            if (req.amt <= accounts[acctNum]) {
                oldBal = accounts[acctNum];
                newBal = accounts[acctNum] - req.amt;
                accounts[acctNum] = newBal;
                sprintf(response, "Old Balance: $%f New Balance: $%f", oldBal, newBal);
            } else {
                sprintf(response, "Insufficient funds!\n");
            }
            break;
        case 4:
            printf("Disconnecting account %d\n", req.acctNum);
            break;
    }
    return response;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    if (argc < 3) {
        fprintf(stderr,"ERROR - usage: %s [port] [inputFilename]\n", argv[0]);
        exit(1);
    }
    std::unordered_map<std::string, double> accounts = readAccounts(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
        error("ERROR opening socket");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    int bindRes = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bindRes < 0) {
        error("ERROR on binding");
    }
    int listenRes = listen(sockfd, 5);
    if (listenRes == 0) {
        printf("Listening...\n");
    }
    clilen = sizeof(cli_addr);

    pid_t pid;
    request req;
    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }
        pid = fork();
        if (pid == 0) {
            int rwBytes;
            while (true) {
                rwBytes = read(newsockfd, &req, sizeof(req));
                if (rwBytes < 0) {
                    error("ERROR reading from socket");
                } else if (rwBytes > 0) {
                    char* response = processRequest(accounts, req);
                    rwBytes = write(newsockfd, response, 100);
                    if (rwBytes < 0) {
                        error("ERROR writing to socket");
                    }
                    if (req.requestType == 4 || strcmp(response, "LOGIN_FAILED") == 0) {
                        shutdown(newsockfd, SHUT_RDWR);
                        exit(0);
                    }
                }
            }
        }
    }

    return 0; 
}

