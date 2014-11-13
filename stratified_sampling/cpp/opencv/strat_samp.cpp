#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <time.h>
#include <vector>
#include "kmeans.h"

using namespace std;

// Use the Knuth shuffle algorithm to sample num_samples points out of the set
// {0, ..., n-1}
vector<int> sampleWithoutReplacement(int n, int num_samples) {
  vector<int> a(n, 0);
  for (int i=0; i<n; i++) {
    a[i] = i;
  }

  int mx = n-1;
  while (mx > 0) {
    int r = (int)( (double)mx*((double) rand() / (RAND_MAX)) );
    int tmp = a[mx];
    a[mx] = a[r];
    a[r] = tmp;
    mx--;
  }

  return a;
}

int main( int argc, char** argv ) {
  srand(time(NULL));

  // Number of points on each side of the grid
  int gridNum = atoi(argv[1]);
  int nPoints = gridNum*gridNum;
  // Number clusters
  int k = atoi(argv[2]);
  // Total number of samples
  int N = atoi(argv[3]);

  if (N%k != 0) {
    std::cout << "Error: N%k!=0. Exiting." << std::endl;
    exit(EXIT_FAILURE);
  }
  int numPerCluster = N/k;

  // Generate the grid of points
  cv::Mat X(nPoints, 2, cv::DataType<float>::type);
  for (int x=0; x<gridNum; x++) {
    for (int y=0; y<gridNum; y++) {
      X.at<float>(x+y*gridNum, 0) = (float)x;
      X.at<float>(x+y*gridNum, 1) = (float)y;
    }
  }

  // Cluster
  cv::Mat labels;
  cv::kmeans(X, k, labels,
              cv::TermCriteria(CV_TERMCRIT_ITER, 10, 1.0),
              1, cv::KMEANS_PP_CENTERS);

  // Stratified sampling
  cv::Mat samples(N, 1, cv::DataType<int>::type);
  for (int i=0; i<k; i++) {
    // The logical indices of the points in this cluster
    cv::Mat kidx = labels==i;
    // The matrix that holds the actual index values
    cv::Mat pidx;
    // This is an undocumented feature of OpenCV which goes against the
    // philosophy of passing masks instead of indices. However, I need the
    // actual indices in this case.
    cv::findNonZero(kidx, pidx);

    // Sample numPerCluster points from each cluster
    int numIndices = pidx.rows;
    vector<int> a = sampleWithoutReplacement(numIndices, numPerCluster);
    for (int j=0; j<numPerCluster; j++) {
      samples.at<int>(i*numPerCluster+j) = pidx.at<int>(a[j], 1);
    }

  }


  ofstream f;
  f.open("opencv_samples.txt");
  for (int i=0; i<samples.rows; i++) {
    f << samples.at<int>(i) << endl;
  }
  f.close();

  // Do the same thing with the standalone kmeans code
  float** objects = (float**)malloc(nPoints * sizeof(float*));
  objects[0] = (float*) malloc(nPoints*2 * sizeof(float));
  for (int i=1; i<nPoints; i++) {
    objects[i] = objects[i-1] + 2;
  }

  for (int x=0; x<gridNum; x++) {
    for (int y=0; y<gridNum; y++) {
      objects[x+y*gridNum][0] = (float)x/(float)gridNum;
      objects[x+y*gridNum][1] = (float)y/(float)gridNum;
    }
  }


  float** clusters;
  float threshold = 1E-8;
  int loopIterations;
  int* membership = (int*) malloc(nPoints * sizeof(int));
  clusters = seq_kmeans(objects, 2, nPoints, k, threshold,
                          membership, &loopIterations);

  free(objects[0]);
  free(objects);


  // Sample numPerCluster from each cluster
  vector<int> kmeansSamples;
  for (int i=0; i<k; i++) {
    vector<int> clusterIdx;
    int nCluster = 0;
    for (int j=0; j<nPoints; j++) {
      if (membership[j] == i) {
        clusterIdx.push_back(j);
        nCluster++;
      }
    }

    vector<int> sampleIdx = sampleWithoutReplacement(nCluster, numPerCluster);
    for (int j=0; j<numPerCluster; j++) {
      kmeansSamples.push_back( clusterIdx[sampleIdx[j]] );
    }

  }

  ofstream fk;
  fk.open("kmeans_samples.txt");
  for (int i=0; i<N; i++) {
    fk << kmeansSamples[i] << endl;
  }
  fk.close();

  free(membership);

  return 0;
}
