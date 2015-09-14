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
