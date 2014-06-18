# set LD_LIBRARY_PATH
export CC  = gcc
export CXX = g++
export NVCC =nvcc

export CFLAGS = -Wall -g -O3 -msse3 -Wno-unknown-pragmas -funroll-loops -I./mshadow/ -I./dpp/ -I../../ -I/projects/grail/local/pkg/intel/mkl/include -I/usr/local/cuda-5.0/include -I/homes/grail/bdol/h/code/include `pkg-config --cflags /homes/grail/bdol/h/code/lib/pkgconfig/opencv.pc`

ifeq ($(blas),1)
 LDFLAGS= -lm -lcudart -lcublas -lcurand -lz `pkg-config --libs opencv` -lblas
 CFLAGS+= -DMSHADOW_USE_MKL=0 -DMSHADOW_USE_CBLAS=1
else
 LDFLAGS= -L/usr/local/cuda-5.0/lib64 -L/projects/grail/local/pkg/intel/mkl/lib/em64t  -L/homes/grail/bdol/h/code/lib -L./dpp -lm -lcudart -lcublas -lmkl_core -lmkl_intel_lp64 -lmkl_intel_thread -liomp5 -lpthread -lcurand -lz ./dpp/liblapacke.a `pkg-config --libs /homes/grail/bdol/h/code/lib/pkgconfig/opencv.pc`
endif
 
export NVCCFLAGS = --use_fast_math -g -O3 -ccbin $(CXX)

# specify tensor path
BIN = bin/cxxnet
OBJ = matrix.o dpp.o cxxnet_data.o cxxnet_nnet_cpu.o
CUOBJ = cxxnet_nnet_gpu.o
CUBIN =
.PHONY: clean all

all: $(BIN) $(OBJ) $(CUBIN) $(CUOBJ)

matrix.o: dpp/Matrix.cpp
dpp.o: matrix.o dpp/DPP.cpp 
cxxnet_nnet_gpu.o: cxxnet/nnet/cxxnet_nnet.cu cxxnet/core/*.hpp cxxnet/core/*.h cxxnet/nnet/*.hpp cxxnet/nnet/*.h 
cxxnet_nnet_cpu.o: cxxnet/nnet/cxxnet_nnet.cpp cxxnet/core/*.hpp cxxnet/core/*.h cxxnet/nnet/*.hpp cxxnet/nnet/*.h
cxxnet_data.o: cxxnet/io/cxxnet_data.cpp cxxnet/io/*.hpp cxxnet/utils/cxxnet_io_utils.h
bin/cxxnet: cxxnet/cxxnet_main.cpp matrix.o dpp.o cxxnet_data.o cxxnet_nnet_cpu.o cxxnet_nnet_gpu.o
$(BIN) :
	$(CXX) $(CFLAGS)  -o $@ $(filter %.cpp %.o %.c, $^) $(LDFLAGS)

$(OBJ) :
	$(CXX) -c $(CFLAGS) -o $@ $(firstword $(filter %.cpp %.c, $^) )

$(CUOBJ) :
	$(NVCC) -c -o $@ $(NVCCFLAGS) -Xcompiler "$(CFLAGS)" $(filter %.cu, $^)
$(CUBIN) :
	$(NVCC) -o $@ $(NVCCFLAGS) -Xcompiler "$(CFLAGS)" -Xlinker  $(filter %.cu %.cpp %.o, $^) "$(LDFLAGS)"

clean:
	$(RM) $(OBJ) $(BIN) $(CUBIN) $(CUOBJ) *~


