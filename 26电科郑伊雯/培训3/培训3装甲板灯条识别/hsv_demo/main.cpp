#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

int main(){
  cv::Mat img = cv::imread("assets/armor.png");
  if(img.empty()){
    std::cerr << "无法读取图片，请检查路径!" << std::endl;
    return -1;
  }

  cv::Mat hsv;
  cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

  std::vector<cv::Mat> channels;
  cv::split(hsv, channels);

  cv::Mat h_channel = channels[0];
  cv::Mat s_channel = channels[1];
  cv::Mat v_channel = channels[2];

  cv::imshow("H Channel", h_channel);
  cv::imshow("S Channel", s_channel);
  cv::imshow("V Channel", v_channel);
  
  cv::Mat m1, m2, mask;
  int s_min = 100;
  int v_min = 70;

  cv::inRange(hsv, cv::Scalar(0, s_min, v_min), cv::Scalar(10, 255, 255), m1);
  cv::inRange(hsv, cv::Scalar(170, s_min, v_min), cv::Scalar(180, 255, 255),m2);
  cv::bitwise_or(m1, m2, mask);

  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
  cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

  cv::imshow("Red Mask", mask);
  cv::imwrite("red_mask.png",mask);
  std::cout << "red_mask.png 已保存在当前目录!" << std::endl;

  cv::waitKey(0);
  return 0;
}