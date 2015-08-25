#include "depthfromfocus.h"


void DepthFromFocus::calculateDepth(){
    const int size_u = 15, size_v = 15;
    const int size_s = input_img.cols/size_u;
    const int size_t = input_img.rows/size_v;

    if (focusCost.empty())
        focusCost = createFocusVolume(size_u, size_v, this->focus_threshold, 1);

    int algorithm=1;

    cv::Mat cost_depth( size_t, size_s, CV_32FC1);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){


        // Shape from Focus
        // algorithm 1
        // coarse resolution depth estimation

        if (algorithm==1){
            float min_f = 9999, max_f = -9999;
            int min_d = size_d, max_d = 0;
            for( int d=0; d < size_d; d++){

                // get minimum and maximum focusCost
                float F_k = focusCost.at<float>(t,s,d);
                //avg_f += focus_cost;

                if (F_k > max_f){
                    max_f = F_k;
                    max_d = d;
                }
            }
            if (max_f < threshold && use_threshold)
                max_d = 0;
            cost_depth.at<float>(t, s) = max_d;
        }

        // Shape from Focus
        // algorithm 2
        // Gaussian interpolation
        else{
            int F_mm=0, F_m=0, F_mp=0, d_m = 0;
            for( int d=2; d < size_d; d++){
                float F_km2 = focusCost.at<float>(t,s,d-2);
                float F_km1 = focusCost.at<float>(t,s,d-1);
                float F_k = focusCost.at<float>(t,s,d);
                if (F_km1 > F_m && F_km1 > F_k && F_km1 > F_km2){
                    F_m = F_km1;
                    F_mm = F_km2;
                    F_mp = F_k;
                    d_m = d-1;
                }
            }
            int d_mm = d_m - 1;
            int d_mp = d_m + 1;

            //(14)
            float d_numerator = (std::log(F_m) - std::log(F_mp))*(std::pow(d_m,2) - std::pow(d_mm,2)) - (std::log(F_m) - std::log(F_mm))*(std::pow(d_m,2) - std::pow(d_mp,2));
            float d_denominator = 2*1*((std::log(F_m)-std::log(F_mm))+(std::log(F_m)-std::log(F_mp)));
            float d_ = d_numerator / d_denominator;

            //(15)
            float sigma_numerator = (std::pow(d_m,2) - std::pow(d_mm,2)) + (std::pow(d_m,2) - std::pow(d_mp,2));
            float sigma_denominator = 2*((std::log(F_m)-std::log(F_mm))+(std::log(F_m)-std::log(F_mp)));
            float sigma = std::sqrt(-sigma_numerator / sigma_denominator);

            //(16)
            float F_peak = F_m / std::exp(-0.5*std::pow((d_m-d_)/sigma,2));

            // does it belong to the background?
            float threshold_3 = this->focus_threshold, threshold_4 = max_variance;
            if (use_max_focus && F_peak < threshold_3
            || use_max_variance && sigma > threshold_4)
                cost_depth.at<float>(t, s) = size_d;
            else
                cost_depth.at<float>(t, s) = d_;
        }
    }
    output_img = cost_depth;
}

cv::Mat DepthFromFocus::createFocusedImage(const cv::Mat image, const int size_u, const int size_v, const float shift){
    int size_s = image.cols/size_u;
    int size_t = image.rows/size_v;
    cv::Mat focused_img = cv::Mat::zeros(size_t, size_s, CV_32FC3);
    cv::Mat shifted_img = cv::Mat::zeros(size_t, size_s, CV_32FC3);
    for (int v=0; v<size_v; v++)
    for (int u=0; u<size_u; u++){
        // (src1 * alpha + src2 * beta + gamma)
        int x = u * size_s + static_cast<int>(shift*(u-size_u/2)+0.5f);
        int y = v * size_t + static_cast<int>(shift*(v-size_v/2)+0.5f);
        int width  = (x + size_s >= image.cols-1) ? image.cols - x -1 : size_s;
        int height = (y + size_t >= image.rows-1) ? image.rows - y -1 : size_t;
        x = x < 0 ? 0 : x;
        y = y < 0 ? 0 : y;
        image(cv::Rect(x, y, width, height)).copyTo(shifted_img(cv::Rect(0, 0, width, height)));
        cv::addWeighted (shifted_img, 1.0/(size_v*size_u), focused_img, 1,0, focused_img);
    }
    return focused_img;
}

cv::Mat DepthFromFocus::createFocusVolume(const int size_u, const int size_v, const float threshold, const int radius){
    cv::Mat input_f;
    input_img.convertTo(input_f, CV_32FC3);
    int step = 1;
    int size_s = input_f.cols/size_u;
    int size_t = input_f.rows/size_v;
    int sizes[] = { size_t, size_s, size_d};
    cv::Mat cost(3, sizes, CV_32FC1);

    for (int d=0; d < size_d; d++){
        float d_min = -0.5f;
        float d_max = 0.8f;
        float d_range = d_max - d_min;
        float d_value = d_min + (d*1.0f/(size_d-1)) * d_range; // -0.5 - 0.8
        cv::Mat focused_img = createFocusedImage(input_f, size_u, size_v, d_value);
        //cv::Mat focused_img = createFocusedImage(input_f, size_u, size_v, 0);
        //cv::Mat testCost(focused_img.rows, focused_img.cols, CV_32F);
        cv::Mat ML(focused_img.rows, focused_img.cols, CV_32F);

        // create Modified Laplacian
        for(int y=0; y < focused_img.rows; y++)
        for(int x=0; x < focused_img.cols; x++){
            cv::Vec3f xy2 = focused_img.at<cv::Vec3f>(y,x) * 2;
            ML.at<float>(y,x) = cv::norm(xy2 -
                                focused_img.at<cv::Vec3f>(y, std::max(x - step,0)) -
                                focused_img.at<cv::Vec3f>(y, std::min(x + step,focused_img.cols-1))) +
                                cv::norm(xy2 -
                                focused_img.at<cv::Vec3f>(std::max(y - step,0),x) -
                                focused_img.at<cv::Vec3f>(std::min(y + step,focused_img.rows-1),x));
        }

        // sum up Modified Laplacian
        int R = radius; // Radius
        for(int y=0; y < focused_img.rows; y++)
        for(int x=0; x < focused_img.cols; x++){
            float sum=0;
            for(int ny=-R; ny <= R; ny++)
            for(int nx=-R; nx <= R; nx++){
                float ML_value = ML.at<float>(std::min(std::max(0,y+ny),focused_img.rows-1),
                                              std::min(std::max(0,x+nx),focused_img.cols-1));
                sum += ML_value;
            }
            cost.at<float>(y, x, d) = (sum >= threshold) ? sum : 0;
        }

    }
    return cost;
}
