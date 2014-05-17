#include "Matrix_culaD.h"

Matrix_culaD::Matrix_culaD(int height, int width, bool rowMajor) {
  this->height = height;
  this->width = width;
  this->rowMajor = rowMajor;

  this->elements = new culaDouble[height*width];

}

void Matrix_culaD::checkCulaStatus(culaStatus status) {
  if (!status) {
    return;
  }

  std::string s = culaGetStatusString(status);
  std::cout << "Cula error: " << s << std::endl;
  std::cout << "Shutting down." << std::endl;

  culaShutdown();

  exit(EXIT_FAILURE);
}

int Matrix_culaD::h() {
  return this->height;
}

int Matrix_culaD::w() {
  return this->width;
}

int Matrix_culaD::N() {
  return this->height*this->width;
}

culaDouble Matrix_culaD::get(int i, int j) {
  if (this->rowMajor) {
    return this->elements[this->width*i+j];
  } else {
    return this->elements[this->height*j+i];
  }
}

culaDouble Matrix_culaD::set(int i, int j, culaDouble d) {
  if (this->rowMajor) {
    this->elements[this->width*i+j] = d;
  } else {
    this->elements[this->height*j+i] = d;
  }
}

culaDouble* Matrix_culaD::getElements() {
  return this->elements;
}

void Matrix_culaD::deleteColumn(int j) {
  if (this->rowMajor) {
    std::cout << "Error in deleteColumn: This matrix is not column-major (row major not supported yet). Doing nothing" << std::endl;
    return;
  }

  for (int k=j; k<this->width; k++) {
    int destIdx = k*this->height;
    int srcIdx = (k+1)*this->height;
    std::copy(&(this->elements[srcIdx]),
              &(this->elements[srcIdx+this->height]),
              &(this->elements[destIdx]));
  }
  this->width -= 1;
}

void Matrix_culaD::copyTo(Matrix_culaD* B) {
  if (this->height != B->h() || this->width != B->w()) {
    std::cout << "Error in copyTo: Matrix dimensions are not equal. Doing nothing" << std::endl;
    return;
  }
  // TODO: check if B == this->rowMajor

  culaDouble* B_elem = B->getElements();
  for (int i=0; i<this->N(); i++) {
    B_elem[i] = this->elements[i];
  }
}

/**
 * In-place matrix transpose.
 * http://rosettacode.org/wiki/Matrix_transposition
 */
void Matrix_culaD::transpose() {

  int start, next, i;
  culaDouble tmp;
  for (int start=0; start<this->N()-1; start++) {
    next = start;
    i = 0;
    do {
      i++;
      next = (next % this->w())*this->h() + next/this->w();
    } while (next > start);

    if (next < start || i == 1) continue;
    tmp = this->elements[next = start];
    do {
      i = (next % this->w())*this->h() + next/this->w();
      this->elements[next] = (i==start) ? tmp : this->elements[i];
      next = i;
    } while (next > start);
  }

  int temp = this->width;
  this->width = this->height;
  this->height = temp;
}

/**
 * Calculates the matrix product A*B and stores the result in C
 */
Matrix_culaD* Matrix_culaD::matMul(Matrix_culaD* B) {
  if ( this->rowMajor || B->rowMajor) {
    std::cout << "Error in matMul: One of the matrices is not column-major (row major not supported yet). Doing nothing" << std::endl;
    return NULL;
  }


  int m = this->height;
  int n = B->w();
  int k = this->width;

  Matrix_culaD* C = new Matrix_culaD(m, n, false);

  if ( k != B->h() ) {
    std::cout << "Error in matMul: Inner matrix dimensions are not equal. Doing nothing" << std::endl;
    return NULL;
  }

  culaStatus status;
  status = culaDgemm('N', 'N', m, n, k,
      1.0, 
      this->elements, m,
      B->getElements(), k,
      0.0,
      C->getElements(), C->h());
  checkCulaStatus(status);

  return C;
}

void Matrix_culaD::eig(Matrix_culaD* V, Matrix_culaD* d) {
  culaStatus status = culaDsyev('V', 'U', V->h(), 
      V->elements, V->h(), d->elements);
  checkCulaStatus(status);
}

Matrix_culaD* Matrix_culaD::qr() {
  Matrix_culaD* Q = new Matrix_culaD(this->height, this->width, this->rowMajor);
  this->copyTo(Q);

  culaDouble* tau = new culaDouble[std::min(Q->h(), Q->w())];
  culaStatus status = culaDgeqrf(Q->h(), Q->w(), Q->getElements(), Q->h(), tau);
  checkCulaStatus(status);
  status = culaDorgqr(Q->h(), Q->w(), Q->w(), Q->getElements(), Q->h(), tau);
  checkCulaStatus(status);

  delete [] tau;

  return Q;
}

void Matrix_culaD::print() {
  for (int i=0; i<this->height; i++) {
    for (int j=0; j<this->width; j++) {
      if (this->rowMajor) {
        std::cout << this->elements[this->width*i+j] << " ";
      } else {
        std::cout << this->elements[this->height*j+i] << " ";
      }
    }
    std::cout << std::endl;
  }
}

void Matrix_culaD::fillRandom() {
  for (int i=0; i<(this->height*this->width); i++) {
    this->elements[i] = (culaDouble)((double)rand() / (double)RAND_MAX);
  }
}

Matrix_culaD::~Matrix_culaD() {
  delete [] this->elements;
}
