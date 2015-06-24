#include "imagedepth.h"

#include <QFileDialog>

ImageDepth::~ImageDepth()
{

}

void ImageDepth::loadImage(){
    QString file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Lightfield Image"),           // window name
                               "../",                                       // relative folder
                               QString("LightField Images(*.PNG *.JPG)"));  // filetype

    if (file.isEmpty()) // Do nothing if filename is empty
        return;

    input_img = cv::imread(file.toStdString());
    output_img = input_img;

    updateLabel("input");
}

void ImageDepth::updateLabel(QString type){
    QImage output;

    if (type == "input"){
        cv::normalize(input_img, output_img, 0, 255, CV_MINMAX, CV_8UC3);
        output = Mat2QImageCol(output_img);
    }

    else if (type == "sobel"){
        output_img = calculateDisparityFromEPI(input_img.clone(), type.toStdString()).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (type == "gauss*sobel"){
        output_img = calculateDisparityFromEPI(input_img.clone(), type.toStdString()).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (type == "disparity"){
        output_img = calculateDisparityFromEPI(input_img.clone(), "phi_mat").first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (type == "depthmap"){
        output_img = calculateDisparityFromEPI(input_img.clone()).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX , CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (type == "depthmap_uvst"){
        generateFromUVST();
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    this->view->setPixmap(QPixmap::fromImage(output));
}

void ImageDepth::generateFromUVST(){

    int u = 15, v = 15;
    int size_s = input_img.cols/u;
    int size_t = input_img.rows/v;

    cv::Mat disparityMap;

    //for(int y=0; y<uvst.rows / v; y++){
    int y = input_img.rows / 2;

        // Create an EPI (Epipolar Image
        cv::Mat epi = cv::Mat(u, size_s, CV_8UC3);
        int epi_y = 0;
        for(int u_pos=0; u_pos < u; u_pos++){
            for(int x=0; x < size_s; x++){
                cv::Vec3b pixel = input_img.at<cv::Vec3b>(y, u_pos*size_s + x);
                epi.at<cv::Vec3b>(epi_y, x) = pixel;
            }
            epi_y++;
        }

        disparityMap = calculateDisparityFromEPI(epi).first;
    //}

    //cv::normalize(Jxx, Jxx, 0, 255, CV_MINMAX, CV_32F);
    //return Mat2QImageCol(Jxx);
    output_img = disparityMap;
    //cv::normalize(phi_mat, phi_mat, 0, 1, CV_MINMAX, CV_32F);
    //return Mat2QImage(phi_mat);
}

std::pair<cv::Mat, cv::Mat> ImageDepth::calculateDisparityFromEPI(cv::Mat epi, std::string result){

    cv::Mat disparitymap(epi.rows, epi.cols, CV_32F);
    cv::Mat reliability(epi.rows, epi.cols, CV_32F);

    // Calculate image derivatives
    cv::Mat dx2(epi.rows, epi.cols, CV_32F);
    cv::Mat dy2(epi.rows, epi.cols, CV_32F);
    cv::Mat dxy(epi.rows, epi.cols, CV_32F);

    //cv::cvtColor(epi,epi, cv::COLOR_BGR2HLS_FULL);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2HSV_FULL);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2Lab);
    //cv::cvtColor(epi,epi, cv::COLOR_BGR2Luv);
    cv::cvtColor(epi,epi, cv::COLOR_BGR2YUV);

    cv::Mat channels[3];
    split(epi,channels);  // planes[2] is the red channel

    float scale = 0.9f;
    int kernel_size = 9; // 3
    cv::Mat filtered[3];
    cv::Sobel(channels[0], filtered[0], CV_32F, 2, 0, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 2, 0, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 2, 0, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++)
            dx2.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p), std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));

    cv::Sobel(channels[0], filtered[0], CV_32F, 0, 2, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 0, 2, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 0, 2, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++)
            dy2.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p), std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));
    cv::Sobel(channels[0], filtered[0], CV_32F, 1, 1, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[1], filtered[1], CV_32F, 1, 1, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(channels[2], filtered[2], CV_32F, 1, 1, kernel_size, scale, 0, cv::BORDER_DEFAULT);
    for (int y_p = 0; y_p < epi.rows; y_p++)
        for (int x_p = 0; x_p < epi.cols; x_p++)
            dxy.at<float>(y_p, x_p) = std::max(filtered[0].at<float>(y_p, x_p), std::max(filtered[1].at<float>(y_p, x_p), filtered[2].at<float>(y_p, x_p)));
    //cv::Scharr(epi, dx2, CV_32F, 1, 0, scale, 0);
    //cv::Scharr(epi, dy2, CV_32F, 0, 1, scale, 0);
    //dxy = dx2.clone();
    //dxy.mul(dy2);
    //dx2.mul(dx2);
    //dy2.mul(dy2);

    if (result == "sobel")
        return std::pair<cv::Mat, cv::Mat>(dx2, reliability);

    // from [Wan14] Variational Light Field Analysis for Disparity...
    cv::Mat Jxx = dx2, Jxy = dxy, Jyy = dy2;
    int blur_size = 3; // 3
    float sigma = 2.1; // 2.1
    cv::GaussianBlur(dx2, Jxx, cv::Size(blur_size, blur_size), sigma, sigma);
    cv::GaussianBlur(dxy, Jxy, cv::Size(blur_size, blur_size), sigma, sigma);
    cv::GaussianBlur(dy2, Jyy, cv::Size(blur_size, blur_size), sigma, sigma);

    if (result == "gauss*sobel")
        return std::pair<cv::Mat, cv::Mat>(Jxx, reliability);

    cv::Mat phi_mat = (Jyy-Jxx);
    phi_mat = phi_mat / (2*Jxy);

    if (result == "phi_mat")
        return std::pair<cv::Mat, cv::Mat>(phi_mat, reliability);
    //cv::cvtColor(phi_mat,phi_mat,cv::COLOR_BGR2GRAY);

    //Mat orientation = Mat::zeros(abs_grad_x.rows, abs_grad_y.cols, CV_32F); //to store the gradients grad_x.convertTo(grad_x,CV_32F);
    //phase(phi_mat, phi_mat, disparitymap);
    float min_v = 99999;
    float max_v = -99999;

    for (int y_p = 0; y_p < epi.rows; y_p++){
        for (int x_p = 0; x_p < epi.cols; x_p++){
            float jyy_jxx = Jyy.at<float>(y_p,x_p) - Jxx.at<float>(y_p,x_p);
            float jxy2 = 2*Jxy.at<float>(y_p,x_p);
            //cv::Vec3b c = phi_mat.at<cv::Vec3b>(y_p,x_p);
            float phi = 0.5f * std::atan(phi_mat.at<float>(y_p,x_p));
            //float phi = 0.5f * std::atan(jyy_jxx / jxy2);
            float d_x = sin(phi);
            float d_s = cos(phi);
            //float disparity = std::max(std::min((d_x / d_s), epi.cols*1.0f), -epi.cols*1.0f);
            float disparity = d_x / d_s;
            if (disparity > epi.rows || disparity < -epi.rows)
                disparity = 0;
            float z = 0.5 * d_s / d_x;
            //float disparity = jxy2 / jyy_jxx;
            float r = (std::pow(jyy_jxx,2) + 4*std::pow(Jxy.at<float>(y_p,x_p),2)) / std::pow(Jyy.at<float>(y_p,x_p) + Jxx.at<float>(y_p,x_p),2);
            reliability.at<float>(y_p, x_p) = r;
            //if (r > 0.05)
                disparitymap.at<float>(y_p, x_p) = disparity;

            if (disparity > max_v)
                max_v = disparity;
            if (disparity < min_v)
                min_v = disparity;
        }
    }
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
