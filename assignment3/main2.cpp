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

int main(int argc, char* argv[]) {
    int numOfClerks = atoi(argv[1]);
    std::string filename = argv[2];
    vector<customer> customers = getCustomers(filename); 
    sem_t *clerksSem = sem_open("clerksSem", O_CREAT, 0600, numOfClerks);
    sem_t *serviceDataMutex = sem_open("serviceDataMutex", O_CREAT, 0600, 1);
    sem_t *mutexSem = sem_open("mutexSem", O_CREAT, 0600, 1);
    pid_t pid;
    int status = 0;

    // Allocate shared memory
    key_t key = 6428;
    int shmid = shmget(key, sizeof(int) * 4, IPC_CREAT | 0666);
    int* totalServiced = (int*) shmat(shmid, NULL, 0);
    int* numWaited = totalServiced + 1;
    int* numNotWaited = numWaited + 1;
    int* clerksAvailVal = totalServiced + 3;
    *clerksAvailVal = numOfClerks;
    shmctl(shmid, IPC_RMID, NULL);

    for (int i = 0; i < customers.size(); i++) {
        if (customers[i].timeLastArrival > 0) {
            sleep(customers[i].timeLastArrival);
        }
        clerksSem = sem_open("clerksSem", O_CREAT);
        if ((pid = fork()) == 0) {
            printf("%s arriving\n", customers[i].name.c_str());
            
            // Record service data
            sem_wait(serviceDataMutex);
            (*totalServiced)++;
            if (*clerksAvailVal > 0) {
                (*numNotWaited)++;
            } else {
                (*numWaited)++;
            }
            //printf("semValue: %d\n", *clerksAvailVal);
            sem_post(serviceDataMutex);

            // Serve customer
            sem_wait(clerksSem);
            sem_wait(mutexSem);
            (*clerksAvailVal)--;
            printf("%s getting helped\n", customers[i].name.c_str());
            sem_post(mutexSem);

            if (customers[i].serviceTime > 0) {
                sleep(customers[i].serviceTime);
            }

            sem_wait(mutexSem);
            (*clerksAvailVal)++;
            printf("%s leaving restaurant\n", customers[i].name.c_str());
            sem_post(mutexSem);
            sem_post(clerksSem);
            break;
        }
    }

    if (pid != 0) {
        while ((pid = wait(&status)) > 0);

        printf("Total customers serviced: %d\n", *totalServiced);
        printf("Number of customers with no wait: %d\n", *numNotWaited);
        printf("Number of customers required to wait: %d\n", *numWaited);

        // Close semaphores and shared memory
        shmdt(totalServiced);
        sem_unlink("clerksSem");
        sem_close(clerksSem);
        sem_unlink("serviceDataMutex");
        sem_close(serviceDataMutex);
        sem_unlink("mutexSem");
        sem_close(mutexSem);
    }
    return 0;
}
