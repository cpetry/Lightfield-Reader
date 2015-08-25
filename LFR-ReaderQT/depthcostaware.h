#ifndef DEPTHCOSTAWARE_H
#define DEPTHCOSTAWARE_H


#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QLabel>
#include <QObject>

#include "myqgraphicsview.h"
#include "depthfromfocus.h"

class DepthCostAware : public DepthFromFocus
{
    Q_OBJECT

public:
    DepthCostAware(MyGraphicsView* view) : DepthFromFocus(view){}
    ~DepthCostAware(){}

    static void DepthCostAware::calcH(double pix_size, double lens_size, float N,
                                  int c_pix, double c_Mx, double c_My,
                                  double c_mux, double c_muy, double d_mu,
                                  double f_M, double exitPupilOffset, float size_d, double* H_data);


public slots:
    void calcFocusVolume();
    void calcConsistencyVolume();

    void setConsistency(bool v) { this->use_consistency = v;  updateLabel();}
    void setFocusCue(bool v) { this->use_focuscue = v;  updateLabel();}
    void setFilterFocusSml0(bool v) { this->use_filter_focus_sml_0 = v;  updateLabel();}
    void setFilterFocusBound(bool v) { this->use_filter_focus_bound = v;  updateLabel();}
    void setFilterConsVariance(bool v) { this->use_filter_cons_variance = v;  updateLabel();}
    void setShowCenterColorImage(bool v) { this->showCenterColorImage = v;  updateLabel();}
    void setFillUpHoles(bool v) { this->fill_up_holes = v;  updateLabel();}
    void setFocusThreshold(double v) { this->focus_threshold = v; calcFocusVolume();}


    bool getUseConsistency() {return this->use_consistency; }
    bool getUseFocusCue() {return this->use_focuscue; }
    bool getFilterFocusSml0() {return this->use_filter_focus_sml_0; }
    bool getFilterFocusBound() {return this->use_filter_focus_bound; }
    bool getFilterConsVariance() {return this->use_filter_cons_variance; }
    bool getShowCenterColorImage() {return this->showCenterColorImage; }
    bool getFillUpHoles() {return this->fill_up_holes; }

private:
    void calculateDepth();
    cv::Mat costAware_createCostVolume(const int size_i, const int size_j);
    cv::Mat fillUpHoles(cv::Mat cost_depth);

    cv::Mat consistancyCost;

    bool use_consistency = false, use_focuscue = true, fill_up_holes = false,
    use_filter_focus_sml_0 = false, use_filter_focus_bound = false,
    use_filter_cons_variance = false, showCenterColorImage = false;


};

#endif // DEPTHCOSTAWARE_H
