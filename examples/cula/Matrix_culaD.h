#ifndef MATRIX_H
#define MATRIX_H

#include <algorithm>
#include <cmath>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Matrix_culaD {
  public:
    Matrix_culaD(int height, int width, bool rowMajor);
    
    // Elementary properties
    int w();
    int h();
    int N();
    culaDouble get(int i, int j);
    culaDouble set(int i, int j, culaDouble d);
    culaDouble* getElements();

    // Data manipulation
    void deleteColumn(int j);
    void copyTo(Matrix_culaD* B);

    // Matrix routines
    void transpose();
    // TODO: return matrices here
    Matrix_culaD* matMul(Matrix_culaD* B);
    void eig(Matrix_culaD* V, Matrix_culaD* d);
    Matrix_culaD* qr();

    void print();
    void fillRandom();

    ~Matrix_culaD();
    
  private:
    void checkCulaStatus(culaStatus status);

    bool rowMajor;
    int height;
    int width;
    culaDouble* elements;
};

#endif
