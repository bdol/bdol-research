CC=g++
CFLAGS =  -O3 -DADD_ -DCUBLAS_GFORTRAN -fopenmp -m64 -DHAVE_CUBLAS -DGPUSHMEM=200
CFLAGS += -I/usr/local/cuda-5.0/include
CFLAGS += -I/projects/grail/local/pkg/intel/mkl/include

LIB += -L/projects/grail/local/pkg/intel/mkl/lib/em64t
LIB += -lmkl_gf_lp64 -lmkl_gnu_thread -lmkl_core -ldl -lpthread -lm -fopenmp
LIB += liblapacke.a

SRC = DPP.cpp Matrix.cpp planes.cpp
OBJ = $(SRC:.cpp=.o)
EXE = planes


.cpp.o:
	$(CC) $(CFLAGS) -c $*.cpp

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LIB) 
