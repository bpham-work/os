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
    sem_t *clerkSem = sem_open("clerks", O_CREAT, 0600, numOfClerks);
    pid_t pid;
    int status = 0;
    int numWaiting;
    for (int i = 0; i < customers.size(); i++) {
        sleep(customers[i].timeLastArrival);
        clerkSem = sem_open("clerks", O_CREAT);
        if ((pid = fork()) == 0) {
            printf("%s arriving\n", customers[i].name.c_str());
            if (sem_getvalue(clerkSem, &numWaiting)) {
                perror("getval");
                return 1;
            }
            printf("WAIT VAL %d\n", numWaiting);
            sem_wait(clerkSem);
            printf("%s getting helped\n", customers[i].name.c_str());
            if (customers[i].serviceTime > 0) {
                sleep(customers[i].serviceTime);
            }
            printf("%s leaving restaurant\n", customers[i].name.c_str());
            sem_post(clerkSem);
            break;
        }
    }

    if (pid != 0) {
        while ((pid = wait(&status)) > 0);
        sem_unlink("clerks");
        sem_close(clerkSem);
    }
    return 0;
}
