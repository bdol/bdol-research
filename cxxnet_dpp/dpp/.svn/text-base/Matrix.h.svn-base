#ifndef MATRIX_H
#define MATRIX_H

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <type_traits>
#include <mkl.h>
#include "mkl_lapacke.h"

template<class T>
class Matrix {
  public:
    Matrix(int height, int width, bool rowMajor);
    Matrix(T* dataPtr, int height, int width, bool rowMajor);
    
    // Elementary properties
    int h();
    int w();
    int N();
    T get(int i, int j);
    T* getElements();
    void set(int i, int j, T d);

    // Data manipulation
    void deleteColumn(int j);
    void copyTo(Matrix<T>* B);
    void copyColumnTo(int srcCol, Matrix<T>* B, int destCol);
    void copyRowTo(int srcRow, Matrix<T>* B, int destRow);

    //// Matrix routines
    //void transpose();
    void elemWiseMinus(Matrix<T>* B);
    //void elemWiseDivide(Matrix* B);
    void elemWiseDivideScalar(T d);
    Matrix<T>* matMul(Matrix<T>* B);
    //void matMulInPlace(Matrix* A, Matrix* B);
    void selfOuterProduct();
    void eig(Matrix<T>* V, Matrix<T>* d);
    //Matrix* qr();
    void qrInPlace();

    void print();
    void writeToFile(std::string fname);
    void fillRandom();

    ~Matrix();
    
  private:
    bool isContainer;
    bool rowMajor;
    int height;
    int width;
    T* elements;
};


#endif
