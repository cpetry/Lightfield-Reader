#include "depthfromepipolarimages.h"

void DepthFromEpipolarImages::calculateDepth(){
    generateFromUVST();
}

std::pair<cv::Mat, cv::Mat> DepthFromEpipolarImages::calculateDisparityFromEPI(cv::Mat epi, std::string result){

    cv::Mat disparitymap(epi.rows, epi.cols, CV_32F);
    cv::Mat reliability(epi.rows, epi.cols, CV_32F);

    // Вычислить производные изображения
    cv::Mat dx2(epi.rows, epi.cols, CV_32F);
    cv::Mat dy2(epi.rows, epi.cols, CV_32F);
    cv::Mat dxy(epi.rows, epi.cols, CV_32F);

    //cv::cvtColor(epi,epi, cv::COLOR_BGR2GRAY);
    //cv::Sobel(epi, dx2, CV_32F, 2, 0, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    //cv::Sobel(epi, dy2, CV_32F, 0, 2, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    //cv::Sobel(epi, dxy, CV_32F, 1, 1, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);

    //cv::cvtColor(epi,epi, cv::COLOR_BGR2HLS_FULL);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2HSV_FULL);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2Lab);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2Luv);
    cv::cvtColor(epi,epi, cv::COLOR_BGR2YUV);

    cv::Mat channels[3];
    split(epi,channels);  // planes[2] это красный канал
    cv::Mat filtered[3];

    cv::Sobel(channels[0], filtered[0], CV_32F, 2, 0, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 2, 0, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 2, 0, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++){
            dx2.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p),
                    std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));
        }

    cv::Sobel(channels[0], filtered[0], CV_32F, 0, 2, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 0, 2, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 0, 2, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++){
            dy2.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p),
                    std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));
        }
    cv::Sobel(channels[0], filtered[0], CV_32F, 1, 1, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 1, 1, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 1, 1, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++){
            dxy.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p),
                    std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));
        }
    //cv::Scharr(epi, dx2, CV_32F, 1, 0, scale, 0);
    //cv::Scharr(epi, dy2, CV_32F, 0, 1, scale, 0);
    //dxy = dx2.clone();
    //dxy.mul(dy2);
    //dx2.mul(dx2);
    //dy2.mul(dy2);

    if (result == "sobel")
        return std::pair<cv::Mat, cv::Mat>(dx2+dy2, reliability);

    // из [Wan14] Вариационный анализ светового поля на предмет несоответствия...
    cv::Mat Jxx = dx2, Jxy = dxy, Jyy = dy2;
    cv::GaussianBlur(dx2, Jxx, cv::Size(gauss_k_size, gauss_k_size), gauss_sigma, gauss_sigma);
    cv::GaussianBlur(dxy, Jxy, cv::Size(gauss_k_size, gauss_k_size), gauss_sigma, gauss_sigma);
    cv::GaussianBlur(dy2, Jyy, cv::Size(gauss_k_size, gauss_k_size), gauss_sigma, gauss_sigma);

    if (result == "gauss*sobel")
        return std::pair<cv::Mat, cv::Mat>(Jxx + Jyy, reliability);

    cv::Mat phi_mat = Jyy.clone();
    phi_mat -= Jxx;
    phi_mat = phi_mat / (2*Jxy);

    if (result == "disparity")
        return std::pair<cv::Mat, cv::Mat>(phi_mat, reliability);
    //cv::cvtColor(phi_mat,phi_mat,cv::COLOR_BGR2GRAY);

    reliability = ((Jyy-Jxx).mul(Jyy-Jxx) + 4*Jxy.mul(Jxy)) / ((Jxx+Jyy).mul(Jxx+Jyy));

    //Mat orientation = Mat::zeros(abs_grad_x.rows, abs_grad_y.cols, CV_32F); //to store the gradients grad_x.convertTo(grad_x,CV_32F);
    //phase(phi_mat, phi_mat, disparitymap);
    //float min_v = 99999.0f;
    //float max_v = -99999.0f;
    //float min_r = 99999.0f;
    //float max_r = -99999.0f;

    for (int y_p = 0; y_p < epi.rows; y_p++){
        for (int x_p = 0; x_p < epi.cols; x_p++){

            float phi = 0.5f * std::atan(phi_mat.at<float>(y_p,x_p));
            float d_x = sin(phi);
            float d_s = cos(phi);
            //плавающее неравенство = std::max(std::min((d_x / d_s), epi.cols*1.0f), -epi.cols*1.0f);
            float disparity = d_x / d_s;
            float z = 0.5 * d_s / d_x;
            //if (reliability.at<float>(y_p, x_p) > 0.4)
                disparitymap.at<float>(y_p, x_p) = disparity;
            /*
            float r = reliability.at<float>(y_p, x_p);
            if (r > max_r)
                max_r = r;
            else if (r < min_r)
                min_r = r;*/
        }
    }

    //for (int y_p = 0; y_p < epi.rows; y_p++)
    //    for (int x_p = 0; x_p < epi.cols; x_p++)
    //       reliability.at<float>(y_p, x_p) = ((reliability.at<float>(y_p, x_p) - min_r) / (max_r-min_r));

    cv::morphologyEx(disparitymap, disparitymap, cv::MORPH_CLOSE, cv::Mat());
    cv::morphologyEx(disparitymap, disparitymap, cv::MORPH_OPEN, cv::Mat());
    cv::GaussianBlur(disparitymap, disparitymap, cv::Size(1, 1), gauss_sigma, gauss_sigma);
    //cv::dilate(disparitymap, disparitymap, cv::Mat());

    /*for (int y_p = 0; y_p < epi.rows; y_p++){
        for (int x_p = 0; x_p < epi.cols; x_p++){
            disparitymap.at<float>(y_p, x_p) = (disparitymap.at<float>(y_p, x_p) - min_v) / (max_v - min_v);
        }
    }*/
    //depthmap.at(y, )*/
    //depthmap = dx2;
    // InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY=0, int borderType=BORDER_DEFAULT
    //cv::GaussianBlur( Sx*Sy, dst, Size( 3, 3 ), 2.1, 2.1 );

    return std::pair<cv::Mat, cv::Mat>(disparitymap, reliability);
}


void DepthFromEpipolarImages::generateFromUVST(bool show_epi){

    int u = 15, v = 15;
    int size_s = input_img.cols/u;
    int size_t = input_img.rows/v;

    cv::Mat disparityMap = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat disparityMapy = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat disparityMapx = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat reliabilityMapy = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat reliabilityMapx = cv::Mat(size_t, size_s, CV_32F);

    for(int y=0; y<size_t; y++){
    //int y = input_img.rows / 2;

        // Создайте EPI (эпиполярное изображение
        cv::Mat epix = cv::Mat(u, size_s, CV_8UC3);
        int uv_y = 0;
        for(int u_pos=0; u_pos < u; u_pos++){
            for(int x=0; x < size_s; x++){
                cv::Vec3b pixel = input_img.at<cv::Vec3b>(y + size_t*7, u_pos*size_s + x);
                epix.at<cv::Vec3b>(uv_y, x) = pixel;
            }
            uv_y++;
        }
        if (show_epi && y == size_t/2){
            output_img = epix;
            return;
        }
        cv::Mat disparityEPI = calculateDisparityFromEPI(epix).first;
        cv::Mat reliabilityEPI = calculateDisparityFromEPI(epix).second;
        for (int x = 0; x < disparityEPI.cols; x++){
            disparityMapy.at<float>(y,x) = disparityEPI.at<float>(disparityEPI.rows / 2 , x);
            reliabilityMapy.at<float>(y,x) = reliabilityEPI.at<float>(reliabilityEPI.rows / 2 , x);
        }
    }

    //output_img = disparityMapy;
    //output_img = reliabilityMapy;
    //return;



    for(int x=0; x<size_s; x++){

        cv::Mat epiy = cv::Mat(size_t, v, CV_8UC3);
        int uv_x = 0;
        for(int v_pos=0; v_pos < v; v_pos++){
            for(int y=0; y < size_t; y++){
                cv::Vec3b pixel = input_img.at<cv::Vec3b>(v_pos*size_t + y, x + size_s*7);
                epiy.at<cv::Vec3b>(y, uv_x) = pixel;
            }
            uv_x++;
        }
        if (show_epi && x == size_s/2){
            output_img = epiy;
            return;
        }
        cv::Mat disparityEPI = calculateDisparityFromEPI(epiy).first;
        cv::Mat reliabilityEPI = calculateDisparityFromEPI(epiy).second;
        for (int y = 0; y < disparityEPI.rows; y++){
            disparityMapx.at<float>(y,x) = disparityEPI.at<float>(y, disparityEPI.cols / 2 );
            reliabilityMapx.at<float>(y,x) = reliabilityEPI.at<float>(y, reliabilityEPI.cols / 2 );
        }
    }

    //output_img = disparityMapx;
    //output_img = reliabilityMapx;
    //return;


    for(int y=0; y<size_t; y++){
        for(int x=0; x<size_s; x++){
            if (reliabilityMapx.at<float>(y,x) > reliabilityMapy.at<float>(y,x))
                disparityMap.at<float>(y,x) = disparityMapx.at<float>(y,x);
            else
                disparityMap.at<float>(y,x) = disparityMapy.at<float>(y,x);
        }
    }

    //cv::normalize(Jxx, Jxx, 0, 255, CV_MINMAX, CV_32F);
    //return Mat2QImageCol(Jxx);
    output_img = disparityMap;
    //cv::normalize(phi_mat, phi_mat, 0, 1, CV_MINMAX, CV_32F);
    //return Mat2QImage(phi_mat);
}
