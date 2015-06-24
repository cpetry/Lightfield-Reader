#include "imagedepth.h"

ImageDepth::ImageDepth()
{

}

ImageDepth::~ImageDepth()
{

}

QImage ImageDepth::generateFromUVST(std::string filename){
    cv::Mat uvst = cv::imread(filename);

    int u = 15, v = 15;
    int size_s = uvst.cols/u;
    int size_t = uvst.rows/v;

    cv::Mat depthmap = cv::Mat(size_t, size_s, CV_8UC1);
    cv::Mat reliability = cv::Mat(u, size_s, CV_32F);

    //for(int y=0; y<uvst.rows / v; y++){
    cv::Mat disparitymap = cv::Mat(u, size_s, CV_32F);
    int y = uvst.rows / 2;
        cv::Mat epi = cv::Mat(u, size_s, CV_8UC3);

        int epi_y = 0;
        for(int u_pos=0; u_pos < u; u_pos++){
            for(int x=0; x < size_s; x++){
                cv::Vec3b pixel = uvst.at<cv::Vec3b>(y, u_pos*size_s + x);
                epi.at<cv::Vec3b>(epi_y, x) = pixel;
            }
            epi_y++;
        }

        //cv::cvtColor(epi, epi, cv::COLOR_BGR2GRAY);

        // Calculate image derivatives
        cv::Mat dx2, dy2, dxy;
        cv::Sobel(epi, dx2, CV_32F, 2, 0, 3, 1, 0);
        cv::Sobel(epi, dy2, CV_32F, 0, 2, 3, 1, 0);
        cv::Sobel(epi, dxy, CV_32F, 1, 1, 3, 1, 0);

        // from [Wan14] Variational Light Field Analysis for Disparity...
        cv::Mat Jxx, Jxy, Jyy;
        cv::GaussianBlur(dx2, Jxx, cv::Size(3, 3), 2.1, 2.1);//0, 0);//
        cv::GaussianBlur(dxy, Jxy, cv::Size(3, 3), 2.1, 2.1);//2.1, 2.1);
        cv::GaussianBlur(dy2, Jyy, cv::Size(3, 3), 2.1, 2.1);//2.1, 2.1);

        cv::Mat phi_mat = (Jyy-Jxx);
        phi_mat /= Jxy*2;
        cv::cvtColor(phi_mat,phi_mat,cv::COLOR_BGR2GRAY);

        //Mat orientation = Mat::zeros(abs_grad_x.rows, abs_grad_y.cols, CV_32F); //to store the gradients grad_x.convertTo(grad_x,CV_32F);
        //phase(phi_mat, phi_mat, disparitymap);

        for (int y_p = 0; y_p < epi.rows; y_p++){
            for (int x_p = 0; x_p < epi.cols; x_p++){
                //float jyy_jxx = Jyy.at<float>(y_p,x_p) - Jxx.at<float>(y_p,x_p);
                //float jxy2 = 2*Jxy.at<float>(y_p,x_p);
                float phi = 0.5f * std::atan(phi_mat.at<float>(y_p,x_p));
                float disparity = sin(phi) / cos(phi);
                //float disparity = jxy2 / jyy_jxx;
                reliability.at<float>(y_p, x_p) = (std::pow(Jyy.at<float>(y_p,x_p) - Jxx.at<float>(y_p,x_p),2) + 4*std::pow(Jxy.at<float>(y_p,x_p),2) / std::pow(Jyy.at<float>(y_p,x_p) + Jxx.at<float>(y_p,x_p),2));
                disparitymap.at<float>(y_p, x_p) = disparity;
            }
        }
        //cv::sum()
        /*int epi_x = 0;
        for(int v_pos=0; v_pos < v; v_pos++){
            for(int epi_y=0; epi_y < size_t; epi_y++){
                cv::Vec3b pixel = uvst.at<cv::Vec3b>(v_pos*size_t + epi_y, epi_x);
                epi.at<cv::Vec3b>(epi_y, epi_x) = pixel;
            }
            epi_x++;
        }

        // Calculate image derivatives
        cv::Mat dx2, dy2, dxy;
        cv::Sobel(epi, dx2, CV_32F, 2, 0, 3, 1, 0);
        cv::Sobel(epi, dy2, CV_32F, 0, 2, 3, 1, 0);
        cv::Sobel(epi, dxy, CV_32F, 1, 1, 3, 1, 0);

        // from [Wan14] Variational Light Field Analysis for Disparity...
        cv::Mat Jxx, Jxy, Jyy;
        cv::GaussianBlur(dx2, Jxx, cv::Size(3, 3), 0, 0);//2.1, 2.1);
        cv::GaussianBlur(dxy, Jxy, cv::Size(3, 3), 0, 0);//2.1, 2.1);
        cv::GaussianBlur(dy2, Jyy, cv::Size(3, 3), 0, 0);//2.1, 2.1);

        for (int y_p = 0; y_p < epi.rows; y_p++){
            for (int x_p = 0; x_p < epi.cols; x_p++){
                float phi = 0.5f * std::atan((Jyy.at<float>(y_p,x_p) - Jxx.at<float>(y_p,x_p)) / 2*Jxy.at<float>(y_p,x_p));
                float disparity = sin(phi) / cos(phi);
                r = (std::pow(Jyy.at<float>(y_p,x_p) - Jxx.at<float>(y_p,x_p),2) + 4*std::pow(Jxy.at<float>(y_p,x_p),2) / std::pow(Jyy.at<float>(y_p,x_p) + Jxx.at<float>(y_p,x_p),2));
                if (r > reliability.at<float>(y_p, x_p))
                    disparitymap.at<float>(y_p, x_p) = disparity;
            }
        }*/
        //depthmap.at(y, )*/
        //depthmap = dx2;
        // InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY=0, int borderType=BORDER_DEFAULT
        //cv::GaussianBlur( Sx*Sy, dst, Size( 3, 3 ), 2.1, 2.1 );
    //}

    //cv::normalize(Jxx, Jxx, 0, 255, CV_MINMAX, CV_32F);
    //return Mat2QImageCol(Jxx);
    //cv::normalize(disparitymap, disparitymap, 0, 1, CV_MINMAX, CV_32F);
    //return Mat2QImage(disparitymap);
    cv::normalize(phi_mat, phi_mat, 0, 1, CV_MINMAX, CV_32F);
    return Mat2QImage(phi_mat);
}

