#include "depthcostaware.h"
#include <QMessageBox>
#include <QCoreApplication>

void DepthCostAware::calculateDepth(){
    const int size_u = 15, size_v = 15;
    const int size_s = input_img.cols/size_u;
    const int size_t = input_img.rows/size_v;

    if (consistancyCost.empty())
        calcConsistencyVolume();

    if (focusCost.empty())
        calcFocusVolume();

    cv::Mat cost_depth( size_t, size_s, CV_32FC1);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){

        float min_cons = 99999999.0f;
        int estimated_depth = 0;
        float cons_depth_sum = 0;

        float min_f = 9999, max_f = -9999;
        int min_d = size_d, max_d = 0;
        int focus_d = 0;
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

                if (focus_cost < focus_threshold)
                    continue;

                avg_f += focus_cost;
                /*if (focus_cost < min_f){
                    min_f = focus_cost;
                    min_d = d;
                }
                if (focus_cost > max_f){
                    max_f = focus_cost;
                    max_d = d;
                }*/
                if (focus_cost > 0){
                    min_d = std::min(d, min_d);
                    max_d = std::max(d, max_d);
                    if (focus_cost > max_f){
                        max_f = focus_cost;
                        focus_d = d;
                    }
                }
            }
        }


        // calculate variance of consistency cost
        float mean = cons_depth_sum / size_d;
        float variance = 0;
        int range = variance_range;
        for( int d=0; d < size_d; d++){
            if (d > estimated_depth - range && d < estimated_depth + range)
            variance += std::pow(consistancyCost.at<float>(t, s, d) - mean,2);
        }
        variance *= (1.0f/range*2);

        // average value of focus cue
        avg_f /= size_d * 1.0f;

        if (use_focuscue &&
        avg_f == 0 && use_filter_focus_sml_0){                   // first  case: SML are all 0
            cost_depth.at<float>(t, s) = 0;
            continue;
        }

        if (use_consistency && use_focuscue && (
        ((estimated_depth < min_d || estimated_depth > max_d) && use_filter_focus_bound))     // second case: estimated depth is not inside bounds
        || (variance < max_variance && use_filter_cons_variance)){// || estimated_depth > max_d)){
            cost_depth.at<float>(t, s) = 0;
            continue;
        }
        else if (use_focuscue && !use_consistency)
            cost_depth.at<float>(t, s) = focus_d;
        else if (!use_focuscue && use_consistency)
            cost_depth.at<float>(t, s) = estimated_depth;
        else
            cost_depth.at<float>(t, s) = estimated_depth;
    }


    if (showCenterColorImage){
        cv::Mat input_f;
        input_img.convertTo(input_f, CV_32FC3);
        // Setup a rectangle to define your region of interest
        cv::Rect myROI(size_s*(size_u/2), size_t*(size_v/2), size_s, size_t);

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
    cv::Rect myROI(size_s*(size_u/2), size_t*(size_v/2), size_s, size_t);

    // Crop the full image to that image contained by the rectangle myROI
    // Note that this doesn't copy the data
    cv::Mat center_img;
    input_img.convertTo(center_img, CV_32FC3);
    center_img = center_img(myROI).clone();

    // Depth propagation
    cv::Mat final_depth( size_t, size_s, CV_32FC1);

    int np = 3; // radius to search for better value
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
                    y = std::max(0, std::min(y, size_t-1));
                    x = std::max(0, std::min(x, size_s-1));
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

    QMessageBox msgBox(view);
    msgBox.setWindowTitle("Processing ....");
    msgBox.setText("Calculating Focus Volume.\nThis could take a while! Just sit back ...");
    msgBox.setStandardButtons(0);
    msgBox.setWindowModality(Qt::NonModal);
    msgBox.show();
    QCoreApplication::processEvents();

    focusCost = createFocusVolume(size_u, size_v, focus_threshold, 1);
    updateLabel();

    msgBox.close();
}

void DepthCostAware::calcConsistencyVolume(){
    const int size_u = 15, size_v = 15;

    QMessageBox msgBox(view);
    msgBox.setWindowTitle("Processing ....");
    msgBox.setText("Calculating Cost Volume.\nThis could take a while! Just sit back ...");
    msgBox.setStandardButtons(0);
    msgBox.setWindowModality(Qt::NonModal);
    msgBox.show();
    QCoreApplication::processEvents();

    consistancyCost = costAware_createCostVolume(size_u, size_v);
    updateLabel();

    msgBox.close();
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
    input_img.convertTo(img, CV_32FC3, 0.5/255.0);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);

    double pix_size = meta_info.mla_pixelPitch; //1.3999999999999999e-6; // in m
    double lens_size = meta_info.mla_lensPitch; //2.0000000000000002e-5; // in m
    float N = 15.0f;//lens_size / pix_size; // 15 number of pixels per lenslet
    int c_pix = 7; // translational pixel offset ?!
    double c_Mx = meta_info.lens_centerOffset_x; //-5.8005247410619631e-5;  // optical center offset in mm
    double c_My = -meta_info.lens_centerOffset_y;//-1.1605224244704004e-5;  // optical center offset in mm
    double c_mux = meta_info.mla_centerOffset_x;  // mla offset in mm
    double c_muy = -meta_info.mla_centerOffset_y;  // mla offset in mm
    double d_mu = meta_info.mla_centerOffset_z;  // mla offset in mm
    double f_M = meta_info.focallength;    // focal length
    //double f_M = 0.0037;    // focal length
    double exitPupilOffset = meta_info.exitPupilOffset; // distance from the pupil to microlens plane
    double H_data[25];
    DepthCostAware::calcH(pix_size, lens_size, N, c_pix, c_Mx, c_My,
                                      c_mux, c_muy, d_mu, f_M, exitPupilOffset, 1, H_data);
    cv::Mat H = cv::Mat(5, 5, cv::DataType<double>::type, H_data);
    std::cout << "H: " << H << std::endl;

    // a = [R, G, B, SxR, SxG, SxB, SyR, SyG, SyB]
    // Calculate image derivatives
    cv::Mat dx, dy, lpl;

    //cv::img_gray;
    //cv::cvtColor(img,img_gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(img, dx, CV_32F, 1, 0, 7, 1, 0, cv::BORDER_REFLECT);
    cv::Sobel(img, dy, CV_32F, 0, 1, 7, 1, 0, cv::BORDER_REFLECT);

    double minVal, maxVal;
    cv::minMaxLoc(dx, &minVal, &maxVal); //find minimum and maximum intensities
    dx.convertTo(dx, CV_32F, 1.0/(maxVal - minVal));
    dx = cv::abs(dx);
    cv::minMaxLoc(dx, &minVal, &maxVal); //find minimum and maximum intensities
    std::cout << "minVal : " << minVal << std::endl << "maxVal : " << maxVal << std::endl;


    cv::minMaxLoc(dy, &minVal, &maxVal); //find minimum and maximum intensities
    dy.convertTo(dy, CV_32F, 1.0/(maxVal - minVal));
    dy = cv::abs(dy);
    cv::minMaxLoc(dy, &minVal, &maxVal); //find minimum and maximum intensities
    std::cout << "minVal : " << minVal << std::endl << "maxVal : " << maxVal << std::endl;

    cv::minMaxLoc(img, &minVal, &maxVal); //find minimum and maximum intensities
    std::cout << "minVal : " << minVal << std::endl << "maxVal : " << maxVal << std::endl;

    //cv::Laplacian(img, lpl, CV_32F, 7);

    //cv::convertScaleAbs( dx, dx );
    //cv::convertScaleAbs( dy, dy );

    //C (t,s,d) = 1/n(S)* sum(|a(i_r,j_r,k,l) - a(i',j',k',l'|)
    // k', l' have to be calculated with H
    int sizes[] = { size_l, size_k, size_d};
    cv::Mat cost(3, sizes, CV_32FC1);

    cv::Mat a(1, 9, CV_32FC1);
    cv::Mat a_r(1, 9, CV_32FC1);
    cv::Vec3f c_, c_r;
    cv::Vec3f cdx_, cdy_, cdx_r, cdy_r;
    cv::Vec3f v_half(0.0f,0.0f,0.0f);

    int r = 7; // reference position 7,7
    double H_00 = H.at<double>(0,0);
    double H_02 = H.at<double>(0,2);
    double H_04 = 0;//H.at<double>(0,4); // 0?
    double H_22 = H.at<double>(2,2);
    double H_24 = H.at<double>(2,4);
    double H_11 = H.at<double>(1,1);
    double H_13 = H.at<double>(1,3);
    double H_14 = 0;//H.at<double>(1,4); // 0?
    double H_31 = H.at<double>(3,1);
    double H_33 = H.at<double>(3,3);
    double H_34 = H.at<double>(3,4);


    for(int l=0; l<size_l; l++)
    for(int k=0; k<size_k; k++){

        c_r = img.at<cv::Vec3f>(l + r*size_l, k + r*size_k);
        cdx_r = dx.at<cv::Vec3f>(l + r*size_l, k + r*size_k) - v_half;
        cdy_r = dy.at<cv::Vec3f>(l + r*size_l, k + r*size_k) - v_half;
        a_r.at<float>(0,0) = c_r[0];
        a_r.at<float>(0,1) = c_r[1];
        a_r.at<float>(0,2) = c_r[2];
        a_r.at<float>(0,3) = std::abs(cdx_r[0]);
        a_r.at<float>(0,4) = std::abs(cdx_r[1]);
        a_r.at<float>(0,5) = std::abs(cdx_r[2]);
        a_r.at<float>(0,6) = std::abs(cdy_r[0]);
        a_r.at<float>(0,7) = std::abs(cdy_r[1]);
        a_r.at<float>(0,8) = std::abs(cdy_r[2]);

        for(int d=0; d<size_d; d++){
            float df = min_d + d*1.0f/size_d * (max_d - min_d);

            double sum = 0;
            int s_count = 0;
            for(int j=0; j<size_j; j++){
            for(int i=0; i<size_i; i++){
                if (i + j <= 2 ||
                    (size_i - i) + j <= 2 ||
                    (size_i - i) + (size_j - j) <= 2 ||
                    i + (size_j - j) <= 2)
                    continue;

                //a(u,v,t,s)
                int i_f = i;
                int j_f = j;
                i_f -= size_i/2.0f;
                //i_f /= size_i;
                j_f -= size_j/2.0f;
                //j_f /= size_j;


                float zx = static_cast<float>(H_00*i_f + H_04 + df*H_02*i_f + df*H_24);
                float nx = static_cast<float>(H_02 + df*H_22);
                float fk_ = k + (- zx) / nx;
                int k_ = (fk_ > 0) ? static_cast<int>(fk_ + 0.5f) : static_cast<int>(fk_ - 0.5f); // round

                float zy = static_cast<float>(H_11*j_f + H_14 + df*H_31*j_f + df*H_34);
                float ny = static_cast<float>(H_13 + df*H_33);
                float fl_ = l + (- zy) / ny;
                int l_ = (fl_ > 0) ? static_cast<int>(fl_ + 0.5f) : static_cast<int>(fl_ - 0.5f); // round

                if (k_ < 0 || l_< 0 || k_ >= size_k || l_ >= size_l)
                    continue;

                s_count++;
                int j_sl = j*size_l;
                int i_sk = i*size_k;
                c_ = img.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk);
                cdx_ = dx.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk) - v_half;
                cdy_ = dy.at<cv::Vec3f>(l_ + j_sl, k_ + i_sk) - v_half;
                a.at<float>(0,0) = c_[0];
                a.at<float>(0,1) = c_[1];
                a.at<float>(0,2) = c_[2];
                a.at<float>(0,3) = std::abs(cdx_[0]);
                a.at<float>(0,4) = std::abs(cdx_[1]);
                a.at<float>(0,5) = std::abs(cdx_[2]);
                a.at<float>(0,6) = std::abs(cdy_[0]);
                a.at<float>(0,7) = std::abs(cdy_[1]);
                a.at<float>(0,8) = std::abs(cdy_[2]);
                double value = cv::norm(a_r,a);
                sum += value;
            }}
            float val = static_cast<float>(sum) / s_count;
            cost.at<float>(l, k, d) = val;
        }
    }

    return cost;
}

