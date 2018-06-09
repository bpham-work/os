#include <fstream>
#include <string>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <map>
using namespace std;

enum class matrix_operations {
    ADD,
    SUBTRACT,
    MULTIPLY
};

class matrix_wrapper {
    public:
        int rowCount;
        int colCount;
        double** matrix;
        matrix_wrapper() {}
        matrix_wrapper(int rowCount, int colCount, double** matrix) : rowCount(rowCount), colCount(colCount), matrix(matrix) {}
        void print() {
            cout << "----- Result Matrix -----" << endl;
            for (int i = 0; i < rowCount; i++) {
                for (int k = 0; k < colCount; k++) {
                    cout << matrix[i][k] << "\t";
                }
                cout << endl;
            }
        }
        void updateMatrix(int row, int col, double val) {
            matrix[row][col] = val;
        }
        double getVal(int row, int col) {
            return matrix[row][col];
        }
};

class arguments {
    public:
        matrix_wrapper m1;
        matrix_wrapper m2;
        matrix_wrapper result;
        long* runtimes;
        int threadIndex;
        int targetRow;
        int targetCol;
        arguments() {}
        arguments(matrix_wrapper& m1, matrix_wrapper& m2, int targetRow, int targetCol, matrix_wrapper& result, long* runtimes, int threadIndex) :
            m1(m1), m2(m2), result(result), runtimes(runtimes), threadIndex(threadIndex), targetRow(targetRow), targetCol(targetCol) {}
        void updateResultMatrix(double val) {
            result.updateMatrix(targetRow, targetCol, val);
        }
        double getM1Val(int row, int col) {
            return m1.getVal(row, col);
        }
        double getM2Val(int row, int col) {
            return m2.getVal(row, col);
        }
        void recordRuntime(long runtime) {
            runtimes[threadIndex] = runtime;
        }
};

class operation_result {
    public:
        matrix_wrapper resultMatrix;
        long* runtimes;
        int numOfElems;
        operation_result(matrix_wrapper& resultMatrix, long* runtimes, int numOfElems) :
            resultMatrix(resultMatrix), runtimes(runtimes), numOfElems(numOfElems) {}
        void printMatrix() { resultMatrix.print(); }
        void printRuntimePerThread() {
            cout << "----- Runtime per thread -----" << endl;
            for (int it = 0; it < numOfElems; ++it) {
                cout << "Thread " << it << ": " << runtimes[it] << " ns" << endl;
            }
        }
        void printAvgRuntimePerThread() {
            double sum = 0;
            for (int it = 0; it < numOfElems; ++it) {
                sum += runtimes[it];
            }
            cout << "Average runtime per thread: " << (sum / numOfElems) << " ns" << endl;
        }
};

class operation {
    public:
        virtual void *execute(void *argsPtr) = 0;
        virtual void validate(matrix_wrapper& m1, matrix_wrapper& m2) = 0;

};

class matrix_operation_thread {
    public:
        arguments args;
        matrix_operation_thread(arguments& args) : args(args) {}
        bool start() {
            return (pthread_create(&thread, NULL, executeOperation, this) == 0);
        }
        void waitForThreadToFinish() {
            pthread_join(thread, NULL);
        }
        virtual void validate(matrix_wrapper& m1, matrix_wrapper& m2) = 0;
    protected:
        virtual void operation(arguments& args) = 0;
    private:
        pthread_t thread;
        static void* executeOperation(void* obj) {
            matrix_operation_thread* threadObj = (matrix_operation_thread *) obj;
            threadObj->operation(threadObj->args);
            return NULL;
        }
};

class add_operation : public matrix_operation_thread {
    public:
        add_operation(arguments& args) : matrix_operation_thread(args) {}
        void validate(matrix_wrapper& m1, matrix_wrapper& m2) {
            if (m1.rowCount != m2.rowCount || m1.colCount != m2.colCount) {
                throw invalid_argument("Matricies have invalid dimensions for addition!");
            }
        }
    protected:
        void operation(arguments& args) {
            chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
            int rowCount = args.m1.rowCount;
            int colCount = args.m1.colCount;
            double sum = args.getM1Val(args.targetRow, args.targetCol) + args.getM2Val(args.targetRow, args.targetCol);
            args.updateResultMatrix(sum);
            chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
            long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
            args.recordRuntime(runtime);
        }
};

//class subtract_operation : public operation {
//    public:
//        void *execute(void *argsPtr) {
//            chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
//            arguments* args = (arguments*) argsPtr;
//            int rowCount = args->m1.rowCount;
//            int colCount = args->m1.colCount;
//            double difference = args->getM1Val(args->targetRow, args->targetCol) - args->getM2Val(args->targetRow, args->targetCol);
//            args->updateResultMatrix(difference);
//            chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
//            long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
//            args->recordRuntime(runtime);
//            return NULL;
//        }
//        void validate(matrix_wrapper& m1, matrix_wrapper& m2) {
//            if (m1.rowCount != m2.rowCount || m1.colCount != m2.colCount) {
//                throw invalid_argument("Matricies have invalid dimensions for subtraction!");
//            }
//        }
//};
//
//class multiply_operation : public operation {
//    public:
//        void *execute(void *argsPtr) {
//            chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
//            arguments* args = (arguments*) argsPtr;
//            int m1ColIter = 0;
//            int m2RowIter = 0;
//            int numOfIterations = args->m1.rowCount;
//            double result = 0;
//            for (int i = 0; i < numOfIterations; i++) {
//                result += args->getM1Val(args->targetRow, m1ColIter) * args->getM2Val(m2RowIter, args->targetCol);
//                m1ColIter++;
//                m2RowIter++;
//            }
//            args->updateResultMatrix(result);
//            chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
//            long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
//            args->recordRuntime(runtime);
//            return NULL;
//        }
//        void validate(matrix_wrapper& m1, matrix_wrapper& m2) {
//            if (m1.colCount != m2.rowCount) {
//                throw invalid_argument("Matricies have invalid dimensions for multiplication!");
//            }
//        }
//};

matrix_wrapper initializeMatrix(int rows, int cols) {
    double** matrix = new double*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new double[cols];
    }
    matrix_wrapper result(rows, cols, matrix);
    return result;
}

arguments** initializeArgsArray(int rows, int cols) {
    arguments** args = new arguments*[rows];
    for (int i = 0; i < rows; i++) {
        args[i] = new arguments[cols];
    }
    return args;
}

vector<matrix_wrapper> createMatriciesFromFile() {
    vector<matrix_wrapper> matricies;
    ifstream infile("input.txt");
    int rowNum;
    int colNum;
    double element;
    while (infile >> rowNum) {
        infile >> colNum;
        matrix_wrapper result = initializeMatrix(rowNum, colNum);
        for (int i = 0; i < rowNum; i++) {
            for (int k = 0; k < colNum; k++) {
                infile >> element;
                result.updateMatrix(i, k, element);
            }
        }
        matricies.push_back(result);
    }
    infile.close();
    return matricies;
}

void *multiplyOp(void *argsPtr) {
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    arguments* args = (arguments*) argsPtr;
    int m1ColIter = 0;
    int m2RowIter = 0;
    int numOfIterations = args->m1.rowCount;
    double result = 0;
    for (int i = 0; i < numOfIterations; i++) {
        result += args->getM1Val(args->targetRow, m1ColIter) * args->getM2Val(m2RowIter, args->targetCol);
        m1ColIter++;
        m2RowIter++;
    }
    args->updateResultMatrix(result);
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
    args->recordRuntime(runtime);
    return NULL;
}

void *addOp(void *argsPtr) {
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    arguments* args = (arguments*) argsPtr;
    int rowCount = args->m1.rowCount;
    int colCount = args->m1.colCount;
    double sum = args->getM1Val(args->targetRow, args->targetCol) + args->getM2Val(args->targetRow, args->targetCol);
    args->updateResultMatrix(sum);
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
    args->recordRuntime(runtime);
    return NULL;
}

void *subtractOp(void *argsPtr) {
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    arguments* args = (arguments*) argsPtr;
    int rowCount = args->m1.rowCount;
    int colCount = args->m1.colCount;
    double difference = args->getM1Val(args->targetRow, args->targetCol) - args->getM2Val(args->targetRow, args->targetCol);
    args->updateResultMatrix(difference);
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
    args->recordRuntime(runtime);
    return NULL;
}


operation_result multiply(matrix_wrapper& m1, matrix_wrapper& m2) {
    if (m1.colCount != m2.rowCount) {
        throw invalid_argument("Matricies have invalid dimensions for multiplication!");
    }
    int resultRowCount = m1.colCount;
    int resultColCount = m2.rowCount;
    int numOfChildThreads = resultRowCount * resultColCount;
    static matrix_wrapper result = initializeMatrix(resultRowCount, resultColCount);
    static map<int, long> runtime_map;
    static long* runtimes = new long[numOfChildThreads];
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    arguments** argArray = initializeArgsArray(resultRowCount, resultColCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
            argArray[i][k] = arguments(m1, m2, i, k, result, runtimes, threadIndex);
            if (pthread_create(&tid[threadIndex++], NULL, multiplyOp, &argArray[i][k])) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        cout << "----- THREAD " << i << " TERMINATED -----" << endl;
        pthread_join(tid[i], NULL);
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

operation_result add(matrix_wrapper& m1, matrix_wrapper& m2) {
    if (m1.rowCount != m2.rowCount || m1.colCount != m2.colCount) {
        throw invalid_argument("Matricies have invalid dimensions for multiplication!");
    }
    int resultRowCount = m1.rowCount;
    int resultColCount = m1.colCount;
    int numOfChildThreads = resultRowCount * resultColCount;
    static matrix_wrapper result = initializeMatrix(resultRowCount, resultColCount);
    static map<int, long> runtime_map;
    static long* runtimes = new long[numOfChildThreads];
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    arguments** argArray = initializeArgsArray(resultRowCount, resultColCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
            argArray[i][k] = arguments(m1, m2, i, k, result, runtimes, threadIndex);
            if (pthread_create(&tid[threadIndex++], NULL, addOp, &argArray[i][k])) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        cout << "----- THREAD " << i << " TERMINATED -----" << endl;
        pthread_join(tid[i], NULL);
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

operation_result subtract(matrix_wrapper& m1, matrix_wrapper& m2) {
    if (m1.rowCount != m2.rowCount || m1.colCount != m2.colCount) {
        throw invalid_argument("Matricies have invalid dimensions for multiplication!");
    }
    int resultRowCount = m1.rowCount;
    int resultColCount = m1.colCount;
    int numOfChildThreads = resultRowCount * resultColCount;
    static matrix_wrapper result = initializeMatrix(resultRowCount, resultColCount);
    static map<int, long> runtime_map;
    static long* runtimes = new long[numOfChildThreads];
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    arguments** argArray = initializeArgsArray(resultRowCount, resultColCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
            argArray[i][k] = arguments(m1, m2, i, k, result, runtimes, threadIndex);
            if (pthread_create(&tid[threadIndex++], NULL, subtractOp, &argArray[i][k])) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        cout << "----- THREAD " << i << " TERMINATED -----" << endl;
        pthread_join(tid[i], NULL);
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

operation_result calculate(matrix_wrapper& m1, matrix_wrapper& m2) {
    //op.validate(m1, m2);
    int resultRowCount = m1.colCount;
    int resultColCount = m2.rowCount;
    int numOfChildThreads = resultRowCount * resultColCount;
    static matrix_wrapper result = initializeMatrix(resultRowCount, resultColCount);
    static map<int, long> runtime_map;
    static long* runtimes = new long[numOfChildThreads];
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    arguments** argArray = initializeArgsArray(resultRowCount, resultColCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
            argArray[i][k] = arguments(m1, m2, i, k, result, runtimes, threadIndex);
            if (pthread_create(&tid[threadIndex++], NULL, addOp, &argArray[i][k])) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        cout << "----- THREAD " << i << " TERMINATED -----" << endl;
        pthread_join(tid[i], NULL);
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

int main() {
    vector<matrix_wrapper> matricies = createMatriciesFromFile();
    operation_result result = calculate(matricies[0], matricies[1]);
    result.printMatrix();
    result.printAvgRuntimePerThread();
    return 0;
}
