#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

int main() {
    int board_w = 10;
    int board_h = 7;
    Size boardSize(board_w, board_h);
    float square_size = 15.0f;

    vector<Point3f> single_objp;
    for (int i = 0; i < board_h; i++) {
        for (int j = 0; j < board_w; j++) {
            single_objp.push_back(Point3f(j * square_size, i * square_size, 0));
        }
    }

    vector<vector<Point3f>> all_objp;
    vector<vector<Point2f>> all_imgp;
    vector<string> img_paths;


    string folder_path = "/home/nq/my_calib_data/cam_mix_img/"; 
    int success_count = 0;
    Size imgSize;

    for (const auto& entry : fs::directory_iterator(folder_path)) {
        string path = entry.path().string();
        if (path.find(".jpg") == string::npos && path.find(".png") == string::npos) continue;

        Mat img = imread(path);
        if (img.empty()) continue;
        
        if (imgSize.area() == 0) imgSize = img.size(); 

        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);

        vector<Point2f> corners;
        // 找角点
        bool found = findChessboardCorners(gray, boardSize, corners);

        if (found) {
            // 亚像素精化
            cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1),
                         TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 30, 0.1));
            
            // 保存第一张带彩色点/线的图
            if (success_count == 0) { 
                Mat img_color = img.clone();
                drawChessboardCorners(img_color, boardSize, corners, found);
                imwrite("corners.jpg", img_color);
            }

            all_objp.push_back(single_objp);
            all_imgp.push_back(corners);
            img_paths.push_back(path);
            success_count++;
        }
    }
    cout << "成功找到角点的图片数量: " << success_count << endl;
    if (success_count < 6) {
        cout << "图片太少，无法标定！" << endl;
        return -1;
    }

    // 3. 第一次标定
    Mat cameraMatrix, distCoeffs;
    vector<Mat> rvecs, tvecs;
    double rms = calibrateCamera(all_objp, all_imgp, imgSize, cameraMatrix, distCoeffs, rvecs, tvecs);
    
    // 4. 算误差，剔除最差的 3 张图
    vector<double> per_view_errors;
    for (size_t i = 0; i < all_objp.size(); i++) {
        vector<Point2f> projected;
        projectPoints(all_objp[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, projected);
        double err = norm(all_imgp[i], projected, NORM_L2) / projected.size();
        per_view_errors.push_back(err);
    }

    vector<size_t> idx(all_objp.size());
    for (size_t i = 0; i < idx.size(); i++) idx[i] = i;
    sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        return per_view_errors[a] > per_view_errors[b];
    });

    // 剔除最差的 3 张
    for (int k = 0; k < 3; k++) {
        cout << "剔除错误图: " << img_paths[idx[k]] << " (误差: " << per_view_errors[idx[k]] << ")" << endl;
        all_objp.erase(all_objp.begin() + idx[k]);
        all_imgp.erase(all_imgp.begin() + idx[k]);
    }

    double final_rms = calibrateCamera(all_objp, all_imgp, imgSize, cameraMatrix, distCoeffs, rvecs, tvecs);
    cout << "=======================" << endl;
    cout << "最终标定 RMS 误差: " << final_rms << endl;
    cout << "内参矩阵 K: \n" << cameraMatrix << endl;
    cout << "畸变系数 D: \n" << distCoeffs << endl;

    Mat test_img = imread(img_paths[0]);
    Mat after;
    undistort(test_img, after, cameraMatrix, distCoeffs);
    Mat compare;
    hconcat(test_img, after, compare);
    imwrite("undistort_compare.jpg", compare);
    cout << "对比图已保存为 undistort_compare.jpg" << endl;

    return 0;
}