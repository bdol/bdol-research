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
  culaFloat* elements;
} Matrix;

void printMatrix(Matrix M) {
  for (int i=0; i<M.h; i++) {
    for (int j=0; j<M.w; j++) {
      printf("%f ", M.elements[M.w*i+j]);
    }
    printf("\n");
  }
}

void culaFloatExample() {
  int m=4;
  int k=4;

  culaStatus status;
  printf("Initializing CULA\n");
  status = culaInitialize();
  checkStatus(status);

  Matrix A, C;
  A.h = m; 
  A.w = k; 
  A.elements = (culaFloat*)malloc(A.h*A.w*sizeof(culaFloat));
  for (int i=0; i<m*k; i++) {
    A.elements[i] = (culaFloat)i;
  }
  printf("A=\n");
  printMatrix(A);

  C.h = m; 
  C.w = m; 
  C.elements = (culaFloat*)malloc(C.h*C.w*sizeof(culaFloat));

  status = culaSgemm('T', 'N', A.h, A.h, A.w, 1.0, A.elements, A.h, A.elements,
      A.w, 0.0,
      C.elements, C.h);
  checkStatus(status);

  printf("A*A' = \n");
  printMatrix(C);

  // Now compute the eigenvectors, eigenvalues
  Matrix w;
  w.h = m;
  w.w = 1;
  w.elements = (culaFloat*)malloc(w.h*w.w*sizeof(culaFloat));

  status = culaSsyev('V', 'U', m, C.elements, m, w.elements);
  checkStatus(status);

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

  culaFloatExample();

  return EXIT_SUCCESS;
}
