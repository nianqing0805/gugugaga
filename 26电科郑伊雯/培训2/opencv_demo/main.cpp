#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc,char** argv) {
    std::string imagePath = "test.jpeg";

    if(argc > 1){
        imagePath = argv[1];
    }
    cv::Mat img = cv ::imread(imagePath);
    if (img.empty()){
        std::cout << "Image not found or failed to load!" << std::endl;
        return -1;
    }
    cv::imshow("My Test Image", img);
    std::cout << "Press any key on the keyboard to close the window..." << std::endl;
    cv::waitKey(0);

    cv::destroyAllWindows();
    
    return 0;
}