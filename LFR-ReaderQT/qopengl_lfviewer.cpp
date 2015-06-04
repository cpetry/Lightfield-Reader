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
      program(0)
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
    frame_max = 10;

    initializeOpenGLFunctions();
    _func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
    //_func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_1>();
    if (_func330)
        _func330->initializeOpenGLFunctions();
    else
    {
        qWarning() << "Could not obtain required OpenGL context version";
        exit(1);
    }
    qWarning() << "Creating Object";
    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute highp vec4 texCoord;\n"
        "varying highp vec4 texc;\n"
        "uniform highp mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    if (texture_is_raw)
        fshader->compileSourceFile("lightfield_raw.fsh");
    else
        fshader->compileSourceFile("lightfield.fsh");

    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 0.0f, 10.0f);
    m.translate(0.0f, 0.0f, -1.0f);

    program->setUniformValue("matrix", m);

    double centerx = meta_infos.mla_centerOffset_x / meta_infos.mla_pixelPitch;
    double centery = meta_infos.mla_centerOffset_y / meta_infos.mla_pixelPitch;
    double radiusx = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_x;
    double radiusy = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_y;
    double lenslet_rotation = meta_infos.mla_rotation;


    float dy_rot_PerLenslet_hori = -tan(lenslet_rotation) * radiusx;
    float dx_rot_PerLenslet_vert = tan(lenslet_rotation) * radiusy;
    QPointF centerLens_pos = QPointF(texture.width()/2.0f + centerx, texture.height()/2.0f - centery);
    float dy_PerLenslet_row = sqrt(pow(radiusx,2) - pow(radiusx/2.0f,2));
    float dx_row_odd = 0.5f*radiusx;
    float lens_letmatrix[] = {radiusx, dx_rot_PerLenslet_vert,
                    dy_rot_PerLenslet_hori, dy_PerLenslet_row};
    QMatrix3x3 ccm(meta_infos.cc);
    QMatrix2x2 lenslet_m(lens_letmatrix);
    program->setUniformValue("lenslet_m", lenslet_m);
    program->setUniformValue("ccm", ccm.transposed());
    program->setUniformValue("modulationExposureBias", float(meta_infos.modulationExposureBias));
    program->setUniformValue("dy_rot_PerLenslet_hori", dy_rot_PerLenslet_hori);
    program->setUniformValue("dx_rot_PerLenslet_vert", dx_rot_PerLenslet_vert);
    program->setUniformValue("dy_PerLenslet_row", dy_PerLenslet_row);
    program->setUniformValue("dx_row_odd", dx_row_odd);
    program->setUniformValue("lenslet_dim", QPoint(radiusx, radiusy));
    program->setUniformValue("centerLens_pos", centerLens_pos);
    program->setUniformValue("tex_dim", QPoint(meta_infos.width, meta_infos.height));
    program->setUniformValue("white_balance", QVector3D(meta_infos.r_bal,1.0f,meta_infos.b_bal));


    glGenTextures(1, &texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glEnable(GL_TEXTURE_2D);

    if (texture_is_raw)
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
                    texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());
    else
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width(),
                    texture.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, texture.bits());

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    //open_video();
    //texture = QImage(0,0);
    //qWarning() << texture.byteCount();

    //QTimer *timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    //timer->start(1000/30);
}

void QOpenGL_LFViewer::paintGL()
{
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (texture.isNull())
    {
        return;
    }

    _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
                texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());




    QMatrix4x4 m;
    m.ortho(-orthosize, +orthosize, +orthosize, -orthosize, 0.0f, 10.0f);
    m.translate(translation.x(), translation.y(), -1.0f);

    program->setUniformValue("view_mode", opengl_view_mode);
    program->setUniformValue("option_wb", opengl_option_wb);
    program->setUniformValue("option_ccm", opengl_option_ccm);
    program->setUniformValue("option_gamma", opengl_option_gamma);
    program->setUniformValue("option_superresolution", opengl_option_superresolution);


    program->setUniformValue("matrix", m);
    program->setUniformValue("lens_pos_view", lens_pos_view);
    /*
    program->setUniformValue("u_coord", cur_u);
    program->setUniformValue("v_coord", cur_v);
    program->setUniformValue("focus", focus);
    program->setUniformValue("focus_radius", focus_radius);
    program->setUniformValue("frame", frame_current);
    */
    glActiveTexture(texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }

    //program->release();

    //myTimer.start();

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
        lens_pos_view = QPointF(lens_pos_view.x() + dx * 0.01f, lens_pos_view.y() - dy * 0.01f);
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

