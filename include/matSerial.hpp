#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
using namespace std;
using namespace cv;

std::vector<unsigned char> matWrite(const Mat& mat);
Mat *matRead(std::vector<unsigned char> buf);
