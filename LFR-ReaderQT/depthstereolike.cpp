#include "depthstereolike.h"

void DepthStereoLike::calculateDepth(){
    size_d = 50;
    stereoLikeTaxonomy();
}

cv::Mat DepthStereoLike::calculateCostVolume(bool horizontal){
    const int size_u = 15, size_v = 15;
    int size_s = input_img.cols/size_u;
    int size_t = input_img.rows/size_v;

    cv::Mat img;
    input_img.convertTo(img, CV_32FC3);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);

    float f = 0.115f;
    int sizes[] = { size_t, size_s, size_d};
    cv::Mat volume = cv::Mat::zeros(3, sizes, CV_32FC3);
    //cv::Mat estimator(size_t, size_s, CV_32FC3);
    cv::Vec3f estimator, sum_val, distance;

    // cost computation
    for (int t=0; t < size_t; t++){
        for (int s=0; s < size_s; s++){
            for(int d=0; d < size_d; d++){
                sum_val = cv::Vec3f(0,0,0);

                if (horizontal){
                    for(int v=0; v < size_v; v++){
                        estimator = cv::Vec3f(0,0,0);
                        for(int u=0; u < size_u; u++){
                            int u_c = -size_u/2 + u;
                            int p_s = std::max(0,std::min(s - int(u_c*d*f + 0.5f), size_s));
                            //estimator.at<cv::Vec3f>(t, s) += img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
                            estimator += img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
                        }
                        estimator /= size_u; // mean

                        for(int u=0; u < size_u; u++){
                            int u_c = -size_u/2 + u;
                            int p_s = std::max(0,std::min(s - int(u_c*d*f + 0.5f), size_s));
                            //cv::Vec3f distance = (estimator.at<cv::Vec3f>(t, s) - img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s));
                            distance = estimator - img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
                            sum_val += cv::Vec3f(std::pow(distance.val[0],2),std::pow(distance.val[1],2),std::pow(distance.val[2],2));
                        }
                    }
                }
                else{
                    for(int u=0; u < size_u; u++){
                        estimator = cv::Vec3f(0,0,0);
                        for(int v=0; v < size_v; v++){
                            int v_c = -size_v/2 + v;
                            int p_t = std::max(0,std::min(t - int(v_c*d*f + 0.5f), size_t));
                            //estimator.at<cv::Vec3f>(t, s) += img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
                            estimator += img.at<cv::Vec3f>(p_t + v*size_t, s + u*size_s);
                        }
                        estimator /= size_v; // mean

                        for(int v=0; v < size_v; v++){
                            int v_c = -size_v/2 + v;
                            int p_t = std::max(0,std::min(t - int(v_c*d*f + 0.5f), size_t));
                            //cv::Vec3f distance = (estimator.at<cv::Vec3f>(t, s) - img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s));
                            distance = estimator - img.at<cv::Vec3f>(p_t + v*size_t, s + u*size_s);
                            sum_val += cv::Vec3f(std::pow(distance.val[0],2),std::pow(distance.val[1],2),std::pow(distance.val[2],2));
                        }
                    }
                }
                volume.at<cv::Vec3f>(t, s, d) = sum_val;
            }
        }
    }
    return volume;
}

void DepthStereoLike::stereoLikeTaxonomy(){
    //old_disparity();
    //return;
    const int size_u = 15, size_v = 15;
    int size_s = input_img.cols/size_u;
    int size_t = input_img.rows/size_v;

    int sizes[] = { size_t, size_s, size_d};
    cv::Mat depth(3, sizes, CV_32FC1);
    cv::Mat depth_hori(3, sizes, CV_32FC1);
    cv::Mat depth_vert(3, sizes, CV_32FC1);
    cv::Mat variation(size_t, size_s, CV_32FC3);
    cv::Mat variation_hori(size_t, size_s, CV_32FC3);
    cv::Mat variation_vert(size_t, size_s, CV_32FC3);

    if (cost_vol_hori.empty())
        cost_vol_hori = calculateCostVolume(true);
    if (cost_vol_vert.empty())
        cost_vol_vert = calculateCostVolume(false);

    cv::Vec3f val;

    for(int d=0; d < size_d; d++){
        for (int t=0; t < size_t; t++){
            for (int s=0; s < size_s; s++){
                //variation_hori.at<cv::Vec3f>(t,s) = cost_vol_hori.at<cv::Vec3f>(t, s, d);
                //variation_vert.at<cv::Vec3f>(t,s) = cost_vol_vert.at<cv::Vec3f>(t, s, d);
                cv::Vec3f vert = cost_vol_vert.at<cv::Vec3f>(t, s, d);
                cv::Vec3f hori = cost_vol_hori.at<cv::Vec3f>(t, s, d);
                float vert_val = vert.val[0] + vert.val[1] + vert.val[2];
                float hori_val = hori.val[0] + hori.val[1] + hori.val[2];
                if (vert_val < hori_val)
                    variation.at<cv::Vec3f>(t,s) = cost_vol_vert.at<cv::Vec3f>(t, s, d);
                else
                    variation.at<cv::Vec3f>(t,s) = cost_vol_hori.at<cv::Vec3f>(t, s, d);
            }
        }

        //cv::GaussianBlur(variation_hori, variation_hori, cv::Size(gauss_k_size,gauss_k_size), 0.3f*(gauss_k_size*0.5f-1)+0.8f);
        //cv::GaussianBlur(variation_vert, variation_vert, cv::Size(gauss_k_size,gauss_k_size), 0.3f*(gauss_k_size*0.5f-1)+0.8f);
        cv::GaussianBlur(variation, variation, cv::Size(gauss_k_size,gauss_k_size), 0.3f*(gauss_k_size*0.5f-1)+0.8f);

        // cost aggregation
        for (int t=0; t < size_t; t++){
            for (int s=0; s < size_s; s++){
                /*val = variation_hori.at<cv::Vec3f>(t,s);
                depth_hori.at<float>(t,s,d) = val.val[0] + val.val[1] + val.val[2];
                val = variation_vert.at<cv::Vec3f>(t,s);
                depth_vert.at<float>(t,s,d) = val.val[0] + val.val[1] + val.val[2];*/
                val = variation.at<cv::Vec3f>(t,s);
                depth.at<float>(t,s,d) = val.val[0] + val.val[1] + val.val[2];
            }
        }

    }

    // disparity computation / optimization
    cv::Mat disparity( size_t, size_s, CV_32F);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){
        float min = 99999999.0f;
        int min_pos = 0;
        for( int d=0; d < size_d; d++){
            //float v1 = std::min(depth_hori.at<float>(t, s, d), depth_vert.at<float>(t, s, d));
            float v1 = depth.at<float>(t, s, d);
            if (v1 <= min){
                min = v1;
                min_pos = d;
            }
        }
        disparity.at<float>(t, s) = min_pos;
    }
    output_img = disparity;
    //output_img = generateDepthMapFromDisparity(disparity);
}

/*
std::pair<cv::Mat, cv::Mat> DepthStereoLike::calculateDisparityFromEPI(cv::Mat epi, std::string result){

    cv::Mat disparitymap(epi.rows, epi.cols, CV_32F);
    cv::Mat reliability(epi.rows, epi.cols, CV_32F);

    // Calculate image derivatives
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
    split(epi,channels);  // planes[2] is the red channel
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

    // from [Wan14] Variational Light Field Analysis for Disparity...
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
            //float disparity = std::max(std::min((d_x / d_s), epi.cols*1.0f), -epi.cols*1.0f);
            float disparity = d_x / d_s;
            float z = 0.5 * d_s / d_x;
            //if (reliability.at<float>(y_p, x_p) > 0.4)
                disparitymap.at<float>(y_p, x_p) = z;

            //float r = reliability.at<float>(y_p, x_p);
            //if (r > max_r)
            //    max_r = r;
            //else if (r < min_r)
            //    min_r = r;
        }
    }

    //for (int y_p = 0; y_p < epi.rows; y_p++)
    //    for (int x_p = 0; x_p < epi.cols; x_p++)
    //       reliability.at<float>(y_p, x_p) = ((reliability.at<float>(y_p, x_p) - min_r) / (max_r-min_r));

    cv::morphologyEx(disparitymap, disparitymap, cv::MORPH_CLOSE, cv::Mat());
    cv::morphologyEx(disparitymap, disparitymap, cv::MORPH_OPEN, cv::Mat());
    cv::GaussianBlur(disparitymap, disparitymap, cv::Size(1, 1), gauss_sigma, gauss_sigma);
    //cv::dilate(disparitymap, disparitymap, cv::Mat());

    //for (int y_p = 0; y_p < epi.rows; y_p++){
    //    for (int x_p = 0; x_p < epi.cols; x_p++){
    //        disparitymap.at<float>(y_p, x_p) = (disparitymap.at<float>(y_p, x_p) - min_v) / (max_v - min_v);
    //    }
    //}
    //depthmap.at(y, )
    //depthmap = dx2;
    // InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY=0, int borderType=BORDER_DEFAULT
    //cv::GaussianBlur( Sx*Sy, dst, Size( 3, 3 ), 2.1, 2.1 );

    return std::pair<cv::Mat, cv::Mat>(disparitymap, reliability);
}*/


/*void ImageDepth::generateFromUVST(bool show_epi){

    int u = 15, v = 15;
    int size_s = input_img.cols/u;
    int size_t = input_img.rows/v;

    cv::Mat disparityMap = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat disparityMapy = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat disparityMapx = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat reliabilityMapy = cv::Mat(size_t, size_s, CV_32F);
    cv::Mat reliabilityMapx = cv::Mat(size_t, size_s, CV_32F);

    /*for(int y=0; y<size_t; y++){
    //int y = input_img.rows / 2;

        // Create an EPI (Epipolar Image
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
            disparityMapy.at<float>(y,x) = disparityEPI.at<float>(disparityEPI.rows /2 , x);
            reliabilityMapy.at<float>(y,x) = reliabilityEPI.at<float>(reliabilityEPI.rows /2 , x)/20.0f;
        }
    }*/

    //output_img = disparityMapy;
    //output_img = reliabilityMapy;
    //return;

/*

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
            disparityMapx.at<float>(y,x) = disparityEPI.at<float>(y, disparityEPI.cols /2 );
            reliabilityMapx.at<float>(y,x) = reliabilityEPI.at<float>(y, reliabilityEPI.cols /2 );
        }
    }

    output_img = disparityMapx;
    //output_img = reliabilityMapx;
    return;


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
}*/

void DepthStereoLike::old_disparity(){
    const int size_u = 15, size_v = 15;
    int size_s = input_img.cols/size_u;
    int size_t = input_img.rows/size_v;
    cv::Mat img;
    input_img.convertTo(img, CV_32FC3);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);

    float f = 0.115f;
    int sizes[] = { size_t, size_s, size_u};
    cv::Mat depth(3, sizes, CV_32FC1);

    int v = 7;
    int di=0;
    // cost computation
    for(int d=-size_d/2; d < size_d/2; d++){
        int u_c = -size_u/2;

        cv::Mat cost_vol(3, sizes, CV_32FC3);
        for(int u=0; u < size_u; u++){

            //cv::Vec3f sum_sd = 0;
            for (int t=0; t < size_t; t++){
            for (int s=0; s < size_s; s++){
                int p_s = s - u_c*d*f;
                cost_vol.at<cv::Vec3f>(t, s, u) += img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
            }
            }
            u_c+=1;

        }

        // calc estimator
        cv::Mat estimator(size_t, size_s, CV_32FC3);
        for (int t=0; t < size_t; t++)
            for (int s=0; s < size_s; s++)
                for(int u=0; u < size_u; u++)
                    estimator.at<cv::Vec3f>(t, s) += cost_vol.at<cv::Vec3f>(t, s, u);

        estimator /= size_u;

        // calc sum of differences
        cv::Mat variation(size_t, size_s, CV_32FC1);
        //variation = estimator.mul(cv::Mat::eye(size_t, size_s, CV_32F)) - cost_vol;
        for (int t=0; t < size_t; t++)
            for (int s=0; s < size_s; s++){
                cv::Vec3f sum = 0;
                for(int u=0; u < size_u; u++){
                    cv::Vec3f val = estimator.at<cv::Vec3f>(t, s) - cost_vol.at<cv::Vec3f>(t, s, u);
                    sum += val.mul(val);
                }
                variation.at<float>(t,s) = sum[0] + sum[1] + sum[2];
            }

        cv::GaussianBlur(variation, variation, cv::Size(gauss_k_size,gauss_k_size), gauss_sigma);
        // cost aggregation
        for (int t=0; t < size_t; t++)
            for (int s=0; s < size_s; s++)
                depth.at<float>(t,s,di) = variation.at<float>(t,s);
        di+=1;
    }



    // disparity computation / optimization
    cv::Mat disparity( size_t, size_s, CV_32F);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){
        float min = 99999999.0f;
        int min_pos = 0;
        for( int d=0; d < size_d; d++){
            float v1 = depth.at<float>(t, s, d);
            //float v2 = cost_voly.at<float>(t, s, d_u);
            //min = std::min(std::min(v1,v2), min);
            if (v1 <= min){
                min = v1;
                min_pos = d;
            }
        }
        disparity.at<float>(t, s) = min_pos * 1.0f / size_d;
    }
    output_img = disparity;
    //output_img = generateDepthMapFromDisparity(disparity);
}
