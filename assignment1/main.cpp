#include <fstream>
#include <string>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <map>
using namespace std;

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

class multiply_args {
    public:
        matrix_wrapper m1;
        matrix_wrapper m2;
        int m1Row;
        int m2Col;
        matrix_wrapper result;
        long* runtimes;
        int threadIndex;
        multiply_args() {}
        multiply_args(matrix_wrapper& m1, matrix_wrapper& m2, int m1Row, int m2Col, matrix_wrapper& result, long* runtimes, int threadIndex) :
            m1(m1), m2(m2), m1Row(m1Row), m2Col(m2Col), result(result), runtimes(runtimes), threadIndex(threadIndex) {
                //cout << "PARENT THREAD - ARGS CONSTRUCTION - RUNTIMES: " << &runtimes << endl;
                //cout << "PARENT THREAD - ARGS CONSTRUCTION - RESULT: " << &result << endl;
            }
        void updateResultMatrix(double val) {
            //cout << "CHILD THREAD - UPDATING - RESULT: " << &result << endl;
            result.updateMatrix(m1Row, m2Col, val);
        }
        double getM1Val(int row, int col) {
            return m1.getVal(row, col);
        }
        double getM2Val(int row, int col) {
            return m2.getVal(row, col);
        }
        void recordRuntime(long runtime) {
            //cout << "CHILD THREAD - UPDATING - RUNTIMES: " << &runtimes << endl;
            runtimes[threadIndex] = runtime;
            //cout << runtimes[threadIndex] << endl;
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

matrix_wrapper initializeMatrix(int rows, int cols) {
    double** matrix = new double*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new double[cols];
    }
    matrix_wrapper result(rows, cols, matrix);
    return result;
}

multiply_args** initializeArgsArray(int rows, int cols) {
    multiply_args** args = new multiply_args*[rows];
    for (int i = 0; i < rows; i++) {
        args[i] = new multiply_args[cols];
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
    multiply_args* args = (multiply_args*) argsPtr;
    int m1ColIter = 0;
    int m2RowIter = 0;
    int numOfIterations = args->m1.rowCount;
    double result = 0;
    for (int i = 0; i < numOfIterations; i++) {
        result += args->getM1Val(args->m1Row, m1ColIter) * args->getM2Val(m2RowIter, args->m2Col);
        m1ColIter++;
        m2RowIter++;
    }
    args->updateResultMatrix(result);
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
    //cout << "PARENT THREAD - RUNTIMES: " << &runtimes << endl;
    //cout << "PARENT THREAD - RESULT: " << &result << endl;
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    multiply_args** argArray = initializeArgsArray(resultRowCount, resultColCount);
    for (int i = 0; i < m1.rowCount; i++) {
        for (int k = 0; k < m2.colCount; k++) {
    //for (int i = 0; i < 1; i++) {
    //    for (int k = 0; k < 1; k++) {
            argArray[i][k] = multiply_args(m1, m2, i, k, result, runtimes, threadIndex);
            if (pthread_create(&tid[threadIndex++], NULL, multiplyOp, &argArray[i][k])) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    return operation_result(result, runtimes, numOfChildThreads);
}

int main() {
    vector<matrix_wrapper> matricies = createMatriciesFromFile();
    operation_result result = multiply(matricies[0], matricies[1]);
    result.printMatrix();
    result.printRuntimePerThread();
    result.printAvgRuntimePerThread();
    return 0;
}
