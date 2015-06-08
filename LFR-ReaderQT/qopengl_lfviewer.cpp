#include "qopengl_lfviewer.h"


#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QGenericMatrix>
#include <QFileDialog>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoWidget>

QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QString file, bool is_video, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent), clearColor(Qt::black), program(0), focusprogram(0) {
    this->meta_infos = meta_infos;

    opengl_option_is_demosaicked = false;
    this->is_video = is_video;

    if (!is_video){
        texture = QImage(file);
        if (texture.format() == QImage::Format_Indexed8){
            texture_is_raw = true;
        }
        else
            texture_is_raw = false;

        texture = texture.mirrored();
    }
    else{
        open_video(file);
    }
}


QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QImage &image, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent), clearColor(Qt::black), program(0), focusprogram(0){
    this->meta_infos = meta_infos;

    if (image.format() == QImage::Format_Indexed8)
        texture_is_raw = true;
    else
        texture_is_raw = false;

    texture = image.mirrored();
    is_video = false;
    opengl_option_is_demosaicked = false;
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

void QOpenGL_LFViewer::open_video(QString filename)
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
    fps = 25;
    _capture->set(CV_CAP_PROP_FPS, fps);

    // Set FPS playback
    int tick_ms = 1000/fps;

    texture_is_raw = true;
    texture = QImage(video_width, video_height, QImage::Format_Indexed8);
    //initializeGL();

    /*_capture->grab();
    _capture->retrieve(frame, CV_CAP_OPENNI_GRAY_IMAGE);
    //frame= frame.reshape(1);
    //_capture->get(CV_CAP_PROP_FORMAT);
    qDebug() << "width : " << QString::number(frame.cols) <<
                ", height: " << QString::number(frame.rows) <<
                ", channels: " << QString::number(frame.channels());*/

    // Start timer to read frames from the capture interface
    channel = cv::Mat::Mat(cv::Size(video_width, video_height), 0); // frame will have depth 0 ?!

    myTimer.start(tick_ms);
    QObject::connect(&myTimer, SIGNAL(timeout()), this, SLOT(_tick()));
}

void QOpenGL_LFViewer::_tick()
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

    int from_to[] = { 0,0 };
    cv::mixChannels( &frame, 1, &channel, 1, from_to, 1 );
    //std::memcpy(texture.scanLine(0), (unsigned char*)channel.data, channel.cols * channel.rows); //frame.channels()
    cv::flip(channel, channel, 0);

    // Copy cv::Mat to QImage
    //texture = texture.mirrored();
    // The same as above, but much worse.
    //QImage img = QImage((uchar*) frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    //*_image = img.copy();

    // Trigger paint event to redraw the window
    update();
}

void QOpenGL_LFViewer::close_video()
{
    qDebug() << "cvWindow::_close";
    emit closed();
}

void QOpenGL_LFViewer::initializeGL()
{
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

    double centerx = meta_infos.mla_centerOffset_x / meta_infos.mla_pixelPitch;
    double centery = meta_infos.mla_centerOffset_y / meta_infos.mla_pixelPitch;
    double radiusx = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_x;
    double radiusy = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_y;
    double lenslet_rotation = meta_infos.mla_rotation;


    float dy_rot_PerLenslet = -tan(lenslet_rotation) * radiusx;
    float dx_rot_PerLenslet = tan(lenslet_rotation) * radiusy;
    QPointF centerLens_pos = QPointF(meta_infos.width/2.0f + centerx, meta_infos.height/2.0f - centery);
    float dy_PerLenslet_row = sqrt(pow(radiusx,2) - pow(radiusx/2.0f,2));
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, meta_infos.width, meta_infos.height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
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

    //qDebug() << "LightfieldID" << program->uniformLocation("lightfield");
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
                    meta_infos.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, ((is_video) ? NULL : texture.bits()));
    else
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, meta_infos.width,
                    meta_infos.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, ((is_video) ? NULL : texture.bits()));


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
    //qDebug() << "programID: " << program->programId();
    //qDebug() << "focusID: " << focusprogram->programId();
    //qDebug() << "Error : " << glGetError();


}

void QOpenGL_LFViewer::paintGL()
{
    //fbo->bind();
    QMatrix4x4 m_result;
    if (opengl_view_mode != 3){
        m_result.ortho(-orthosize, +orthosize, +orthosize, -orthosize, 0.0f, 10.0f);
        m_result.translate(translation.x(), translation.y(), -1.0f);
    }
    else{
        m_result.ortho(-1, +1, +1, -1, 0.0f, 10.0f);
        m_result.translate(0, 0, -1.0f);
    }

    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (texture_is_raw || !opengl_option_is_demosaicked)
        restructureImageToUVST();


    //glBindFramebuffer( GL_FRAMEBUFFER, defaultFramebufferObject());
    focusprogram->bind();
    focusprogram->setUniformValue("matrix", m_result);
    focusprogram->setUniformValue("lens_pos_view", lens_pos_view);
    focusprogram->setUniformValue("uv_coord", QPointF(0,0));
    focusprogram->setUniformValue("focus", focus);
    focusprogram->setUniformValue("view_mode", opengl_view_mode);

    glViewport(0,0,width(),height()); // Render on the screen
    glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT );

    if (texture_is_raw || !opengl_option_is_demosaicked)
        glBindTexture(GL_TEXTURE_2D, renderedTexture_id);
    else
        glBindTexture(GL_TEXTURE_2D, texture_id);
    glUniform1i(focusprogram->uniformLocation("renderedTexture"), 0);
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }


    //glDisable(GL_TEXTURE_2D);
    //glFlush ();
}

void QOpenGL_LFViewer::restructureImageToUVST(){
    QMatrix4x4 m;
    m.ortho(-1, +1, +1, -1, 0.0f, 10.0f);
    m.translate(0,0, -1.0f);
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
    glViewport(0,0, meta_infos.width, meta_infos.height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, texture_id);
    glUniform1i(program->uniformLocation("lightfield"), 0);
    if (texture_is_raw){
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, meta_infos.width,
                    meta_infos.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, ((is_video) ? channel.data : texture.bits()));
        program->setUniformValue("is_raw", true);
    }
    else{
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, meta_infos.width,
                    meta_infos.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, ((is_video) ? channel.data : texture.bits()));
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
        focus -= dy * 0.001f;
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

