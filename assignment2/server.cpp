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
#include <vector>
#include <iostream>
#include <sys/shm.h>
#include "request.h"

void error(char *msg) {
    perror(msg);
    exit(1);
}

class account {
    public:
        std::string acctNum;
        double bal;
        account() {}
        account(std::string acctNum, double bal) : acctNum(acctNum), bal(bal) {} 
        void deposit(int amt) { bal += amt; }
        void withdraw(int amt) { bal -= amt; }
};

std::vector<account> readAccounts(std::string filename) {
    std::ifstream infile(filename);
    if (infile.fail())
        throw std::invalid_argument("Could not open file - invalid file name");
    std::string record;
    std::vector<account> accounts;
    while (getline(infile, record)) {
        int firstSpace = record.find(" ");
        int secondSpace = record.find(" ", firstSpace + 1);
        std::string acctNum = record.substr(0, firstSpace);
        std::string acctBal = record.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        accounts.push_back(account(acctNum, std::stod(acctBal)));
    }
    infile.close();

    return accounts;
}

char* processRequest(std::unordered_map<std::string, int> indicies, account* accts, request& req) {
    char* response = new char[100];;
    std::string acctNum = std::to_string(req.acctNum);
    double oldBal;
    double newBal;
    switch (req.requestType) {
        case 0:
            if (indicies.find(acctNum) != indicies.end()) {
                printf("Connecting account %d\n", req.acctNum);
                sprintf(response, "LOGIN_SUCCESS");
            } else {
                sprintf(response, "LOGIN_FAILED");
            }
            break;
        case 1:
            sprintf(response, "Account number: %s Balance: $%f", acctNum.c_str(), accts[indicies[acctNum]].bal);
            break;
        case 2:
            oldBal = accts[indicies[acctNum]].bal;
            accts[indicies[acctNum]].deposit(req.amt);
            newBal = accts[indicies[acctNum]].bal; 
            sprintf(response, "Old Balance: $%f New Balance: $%f", oldBal, newBal);
            break;
        case 3:
            if (req.amt <= accts[indicies[acctNum]].bal) {
                oldBal = accts[indicies[acctNum]].bal;
                accts[indicies[acctNum]].withdraw(req.amt);
                newBal = accts[indicies[acctNum]].bal; 
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

    std::unordered_map<std::string, int> indicies;
    std::vector<account> acctVector = readAccounts(argv[2]);
    key_t key = 6428;
	int shmid = shmget(key, sizeof(account) * acctVector.size(), IPC_CREAT | 0666);
	account* shared_mem_accts = (account*) shmat(shmid, 0, 0);
    shmctl(shmid, IPC_RMID, NULL); // Mark shared memory segment to be deleted after last process detaches
    for (int i = 0; i < acctVector.size(); i++) {
        indicies[acctVector[i].acctNum] = i;
        shared_mem_accts[i] = acctVector[i];
    }

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
                    char* response = processRequest(indicies, shared_mem_accts, req);
                    rwBytes = write(newsockfd, response, 100);
		    delete response;
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

