#include "DPP.h"

template<class T>
DPP<T>::DPP(Matrix<T>* L) {
  // Decompose the kernel and store the eigendecomposition
  this->V_full = new Matrix<T>(L->h(), L->w(), false);
  this->d = new Matrix<T>(L->h(), 1, false);

  L->eig(this->V_full, this->d);
}

template<class T>
Matrix<T>* DPP<T>::sample() {
  int k = 0;

  // Convert the L eigenvalues to K eigenvalues, and sample eigenvectors
  std::vector<int> v_idx;
  for (int i=0; i<this->d->h(); i++) {
    T d_K = this->d->get(i, 0);
    d_K = d_K/(1+d_K);
    T r = (T)((double)rand() / RAND_MAX);
    if (r <= d_K) {
      v_idx.push_back(i);
      k++;
    }
  }
  
  return this->sample_dpp(k, v_idx);
}

template<class T>
Matrix<T>* DPP<T>::sample(int k) {
  Matrix<T>* E = this->esym_poly(k);
  std::vector<int> v_idx;

  int remaining = k-1;
  int i = this->d->h()-1;
  int j = 0;

  while (remaining >= 0) {
    double marg = 0.0;
    if (i==remaining) {
      marg = 1.0;
    } else {
      if (E->get(remaining+1, i+1) == 0) {
        i--;
        continue;
      }

      marg = this->d->get(i, 0) * E->get(remaining, i)/E->get(remaining+1, i+1); 
    }

    double r = (double)rand() / (double)RAND_MAX;
    if (r < marg) {
      v_idx.push_back(i);
      j++;
      remaining--;
    }

    i--;
  }

  delete E;
  return this->sample_dpp(k, v_idx);
}

template<class T>
Matrix<T>* DPP<T>::sample_dpp(int k, std::vector<int> v_idx) {
  Matrix<T>* V = new Matrix<T>(this->V_full->h(), k, false);
  int j = 0;
  for (std::vector<int>::iterator it = v_idx.begin(); it != v_idx.end(); ++it) {
    this->V_full->copyColumnTo(*it, V, j);
    j++;
  }

  Matrix<T>* Y = new Matrix<T>(k, 1, false);
  Matrix<T>* Pr = new Matrix<T>(V->h(), 1, false);
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

      Matrix<T>* Vj = new Matrix<T>(V->h(), 1, false);
      V->copyColumnTo(colIdx, Vj, 0);
      V->deleteColumn(colIdx);

      Matrix<T>* Vi = new Matrix<T>(1, V->w(), false);
      V->copyRowTo(sampleIdx, Vi, 0);
      Vi->elemWiseDivideScalar(Vj->get(sampleIdx, 0));

      Matrix<T>* Vshift = Vj->matMul(Vi);
      V->elemWiseMinus(Vshift);

      V->qrInPlace();

      delete Vj;
      delete Vi;
      delete Vshift;
    }
  }

  delete Pr;
  delete V;

  return Y;
}

template<class T>
Matrix<T>* DPP<T>::esym_poly(int k) {
  int N = this->d->h();
  Matrix<T>* E = new Matrix<T>(k+1, N+1, false);
  for (int i=0; i<E->w(); i++) {
    E->set(0, i, 1.0);
  }

  for (int i=1; i<k+1; i++) {
    for (int j=1; j<N+1; j++) {
      double e = E->get(i, j-1) + this->d->get(j-1, 0)*E->get(i-1, j-1);
      E->set(i, j, e);
    }
  }

  return E;
}

template<class T>
DPP<T>::~DPP() {
  delete V_full;
  delete d;
}

template class DPP<float>;
template class DPP<double>;

