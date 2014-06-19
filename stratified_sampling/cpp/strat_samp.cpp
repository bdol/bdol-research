#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>

using namespace std;

int main( int argc, char** argv ) {
  srand(time(NULL));


  // Number of points on each side of the grid
  int gridNum = atoi(argv[1]);
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
  cv::Mat X(gridNum*gridNum, 2, cv::DataType<float>::type);
  for (int x=0; x<gridNum; x++) {
    for (int y=0; y<gridNum; y++) {
      X.at<float>(x+y*gridNum, 1) = (float)x;
      X.at<float>(x+y*gridNum, 2) = (float)y;
    }
  }

  // Cluster
  cv::Mat labels;
  cv::kmeans(X, k, labels,
              cv::TermCriteria(CV_TERMCRIT_ITER, 10, 1.0),
              1, cv::KMEANS_PP_CENTERS);

  // Stratified sampling
  cv::Mat samples(k, 1, cv::DataType<int>::type);
  for (int i=0; i<k; i++) {
    // The logical indices of the points in this cluster
    cv::Mat kidx = labels==i;
    // The matrix that holds the actual index values
    cv::Mat pidx;
    // This is an undocumented feature of OpenCV which goes against the
    // philosophy of passing masks instead of indices. However, I need the
    // actual indices in this case.
    cv::findNonZero(kidx, pidx);

    // Sample a point
    int numIndices = pidx.rows;
    double r = ((double) rand() / (RAND_MAX));
    int idx = floor(r*(double)numIndices);
    samples.at<int>(i) = pidx.at<int>(idx, 1);
  }


  ofstream f;
  f.open("samples.txt");
  for (int i=0; i<samples.rows; i++) {
    f << samples.at<int>(i) << endl;
  }
  f.close();

  return 0;
}
