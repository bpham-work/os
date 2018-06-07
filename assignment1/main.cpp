#include <fstream>
#include <string>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <stdexcept>
using namespace std;

class matrix_wrapper {
    public:
        int rowCount;
        int colCount;
        double** matrix;
        matrix_wrapper() {}
        matrix_wrapper(int rowCount, int colCount, double** matrix) : rowCount(rowCount), colCount(colCount), matrix(matrix) {}
        void print() {
            for (int i = 0; i < rowCount; i++) {
                for (int k = 0; k < colCount; k++) {
                    cout << matrix[i][k] << "\t";
                }
                cout << endl;
            }
        }
};

class multiply_args {
    public:
        matrix_wrapper m1;
        matrix_wrapper m2;
        int m1Row;
        int m2Col;
        matrix_wrapper result;
        multiply_args(matrix_wrapper& m1, matrix_wrapper& m2, int m1Row, int m2Col, matrix_wrapper& result) :
            m1(m1), m2(m2), m1Row(m1Row), m2Col(m2Col), result(result) {}
};

matrix_wrapper initializeMatrix(int rows, int cols) {
    double** matrix = new double*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new double[cols];
    }
    matrix_wrapper result(rows, cols, matrix);
    return result;
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
                result.matrix[i][k] = element;
            }
        }
        matricies.push_back(result);
    }
    infile.close();
    return matricies;
}

void *multiplyOp(void *argsPtr) {
    multiply_args* args = (multiply_args*) argsPtr;
    int m1IndexShifter = 0;
    int m2IndexShifter = 0;
    int numOfIterations = args->m1.rowCount;
    double result = 0;
    //cout << args->m2Col << endl;
    cout << args->m1Row << endl;
    while (numOfIterations > 0) {
        result += args->m1.matrix[args->m1Row][m1IndexShifter] * args->m2.matrix[m2IndexShifter][args->m2Col];
        m1IndexShifter++;
        m2IndexShifter++;
        numOfIterations--;
    }
    args->result.matrix[args->m1Row][args->m2Col] = result;
    return NULL;
}

void *testop(void *args) {
    int* a = (int*) args;
    cout << *a;
    return NULL;
}

matrix_wrapper multiply(matrix_wrapper& m1, matrix_wrapper& m2) {
    if (m1.colCount != m2.rowCount) {
        throw invalid_argument("Matricies have invalid dimensions for multiplication!");
    }
    int resultRowCount = m1.colCount;
    int resultColCount = m2.rowCount;
    static matrix_wrapper result = initializeMatrix(resultRowCount, resultColCount);
    
    int numOfChildThreads = resultRowCount * resultColCount;
    pthread_t tid[numOfChildThreads];
    int threadIndex = 0;
    //for (int i = 0; i < m1.rowCount; i++) {
    int rowIndexArr[2];
    int colIndexArr[1];
    for (int i = 0; i < 2; i++) {
        rowIndexArr[i] = i;
        //for (int k = 0; k < m2.colCount; k++) {
        for (int k = 0; k < 1; k++) {
            colIndexArr[k] = k;
            multiply_args args(m1, m2, rowIndexArr[i], colIndexArr[k], result);
            if (pthread_create(&tid[threadIndex++], NULL, multiplyOp, &args)) {
            //if (pthread_create(&tid[threadIndex++], NULL, testop, r)) {
                cout << "Error creating thread" << endl;
            }
        }
    }
    for (int i = 0; i < numOfChildThreads; i++) {
        pthread_join(tid[i], NULL);
    }
    return result;
}

int main() {
    vector<matrix_wrapper> matricies = createMatriciesFromFile();
    matrix_wrapper result = multiply(matricies[0], matricies[1]);
    result.print();
    return 0;
}
