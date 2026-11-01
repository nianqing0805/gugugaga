#include <opencv2/opencv.hpp>
#include <iostream>

int main(){
    cv::Mat img = cv ::imread("test.jpeg");
    if (img.empty()){
        std::cout << "Image not found or failed to load!" << std::endl;
        return -1;
    }
    cv::imshow("My Test Image", img);
    std::cout << "Press any key on the keyboard to close the window..." << std::endl;
    cv::waitKey(0);
    return 0;
}