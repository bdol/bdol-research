#ifndef DPP_H
#define DPP_H

#include "Matrix.h"
#include <vector>

class DPP {
  public:
    DPP(Matrix<double>* L);
    Matrix<double>* sample();

    ~DPP();

  private:
    Matrix<double>* V_full;
    Matrix<double>* d;


};

#endif
