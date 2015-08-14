#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_2_Core>
//#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QMovie>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


#include "lfp_reader.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class QOpenGL_LFViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /*
     * Initializes the viewer with a lightfield-image
     */
    QOpenGL_LFViewer(QWidget *parent, QString file, bool is_video, LFP_Reader::lf_meta meta_infos);
    QOpenGL_LFViewer(QWidget *parent, QStringList files, bool is_video, LFP_Reader::lf_meta meta_infos);
    ~QOpenGL_LFViewer();

    /*
     * Defines the minimum size of the widget
     */
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setClearColor(const QColor &color);

public slots:
    void focus_changed(int value);
    void focus_radius_changed(int value);
    void setOverlap(double o);

    void buttonGrayClicked(){ opengl_view_mode = 0; update();}
    void buttonBayerClicked(){ opengl_view_mode = 1; update();}
    void buttonDemosaicClicked(){ opengl_view_mode = 2; update();}
    void buttonUVModeClicked(){ opengl_view_mode = 3; update();}
    void buttonDisplayClicked(){ opengl_view_mode = 4; update();}
    void setDecodeMode(int m){ opengl_decode_mode = m; update();}
    void toggleWhiteBalance(bool v){ opengl_option_wb = v; update();}
    void toggleCCM(bool v){ opengl_option_ccm = v;  update();}
    void toggleGamma(bool v){ opengl_option_gamma = v;  update();}
    void toggleSuperResolution(bool v){ opengl_option_superresolution = v; update();}
    void setRotation(double r){

        float R_matrix[] = {std::cos(r), std::sin(r),
                           -std::sin(r), std::cos(r)};
        R_m = QMatrix2x2(R_matrix);
        program->bind();
        program->setUniformValue("R_m", R_m);

        update();
    }
    void renderDemosaic(bool v) {opengl_option_is_demosaicked = v; update();}
    void setDemosaicingMode(int v) {opengl_option_demosaicking_mode = v; update();}
    void renderFrames(bool v) {opengl_option_render_frames = v; update();}
    void saveImage();
    void saveRaw();
    cv::Mat getDemosaicedImage(int mode);

    void start_video();
    void stop_video();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    /*
     * Events
     */
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent * event) Q_DECL_OVERRIDE;
    QOpenGLContext *m_context;

private slots:
    void _tick();

private:
    void open_video(QString filename);
    void open_image_sequence(QStringList filenames);
    void makeObject();
    void restructureImageToUVST();
    cv::VideoCapture* _capture = NULL;
    QColor clearColor;
    cv::Mat texture;
    QStringList texture_stringlist;
    //std::vector<QImage> texture_list;
    std::vector<cv::Mat> texture_list;
    LFP_Reader::lf_meta meta_infos;
    GLuint pboIds[2];                   // IDs of PBO

    QOpenGLShaderProgram *program, *focusprogram;
    QOpenGLBuffer vbo;
    QOpenGLFunctions_4_2_Core *_func330;
    //QOpenGLFunctions_3_1 *_func330;
    QOpenGLContext *_context;
    QPoint lastPos;
    Qt::MouseButton currentButton = Qt::MidButton;
    QTimer myTimer;
    QElapsedTimer fps_timer;
    int fps_frames_elapsed = 0;
    int fps_time_elapsed = 0;
    cv::Mat pretexture;
    int tick_ms = 5;
    int index = 0;
    cv::Mat save_img;

    GLuint texture_id, framebuffer, renderedTexture_id, renderedTex_id, lightfield_id;
    float orthosize = 1.0f;
    QPointF translation = QPointF(0.0f, 0.0f);
    QPointF lens_pos_view = QPointF(0.0f, 0.0f);
    QMatrix4x4 H;
    QMatrix2x2 R_m;

    int tex_index = 0;
    int time_count = 0;
    int frame_count = 0;
    int frames_total = 0;
    float cur_u = 0, cur_v = 0;
    float focus = 0, focus_radius=0, focus_spread = 1;
    int frame_current = 0;
    int frame_max = 1;
    double overlap = 14.29;
    int opengl_view_mode = 4;
    bool opengl_option_wb = true;
    bool opengl_option_ccm = true;
    bool opengl_option_gamma = true;
    bool opengl_option_superresolution = true;
    bool opengl_option_is_demosaicked = false;
    int opengl_option_display_mode = 1;
    int opengl_decode_mode = 0;
    bool opengl_option_render_frames = false;
    bool opengl_save_current_image = false;
    bool texture_is_raw = false;
    bool is_video = false, is_imagelist = false;
    bool video_playing = false;
    int opengl_option_demosaicking_mode = 1;
    float D = 22;

signals:
    void closed();
    void refreshFPS(QString fps_text);
};
