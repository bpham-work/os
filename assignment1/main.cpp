#include <fstream>
#include <string>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <chrono>
using namespace std;

enum class matrix_operation {
    ADD,
    SUBTRACT,
    MULTIPLY
};

template <class T>
T** initialize2DArray(int rows, int cols) {
    T** matrix = new T*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new T[cols];
    }
    return matrix;
}

class matrix_wrapper {
    public:
        int rowCount;
        int colCount;
        double** matrix;
        matrix_wrapper() {}
        matrix_wrapper(int rowCount, int colCount) : rowCount(rowCount), colCount(colCount) {
            matrix = initialize2DArray<double>(rowCount, colCount);
        }
        matrix_wrapper(matrix_wrapper& m1, matrix_wrapper& m2, matrix_operation opType) {
            rowCount = m1.rowCount;
            colCount = m1.colCount;
            if (opType == matrix_operation::MULTIPLY) {
                rowCount = m1.rowCount;
                colCount = m2.colCount;
            }
            matrix = initialize2DArray<double>(rowCount, colCount);
        }
        void print() {
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
        operation_result() {}
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

vector<matrix_wrapper> createMatriciesFromFile() {
    vector<matrix_wrapper> matricies;
    string filename;
    cout << "Enter full name and extension of input file: ";
    cin >> filename;
    ifstream infile(filename);
    if (infile.fail()) {
        throw invalid_argument("Could not open file - invalid file name");        
    }
    int rowNum;
    int colNum;
    double element;
    while (infile >> rowNum) {
        infile >> colNum;
        matrix_wrapper result(rowNum, colNum);
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
    int numOfIterations = args->m1.colCount;
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
    double difference = args->getM1Val(args->targetRow, args->targetCol) - args->getM2Val(args->targetRow, args->targetCol);
    args->updateResultMatrix(difference);
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    long runtime = chrono::duration_cast<chrono::nanoseconds>( t2 - t1 ).count();
    args->recordRuntime(runtime);
    return NULL;
}

void validateMatricies(matrix_wrapper& m1, matrix_wrapper& m2, matrix_operation opType) {
    switch (opType) {
        case matrix_operation::MULTIPLY:
            if (m1.colCount != m2.rowCount) {
                throw invalid_argument("Matricies have invalid dimensions for multiplication!");
            }
            break;
        default:
            if (m1.rowCount != m2.rowCount || m1.colCount != m2.colCount) {
                throw invalid_argument("Matricies have invalid dimensions for addition/subtraction!");
            }
    }
}

operation_result calculate(matrix_wrapper& m1, matrix_wrapper& m2, matrix_operation opType) {
    validateMatricies(m1, m2, opType);
    static matrix_wrapper result = matrix_wrapper(m1, m2, opType);
    int numOfChildThreads = result.rowCount * result.colCount;
    static long* runtimes = new long[numOfChildThreads];
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    arguments** argArray = initialize2DArray<arguments>(result.rowCount, result.colCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
            argArray[i][k] = arguments(m1, m2, i, k, result, runtimes, threadIndex);
            int threadCreateStatus;
            switch (opType) {
                case matrix_operation::ADD:
                    threadCreateStatus = pthread_create(&tid[threadIndex++], NULL, addOp, &argArray[i][k]);
                    break;
                case matrix_operation::SUBTRACT:
                    threadCreateStatus = pthread_create(&tid[threadIndex++], NULL, subtractOp, &argArray[i][k]);
                    break;
                case matrix_operation::MULTIPLY:
                    threadCreateStatus = pthread_create(&tid[threadIndex++], NULL, multiplyOp, &argArray[i][k]);
                    break;
            }
            if (threadCreateStatus) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        cout << "----- THREAD " << i << " TERMINATED -----" << endl;
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

int main() {
    try {
        vector<matrix_wrapper> matricies = createMatriciesFromFile();
        int input;
        cout << "Matrix 1" << endl;
        matricies[0].print();
        cout << "Matrix 2" << endl;
        matricies[1].print();
        while (input != 4) {
            cout << "1. Add Matricies" << endl;
            cout << "2. Subtract Matricies" << endl;
            cout << "3. Multiply Matricies" << endl;
            cout << "4. Exit" << endl;
            cout << "Input: ";
            cin >> input;
            operation_result result;
            switch (input) {
                case 1:
                    cout << "----- ADDING MATRICIES -----" << endl;
                    result = calculate(matricies[0], matricies[1], matrix_operation::ADD);
                    break;
                case 2:
                    cout << "----- SUBTRACTING MATRICIES -----" << endl;
                    result = calculate(matricies[0], matricies[1], matrix_operation::SUBTRACT);
                    break;
                case 3:
                    cout << "----- MULTIPLYING MATRICIES -----" << endl;
                    result = calculate(matricies[0], matricies[1], matrix_operation::MULTIPLY);
                    break;
                case 4:
                    cout << "----- Exiting -----" << endl;
                    continue;
                default:
                    cout << "Invalid option" << endl;
                    continue;
            }
            cout << "----- Result Matrix -----" << endl;
            result.printMatrix();
            result.printAvgRuntimePerThread();
        }
        return 0;
    } catch (invalid_argument e) {
        cout << e.what() << endl;
    }
}
