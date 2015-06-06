#include "qopengl_lfviewer.h"


#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QGenericMatrix>
#include <QFileDialog>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoWidget>

QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QImage &image, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent),
      clearColor(Qt::black),
      program(0),focusprogram(0)
{
    this->meta_infos = meta_infos;

    if (image.format() == QImage::Format_Indexed8){
        texture_is_raw = true;
    }
    else
        texture_is_raw = false;

    texture = image.mirrored();
}

QOpenGL_LFViewer::~QOpenGL_LFViewer()
{
    makeCurrent();
    vbo.destroy();
    delete program;
    delete focusprogram;
    doneCurrent();
}

QSize QOpenGL_LFViewer::minimumSizeHint() const
{
    return QSize(625, 433);
}

QSize QOpenGL_LFViewer::sizeHint() const
{
    return QSize(625, 433);
}


void QOpenGL_LFViewer::setClearColor(const QColor &color)
{
    clearColor = color;
    update();
}

void QOpenGL_LFViewer::initializeGL()
{

    initializeOpenGLFunctions();
    _func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_2_Core>();
    //_func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_1>();
    if (_func330)
        _func330->initializeOpenGLFunctions();
    else
    {
        qWarning() << "Could not obtain required OpenGL context version";
        exit(1);
    }
    makeCurrent();
    qWarning() << "Creating Object";
    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);


#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    double centerx = meta_infos.mla_centerOffset_x / meta_infos.mla_pixelPitch;
    double centery = meta_infos.mla_centerOffset_y / meta_infos.mla_pixelPitch;
    double radiusx = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_x;
    double radiusy = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_y;
    double lenslet_rotation = meta_infos.mla_rotation;


    float dy_rot_PerLenslet = -tan(lenslet_rotation) * radiusx;
    float dx_rot_PerLenslet = tan(lenslet_rotation) * radiusy;
    QPointF centerLens_pos = QPointF(texture.width()/2.0f + centerx, texture.height()/2.0f - centery);
    float dy_PerLenslet_row = sqrt(pow(radiusx,2) - pow(radiusx/2.0f,2));
    float dx_row_odd = 0.5f*radiusx;
    float lenslet_matrix[] = {radiusx, dx_rot_PerLenslet,
                    dy_rot_PerLenslet, dy_PerLenslet_row};
    QMatrix3x3 ccm(meta_infos.cc);
    QMatrix2x2 lenslet_m(lenslet_matrix);

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 0.0f, 10.0f);
    m.translate(0.0f, 0.0f, -1.0f);

    // FRAMEBUFFER

    /*fbo = new QOpenGLFramebufferObject(texture.width(), texture.height(),
                        QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGB8);*/

    framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // The texture we're going to render to
    renderedTexture_id;
    glGenTextures(1, &renderedTexture_id);
    glBindTexture(GL_TEXTURE_2D, renderedTexture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width(), texture.height(), 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _func330->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture_id, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    _func330->glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers


    // SHADER
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile("lightfield_raw.vert");

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceFile("lightfield_raw.fsh");

    QOpenGLShader* vfocusshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vfocusshader->compileSourceFile("uvlightfield_focus.vert");

    QOpenGLShader* ffocusshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    ffocusshader->compileSourceFile("uvlightfield_focus.fsh");

    // PROGRAMS
    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->bind();  // gets automatically linked

    qDebug() << "LightfieldID" << program->uniformLocation("lightfield");
    program->setUniformValue("matrix", m);
    program->setUniformValue("lenslet_m", lenslet_m);
    program->setUniformValue("ccm", ccm.transposed());
    program->setUniformValue("modulationExposureBias", float(meta_infos.modulationExposureBias));
    program->setUniformValue("lenslet_dim", QPoint(radiusx, radiusy));
    program->setUniformValue("size_st", QPoint(meta_infos.width/radiusx, meta_infos.height/dy_PerLenslet_row));
    program->setUniformValue("centerLens_pos", centerLens_pos);
    program->setUniformValue("tex_dim", QPoint(meta_infos.width, meta_infos.height));
    program->setUniformValue("white_balance", QVector3D(meta_infos.r_bal,1.0f,meta_infos.b_bal));

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    glGenTextures(1, &texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    if (texture_is_raw)
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
                    texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());
    else
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width(),
                    texture.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, texture.bits());


    focusprogram = new QOpenGLShaderProgram;
    focusprogram->addShader(vfocusshader);
    focusprogram->addShader(ffocusshader);
    focusprogram->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    focusprogram->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    focusprogram->bind(); // gets automatically linked

    focusprogram->setUniformValue("lenslet_dim", QPoint(radiusx, radiusy));
    focusprogram->setUniformValue("size_st", QPoint(meta_infos.width/radiusx, meta_infos.height/dy_PerLenslet_row));
    focusprogram->setUniformValue("tex_dim", QPoint(meta_infos.width, meta_infos.height));

    focusprogram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    focusprogram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    focusprogram->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    focusprogram->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    //framebuffer;

        //qDebug() << "render_id: " << fbo->texture();
    qDebug() << "programID: " << program->programId();
    qDebug() << "focusID: " << focusprogram->programId();
    //qDebug() << "Error : " << glGetError();


}

void QOpenGL_LFViewer::paintGL()
{
    //fbo->bind();
    QMatrix4x4 m;
    m.ortho(-1, +1, +1, -1, 0.0f, 10.0f);
    m.translate(0,0, -1.0f);
    QMatrix4x4 m_result;
    m_result.ortho(-orthosize, +orthosize, +orthosize, -orthosize, 0.0f, 10.0f);
    m_result.translate(translation.x(), translation.y(), -1.0f);


    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    program->bind();
    //qDebug() << "program bind : " << program->bind();
    program->setUniformValue("view_mode", opengl_view_mode);
    program->setUniformValue("option_wb", opengl_option_wb);
    program->setUniformValue("option_ccm", opengl_option_ccm);
    program->setUniformValue("option_gamma", opengl_option_gamma);
    program->setUniformValue("option_superresolution", opengl_option_superresolution);

    program->setUniformValue("matrix", m); // m_result, m
    program->setUniformValue("lens_pos_view", lens_pos_view);
    program->setUniformValue("focus", focus);

    //glGetError();
    //qDebug() << "Error : " << glGetError();



    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR framebuffer incomplete";

    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,texture.width(),texture.height());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, texture_id);
    glUniform1i(program->uniformLocation("lightfield"), 0);
    if (texture_is_raw){
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
                    texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());
        program->setUniformValue("is_raw", true);
    }
    else{
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width(),
                    texture.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, texture.bits());
        program->setUniformValue("is_raw", false);
    }
    if (texture.isNull())
        return;

    glClearColor(0.7,0.8,0.2,1);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < 1; ++i) {
      glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }

    // restore default FBO
    glBindFramebuffer( GL_FRAMEBUFFER, defaultFramebufferObject());

    focusprogram->bind();
    focusprogram->setUniformValue("matrix", m_result);
    focusprogram->setUniformValue("lens_pos_view", lens_pos_view);
    focusprogram->setUniformValue("uv_coord", QPointF(0,0));
    focusprogram->setUniformValue("focus", focus);

    glViewport(0,0,width(),height()); // Render on the screen
    glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT );

    glBindTexture(GL_TEXTURE_2D, renderedTexture_id);
    glUniform1i(focusprogram->uniformLocation("renderedTexture"), 0);
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }


    //glDisable(GL_TEXTURE_2D);
    //glFlush ();
}



void QOpenGL_LFViewer::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void QOpenGL_LFViewer::mousePressEvent(QMouseEvent *event)
{
    currentButton = event->button();
    lastPos = event->pos();
}

void QOpenGL_LFViewer::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (currentButton == Qt::LeftButton){
        translation = QPointF(translation.x() + dx * 0.004f * orthosize, translation.y() + dy * 0.004f * orthosize);
    }
    else if(currentButton == Qt::RightButton){
        //lens_pos_view = QPointF(lens_pos_view.x() + dx * 0.01f, lens_pos_view.y() - dy * 0.01f);
        focus += dy * 0.001f;
    }
    lastPos = event->pos();
    if (_capture == NULL || !_capture->isOpened())
        update();
}

void QOpenGL_LFViewer::setOverlap(double o){
    overlap = o;
    //update();
}

void QOpenGL_LFViewer::focus_changed(int value){
    focus = value / 100.0f;

    if (_capture == NULL || !_capture->isOpened())
        update();
}

void QOpenGL_LFViewer::focus_radius_changed(int value){
    int max_value = 14;
    int min_value = 2;
    focus_radius = value / 100.0f;
    if (focus_radius + cur_u >= max_value)
        cur_u = max_value - focus_radius;
    else if (cur_u - focus_radius <= min_value)
        cur_u = min_value + focus_radius;
    if (focus_radius + cur_v >= max_value)
        cur_v = max_value - focus_radius;
    else if (cur_v - focus_radius <= min_value)
        cur_v = min_value + focus_radius;
    if (_capture == NULL || !_capture->isOpened())
        update();
}

void QOpenGL_LFViewer::wheelEvent(QWheelEvent * event){
    double scaleFactor = 1.15;
    if(event->delta() > 0) {
        orthosize *= 1.0 / scaleFactor;
    } else {
        // Zooming out
        orthosize *= scaleFactor;
    }

    if (_capture == NULL || !_capture->isOpened())
        update();
}

void QOpenGL_LFViewer::mouseReleaseEvent(QMouseEvent * /* event */)
{
    currentButton = Qt::MidButton;
}

void QOpenGL_LFViewer::makeObject(){
    static const int coords[4][3] = {
        { +1, -1, 0 }, { -1, -1, 0 }, { -1, +1, 0 }, { +1, +1, 0 }
    };

    QVector<GLfloat> vertData;
    for (int j = 0; j < 4; ++j) {
        // vertex position
        vertData.append(coords[j][0]);
        vertData.append(coords[j][1]);
        vertData.append(coords[j][2]);
        // texture coordinate
        vertData.append(j == 0 || j == 3);
        vertData.append(j == 0 || j == 1);
    }

    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}

void QOpenGL_LFViewer::saveRaw(){
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");

    QVector<QRgb> colorTable;
    QImage retImg(texture.width(),texture.height(),QImage::Format_Indexed8);
    QVector<QRgb> table( 256 );
    for( int i = 0; i < 256; ++i )
    {
        table[i] = qRgb(i,i,i);
    }
    retImg.setColorTable(table);
    for(int i =0; i< texture.width();i++)
    {
        for(int j=0; j< texture.height();j++)
        {
            QRgb value = texture.pixel(i,j);
            retImg.setPixel(i,j,qRed(value));
        }

    }

    if (filename != ""){
        retImg.save(filename);
    }

    //saveMetaInfo(filename);
}


void QOpenGL_LFViewer::saveImage(){
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
    //update();
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");
    if (filename != ""){
        this->grabFramebuffer().save(filename);
    }

    //saveMetaInfo(filename);
}

