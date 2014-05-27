#ifndef DPP_H
#define DPP_H

#include "Matrix.h"
#include <vector>

template<class T>
class DPP {
  public:
    DPP(Matrix<T>* L);
    Matrix<T>* sample();
    Matrix<T>* sample(int k);

    ~DPP();

  private:
    Matrix<T>* sample_dpp(int k, std::vector<int> v_idx);
    Matrix<T>* esym_poly(int k);

    Matrix<T>* V_full;
    Matrix<T>* d;
};

#endif
