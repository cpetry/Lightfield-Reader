#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class QOpenGL_LFViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    QOpenGL_LFViewer(QWidget *parent, QImage image);
    ~QOpenGL_LFViewer();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setClearColor(const QColor &color);
    void setTextures(QList<QImage*> images);

public slots:
    void focus_changed(int value);
    void focus_radius_changed(int value);
    void getNextFrame();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent * event) Q_DECL_OVERRIDE;
    QOpenGLContext *m_context;

private:
    void makeObject();
    QColor clearColor;
    QImage texture;
    QOpenGLShaderProgram *program;
    QOpenGLBuffer vbo;
    QOpenGLFunctions_3_3_Core *_func330;
    QOpenGLContext *_context;
    QPoint lastPos;
    GLuint texture_id;
    float orthosize = 1.0f;
    QPointF translation = QPointF(0.0f, 0.0f);
    QPointF lens_pos_view = QPointF(0.0f, 0.0f);

    int tex_index = 0;
    float cur_u = 0, cur_v = 0;
    float focus = 0, focus_radius=0;
    int frame_current = 0;
    int frame_max = 1;
};
