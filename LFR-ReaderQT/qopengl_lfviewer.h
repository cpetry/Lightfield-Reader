#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
//#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLFunctions_3_3_Core>
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
    QOpenGL_LFViewer(QWidget *parent, QImage &image, LFP_Reader::lf_meta meta_infos);
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
    void buttonDisplayClicked(){ opengl_view_mode = 3; update();}
    void toggleWhiteBalance(bool v){ opengl_option_wb = v; update();}
    void toggleCCM(bool v){ opengl_option_ccm = v;  update();}
    void toggleGamma(bool v){ opengl_option_gamma = v;  update();}
    void toggleSuperResolution(bool v){ opengl_option_superresolution = v; update();}
    void renderDemosaic(bool v) {opengl_option_is_demosaicked = v; update();}
    void saveImage();
    void saveRaw();

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
    void close_video();
    void makeObject();
    void restructureImageToUVST();
    cv::VideoCapture* _capture = NULL;
    QColor clearColor;
    QImage texture;
    LFP_Reader::lf_meta meta_infos;

    QOpenGLShaderProgram *program, *focusprogram;
    QOpenGLBuffer vbo;
    QOpenGLFunctions_3_3_Core *_func330;
    //QOpenGLFunctions_3_1 *_func330;
    QOpenGLContext *_context;
    QPoint lastPos;
    Qt::MouseButton currentButton = Qt::MidButton;
    QTimer myTimer;
    cv::Mat channel;

    GLuint texture_id, framebuffer, renderedTexture_id, renderedTex_id, lightfield_id;
    float orthosize = 1.0f;
    QPointF translation = QPointF(0.0f, 0.0f);
    QPointF lens_pos_view = QPointF(0.0f, 0.0f);

    int tex_index = 0;
    int time_count = 0;
    int frame_count = 0;
    int frames_total = 0;
    float cur_u = 0, cur_v = 0;
    float focus = 0, focus_radius=0;
    int frame_current = 0;
    int frame_max = 1;
    double overlap = 14.29;
    int opengl_view_mode = 3;
    bool opengl_option_wb = true;
    bool opengl_option_ccm = true;
    bool opengl_option_gamma = true;
    bool opengl_option_superresolution = true;
    bool opengl_option_is_demosaicked = false;
    bool texture_is_raw = false;
    bool is_video = false;

signals:
    void closed();
};
