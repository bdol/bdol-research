
#include <stdio.h>
#include <stdlib.h>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>
#include <iostream>
#include "Matrix_culaD.h"

int main(int argc, char* argv[]) {
  culaInitialize();

  int m = 3;
  int k = 2;

  Matrix_culaD* A = new Matrix_culaD(m, k, false);
  int c = 1;
  for (int i=0; i<A->h(); i++) {
    for (int j=0; j<A->w(); j++) {
      A->set(i, j, (culaDouble)c); 
      c++;
    }
  }
  std::cout << "A = " << std::endl;
  A->print();

  Matrix_culaD* B = new Matrix_culaD(m, k, false);
  A->copyTo(B);
  B->transpose();

  Matrix_culaD* L = A->matMul(B);
  std::cout << "L = " << std::endl;
  L->print();

  Matrix_culaD* V = new Matrix_culaD(m, m, false);
  Matrix_culaD* d = new Matrix_culaD(m, 1, false);
  L->copyTo(V);
  L->eig(V, d);
  std::cout << "V = " << std::endl;
  V->print();
  std::cout << "d = " << std::endl;
  d->print();

  culaShutdown();

  return 0;
}

