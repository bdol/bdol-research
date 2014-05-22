#include "DPP.h"
#include "Matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Matrix<double>* gaussian_kernel(Matrix<double>* X) {
  Matrix<double> *L = new Matrix<double>(X->h(), X->h(), false);

  for (int i=0; i<X->h(); i++) {
    double x_i = X->get(i, 0);
    double y_i = X->get(i, 1);

    for (int j=0; j<X->h(); j++) {
      double x_j = X->get(j, 0);
      double y_j = X->get(j, 1);
      double g = (double)exp( - (pow(x_i-x_j, 2) + pow(y_i-y_j, 2))/0.03);
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
  Matrix<double> *X = new Matrix<double>(N*N, 2, false);
  int c = 0;
  for (int x=1; x<=N; x++) {
    for (int y=1; y<=N; y++) {
      X->set(c, 0, (double)x/(double)N);
      X->set(c, 1, (double)y/(double)N);
      c++;
    }
  }
  Matrix<double> *L = gaussian_kernel(X);
  
  std::cout << "Forming DPP with eigendecomposition..." << std::endl;
  DPP* dpp = new DPP(L);
  std::cout << "DPP created. Sampling..." << std::endl;

  Matrix<double>* Y = dpp->sample();
  Y->writeToFile("Y1.txt");

  //culaShutdown();

  printf("Done!\n");
  return 0;
}
