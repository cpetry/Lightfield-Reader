#include "qopengl_lfvideoplayer.h"

#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QGenericMatrix>
#include <QFileDialog>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoWidget>

QOpengl_LFVideoPlayer::QOpengl_LFVideoPlayer(QWidget *parent, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent),
      clearColor(Qt::black),
      program(0),
      focusprogram(0)
{
    this->meta_infos = meta_infos;
}

QOpengl_LFVideoPlayer::~QOpengl_LFVideoPlayer()
{
    makeCurrent();
    vbo.destroy();
    delete program;
    doneCurrent();
}

QSize QOpengl_LFVideoPlayer::minimumSizeHint() const
{
    return QSize(625, 433);
}

QSize QOpengl_LFVideoPlayer::sizeHint() const
{
    return QSize(625, 433);
}


void QOpengl_LFVideoPlayer::setClearColor(const QColor &color)
{
    clearColor = color;
    update();
}

void QOpengl_LFVideoPlayer::initializeGL()
{
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

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 0.0f, 10.0f);
    m.translate(0.0f, 0.0f, -1.0f);

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


    // FRAMEBUFFER
    /*framebuffer = 0;
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
    _func330->glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers*/

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile("lightfield_raw.vert");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
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
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, meta_infos.width,
                    meta_infos.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
    else
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, meta_infos.width,
                    meta_infos.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);


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

}



void QOpengl_LFVideoPlayer::open_video(QString filename)
{
    if (filename.isEmpty()) // Do nothing if filename is empty
        return;

    // If its already capturing, stop and release it!
    if (_capture != NULL && _capture->isOpened())
    {
        _capture->release();
        delete _capture;
    }

    // Create a new capture interface based on the filename
    _capture = new cv::VideoCapture(filename.toStdString());
    if (!_capture->isOpened())
    {
        qDebug() << "cvWindow::_open !!! Unable to open " << filename;
        return;
    }

    // Set the filename as the window title
    setWindowTitle(filename);

    // Retrieve the width/height of the video. If not possible, then use the current size of the window
    int video_width = 0;
    video_width = _capture->get(CV_CAP_PROP_FRAME_WIDTH);
    int video_height = 0;
    video_height = _capture->get(CV_CAP_PROP_FRAME_HEIGHT);
    qDebug() << "cvWindow::_open default size is " << video_width << "x" << video_height;

    int bit_depth = _capture->get(CV_CAP_PROP_FORMAT);
    if(bit_depth == CV_8U)
        qDebug() << "cvWindow::_open format is CV_8U";
    if(bit_depth == CV_8UC1)
        qDebug() << "cvWindow::_open format is CV_8UC1";
    if(bit_depth == CV_8S)
        qDebug() << "cvWindow::_open format is CV_8S";

    if (!video_width || !video_height)
    {
        video_width = width();
        video_height = height();
    }

    resize(625, 433);
    // Retrieve fps from the video. If not available, default will be 25
    int fps = 0;
    frames_total = _capture->get(CV_CAP_PROP_FRAME_COUNT);
    fps = _capture->get(CV_CAP_PROP_FPS);
    qDebug() << "cvWindow::_open default fps is " << fps;

    if (!fps)
        fps = 30;
    // overriding frames
    //fps = 10;

    // Set FPS playback
    int tick_ms = 1000/fps;

    texture_is_raw = true;
    texture = QImage(video_width, video_height, QImage::Format_Indexed8);
    initializeGL();

    /*_capture->grab();
    _capture->retrieve(frame, CV_CAP_OPENNI_GRAY_IMAGE);
    //frame= frame.reshape(1);
    //_capture->get(CV_CAP_PROP_FORMAT);
    qDebug() << "width : " << QString::number(frame.cols) <<
                ", height: " << QString::number(frame.rows) <<
                ", channels: " << QString::number(frame.channels());*/

    // Start timer to read frames from the capture interface
    myTimer.start(tick_ms);
    QObject::connect(&myTimer, SIGNAL(timeout()), this, SLOT(_tick()));
}

void QOpengl_LFVideoPlayer::_tick()
{
    /* This method is called every couple of milliseconds.
     * It reads from the OpenCV's capture interface and saves a frame as QImage
     */
    /*qDebug() << "width : " << QString::number(frame.cols) <<
                ", height: " << QString::number(frame.rows) <<
                ", channels: " << QString::number(frame.channels());*/
    //frame= frame.reshape(1);
    frame_count++;
    cv::Mat frame;
    if (frame_count > frames_total)
    {
        _capture->set(CV_CAP_PROP_POS_FRAMES, 0);
        *_capture >> frame;
        //_capture->grab()
        //_capture->retrieve(frame);
        frame_count = 1;
        // should now work again :)
        if (_capture->get(CV_CAP_PROP_POS_FRAMES) == 0 && frame.empty()){
            qDebug() << "cvWindow::_tick !!! cant rewind";
            myTimer.stop();
            return;
        }
    }
    else
    {
        *_capture >> frame;
        //_capture->grab();
        //_capture->retrieve(frame);
    }
    //std::vector<cv::Mat> channels;
    //cv::split(frame, channels);
    //texture = QImage(channels[0].data, channels[0].cols, channels[0].rows, channels[0].step, QImage::Format_Indexed8);
    //std::memcpy(texture.scanLine(0), (unsigned char*)channels[0].data, frame.cols * frame.rows * 1); //frame.channels()

    cv::Mat channel = cv::Mat::Mat(frame.size(), frame.depth());
    int from_to[] = { 0,0 };
    cv::mixChannels( &frame, 1, &channel, 1, from_to, 1 );
    std::memcpy(texture.scanLine(0), (unsigned char*)channel.data, channel.cols * channel.rows * 1); //frame.channels()
    //cv::flip(channels[0], channels[0], 0);

    // Copy cv::Mat to QImage
    texture = texture.mirrored();
    // The same as above, but much worse.
    //QImage img = QImage((uchar*) frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    //*_image = img.copy();

    // Trigger paint event to redraw the window
    update();
}

void QOpengl_LFVideoPlayer::close_video()
{
    qDebug() << "cvWindow::_close";
    emit closed();
}

void QOpengl_LFVideoPlayer::paintGL()
{
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    m.ortho(-1, +1, +1, -1, 0.0f, 10.0f);
    m.translate(0, 0, -1.0f);
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

    //qDebug() << "Error : " << glGetError();

    glClearColor(0.7,0.8,0.2,1);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < 1; ++i) {
      glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
    //program->release();

    //myTimer.start();

}

void QOpengl_LFVideoPlayer::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void QOpengl_LFVideoPlayer::mousePressEvent(QMouseEvent *event)
{
    currentButton = event->button();
    lastPos = event->pos();
}

void QOpengl_LFVideoPlayer::mouseMoveEvent(QMouseEvent *event)
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
    //if (_capture == NULL || !_capture->isOpened())
        update();
}

void QOpengl_LFVideoPlayer::setOverlap(double o){
    overlap = o;
    //update();
}

void QOpengl_LFVideoPlayer::focus_changed(int value){
    focus = value;

    if (_capture == NULL || !_capture->isOpened())
        update();
}

/*void QOpengl_LFVideoPlayer::focus_radius_changed(int value){
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
}*/

void QOpengl_LFVideoPlayer::wheelEvent(QWheelEvent * event){
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

void QOpengl_LFVideoPlayer::mouseReleaseEvent(QMouseEvent * /* event */)
{
    currentButton = Qt::MidButton;
}

void QOpengl_LFVideoPlayer::getNextFrame(){
    // leeres Texture-Array-Objekt im Grafikspeicher anlegen:
    frame_current++;
    if (frame_current > frame_max)
        frame_current = 0;
    qWarning() << "current frame: " + QString::number(frame_current);
    //update();
}

void QOpengl_LFVideoPlayer::makeObject(){
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
