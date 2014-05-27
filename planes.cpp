#include "DPP.h"
#include "Matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define real_t float

Matrix<real_t>* gaussian_kernel(Matrix<real_t>* X) {
  Matrix<real_t> *L = new Matrix<real_t>(X->h(), X->h(), false);

  for (int i=0; i<X->h(); i++) {
    real_t x_i = X->get(i, 0);
    real_t y_i = X->get(i, 1);

    for (int j=0; j<X->h(); j++) {
      real_t x_j = X->get(j, 0);
      real_t y_j = X->get(j, 1);
      real_t g = (real_t)exp( - (pow(x_i-x_j, 2) + pow(y_i-y_j, 2))/0.01);
      L->set(i, j, g);
    }
  }

  return L;
}

int main(int argc, char* argv[]) {

  if (argc < 3) {
    std::cout << "Usage: ./planes <size> <random seed>" << std::endl;
    exit(EXIT_FAILURE);
  }
  srand(atoi(argv[2]));

  // The number of points on each side of the square
  int N = atoi(argv[1]);

  // Generate the points
  Matrix<real_t> *X = new Matrix<real_t>(N*N, 2, false);
  int c = 0;
  for (int x=1; x<=N; x++) {
    for (int y=1; y<=N; y++) {
      X->set(c, 0, (real_t)x/(real_t)N);
      X->set(c, 1, (real_t)y/(real_t)N);
      c++;
    }
  }
  Matrix<real_t> *L = gaussian_kernel(X);
  
  std::cout << "Forming DPP with eigendecomposition..." << std::endl;
  DPP<real_t>* dpp = new DPP<real_t>(L);
  std::cout << "DPP created. Sampling..." << std::endl;

  Matrix<real_t>* Y = dpp->sample(100);
  Y->writeToFile("Y1.txt");

  printf("Done!\n");
  return 0;
}
