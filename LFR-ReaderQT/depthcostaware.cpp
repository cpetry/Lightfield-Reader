#include "depthcostaware.h"

void DepthCostAware::calculateDepth(){
    const int size_u = 15, size_v = 15;
    const int size_s = input_img.cols/size_u;
    const int size_t = input_img.rows/size_v;

    if (consistancyCost.empty())
        consistancyCost = costAware_createCostVolume(size_u, size_v);
    if (focusCost.empty())
        focusCost = createFocusVolume(size_u, size_v, focus_threshold, 1);


    cv::Mat cost_depth( size_t, size_s, CV_32FC1);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){

        float min_cons = 99999999.0f;
        int estimated_depth = 0;
        float cons_depth_sum = 0;

        float min_f = 9999, max_f = -9999;
        int min_d = size_d, max_d = 0;
        float avg_f = 0;

        for( int d=0; d < size_d; d++){

            // get minimum consistancyCost
            float v1 = consistancyCost.at<float>(t, s, d);
            if (v1 < min_cons){
                min_cons = v1;
                estimated_depth = d;
            }
            cons_depth_sum += v1;


            // get minimum and maximum focusCost
            if (!focusCost.empty()){
                float focus_cost = focusCost.at<float>(t,s,d);
                avg_f += focus_cost;
                if (focus_cost < min_f){
                    min_f = focus_cost;
                    min_d = d;
                }
                if (focus_cost > max_f){
                    max_f = focus_cost;
                    max_d = d;
                }
            }
        }


        // calculate variance of consistency cost
        float mean = cons_depth_sum / size_d;
        float variance = 0;
        for( int d=0; d < size_d; d++){
            if (d > estimated_depth && d < estimated_depth)
            variance += std::pow(consistancyCost.at<float>(t, s, d) - mean,2);
        }
        variance *= (1.0f/size_d);

        // average value of focus cue
        avg_f /= size_d * 1.0f;

        if (use_focuscue &&
        avg_f == 0 && use_filter_focus_sml_0){                   // first  case: SML are all 0
            cost_depth.at<float>(t, s) = 0;
            continue;
        }

        if (use_consistency && use_focuscue && (
        ((estimated_depth > min_d && estimated_depth < max_d) && use_filter_focus_bound))     // second case: estimated depth is not inside bounds
        || (variance < max_variance*10 && use_filter_cons_variance)){// || estimated_depth > max_d)){
            cost_depth.at<float>(t, s) = 0;
            continue;
        }
        else if (use_focuscue && !use_consistency)
            cost_depth.at<float>(t, s) = max_d;
        else if (!use_focuscue && use_consistency)
            cost_depth.at<float>(t, s) = estimated_depth;
        else
            cost_depth.at<float>(t, s) = estimated_depth;
    }


    if (showCenterColorImage){
        cv::Mat input_f;
        input_img.convertTo(input_f, CV_32FC3);
        // Setup a rectangle to define your region of interest
        cv::Rect myROI(size_t*size_v/2, size_s*size_u/2, size_t*(size_v/2+1), size_s*(size_u/2+1));

        // Crop the full image to that image contained by the rectangle myROI
        // Note that this doesn't copy the data
        cv::Mat center_img = input_img(myROI);

        output_img = center_img.clone();
        return;
    }

     if (fill_up_holes)
        output_img = fillUpHoles(cost_depth);
     else
         output_img = cost_depth;
}

cv::Mat DepthCostAware::fillUpHoles(cv::Mat cost_depth){
    const int size_u = 15, size_v = 15;
    const int size_s = input_img.cols/size_u;
    const int size_t = input_img.rows/size_v;

    // Setup a rectangle to define your region of interest
    cv::Rect myROI(size_t*size_v/2, size_s*size_u/2, size_t*(size_v/2+1), size_s*(size_u/2+1));

    // Crop the full image to that image contained by the rectangle myROI
    // Note that this doesn't copy the data
    cv::Mat center_img = input_img(myROI);

    // Depth propagation
    cv::Mat final_depth( size_t, size_s, CV_32FC1);

    int np = 5; // radius to search for better value
    int pixels_empty = 1; // lets assume at least 1 pixel is incorrect

    // fill all pixels!
    while(pixels_empty != 0){
        pixels_empty = 0;
        for (int t=0; t < size_t; t++){
        for (int s=0; s < size_s; s++){
            float current_depth = cost_depth.at<float>(t, s);
            // Fill in only empty depth values
            if (current_depth == 0){
                float best_appr = 0;
                cv::Vec3f c_col = center_img.at<cv::Vec3f>(t,s);
                float col_dist_neighbour = 999999;

                // Search all neighbours for a good value
                for( int py=-np; py < np; py++){
                for( int px=-np; px < np; px++){
                    if (py==0 && px == 0)
                        continue;
                    int y = t+py;
                    int x = s+px;
                    y = std::max(0, std::min(y, size_t));
                    x = std::max(0, std::min(x, size_s));
                    float neighbour = cost_depth.at<float>(y, x);

                    // good value found -> search best value
                    if (neighbour > 0){
                        cv::Vec3f n_col = center_img.at<cv::Vec3f>(y, x);
                        if (cv::norm(c_col - n_col) < col_dist_neighbour){
                            col_dist_neighbour = cv::norm(c_col - n_col);
                            best_appr = neighbour;
                        }
                    }
                }}
                if (best_appr == 0) // no good solution found? again next time!
                    pixels_empty++;
                else
                    final_depth.at<float>(t, s) = best_appr;
            }
            else
                final_depth.at<float>(t, s) = current_depth;
        }}
        cost_depth = final_depth.clone();
    }
    return final_depth;
}

void DepthCostAware::calcFocusVolume(){
    const int size_u = 15, size_v = 15;
    focusCost = createFocusVolume(size_u, size_v, focus_threshold, 1);
    updateLabel();
}

void DepthCostAware::calcConsistencyVolume(){
    const int size_u = 15, size_v = 15;
    consistancyCost = costAware_createCostVolume(size_u, size_v);
    updateLabel();
}

void DepthCostAware::calcH(double pix_size, double lens_size, float N,
                              int c_pix, double c_Mx, double c_My,
                              double c_mux, double c_muy, double d_mu,
                              double f_M, double exitPupilOffset, float size_d, double* H_data){

    double M_proj = 1.0/(1 + d_mu/exitPupilOffset);
    double M_s = (15.0/(lens_size/pix_size));
    double M_hex = 2.0 / std::sqrt(3.0);

    double F_s = 1.0/pix_size; //
    F_s = M_s * M_proj * F_s;
    double F_t = 1.0/pix_size; //
    F_t = M_s * M_proj * F_t;
    double F_u = 1.0/lens_size;//(14.29*pix_size);//1.0/lens_size; // = F_v
    F_u = M_hex * F_u;
    double F_v = 1.0/lens_size;//(14.29*pix_size);//1.0/1.732050808e-5;
    F_v = M_hex * F_v;

    double d_M = exitPupilOffset; // exitPupilOffset - d_mu
    double D = size_d;//1; // we can choose 1 , size_d ?!
    double H_abs_rel_data[9] = {1, N, -c_pix,
                              0, 1, 0,
                              0, 0, 1};
    double H_abs_phi_datax[9] = {1.0/F_s, 0, -c_Mx/F_s,
                              0, 1.0/F_u, -c_mux/F_u,
                              0, 0, 1};
    double H_abs_phi_datay[9] = {1.0/F_t, 0, -c_My/F_t,
                              0, 1.0/F_v, -c_muy/F_v,
                              0, 0, 1};
    double H_phi_phi_data[9] = {1, 0, 0,
                              -1.0/d_mu, 1.0/d_mu, 0,
                              0, 0, 1};
    double H_T_data[9] = {1, d_mu+d_M, 0,
                          0, 1, 0,
                          0, 0, 1};
    double H_M_data[9] = {1, 0, 0,
                          -1.0/f_M, 1, 0,
                          0, 0, 1};
    double H_phi_phi2_data[9] = {1, D, 0,
                                0, 1, 0,
                                0, 0, 1};
    cv::Mat H_abs_rel = cv::Mat(3, 3, cv::DataType<double>::type,H_abs_rel_data);
    cv::Mat H_abs_phi_x = cv::Mat(3, 3, cv::DataType<double>::type,H_abs_phi_datax);
    cv::Mat H_abs_phi_y = cv::Mat(3, 3, cv::DataType<double>::type,H_abs_phi_datay);
    cv::Mat H_phi_phi = cv::Mat(3, 3, cv::DataType<double>::type,H_phi_phi_data);
    cv::Mat H_T = cv::Mat(3, 3, cv::DataType<double>::type,H_T_data);
    cv::Mat H_M = cv::Mat(3, 3, cv::DataType<double>::type,H_M_data);
    cv::Mat H_phi_phi2 = cv::Mat(3, 3, cv::DataType<double>::type,H_phi_phi2_data);

    cv::Mat Hx = H_phi_phi2 * H_M * H_T * H_phi_phi * H_abs_phi_x * H_abs_rel;
    cv::Mat Hy = H_phi_phi2 * H_M * H_T * H_phi_phi * H_abs_phi_y * H_abs_rel;

    /*double data[25] = {Hx.at<double>(0,0), 0, Hx.at<double>(0,1),0, Hx.at<double>(0,2),
                        0, Hy.at<double>(0,0), 0, Hy.at<double>(0,1), Hy.at<double>(0,2),
                        Hx.at<double>(1,0), 0, Hx.at<double>(1,1), 0, Hx.at<double>(1,2),
                        0, Hy.at<double>(1,0), 0, Hy.at<double>(1,1), Hy.at<double>(1,2),
                        0,       0, 0,       0, 1};*/
    H_data[0] = Hx.at<double>(0,0);
    H_data[1] = 0;
    H_data[2] = Hx.at<double>(0,1);
    H_data[3] = 0;
    H_data[4] = Hx.at<double>(0,2);
    H_data[5] = 0;
    H_data[6] = Hy.at<double>(0,0);
    H_data[7] = 0;
    H_data[8] = Hy.at<double>(0,1);
    H_data[9] = Hy.at<double>(0,2);
    H_data[10] = Hx.at<double>(1,0);
    H_data[11] = 0;
    H_data[12] = Hx.at<double>(1,1);
    H_data[13] = 0;
    H_data[14] = Hx.at<double>(1,2);
    H_data[15] = 0;
    H_data[16] = Hy.at<double>(1,0);
    H_data[17] = 0;
    H_data[18] = Hy.at<double>(1,1);
    H_data[19] = Hy.at<double>(1,2);
    H_data[20] = 0;
    H_data[21] = 0;
    H_data[22] = 0;
    H_data[23] = 0;
    H_data[24] = 1;

    //std::cout << "H: " << H << std::endl;
}

cv::Mat DepthCostAware::costAware_createCostVolume(const int size_i, const int size_j){
    int size_k = input_img.cols/size_i;
    int size_l = input_img.rows/size_j;

    cv::Mat img;
    input_img.convertTo(img, CV_32FC3);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);

    double pix_size = 1.3999999999999999e-6; // in m
    double lens_size = 2.0000000000000002e-5; // in m
    float N = 15.0f;//lens_size / pix_size; // 15 number of pixels per lenslet
    int c_pix = 7; // translational pixel offset ?!
    double c_Mx = -5.8005247410619631e-5;   // optical center offset in mm
    double c_My = -1.1605224244704004e-5;    // optical center offset in mm
    double c_mux = -7.3299932479858395e-6;  // mla offset in mm
    double c_muy = -5.5686492919921878e-6;   // mla offset in mm
    double d_mu = 3.6999999999999998e-5;  // mla offset in mm
    double f_M = 0.011542153404246169;    // focal length
    double exitPupilOffset = 0.11559105682373047; // distance from the pupil to microlens plane
    double min_d = 0;
    double max_d = 1;
    double H_data[25];
    DepthCostAware::calcH(pix_size, lens_size, N, c_pix, c_Mx, c_My,
                                      c_mux, c_muy, d_mu, f_M, exitPupilOffset, 22, H_data);
    cv::Mat H = cv::Mat(5, 5, cv::DataType<double>::type, H_data);
    std::cout << "H: " << H << std::endl;

    // a = [R, G, B, SxR, SxG, SxB, SyR, SyG, SyB]
    // Calculate image derivatives
    cv::Mat dx;
    cv::Mat dy;

    //cv::img_gray;
    //cv::cvtColor(img,img_gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(img, dx, CV_32F, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::Sobel(img, dy, CV_32F, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);

    //C (t,s,d) = 1/n(S)* sum(|a(i_r,j_r,k,l) - a(i',j',k',l'|)
    // k', l' have to be calculated with H
    int sizes[] = { size_l, size_k, size_d};
    cv::Mat cost(3, sizes, CV_32FC1);

    cv::Mat a(1, 9, CV_32FC1);
    cv::Mat a_r(1, 9, CV_32FC1);
    float* pa = a.ptr<float>(0);
    float* pa_r = a_r.ptr<float>(0);
    cv::Vec3f c_, cdx_, cdy_;
    cv::Vec3f c_r, cdx_r, cdy_r;

    int r = 7; // reference position 7,7
    double H_00 = H.at<double>(0,0);
    double H_02 = H.at<double>(0,2);
    double H_04 = H.at<double>(0,4);
    double H_22 = H.at<double>(2,2);
    double H_24 = H.at<double>(2,4);
    double H_11 = H.at<double>(1,1);
    double H_13 = H.at<double>(1,3);
    double H_14 = H.at<double>(1,4);
    double H_31 = H.at<double>(3,1);
    double H_33 = H.at<double>(3,3);
    double H_34 = H.at<double>(3,4);


    for(int l=0; l<size_l; l++)
    for(int k=0; k<size_k; k++){

        c_r = img.at<cv::Vec3f>(l + r*size_l, k + r*size_k);
        //cdx_r = dx.at<cv::Vec3f>(l + r*size_l, k + r*size_k);
        //cdy_r = dy.at<cv::Vec3f>(l + r*size_l, k + r*size_k);
        pa_r[0] = c_r[0];
        pa_r[1] = c_r[1];
        pa_r[2] = c_r[2];
        pa_r[3] = cdx_r[0];
        pa_r[4] = cdx_r[1];
        pa_r[5] = cdx_r[2];
        pa_r[6] = cdy_r[0];
        pa_r[7] = cdy_r[1];
        pa_r[8] = cdy_r[2];
        /*a_r.at<float>(0,0) = c_r[0];
        a_r.at<float>(0,1) = c_r[1];
        a_r.at<float>(0,2) = c_r[2];
        a_r.at<float>(0,3) = cdx_r[0];
        a_r.at<float>(0,4) = cdx_r[1];
        a_r.at<float>(0,5) = cdx_r[2];
        a_r.at<float>(0,6) = cdy_r[0];
        a_r.at<float>(0,7) = cdy_r[1];
        a_r.at<float>(0,8) = cdy_r[2];*/

        //for(int d=0; d<size_d; d++){
        for(int d=0; d<size_d; d++){

            float sum = 0;
            int s_count = 0;
            for(int j=0; j<size_j; j++){
            for(int i=0; i<size_i; i++){
                //a(u,v,t,s)
                int i_f = i;
                int j_f = j;
                i_f -= size_i/2;
                //i_f /= size_i;
                j_f -= size_j/2;
                //j_f /= size_j;

                float df = min_d + d*1.0f/size_d * (max_d - min_d);

                float zx = static_cast<float>(H_00*i_f + H_04 + df*H_02*i_f + df*H_24);
                float nx = static_cast<float>(H_02 + df*H_22);
                int k_ = k + (- zx) / nx + 0.5f;

                float zy = static_cast<float>(H_11*j_f + H_14 + df*H_31*j_f + df*H_34);
                float ny = static_cast<float>(H_13 + df*H_33);
                int l_ = l + (- zy) / ny + 0.5f;

                if (k_ < 0 || l_< 0 || k_ >= size_k || l_ >= size_l)
                    continue;

                s_count++;
                int j_sl = j*size_l;
                int i_sk = i*size_k;
                c_ = img.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk);
                cdx_ = dx.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk);
                cdy_ = dy.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk);
                pa[0] = c_[0];
                pa[1] = c_[1];
                pa[2] = c_[2];
                pa[3] = cdx_[0];
                pa[4] = cdx_[1];
                pa[5] = cdx_[2];
                pa[6] = cdy_[0];
                pa[7] = cdy_[1];
                pa[8] = cdy_[2];
                /*a.at<float>(0,0) = c_[0];
                a.at<float>(0,1) = c_[1];
                a.at<float>(0,2) = c_[2];
                a.at<float>(0,3) = cdx_[0];
                a.at<float>(0,4) = cdx_[1];
                a.at<float>(0,5) = cdx_[2];
                a.at<float>(0,6) = cdy_[0];
                a.at<float>(0,7) = cdy_[1];
                a.at<float>(0,8) = cdy_[2];*/
                sum += cv::norm(a_r-a);
            }}
            cost.at<float>(l, k, d) = sum * 1.0f / s_count;
        }
    }

    return cost;
}

