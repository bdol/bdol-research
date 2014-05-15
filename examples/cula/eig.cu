#include <stdio.h>
#include <stdlib.h>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>
#include <cublas_v2.h>

void checkStatus(culaStatus status) {
  if (!status) {
    return;
  }

  char buf[80];
  culaGetErrorInfoString(status, culaGetErrorInfo(), buf, sizeof(buf));
  printf("%s\n", buf);
  
  culaShutdown();

  exit(EXIT_FAILURE);
}

typedef struct {
  int w;
  int h;
  culaDouble* elements;
} Matrix;

void printMatrix(Matrix M) {
  for (int i=0; i<M.h; i++) {
    for (int j=0; j<M.w; j++) {
      printf("%f ", M.elements[M.w*i+j]);
    }
    printf("\n");
  }
}

void transposeSquareMatrix(Matrix A) {
  if (A.h != A.w) {
    printf("Error in transposeSquareMatrix(): matrix is not square!\n");
    culaShutdown();
    exit(EXIT_FAILURE);
  }

  for (int i=0; i<A.h; i++) {
    for (int j=i+1; j<A.w; j++) {
      culaDouble temp = A.elements[i*A.w + j];
      A.elements[i*A.w + j] = A.elements[j*A.w + i];
      A.elements[j*A.w + i] = temp;
    }
  }

  return;
}

/**
 * Computes the eigenvalues and eigenvectors of symmetric matrix A. Returns the
 * eigenvectors in A (note that the original contents of A are destroyed) and
 * the eigenvalues in w.
 */
void eig(Matrix A, Matrix w) {
  // First check to make sure the dimensions of everything are correct.
  int N = A.h;
  if (w.h != N) {
    printf("Error in EIG: w height is not %d.\n", N);
    culaShutdown();
    exit(EXIT_FAILURE);
  }
  if (w.w != 1) {
    printf("Error in EIG: w height is not %d.\n", 1);
    culaShutdown();
    exit(EXIT_FAILURE);
  }

  // Compute the eigendecomposition
  culaStatus status = culaDsyev('V', 'U', N, A.elements, N, w.elements);
  checkStatus(status);

  // CULA stores the result in column-major order, switch it to row-major
  transposeSquareMatrix(A);

  return;
}

void eigExample() {
  int m=4;
  int k=5;

  culaStatus status;
  printf("Initializing CULA\n");
  status = culaInitialize();
  checkStatus(status);

  Matrix A, C;
  A.h = m; 
  A.w = k; 
  A.elements = (culaDouble*)malloc(A.h*A.w*sizeof(culaDouble));
  for (int i=0; i<m*k; i++) {
    A.elements[i] = (culaDouble)i;
  }
  printf("A=\n");
  printMatrix(A);

  C.h = m; 
  C.w = m; 
  C.elements = (culaDouble*)malloc(C.h*C.w*sizeof(culaDouble));

  status = culaDgemm('T', 'N', A.h, A.h, A.w, 1.0, A.elements, A.w, A.elements,
      A.w, 0.0,
      C.elements, C.h);
  checkStatus(status);

  printf("A*A' = \n");
  printMatrix(C);

  // Now compute the eigenvectors, eigenvalues
  Matrix w;
  w.h = m;
  w.w = 1;
  w.elements = (culaDouble*)malloc(w.h*w.w*sizeof(culaDouble));

  eig(C, w);

  printf("V = \n");
  printMatrix(C);
  printf("D = \n");
  printMatrix(w);

  culaShutdown();
  
  free(A.elements);
  free(C.elements);
  free(w.elements);

  printf("Done!\n");

}

int main(int argc, char* argv[]) {

  eigExample();

  return EXIT_SUCCESS;
}
