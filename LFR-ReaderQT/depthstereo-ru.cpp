#include "depthstereo.h"

void DepthStereo::calculateDepth(){
    int u = 15, v = 15;
    int sub_w = input_img.cols / u;
    int sub_h = input_img.rows / v;

    cv::StereoBM stereo;
    cv::Mat input_gray;
    cv::Mat output_gray;
    cvtColor(input_img, input_gray, CV_RGB2GRAY);

    // Настройте прямоугольник, чтобы определить интересующую вас область.
    int y = static_cast<int>(v/2.0);
    for(int x = 2; x<4; x++){
        cv::Mat left_image = input_gray(cv::Rect(sub_w*x, sub_h*y, sub_w, sub_h)); //Обратите внимание, что это не копирует данные
        cv::Mat right_image = input_gray(cv::Rect(sub_w*(u-x), sub_h*y, sub_w, sub_h)); // Обратите внимание, что это не копирует данные

        if (output_gray.empty())
            stereo(left_image, right_image, output_img);
        stereo(left_image, right_image, output_gray);

        for (int y = 0; y < output_gray.rows; ++y) {
            for (int x = 0; x < output_gray.cols; ++x) {
                if (output_img.at<uchar>(y,x) == 0)
                    output_img.at<uchar>(y,x) = output_gray.at<uchar>(y,x);
            }
        }
    }
}
