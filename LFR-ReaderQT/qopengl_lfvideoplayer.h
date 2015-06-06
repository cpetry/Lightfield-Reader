#ifndef QOPENGL_LFVIDEOPLAYER_H
#define QOPENGL_LFVIDEOPLAYER_H

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
//#include <QOpenGLFunctions_3_1>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <QTimer>
#include <QMovie>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


#include "lfp_reader.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class QOpengl_LFVideoPlayer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /*
     * Initializes the viewer with a lightfield-image
     */
    QOpengl_LFVideoPlayer(QWidget *parent, LFP_Reader::lf_meta meta_infos);
    ~QOpengl_LFVideoPlayer();

    /*
     * Defines the minimum size of the widget
     */
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setClearColor(const QColor &color);

public slots:
    void focus_changed(int value);
    //void focus_radius_changed(int value);
    void getNextFrame();
    void setOverlap(double o);

    void buttonGrayClicked(){ opengl_view_mode = 0; update();}
    void buttonBayerClicked(){ opengl_view_mode = 1; update();}
    void buttonDemosaicClicked(){ opengl_view_mode = 2; update();}
    void buttonDisplayClicked(){ opengl_view_mode = 3; update();}
    void toggleWhiteBalance(bool v){ opengl_option_wb = v; update();}
    void toggleCCM(bool v){ opengl_option_ccm = v;  update();}
    void toggleGamma(bool v){ opengl_option_gamma = v;  update();}
    void toggleSuperResolution(bool v){ opengl_option_superresolution = v; update();}
    void open_video();

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
    void close_video();
    void makeObject();
    cv::VideoCapture* _capture = NULL;
    QColor clearColor;
    QImage texture;

    bool texture_is_raw = false;
    LFP_Reader::lf_meta meta_infos;

    QOpenGLShaderProgram *program, *focusprogram;
    QOpenGLBuffer vbo;
    QOpenGLFunctions_3_3_Core *_func330;
    //QOpenGLFunctions_3_1 *_func330;
    QOpenGLContext *_context;
    QPoint lastPos;
    Qt::MouseButton currentButton = Qt::MidButton;
    QTimer myTimer;


    float orthosize = 1.0f;
    QPointF translation = QPointF(0.0f, 0.0f);
    QPointF lens_pos_view = QPointF(0.0f, 0.0f);
    GLuint texture_id, framebuffer, renderedTexture_id, renderedTex_id, lightfield_id;

    int tex_index = 0;
    int time_count = 0;
    int frame_count = 0;
    int frames_total = 0;
    float focus = 0, focus_radius=15;
    int frame_current = 0;
    int frame_max = 1;
    double overlap = 14.29;
    int opengl_view_mode = 0;
    bool opengl_option_wb = true;
    bool opengl_option_ccm = true;
    bool opengl_option_gamma = true;
    bool opengl_option_superresolution = true;

signals:
    void closed();
};

#endif // QOPENGL_LFVIDEOPLAYER_H
