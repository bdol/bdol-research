#include "DPP.h"

DPP::DPP(Matrix<double>* L) {
  // Decompose the kernel and store the eigendecomposition
  this->V_full = new Matrix<double>(L->h(), L->w(), false);
  this->d = new Matrix<double>(L->h(), 1, false);

  L->eig(this->V_full, this->d);
}

Matrix<double>* DPP::sample() {
  int k = 0;

  // Convert the L eigenvalues to K eigenvalues, and sample eigenvectors
  std::vector<int> v_idx(this->V_full->w(), 0.0);
  for (int i=0; i<this->d->h(); i++) {
    double d_K = this->d->get(i, 0);
    d_K = d_K/(1+d_K);
    double r = ((double)rand() / RAND_MAX);
    if (r <= d_K) {
      k++;
      v_idx[i] = 1.0;
    }
  }
  
  Matrix<double>* V = new Matrix<double>(this->V_full->h(), k, false);
  int j = 0;
  for (int i=0; i<this->V_full->w(); i++) {
    if (v_idx[i] == 1.0) {
      this->V_full->copyColumnTo(i, V, j);
      j++;
    }
  }

  Matrix<double>* Y = new Matrix<double>(k, 1, false);
  Matrix<double>* Pr = new Matrix<double>(V->h(), 1, false);
  for (int i=k-1; i>=0; i--) {

    // Form distribution over items
    double Pr_sum = 0.0;
    for (int a=0; a<Pr->h(); a++) {
      Pr->set(a, 0, 0.0);
      for (int b=0; b<V->w(); b++) {
        Pr->set(a, 0, Pr->get(a, 0)+pow(V->get(a, b), 2));
      }
      Pr_sum += Pr->get(a, 0);
    }

    for (int a=0; a<Pr->h(); a++) {
      Pr->set(a, 0, (Pr->get(a, 0))/Pr_sum);
    }

    // Sample an item from this distribution
    double r = ((double)rand() / RAND_MAX);
    double csum = 0.0;
    int sampleIdx = 0;
    for (int a=0; a<Pr->h(); a++) {
      csum += Pr->get(a, 0);
      if (r <= csum) {
        Y->set(i, 0, (double)a);
        sampleIdx = a;
        break;
      }
    }

    // Orthogonalize V
    if (i > 0) {
      int colIdx = 0;
      for (int j=0; j<V->w(); j++) {
        if (V->get(sampleIdx, j) != 0) {
          colIdx = j;
          break;
        }
      }

      //V->writeToFile("V_before_col_del.txt");
      Matrix<double>* Vj = new Matrix<double>(V->h(), 1, false);
      V->copyColumnTo(colIdx, Vj, 0);
      V->deleteColumn(colIdx);
      //Vj->writeToFile("Vj.txt");
      //V->writeToFile("V_col_del.txt");

      Matrix<double>* Vi = new Matrix<double>(1, V->w(), false);
      V->copyRowTo(sampleIdx, Vi, 0);
      //Vi->writeToFile("Vi_before_scale.txt");
      Vi->elemWiseDivideScalar(Vj->get(sampleIdx, 0));
      //Vi->writeToFile("Vi_after_scale.txt");

      Matrix<double>* Vshift = Vj->matMul(Vi); // TODO: this is giving weird values in some of the entries...?
      //Vshift->writeToFile("Vshift.txt");
      V->elemWiseMinus(Vshift);
      //V->writeToFile("V_after_minus.txt");

      V->qrInPlace(); // TODO: This is also giving weird values in some of the entries.
      //V->writeToFile("V_after_qr.txt");

      delete Vj;
      delete Vi;
      delete Vshift;

      //exit(EXIT_SUCCESS);
    }
  }

  delete Pr;
  delete V;

  return Y;
}

DPP::~DPP() {
  delete V_full;
  delete d;
}
