#include <iostream>
#include <fstream>
#include <string>
#include "../include/matSerial.hpp"

using namespace std;
using namespace cv;

std::vector<unsigned char> matWrite(const Mat& mat)
{

    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(90);
    vector<unsigned char> buf;
    cv::imencode(".jpg", _InputArray(mat), buf ,params);
    return buf;
}


Mat *matRead(std::vector<unsigned char> buf)
{
    cv::Mat *a = new cv::Mat();
    cv::imdecode(buf, CV_LOAD_IMAGE_COLOR , a);
    return a;
}
