#include <stdio.h>
#include <stdlib.h>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>
#include <iostream>
#include "Matrix_culaD.h"

int main(int argc, char* argv[]) {
  culaInitialize();

  int m = 2;
  int k = 2;
  int n = 2;

  Matrix_culaD* A = new Matrix_culaD(m, k, false);
  int c = 1;
  for (int i=0; i<A->h(); i++) {
    for (int j=0; j<A->w(); j++) {
      A->set(i, j, (culaDouble)c); 
      c++;
    }
  }
  std::cout << "A =" << std::endl;
  A->print();

  Matrix_culaD* B = new Matrix_culaD(k, n, false);
  for (int i=0; i<B->h(); i++) {
    for (int j=0; j<B->w(); j++) {
      B->set(i, j, (culaDouble)c); 
      c++;
    }
  }
  std::cout << "B =" << std::endl;
  B->print();

  Matrix_culaD* C = A->matMul(B);
  std::cout << "A*B =" << std::endl;
  C->print();

  culaShutdown();

  return 0;
}
