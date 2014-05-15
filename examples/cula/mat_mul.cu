#include <stdio.h>
#include <stdlib.h>
#include <cula.h>
#include <cula_blas.h>
#include <cula_lapack.h>

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
  int m=5;
  int k=3;
  int n=2;

  culaStatus status;
  printf("Initializing CULA\n");
  status = culaInitialize();
  checkStatus(status);

  Matrix A, B, C;
  A.h = m; 
  A.w = k; 
  A.elements = (culaFloat*)malloc(m*k*sizeof(culaFloat));
  for (int i=0; i<m*k; i++) {
    A.elements[i] = (culaFloat)i;
  }
  printf("A=\n");
  printMatrix(A);

  B.h = k; 
  B.w = n; 
  B.elements = (culaFloat*)malloc(k*n*sizeof(culaFloat));
  for (int i=0; i<k*n; i++) {
    B.elements[i] = 1.0;
  }
  printf("B=\n");
  printMatrix(B);

  C.h = m; 
  C.w = n; 
  C.elements = (culaFloat*)malloc(m*n*sizeof(culaFloat));

  status = culaSgemm('N', 'N', n, m, k, 1.0, B.elements, n, A.elements, k, 0.0,
      C.elements, n);
  checkStatus(status);

  printf("A*B = \n");
  printMatrix(C);

  culaShutdown();
  
  free(A.elements);
  free(B.elements);
  free(C.elements);

  printf("Done!\n");

}

int main(int argc, char* argv[]) {

  culaFloatExample();

  return EXIT_SUCCESS;
}
