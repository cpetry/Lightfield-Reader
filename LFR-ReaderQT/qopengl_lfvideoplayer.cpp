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
      program(0)
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
    fshader->compileSourceFile("lightfield_raw.fsh");

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
    program->setUniformValue("lenslet_count", QPoint(meta_infos.width/radiusx, meta_infos.height/dy_PerLenslet_row));
    program->setUniformValue("centerLens_pos", centerLens_pos);
    program->setUniformValue("tex_dim", QPoint(meta_infos.width, meta_infos.height));
    program->setUniformValue("white_balance", QVector3D(meta_infos.r_bal,1.0f,meta_infos.b_bal));
    qDebug() << "Metainfos: "
             << QString::number(meta_infos.width) << ", "
             << QString::number(meta_infos.height) << ", "
             << QString::number(radiusx) << ", "
            << QString::number(radiusx) << ", ";


    glGenTextures(1, &texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glEnable(GL_TEXTURE_2D);

    _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
         texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

}



void QOpengl_LFVideoPlayer::open_video()
{
    // Display dialog so the user can select a file
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open Video"),
                                                    QDir::currentPath(),
                                                    tr("Files (*.*)"));

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

    // Resize the window to fit video dimensions
    //resize(video_width, video_height);
    resize(625, 433);
    //resize(this->width(), this->height());

    //resizeGL(video_width, video_height);

    // Retrieve fps from the video. If not available, default will be 25
    int fps = 0;
    frames_total = _capture->get(CV_CAP_PROP_FRAME_COUNT);
    fps = _capture->get(CV_CAP_PROP_FPS);
    qDebug() << "cvWindow::_open default fps is " << fps;

    if (!fps)
        fps = 30;
    // overriding frames
    fps = 10;

    // Set FPS playback
    int tick_ms = 1000/fps;

    LFP_Reader reader;
    std::string meta_filename = filename.split('.')[0].toStdString() + ".txt";
    std::basic_ifstream<unsigned char> input(meta_filename, std::ifstream::binary);
    if(input.is_open()){
        qDebug() << "Metainfo fount at " << QString::fromStdString(meta_filename);
        std::string text = reader.readText(input);
        QString meta_info = QString::fromStdString(text);
        reader.parseLFMetaInfo(meta_info);
        this->meta_infos = reader.meta_infos;
        qDebug() << "Metainfos loaded";
    }
    else
        qDebug() << "ERROR : Metainfo NOT found at " << QString::fromStdString(meta_filename);

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

    //texture = texture.mirrored();
    if(texture.isNull() )
        return;

    _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texture.width(),
                texture.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, texture.bits());
    //_func330->glTexImage2D(GL_TEXTURE_2D, 0, 1, frame.cols,
    //                frame.rows, 0, GL_RED, GL_UNSIGNED_BYTE, frame.data);


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

    program->setUniformValue("focus", focus);
    program->setUniformValue("focus_radius", 15);
    //program->setUniformValue("frame", frame_current);

    glActiveTexture(texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

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
