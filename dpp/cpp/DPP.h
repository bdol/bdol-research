#ifndef DPP_H
#define DPP_H

#include "Matrix.h"
#include <vector>

class DPP {
  public:
    DPP(Matrix<double>* L);
    Matrix<double>* sample();
    Matrix<double>* sample(int k);

    ~DPP();

  private:
    Matrix<double>* sample_dpp(int k, std::vector<int> v_idx);
    Matrix<double>* esym_poly(int k);

    Matrix<double>* V_full;
    Matrix<double>* d;


};

#endif
