
#include <stdio.h>
#include <stdlib.h>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>
#include <iostream>
#include "Matrix_culaD.h"

int main(int argc, char* argv[]) {
  culaInitialize();

  int m = 5;
  int k = 3;

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

  std::cout << "QR" << std::endl;
  Matrix_culaD* Q = L->qr();;
  std::cout << "done" << std::endl;
  std::cout << "Q = " << std::endl;
  Q->print();


  culaShutdown();

  return 0;
}


