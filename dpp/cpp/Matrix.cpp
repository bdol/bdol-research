#include "Matrix.h"

template<class T>
Matrix<T>::Matrix(int height, int width, bool rowMajor) {
  this->height = height;
  this->width = width;
  this->rowMajor = rowMajor;

  this->elements = new T[height*width];

}
template<class T>
int Matrix<T>::h() {
  return this->height;
}

template<class T>
int Matrix<T>::w() {
  return this->width;
}

template<class T>
int Matrix<T>::N() {
  return this->height*this->width;
}

template<class T>
T Matrix<T>::get(int i, int j) {
  if (i >= this->height) {
    std::cout << "Error in get: index " << i << " is out of bounds for matrix with " << this->height << " row(s)." << std::endl;
    exit(EXIT_FAILURE);
  }
  if (j >= this->width) {
    std::cout << "Error in get: index " << j << " is out of bounds for matrix with " << this->width << " column(s)." << std::endl;
    exit(EXIT_FAILURE);
  }
  
  if (this->rowMajor) {
    return this->elements[this->width*i+j];
  } else {
    return this->elements[this->height*j+i];
  }
}

template<class T>
T* Matrix<T>::getElements() {
  return this->elements;
}

template<class T>
void Matrix<T>::set(int i, int j, T d) {
  if (i >= this->height) {
    std::cout << "Error: index " << i << " is out of bounds for matrix with " << this->height << " row(s)." << std::endl;
    exit(EXIT_FAILURE);
  }
  if (j >= this->width) {
    std::cout << "Error: index " << j << " is out of bounds for matrix with " << this->width << " column(s)." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (this->rowMajor) {
    this->elements[this->width*i+j] = d;
  } else {
    this->elements[this->height*j+i] = d;
  }
}

template<class T>
void Matrix<T>::deleteColumn(int j) {
  if (this->rowMajor) {
    std::cout << "Error in deleteColumn: This matrix is not column-major (row major not supported yet)." << std::endl;
    exit(EXIT_FAILURE);
  }

  for (int k=j; k<this->width-1; k++) {
    int destIdx = k*this->height;
    int srcIdx = (k+1)*(this->height);
    std::copy(&(this->elements[srcIdx]),
              &(this->elements[srcIdx+this->height]),
              &(this->elements[destIdx]));
  }
  this->width -= 1;
}





//void Matrix::checkCulaStatus(culaStatus status) {
  //std::cout << "Status: " << status << std::endl;
  //if (!status) {
    //return;
  //}

  //std::string s = culaGetStatusString(status);
  //std::cout << "Cula error: " << s << std::endl;
  //std::cout << "Shutting down." << std::endl;

  //culaShutdown();

  //exit(EXIT_FAILURE);
//}





template<class T>
void Matrix<T>::copyTo(Matrix<T>* B) {
  if (this->height != B->h() || this->width != B->w()) {
    std::cout << "Error in copyTo: Matrix dimensions are not equal. Doing nothing" << std::endl;
    return;
  }
  // TODO: check if B == this->rowMajor

  T* B_elem = B->getElements();
  for (int i=0; i<this->N(); i++) {
    B_elem[i] = this->elements[i];
  }
}

template<class T>
void Matrix<T>::copyColumnTo(int srcCol, Matrix<T>* B, int destCol) {
  if (srcCol >= this->w()) {
    std::cout << "Error in copyColumnTo: source column index " << srcCol << " is out of range for matrix with " << this->w() << " columns." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (destCol >= B->w()) {
    std::cout << "Error in copyColumnTo: destination column index " << srcCol << " is out of range for matrix with " << B->w() << " columns." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (this->h() != B->h()) {
    std::cout << "Error in copyColumnTo: sourcehis matrix has height " << this->h() << ", but destination matrix has height " << B->h() << "." << std::endl;
    exit(EXIT_FAILURE);

  }

  for (int i=0; i<B->h(); i++) {
    B->set(i, destCol, this->get(i, srcCol));
  }
}

template<class T>
void Matrix<T>::copyRowTo(int srcRow, Matrix<T>* B, int destRow) {
  if (srcRow>= this->h()) {
    std::cout << "Error in copyRowTo: source row index " << srcRow << " is out of range for matrix with " << this->h() << " rows." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (destRow >= B->h()) {
    std::cout << "Error in copyRowTo: destination row index " << srcRow << " is out of range for matrix with " << B->h() << " rows." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (this->w() != B->w()) {
    std::cout << "Error in copyRowTo: source matrix has width " << this->w() << ", but destination matrix has row " << B->w() << "." << std::endl;
    exit(EXIT_FAILURE);

  }

  for (int j=0; j<B->w(); j++) {
    B->set(destRow, j, this->get(srcRow, j));
  }
}

/**
 * In-place matrix transpose.
 * http://rosettacode.org/wiki/Matrix_transposition
 */
//void Matrix::transpose() {

  //int start, next, i;
  //culaDouble tmp;
  //for (int start=0; start<this->N()-1; start++) {
    //next = start;
    //i = 0;
    //do {
      //i++;
      //next = (next % this->w())*this->h() + next/this->w();
    //} while (next > start);

    //if (next < start || i == 1) continue;
    //tmp = this->elements[next = start];
    //do {
      //i = (next % this->w())*this->h() + next/this->w();
      //this->elements[next] = (i==start) ? tmp : this->elements[i];
      //next = i;
    //} while (next > start);
  //}

  //int temp = this->width;
  //this->width = this->height;
  //this->height = temp;
//}

template<class T>
void Matrix<T>::elemWiseMinus(Matrix<T>* B) {
  if (this->h() != B->h()) {
    std::cout << "error in elemWiseMinus: this matrix has height " << this->h() << ", but the other has height " << B->h() << "." << std::endl;
    exit(EXIT_FAILURE);
  }
  if (this->w() != B->w()) {
    std::cout << "error in elemWiseMinus: this matrix has width " << this->w() << ", but the other has width " << B->w() << "." << std::endl;
    exit(EXIT_FAILURE);
  }

  for (int i=0; i<this->h(); i++) {
    for (int j=0; j<this->w(); j++) {
      this->set(i, j, this->get(i, j) - B->get(i, j));
    }
  }

}

//void Matrix::elemWiseDivide(Matrix* B) {
  //if (this->h() != B->h()) {
    //std::cout << "error in elemWiseDivide: this matrix has height " << this->h() << ", but the other has height " << B->h() << "." << std::endl;
    //exit(EXIT_FAILURE);
  //}
  //if (this->w() != B->w()) {
    //std::cout << "error in elemWiseDivide: this matrix has width " << this->w() << ", but the other has width " << B->w() << "." << std::endl;
    //exit(EXIT_FAILURE);
  //}

  //for (int i=0; i<this->h(); i++) {
    //for (int j=0; j<this->w(); j++) {
      //this->set(i, j, this->get(i, j)/B->get(i, j));
    //}
  //}
//}

template<class T>
void Matrix<T>::elemWiseDivideScalar(T d) {
  for (int i=0; i<this->h(); i++) {
    for (int j=0; j<this->w(); j++) {
      this->set(i, j, this->get(i, j)/d);
    }
  }
}

/**
 * Calculates the matrix product A*B returns the result (a new matrix)
 */
template<class T>
Matrix<T>* Matrix<T>::matMul(Matrix<T>* B) {

}
template<>
Matrix<float>* Matrix<float>::matMul(Matrix<float>* B) {
  std::cout << "Error in Matrix::matMul(): matMul() for floats is not implemented yet" << std::endl;
  exit(EXIT_FAILURE);
}

template<>
Matrix<double>* Matrix<double>::matMul(Matrix<double>* B) {
  if ( this->rowMajor || B->rowMajor) {
    std::cout << "Error in matMul: One of the matrices is not column-major (row major not supported yet)." << std::endl;
    exit(EXIT_FAILURE);
  }


  int m = this->height;
  int n = B->w();
  int k = this->width;

  Matrix<double>* C = new Matrix<double>(m, n, false);

  if ( k != B->h() ) {
    std::cout << "Error in matMul: Inner matrix dimensions are not equal." << std::endl;
    exit(EXIT_FAILURE);
  }

  cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
      m, n, k, 1.0, 
      this->elements, m,
      B->elements, k,
      0.0, C->elements, m);

  return C;
}

/**
 * Calculates the matrix product A*B stores the result in this matrix
 */
//void Matrix::matMulInPlace(Matrix* A, Matrix* B) {
  //if ( this->rowMajor || B->rowMajor) {
    //std::cout << "Error in matMulInPlace: One of the matrices is not column-major (row major not supported yet). Doing nothing" << std::endl;
    //exit(EXIT_FAILURE);
  //}

  //int m = A->h();
  //int n = B->w();
  //int k = A->w();

  //if (this->h() != m) {
    //std::cout << "error in matmulinplace: this matrix has height " << this->h() << ", but product has height " << m << "." << std::endl;
    //exit(EXIT_FAILURE);
  //}

  //if (this->w() != n) {
    //std::cout << "error in matmulinplace: this matrix has width " << this->w() << ", but product has width " << n << "." << std::endl;
    //exit(EXIT_FAILURE);
  //}

  //if ( k != B->h() ) {
    //std::cout << "Error in matMulInPlace: Inner matrix dimensions are not equal. Doing nothing" << std::endl;
    //exit(EXIT_FAILURE);
  //}

  //culaStatus status;
  //status = culaDgemm('N', 'N', m, n, k,
      //1.0, 
      //A->getElements(), m,
      //B->getElements(), k,
      //0.0,
      //this->getElements(), this->h());
  //checkCulaStatus(status);

//}

template<class T>
void Matrix<T>::selfOuterProduct() {

}

template<>
void Matrix<float>::selfOuterProduct() {
  std::cout << "Error in Matrix::selfOuterProduct(): selfOuterProduct() for floats is not implemented yet" << std::endl;
  exit(EXIT_FAILURE);
}

template<>
void Matrix<double>::selfOuterProduct() {
  if (this->rowMajor) {
    std::cout << "Error in Matrix::selfOuterProduct: selfOuterProduct() is only implemented for column major matrices." << std::endl;
    exit(EXIT_FAILURE);
  }

  // TODO: this probably isn't the most efficient way to do this
  int N = this->height;
  Matrix<double> *A = new Matrix<double>(N, N, this->rowMajor);
  cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans,
      N, N, N, 1.0, this->elements, N, this->elements, N,
      0.0, A->elements, N);
  A->copyTo(this);

}

template<class T>
void Matrix<T>::eig(Matrix<T>* V, Matrix<T>* d) {

}

// We need to specialize float/double because MKL handles each with a different
// function
template<>
void Matrix<float>::eig(Matrix<float>* V, Matrix<float>* d) {
  std::cout << "Error in Matrix::eig: eig() for floats is not implemented yet" << std::endl;
  exit(EXIT_FAILURE);
}

template<>
void Matrix<double>::eig(Matrix<double>* V, Matrix<double>* d) {
  if (this->rowMajor) {
    std::cout << "Error in Matrix::eig: eig() is only implemented for column major matrices." << std::endl;
    exit(EXIT_FAILURE);
  }

  this->copyTo(V);
  MKL_INT info;
  info = LAPACKE_dsyev( LAPACK_COL_MAJOR,
                        'V', 'U',
                        this->height, V->getElements(),
                        this->height, d->getElements() );
  if (info != 0) {
    std::cout << "Error, eig returned error code " << info << std::endl;
    exit(EXIT_FAILURE);
  }
}


//Matrix* Matrix::qr() {
  //Matrix* Q = new Matrix(this->height, this->width, this->rowMajor);
  //this->copyTo(Q);

  //culaDouble* tau = new culaDouble[std::min(Q->h(), Q->w())];
  //culaStatus status = culaDgeqrf(Q->h(), Q->w(), Q->getElements(), Q->h(), tau);
  //checkCulaStatus(status);
  //status = culaDorgqr(Q->h(), Q->w(), Q->w(), Q->getElements(), Q->h(), tau);
  //checkCulaStatus(status);

  //delete [] tau;

  //return Q;
//}

template<class T>
void Matrix<T>::qrInPlace() {

}

template<>
void Matrix<float>::qrInPlace() {
  std::cout << "Error in Matrix::qrInPlace(): qrInPlace() for floats is not implemented yet" << std::endl;
  exit(EXIT_FAILURE);
}

template<>
void Matrix<double>::qrInPlace() {
  double* tau = new double[std::min(this->h(), this->w())];
  MKL_INT info;
  info = LAPACKE_dgeqrf(CblasColMajor, 
    this->h(), this->w(), this->getElements(), 
    this->h(), tau);
  if (info != 0) {
    std::cout << "Error, dgeqrf in qrInPlace() returned error code " << info << std::endl;
    exit(EXIT_FAILURE);
  }

  info = LAPACKE_dorgqr(CblasColMajor,
      this->h(), this->w(), this->w(), this->getElements(), this->h(), tau);
  if (info != 0) {
    std::cout << "Error, dorgqr in qrInPlace() returned error code " << info << std::endl;
    exit(EXIT_FAILURE);
  }

  delete [] tau;
}

template<class T>
void Matrix<T>::print() {
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

template<class T>
void Matrix<T>::writeToFile(std::string filename) {
  std::ofstream f;
  f.open(filename.c_str());
  for (int i=0; i<this->height; i++) {
    for (int j=0; j<this->width; j++) {
      if (this->rowMajor) {
        f << std::setprecision(32) << this->elements[this->width*i+j] << " ";
      } else {
        f << std::setprecision(32) << this->elements[this->height*j+i] << " ";
      }
    }
    f << "\n";
  }
  
  f.close();

}

template<class T>
void Matrix<T>::fillRandom() {
  for (int i=0; i<(this->height*this->width); i++) {
    this->elements[i] = (T)((double)rand() / (double)RAND_MAX);
  }
}

template<class T>
Matrix<T>::~Matrix() {
  delete [] this->elements;
}

template class Matrix<float>;
template class Matrix<double>;
