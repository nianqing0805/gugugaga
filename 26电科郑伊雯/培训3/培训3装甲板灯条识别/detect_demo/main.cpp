#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace cv;
using namespace cv::dnn;
using namespace std;

static const int    INPUT_W = 640;               // 网络输入宽
static const int    INPUT_H = 640;               // 网络输入高
static const float  CONF_THRESHOLD = 0.35f;      // 粗定位置信度阈值
static const float  NMS_THRESHOLD  = 0.45f;      // NMS 去重阈值
static const float  LIGHT_BAR_MIN_RATIO = 2.5f;  // 灯条最小长宽比（
static const double LIGHT_BAR_MIN_AREA  = 40.0;  // 内灯条最小面积
static const int    ROI_MARGIN = 10;             

static const int H_R1_L = 0,   H_R1_H = 10;      
static const int H_R2_L = 156, H_R2_H = 180;     
static const int S_MIN = 43, V_MIN = 46;

static Mat letterbox(const Mat& img, int tw, int th, float& scale, int& padX, int& padY) {
    float s = min((float)tw / img.cols, (float)th / img.rows);
    int nw = max(1, cvRound(img.cols * s));
    int nh = max(1, cvRound(img.rows * s));
    Mat resized;
    resize(img, resized, Size(nw, nh));
    Mat canvas(th, tw, img.type(), Scalar(114, 114, 114));
    padX = (tw - nw) / 2;
    padY = (th - nh) / 2;
    resized.copyTo(canvas(Rect(padX, padY, nw, nh)));
    scale = s;
    return canvas;
}

// 画十字星
static void drawCross(Mat& img, const Point2f& p, const Scalar& c, int half = 6) {
    int x = cvRound(p.x), y = cvRound(p.y);
    if (x < 0 || y < 0 || x >= img.cols || y >= img.rows) return;
    line(img, Point(max(0, x - half), y), Point(min(img.cols - 1, x + half), y), c, 1, LINE_AA);
    line(img, Point(x, max(0, y - half)), Point(x, min(img.rows - 1, y + half)), c, 1, LINE_AA);
}

struct Quad { Point2f p1, p2, p3, p4; };

static void sidePoints(const RotatedRect& r, bool left, Point2f& top, Point2f& bottom) {
    Point2f pts[4];
    r.points(pts);
    if (left)
        sort(pts, pts + 4, [](const Point2f& a, const Point2f& b) { return a.x < b.x; });
    else
        sort(pts, pts + 4, [](const Point2f& a, const Point2f& b) { return a.x > b.x; });
    // pts[0]、pts[1] 就是最左(或最右)的两个点
    if (pts[0].y < pts[1].y) { top = pts[0]; bottom = pts[1]; }
    else                     { top = pts[1]; bottom = pts[0]; }
}

static vector<RotatedRect> findLightBars(const Mat& roi) {
    vector<RotatedRect> bars;

    Mat hsv, m1, m2, mask;
    cvtColor(roi, hsv, COLOR_BGR2HSV);
    inRange(hsv, Scalar(H_R1_L, S_MIN, V_MIN), Scalar(H_R1_H, 255, 255), m1);
    inRange(hsv, Scalar(H_R2_L, S_MIN, V_MIN), Scalar(H_R2_H, 255, 255), m2);
    mask = m1 | m2;

    Mat k = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(mask, mask, MORPH_CLOSE, k);
    dilate(mask, mask, k);

    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area < LIGHT_BAR_MIN_AREA) continue;

        RotatedRect r = minAreaRect(contours[i]);
        float rw = r.size.width, rh = r.size.height;
        if (rw > rh) { float t = rw; rw = rh; rh = t; }   
        if (rh < rw * LIGHT_BAR_MIN_RATIO) continue;        

        Point2f p[4];
        r.points(p);
        float minX = p[0].x, maxX = p[0].x, minY = p[0].y, maxY = p[0].y;
        for (int j = 1; j < 4; j++) {
            minX = min(minX, p[j].x); maxX = max(maxX, p[j].x);
            minY = min(minY, p[j].y); maxY = max(maxY, p[j].y);
        }
        if ((maxY - minY) < (maxX - minX)) continue;

        bars.push_back(r);
    }

    sort(bars.begin(), bars.end(), [](const RotatedRect& a, const RotatedRect& b) {
        return a.size.area() > b.size.area();
    });
    if (bars.size() > 2) bars.resize(2);

    sort(bars.begin(), bars.end(), [](const RotatedRect& a, const RotatedRect& b) {
        return a.center.x < b.center.x;
    });
    return bars;
}

int main(int argc, char** argv) {
    string modelPath = "armor.onnx";
    string videoPath = "train3.mp4";
    if (argc >= 2) modelPath = argv[1];
    if (argc >= 3) videoPath = argv[2];

    // 1. 加载 ONNX 模型
    Net net = readNetFromONNX(modelPath);
    if (net.empty()) { cerr << "[ERR] 模型加载失败: " << modelPath << endl; return -1; }
    net.setPreferableBackend(DNN_BACKEND_DEFAULT);
    net.setPreferableTarget(DNN_TARGET_CPU);

    
    VideoCapture cap(videoPath);
    if (!cap.isOpened()) { cerr << "[ERR] 视频打开失败: " << videoPath << endl; return -1; }

    Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // 神经网络
        float scale; int padX, padY;
        Mat input = letterbox(frame, INPUT_W, INPUT_H, scale, padX, padY);
        Mat blob = blobFromImage(input, 1.0 / 255.0, Size(INPUT_W, INPUT_H), Scalar(), true, true);
        net.setInput(blob);
        Mat out = net.forward();
        if (out.type() != CV_32F) { Mat t; out.convertTo(t, CV_32F); out = t; }

        int dims = out.dims;
        const float* data = (float*)out.data;
        int N = 0, predLen = 0;
        bool transposed = false;
        if (dims == 3) {
            int d1 = (int)out.size[1], d2 = (int)out.size[2];
            if (d2 > d1) { transposed = true; predLen = d1; N = d2; }
            else         { predLen = d2; N = d1; }
        } else if (dims == 2) {
            int r = (int)out.size[0], c = (int)out.size[1];
            if (c > r) { transposed = true; predLen = r; N = c; }
            else       { predLen = c; N = r; }
        } else {
            cerr << "[ERR] 输出维度异常" << endl;
            break;
        }
        int nc = predLen - 5;
        if (nc < 1) { cerr << "[ERR] 输出通道异常" << endl; break; }

        vector<Rect2f> boxes;
        vector<float> confs;
        for (int i = 0; i < N; i++) {
            float cx, cy, bw, bh, obj;
            float bestCls = -1.0f;
            if (!transposed) {
                const float* p = data + (size_t)i * predLen;
                cx = p[0]; cy = p[1]; bw = p[2]; bh = p[3]; obj = p[4];
                for (int c = 0; c < nc; c++) { float s = p[5 + c]; if (s > bestCls) bestCls = s; }
            } else {
                cx = data[0 * N + i]; cy = data[1 * N + i];
                bw = data[2 * N + i]; bh = data[3 * N + i]; obj = data[4 * N + i];
                for (int c = 0; c < nc; c++) { float s = data[(5 + c) * N + i]; if (s > bestCls) bestCls = s; }
            }
            float conf = obj * bestCls;
            if (conf < CONF_THRESHOLD) continue;

            float x1 = (cx - bw / 2.0f - padX) / scale;
            float y1 = (cy - bh / 2.0f - padY) / scale;
            float x2 = (cx + bw / 2.0f - padX) / scale;
            float y2 = (cy + bh / 2.0f - padY) / scale;
            boxes.push_back(Rect2f(x1, y1, x2 - x1, y2 - y1));
            confs.push_back(conf);
        }
）
        vector<Rect> intBoxes;
        for (size_t i = 0; i < boxes.size(); i++) {
            const Rect2f& b = boxes[i];
            intBoxes.push_back(Rect(cvRound(b.x), cvRound(b.y),
                                    cvRound(b.width), cvRound(b.height)));
        }
        vector<int> keep;
        NMSBoxes(intBoxes, confs, CONF_THRESHOLD, NMS_THRESHOLD, keep);

        for (size_t k = 0; k < keep.size(); k++) {
            Rect2f b = boxes[keep[k]];
            int x = max(0, cvRound(b.x) - ROI_MARGIN);
            int y = max(0, cvRound(b.y) - ROI_MARGIN);
            int x2 = min(frame.cols, cvRound(b.x + b.width) + ROI_MARGIN);
            int y2 = min(frame.rows, cvRound(b.y + b.height) + ROI_MARGIN);
            int w = x2 - x, h = y2 - y;
            if (w <= 0 || h <= 0) continue;

            Mat roi = frame(Rect(x, y, w, h));
            vector<RotatedRect> bars = findLightBars(roi);
            if (bars.size() < 2) continue;

            RotatedRect lb = bars[0], rb = bars[1];
            lb.center.x += x; lb.center.y += y;   // ROI 坐标还原到全图
            rb.center.x += x; rb.center.y += y;

            //取四个顶点
            Quad q;
            sidePoints(lb, true,  q.p1, q.p4);   // 左灯条：最左两点 → 左上 P1、左下 P4
            sidePoints(rb, false, q.p2, q.p3);   // 右灯条：最右两点 → 右上 P2、右下 P3

            Point2f lp[4], rp[4];
            lb.points(lp); rb.points(rp);
            vector<Point> lv, rv;
            for (int i = 0; i < 4; i++) {
                lv.push_back(Point(cvRound(lp[i].x), cvRound(lp[i].y)));
                rv.push_back(Point(cvRound(rp[i].x), cvRound(rp[i].y)));
            }
            polylines(frame, lv, true, Scalar(0, 255, 255), 2, LINE_AA);  // 黄色：灯条外框
            polylines(frame, rv, true, Scalar(0, 255, 255), 2, LINE_AA);

            vector<Point> quad = {
                Point(cvRound(q.p1.x), cvRound(q.p1.y)),
                Point(cvRound(q.p2.x), cvRound(q.p2.y)),
                Point(cvRound(q.p3.x), cvRound(q.p3.y)),
                Point(cvRound(q.p4.x), cvRound(q.p4.y))
            };
           
            line(frame, Point(cvRound(q.p1.x), cvRound(q.p1.y)), Point(cvRound(q.p2.x), cvRound(q.p2.y)), Scalar(0, 255, 0), 1);
            line(frame, Point(cvRound(q.p2.x), cvRound(q.p2.y)), Point(cvRound(q.p3.x), cvRound(q.p3.y)), Scalar(0, 255, 0), 1);
            line(frame, Point(cvRound(q.p3.x), cvRound(q.p3.y)), Point(cvRound(q.p4.x), cvRound(q.p4.y)), Scalar(0, 255, 0), 1);
            line(frame, Point(cvRound(q.p4.x), cvRound(q.p4.y)), Point(cvRound(q.p1.x), cvRound(q.p1.y)), Scalar(0, 255, 0), 1);

            auto drawStar = [&](Point pt) {
                circle(frame, pt, 3, Scalar(0, 255, 0), -1); // 中心实心点
                line(frame, Point(pt.x - 8, pt.y), Point(pt.x + 8, pt.y), Scalar(0, 255, 0), 1); // 横线
                line(frame, Point(pt.x, pt.y - 8), Point(pt.x, pt.y + 8), Scalar(0, 255, 0), 1); // 竖线
            };
            drawStar(Point(cvRound(q.p1.x), cvRound(q.p1.y)));
            drawStar(Point(cvRound(q.p2.x), cvRound(q.p2.y)));
            drawStar(Point(cvRound(q.p3.x), cvRound(q.p3.y)));
            drawStar(Point(cvRound(q.p4.x), cvRound(q.p4.y)));

           imshow("armor", frame);
           if (waitKey(1) == 27) break;  // ESC 退出
          }
        }

           destroyAllWindows();
           return 0;
}