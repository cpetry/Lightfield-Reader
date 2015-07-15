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
    setViewMode("input");
}
void ImageDepth::calcFocusVolume(){
    const int size_u = 15, size_v = 15;
    const int size_d = 10;
    focusCost = costAware_createFocusVolume(size_u, size_v, size_d);
    updateLabel();
}

void ImageDepth::calcConsistencyVolume(){
    const int size_u = 15, size_v = 15;
    const int size_d = 10;
    consistancyCost = costAware_createCostVolume(size_u, size_v, size_d);
    updateLabel();
}

void ImageDepth::updateLabel(){
    QImage output;
    //stereoLikeTaxonomy();
    costAwareDepthMapEstimation();
    cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
    output = EveryMat2QImageCol(output_img);
    this->view->setPixmap(QPixmap::fromImage(output));
    return;

    if (view_mode == "input"){
        cv::normalize(input_img, output_img, 0, 255, CV_MINMAX, CV_8UC3);
        output = Mat2QImageCol(output_img);
    }

    else if (view_mode == "sobel"){
        output_img = calculateDisparityFromEPI(input_img.clone(), view_mode).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (view_mode == "gauss*sobel"){
        output_img = calculateDisparityFromEPI(input_img.clone(), view_mode).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (view_mode == "disparity"){
        output_img = calculateDisparityFromEPI(input_img.clone(), view_mode).first;
        //cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (view_mode == "depthmap"){
        output_img = calculateDisparityFromEPI(input_img.clone()).first;
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX , CV_32F);
        output = Mat2QImage(output_img);
    }

    else if (view_mode == "epi_uvst"){
        generateFromUVST(true);
        cv::normalize(output_img, output_img, 0, 255, CV_MINMAX, CV_8UC3);
        output = Mat2QImageCol(output_img);
    }

    else if (view_mode == "depthmap_uvst"){
        generateFromUVST();
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        output = Mat2QImage(output_img);
    }

    this->view->setPixmap(QPixmap::fromImage(output));
}

void ImageDepth::generateFromUVST(bool show_epi){

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
}

cv::Mat ImageDepth::costAware_createCostVolume(const int size_i, const int size_j, const int size_d){
    int size_k = input_img.cols/size_i;
    int size_l = input_img.rows/size_j;

    cv::Mat img;
    input_img.convertTo(img, CV_32FC3);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);

    int N = 15; //??
    int c_pix = 7; //??
    double F_s = 1.3999e-6; // = F_t
    double F_u = 2.0e-5; // = F_v
    double c_Mx = -5.800524e-5;
    double c_My = 1.160522e-5;
    double c_mux = -7.32999324e-6;
    double c_muy = 5.56864929e-6;
    double d_mu = 3.6999e-5;
    double d_M = 0.115591 - d_mu; // =! 0.115591 or focal length? 0.0115421534
    double f_M = 0.011542910; // focal length
    double D = 1; // we can choose
    float H_abs_rel_data[9] = {1, N, -c_pix,
                              0, 1, 0,
                              0, 0, 1};
    float H_abs_phi_datax[9] = {1.0/F_s, 0, -c_Mx/F_s,
                              0, 1.0/F_u, -c_mux/F_u,
                              0, 0, 1};
    float H_abs_phi_datay[9] = {1.0/F_s, 0, -c_My/F_s,
                              0, 1.0/F_u, -c_muy/F_u,
                              0, 0, 1};
    float H_phi_phi_data[9] = {1, 0, 0,
                              -1.0/d_mu, 1.0/d_mu, 0,
                              0, 0, 1};
    float H_T_data[9] = {1, d_mu+d_M, 0,
                          0, 1, 0,
                          0, 0, 1};
    float H_M_data[9] = {1, 0, 0,
                          -1.0/f_M, 1, 0,
                          0, 0, 1};
    float H_phi_phi2_data[9] = {1, D, 0,
                                0, 1, 0,
                                0, 0, 1};
    cv::Mat H_abs_rel = cv::Mat(3, 3, CV_32F,H_abs_rel_data);
    cv::Mat H_abs_phi_x = cv::Mat(3, 3, CV_32F,H_abs_phi_datax);
    //std::cout << "H_abs_phi_x: " << H_abs_phi_x << std::endl;
    cv::Mat H_abs_phi_y = cv::Mat(3, 3, CV_32F,H_abs_phi_datay);
    cv::Mat H_phi_phi = cv::Mat(3, 3, CV_32F,H_phi_phi_data);
    //std::cout << "H_phi_phi: " << H_phi_phi << std::endl;
    cv::Mat H_T = cv::Mat(3, 3, CV_32F,H_T_data);
    //std::cout << "H_T: " << H_T << std::endl;
    cv::Mat H_M = cv::Mat(3, 3, CV_32F,H_M_data);
    //std::cout << "H_M: " << H_M << std::endl;
    cv::Mat H_phi_phi2 = cv::Mat(3, 3, CV_32F,H_phi_phi2_data);
    //std::cout << "H_phi_phi2: " << H_phi_phi2 << std::endl;


    /*float Hx_data[9] = {-0.0006 ,  -0.0021 ,  0.0000,
                        -0.0001 ,  -0.0018 ,  0.0000,
                              0 ,        0 ,   1.0000};
    float Hy_data[9] = {-0.0006 ,  -0.0021 ,  0.0039,
                        -0.0001 ,  -0.0018 ,  0.0008,
                              0 ,        0 ,   1.0000};*/
    cv::Mat Hx = ((((H_phi_phi2 * H_M) * H_T) * H_phi_phi) * H_abs_phi_x) * H_abs_rel;
    //cv::Mat Hx = H_phi_phi2.mul(H_M).mul(H_T).mul(H_phi_phi).mul(H_abs_phi_x).mul(H_abs_rel);
    cv::Mat Hy = H_phi_phi2 * H_M * H_T * H_phi_phi * H_abs_phi_y * H_abs_rel;
    //cv::Mat Hy = H_phi_phi2.mul(H_M).mul(H_T).mul(H_phi_phi).mul(H_abs_phi_y).mul(H_abs_rel);
    //std::cout << "Hx: " << Hx << std::endl;
    //std::cout << "Hy: " << Hy << std::endl;


    // five dimensional H
    float H_data[25] = { Hx.at<float>(0,0), 0, Hx.at<float>(0,1), 0, Hx.at<float>(0,2),
                        0, Hy.at<float>(0,0), 0, Hy.at<float>(0,1), Hy.at<float>(0,2),
                        Hx.at<float>(1,0), 0, Hx.at<float>(1,1), 0, Hx.at<float>(1,2),
                        0, Hy.at<float>(1,0), 0, Hy.at<float>(1,1), Hy.at<float>(1,2),
                        0,       0, 0,       0, 1};
    cv::Mat H = cv::Mat(5, 5, CV_32F, H_data);
    //std::cout << "H: " << H << std::endl;

    // a = [R, G, B, SxR, SxG, SxB, SyR, SyG, SyB]
    // Calculate image derivatives
    cv::Mat dx(img.cols, img.rows, CV_32F);
    cv::Mat dy(img.cols, img.rows, CV_32F);

    //cv::img_gray;
    //cv::cvtColor(img,img_gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(img, dx, CV_32F, 1, 0, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);
    cv::Sobel(img, dy, CV_32F, 0, 1, sobel_k_size, sobel_scale, 0, cv::BORDER_DEFAULT);

    //C (t,s,d) = 1/n(S)* sum(|a(i_r,j_r,k,l) - a(i',j',k',l'|)
    // k', l' have to be calculated with H
    int i_r = 7;
    int j_r = 7;
    int sizes[] = { size_l, size_k, size_d};
    cv::Mat cost(3, sizes, CV_32FC1);

    for(int l=0; l<size_l; l++)
    for(int k=0; k<size_k; k++){
        cv::Vec3f c = img.at<cv::Vec3f>(l + j_r*size_l, k + i_r*size_k);
        cv::Vec3f c_dx = dx.at<cv::Vec3f>(l + j_r*size_l, k + i_r*size_k);
        cv::Vec3f c_dy = dy.at<cv::Vec3f>(l + j_r*size_l, k + i_r*size_k);
        for(int d=0; d<size_d; d++){

            float sum = 0;
            int s_count = 0;
            for(int j=0; j<size_j; j++)
            for(int i=0; i<size_i; i++){
                //a(u,v,t,s)
                float df = d*1.0f;
                df = d*1.0f/size_d;
                float i_f = i-size_i/2;
                float j_f = j-size_j/2;
                float li = l;
                float ki = k;
                float zx = H.at<float>(0,0)*i_f + H.at<float>(0,4) + df*H.at<float>(0,2)*i_f + df*H.at<float>(2,4);
                float nx = H.at<float>(0,2) + df*H.at<float>(2,2);
                //int k_ = (ki/size_k-zx) / nx;
                int k_ = ki-zx / nx;
                float zy = H.at<float>(1,1)*j_f + H.at<float>(1,4) + df*H.at<float>(3,1)*j_f + df*H.at<float>(3,4);
                float ny = H.at<float>(1,3) + df*H.at<float>(3,3);
                //int l_ = (li/size_l-zy) / ny;
                int l_ = li-zy / ny;
                if (k_ < 0 || l_< 0 || k_ >= size_k || l_ >= size_l)
                    continue;
                s_count++;
                cv::Vec3f c_ = img.at<cv::Vec3f>(l_ + j*size_l, k_ + i*size_k);
                cv::Vec3f cdx_ = dx.at<cv::Vec3f>(l_ + j*size_l, k_ + i*size_k);
                cv::Vec3f cdy_ = dy.at<cv::Vec3f>(l_ + j*size_l, k_ + i*size_k);
                sum += cv::norm(c - c_);
                sum += cv::norm(c_dx - cdx_);
                sum += cv::norm(c_dy - cdy_);
            }
            cost.at<float>(l, k, d) = sum / s_count;
        }
    }

    return cost;
}

cv::Mat ImageDepth::createFocusedImage(const cv::Mat image, const int size_u, const int size_v, const float shift){
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

cv::Mat ImageDepth::costAware_createFocusVolume(const int size_u, const int size_v, const int size_d){
    cv::Mat input_f;
    input_img.convertTo(input_f, CV_32FC3);
    int step = 1;
    int size_s = input_f.cols/size_u;
    int size_t = input_f.rows/size_v;
    int sizes[] = { size_t, size_s, size_d};
    cv::Mat cost(3, sizes, CV_32FC1);

    for (int d=0; d < size_d; d++){
        float d_min = -0.5;
        float d_max = 0.8;
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
        int R = 1; // Radius
        for(int y=0; y < focused_img.rows; y++)
        for(int x=0; x < focused_img.cols; x++){
            float sum=0;
            for(int ny=-R; ny <= R; ny++)
            for(int nx=-R; nx <= R; nx++){
                float ML_value = ML.at<float>(std::min(std::max(0,y+ny),focused_img.rows-1),
                                              std::min(std::max(0,x+nx),focused_img.cols-1));
                sum += (ML_value >= focus_threshold) ? ML_value : 0;
            }
            cost.at<float>(y, x, d) = sum;
        }

    }
    return cost;
}

void ImageDepth::costAwareDepthMapEstimation(){
    const int size_u = 15, size_v = 15;
    const int size_d = 10;
    const int size_s = input_img.cols/size_u;
    const int size_t = input_img.rows/size_v;

    if (consistancyCost.empty())
        calcConsistencyVolume();
    if (focusCost.empty())
        calcFocusVolume();

    cv::Mat cost_depth( size_t, size_s, CV_32FC1);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){

        float min_cons = 99999999;
        int estimated_depth = 0;
        float cons_depth_sum = 0;

        float min_f = 9999, max_f = -9999;
        int min_d = size_d, max_d = 0;
        float avg_f = 0;

        for( int d=0; d < size_d; d++){

            // get minimum consistancyCost
            float v1 = consistancyCost.at<float>(t, s, d);
            if (v1 <= min_cons){
                min_cons = v1;
                cons_depth_sum += v1;
                estimated_depth = d;
            }


            // get minimum and maximum focusCost
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


        // calculate variance of consistency cost
        float mean = cons_depth_sum / size_d;
        float variance = 0;
        for( int d=0; d < size_d; d++){
            variance += std::pow(consistancyCost.at<float>(t, s, d) - mean,2);
        }
        variance *= (1.0f/size_d);

        // average value of focus cue
        avg_f /= size_d * 1.0f;

        if (use_focuscue && (
                (avg_f == 0 && use_filter_focus_sml_0)                   // first  case: SML are all 0
            || ((estimated_depth > size_d - max_d) && use_filter_focus_bound))     // second case: estimated depth is not inside bounds
            || (variance > max_variance*10 && use_filter_cons_variance)){// || estimated_depth > max_d)){
            cost_depth.at<float>(t, s) = 0;
            continue;
        }
        else{
            if (use_consistency)
                cost_depth.at<float>(t, s) = estimated_depth / 256.0f;
            else if (use_focuscue){
                cost_depth.at<float>(t, s) = size_d - max_d;
            }
        }



    }
    output_img = cost_depth;
}

void ImageDepth::stereoLikeTaxonomy(){
    const int size_u = 15, size_v = 15;
    int size_s = input_img.cols/size_u;
    int size_t = input_img.rows/size_v;

    cv::Mat img;
    input_img.convertTo(img, CV_32FC3);
    cv::cvtColor(img,img, cv::COLOR_BGR2RGB);
    //cv::Mat channels[3];
    //split(input_img, channels);  // planes[2] is the red channel

    // estimator = schätzwert ( coord u )
    // sum_sd == sum_sad(absolute) || sum_sqd (square)
    //foreach d_u sum_c(sum_sd(estimator(u, v, s-u*d_u, t)*I - p(u, v, s-u*d_u, t))

    float size_d = 20;
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

            /*
            int v = 7;

            cv::Vec3f estimator = 0;
            for( int u=0; u < size_u; u++){
                int u_c = (u-size_u/2);
                int p_s = s - u_c*d_c*f;
                p_s = std::max(0,std::min(p_s, size_s-1));
                estimator += img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
            }
            estimator /= (size_u);// * size_v);

            cv::Vec3f sum_sd = 0;
            for( int u=0; u < size_u; u++){
                int u_c = (u-size_u/2);
                int p_s = s - u_c*d_c*f;
                p_s = std::max(0,std::min(p_s, size_s-1));
                cv::Vec3f val = estimator - img.at<cv::Vec3f>(t + v*size_t, p_s + u*size_s);
                sum_sd += val.mul(val);
            }
            cost_volx.at<float>(t, s, d) = sum_sd[0] + sum_sd[1] + sum_sd[2];

            int u = 7;

            estimator = 0;
            for( int v=0; v < size_v; v++){
                int v_c = (v-size_v/2);
                int p_t = t - v_c*d_c*f;
                p_t = std::max(0,std::min(p_t, size_t-1));
                estimator += img.at<cv::Vec3f>(p_t + v*size_t, s + u*size_s);
            }
            estimator /= (size_v);

            sum_sd = 0;
            for( int v=0; v < size_v; v++){
                int v_c = (v-size_v/2);
                int p_t = t - v_c*d_c*f;
                p_t = std::max(0,std::min(p_t, size_t-1));
                cv::Vec3f val = estimator - img.at<cv::Vec3f>(p_t + v*size_t, s + u*size_s);
                sum_sd += val.mul(val);
            }
            cost_volx.at<float>(t, s, d) += sum_sd[0] + sum_sd[1] + sum_sd[2];

        }
    }*/

    /*
    int u = 7;
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){
        for( int d_v=0; d_v < size_d; d_v++){
            cv::Vec3b sum_sd = 0;
            cv::Vec3b estimator;
            for(int v=0; v < size_v; v++){
                int y_pos = t+(d_v-size_d/2);
                y_pos = std::max(0, std::min(y_pos, size_t-1));
                estimator += img.at<cv::Vec3b>(y_pos + v*size_t, s + u*size_s);
            }
            estimator /= size_d;

            for(int v=0; v < size_v; v++){
                int y_pos = t + (d_v-size_d/2);
                y_pos = std::max(0, std::min(y_pos, size_t-1));
                cv::Vec3b val = estimator - img.at<cv::Vec3b>(y_pos + v*size_t, s + u*size_s);
                //sum_sd += val*std::abs(v - size_v/2); // absolute distance
                sum_sd += val*std::pow(v - size_v/2,2); // square distance
            }
            cost_voly.at<float>(t, s, d_v) = sum_sd[0] + sum_sd[1] + sum_sd[2];
        }
    }*/

    //cost aggregation
    /*cv::Mat channels[size_d];
    split(cost_volx, channels);
    for(int i=0; i<size_d; i++)
        cv::GaussianBlur(channels[i], channels[i], cv::Size(gauss_k_size, gauss_k_size), gauss_sigma, gauss_sigma);
    split(cost_voly, channels);
    for(int i=0; i<size_d; i++)
        cv::GaussianBlur(channels[i], channels[i], cv::Size(gauss_k_size, gauss_k_size), gauss_sigma, gauss_sigma);
    */


    // disparity computation / optimization
    cv::Mat disparity( size_t, size_s, CV_32F);
    for (int t=0; t < size_t; t++)
    for (int s=0; s < size_s; s++){
        float min = 99999999;
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
    //output_img = disparity;
    output_img = generateDepthMapFromDisparity(disparity);
}

cv::Mat ImageDepth::generateDepthMapFromDisparity(cv::Mat dis){
    double f = 0.0115421 / (1.399*pow(10,-6)); // focal length (in pixel)
    float B = 0.00002f; // baseline (distance of two lenses, in meter)
    cv::Mat Z = dis.clone();
    for (int t=0; t < dis.rows; t++)
        for (int s=0; s < dis.cols; s++){
            float d = dis.at<float>(t,s);
            //Z.at<float>(t,s) = f*B/d;
            Z.at<float>(t,s) = d;
            //Z.at<float>(t,s) = dis.at<float>(t,s);
        }
    return Z;
}

std::pair<cv::Mat, cv::Mat> ImageDepth::calculateDisparityFromEPI(cv::Mat epi, std::string result){

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
    float min_v = 99999;
    float max_v = -99999;
    float min_r = 99999;
    float max_r = -99999;

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
