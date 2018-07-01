#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sys/shm.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "splitter.h"

class customer {
    public:
        string name;
        int timeLastArrival;
        int serviceTime;
        customer() {}
        customer(std::string& record) {
            std::vector<string> fields = util::split(record, ' ');
            this->name = fields[0];
            this->timeLastArrival = stoi(fields[1]);
            this->serviceTime = stoi(fields[2]);
        }
};

vector<customer> getCustomers(string& filename) {
    std::ifstream infile(filename);
    if (infile.fail())
        throw std::invalid_argument("Could not open file");
    std::string record;
    std::vector<customer> customers;
    while (getline(infile, record)) {
        customers.push_back(customer(record));        
    }
    infile.close();
    return customers;
}

//sem_t clerksAvail;

int main(int argc, char* argv[]) {
    int numOfClerks = atoi(argv[1]);
    std::string filename = argv[2];
    vector<customer> customers = getCustomers(filename); 
    int* value = new int;
    //sem_init(&clerksAvail, 1, numOfClerks);
    sem_t* clerksSem = sem_open("clerks", O_CREAT, 0600, numOfClerks);
    pid_t pid;
    int status = 0;

    key_t key = 6428;
    //int waitCountSharedMemId = shmget(key, sizeof(int), IPC_CREAT | 0666);
    int clerksSemSharedMemId = shmget(key, sizeof(sem_t), IPC_CREAT | 0666);
    //int* waitCount = (int*) shmat(waitCountSharedMemId, 0, 0);
    clerksSem = (sem_t*) shmat(clerksSemSharedMemId, 0, 0);
    //*waitCount = 0;
    int waitVal;
    //sem_init(clerksSem, 1, numOfClerks);
    for (int i = 0; i < customers.size(); i++) {
        sleep(customers[i].timeLastArrival);
        printf("%s arriving\n", customers[i].name.c_str());
        if ((pid = fork()) == 0) {
	    clerksSem = sem_open("clerks", 0);
            if (sem_getvalue(clerksSem, &waitVal)) {
                perror("getval");
                return 1;
            }
            //printf("WAIT VAL %d\n", waitVal);
            //printf("%s WAITING\n", customers[i].name.c_str());
            //if (customers[i].serviceTime > 0) {
            //    sleep(customers[i].serviceTime);
            //}
            sem_wait(clerksSem);
            printf("%s getting helped\n", customers[i].name.c_str());
            printf("%s leaving restaurant\n", customers[i].name.c_str());
            sem_post(clerksSem);
            break;
        }
    }
    if (pid != 0) {
        while ((pid = waitpid(-1, &status, 0)) != -1);
        //sem_destroy(&clerksAvail);
        sem_unlink("clerks");
        //shmdt(waitCount);
	//shmctl(waitCountSharedMemId, IPC_RMID, NULL);
    }
    return 0;
}
