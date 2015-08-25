#include "qopengl_lfviewer.h"


#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QGenericMatrix>
#include <QFileDialog>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QMessageBox>
#include <QOpenGLShaderProgram>

#include "depthcostaware.h"

QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QString file, bool is_video, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent), clearColor(Qt::black), program(0), focusprogram(0) {
    this->meta_infos = meta_infos;

    opengl_option_is_demosaicked = false;
    this->is_video = is_video;
    this->is_imagelist = false;

    if (!is_video){
        int channels = (QImage(file).format() == QImage::Format_Indexed8) ? 1 : 3;
        cv::Mat tex = cv::imread(file.toStdString(), 1); //loads color if it is available

        texture_is_raw = (channels == 1);
        if (texture_is_raw){
            int from_to[] = { 0,0 };
            texture = cv::Mat::Mat(cv::Size(tex.cols, tex.rows), 0); // frame will have depth 0 ?!
            cv::mixChannels( &tex, 1, &texture, 1, from_to,1 );
        }
        else{
            cv::cvtColor(tex, texture, CV_BGR2RGBA); // has to be RGBA ... -.-
        }

        if (this->meta_infos.width != texture.cols
            || this->meta_infos.height != texture.rows){
            int ret = QMessageBox::warning(this, tr("Warning!"),
                                 QString(QString("Meta Info seems not to be correct.\n") +
                                   "Width: Meta-Image " + QString::number(this->meta_infos.width) + " " + QString::number(texture.cols) +"\n"+
                                   "Height: Meta-Image " + QString::number(this->meta_infos.height) + " " + QString::number(texture.rows) +"\n"+
                                   "Do you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
        }
        this->meta_infos.width = texture.cols;
        this->meta_infos.height = texture.rows;

        cv::flip(texture, texture, 0);
    }
    else{
        open_video(file);
    }
}


QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QStringList files, bool is_video, LFP_Reader::lf_meta meta_infos)
    : QOpenGLWidget(parent), clearColor(Qt::black), program(0), focusprogram(0) {

    this->meta_infos = meta_infos;

    opengl_option_is_demosaicked = false;
    this->is_video = is_video;
    this->is_imagelist = true;
    texture_stringlist = files;
    open_image_sequence(files);
}


QOpenGL_LFViewer::~QOpenGL_LFViewer()
{
    makeCurrent();
    if (pboIds[0] != 0 && pboIds[1] != 0)
        _func330->glDeleteBuffers(2, pboIds);
    texture.release();
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

void QOpenGL_LFViewer::open_image_sequence(QStringList filenames){
    texture = cv::imread(filenames.at(0).toStdString(), 1); //loads color if it is available
    int channels = (QImage(filenames.at(0)).format() == QImage::Format_Indexed8) ? 1 : 3;
    texture_is_raw = (channels == 1);

    // read in first texture to get some info
    if (texture_is_raw){
        int from_to[] = { 0,0 };
        pretexture = cv::Mat::Mat(cv::Size(texture.cols, texture.rows), 0);
        cv::mixChannels( &texture, 1, &pretexture, 1, from_to, 1);
        cv::flip(pretexture, pretexture, 0);
    }
    else{
        cv::cvtColor(texture, texture, CV_BGR2RGBA); // has to be RGBA ... -.-
        pretexture = cv::Mat::Mat(cv::Size(texture.cols, texture.rows), texture.channels());
        cv::flip(texture, texture, 0);
    }
    this->meta_infos.width = texture.cols;
    this->meta_infos.height = texture.rows;

    // read in every other texture!
    for(int i=0; i< filenames.length(); i++){
        cv::Mat img = cv::imread(filenames[i].toStdString(), 1);//loads color if it is available
        if (texture_is_raw){
            int from_to[] = { 0,0 };
            cv::Mat channel = cv::Mat::Mat(cv::Size(img.cols, img.rows), 0);
            cv::mixChannels( &img, 1, &channel, 1, from_to, 1);
            cv::flip(channel, channel, 0);
            texture_list.push_back(channel);
        }
        else{
            cv::cvtColor(img, img, CV_BGR2RGBA); // has to be RGBA ... -.-
            cv::flip(img, img, 0);
            texture_list.push_back(img);
        }
    }
    frames_total = filenames.length();
    opengl_option_is_demosaicked = true;

    //int fps = 1;
    //tick_ms = 1000/fps;

    //start_video();
    //myTimer.singleShot(200,SLOT(_tick()));
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
    qDebug() << "Bit Depth: " << QString::number(bit_depth);
    if(bit_depth == CV_8U)
        qDebug() << "cvWindow::_open format is CV_8U";
    else if(bit_depth == CV_8UC1)
        qDebug() << "cvWindow::_open format is CV_8UC1";
    else if(bit_depth == CV_8UC2)
        qDebug() << "cvWindow::_open format is CV_8UC2";
    else if(bit_depth == CV_8UC3)
        qDebug() << "cvWindow::_open format is CV_8UC3";
    else if(bit_depth == CV_8UC4)
        qDebug() << "cvWindow::_open format is CV_8UC4";
    else if(bit_depth == CV_8S)
        qDebug() << "cvWindow::_open format is CV_8S";
    else if(bit_depth == CV_32F)
        qDebug() << "cvWindow::_open format is CV_32F";
    else if(bit_depth == CV_32S)
        qDebug() << "cvWindow::_open format is CV_32S";
    else if(bit_depth == CV_32SC1)
        qDebug() << "cvWindow::_open format is CV_32SC1";
    else if(bit_depth == CV_32SC3)
        qDebug() << "cvWindow::_open format is CV_32SC3";
    else if(bit_depth == CV_64F)
        qDebug() << "cvWindow::_open format is CV_64F";

    if (this->meta_infos.width != video_width
        || this->meta_infos.height != video_height){
        int ret = QMessageBox::warning(this, tr("Warning!"),
                             QString(QString("Meta Info seems not to be correct.\n") +
                               "Width: Meta-Image " + QString::number(this->meta_infos.width) + " " + QString::number(video_width) +"\n"+
                               "Height: Meta-Image " + QString::number(this->meta_infos.height) + " " + QString::number(video_height) +"\n"+
                               "Do you want to continue?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }

    if (!video_width || !video_height){
        video_width = width();
        video_height = height();
    }
    else{
        meta_infos.width = video_width;
        meta_infos.height = video_height;
    }

    frames_total = _capture->get(CV_CAP_PROP_FRAME_COUNT);

    int image_type = QMessageBox::warning(this, tr("Image Format"),
                         tr("What kind of video?\n"
                            "Raw, Demosaiced or UV-ST?"),
                         "Raw", "Demosaiced", "UV-ST",0);
    texture_is_raw = (image_type == 0);
    opengl_option_is_demosaicked = (image_type == 2);


    qDebug() << "Reading first frame";
    cv::Mat frame;

    if (texture_is_raw){
        _capture->read(frame);
        int from_to[] = { 0,0 };
        texture = cv::Mat::Mat(cv::Size(frame.cols, frame.rows), 0);
        cv::mixChannels( &frame, 1, &texture, 1, from_to,1 );
        // init single channel
        pretexture = cv::Mat::Mat(cv::Size(frame.cols, frame.rows), 0);
    }
    else{
        //qDebug() << QString::number(_capture->get(CV_CAP_PROP_FOURCC));
        //return;
        _capture->read(frame);

        cv::cvtColor(frame, texture, CV_BGR2RGBA); // has to be RGBA ... -.-
        // init single channel
        pretexture = cv::Mat::Mat(cv::Size(frame.cols, frame.rows), frame.channels());
    }
    cv::flip(texture, texture, 0);
    //
    //qDebug() << "Channel has x: " << QString::number(channel.cols) << ", y:" << QString::number(channel.rows) << " channels";
    //qDebug() << "Channel has " << QString::number(channel.channels()) << " channels";




    // reset to position 0
    _capture->set(CV_CAP_PROP_POS_FRAMES, 0);

    resize(625, 433);
    // Retrieve fps from the video. If not available, default will be 25

    //start_video();
}

void QOpenGL_LFViewer::start_video(){
    video_playing = !video_playing;
    if (video_playing)
        myTimer.singleShot(200,SLOT(_tick()));
    //QObject::connect(&myTimer, SIGNAL(timeout()), this, SLOT(_tick()));
}

void QOpenGL_LFViewer::_tick()
{
    /* This method is called every couple of milliseconds.
     * It reads from the OpenCV's capture interface and saves a frame as QImage
     */
    //qDebug() << "tick!";
    /*qDebug() << "width : " << QString::number(frame.cols) <<
                ", height: " << QString::number(frame.rows) <<
                ", channels: " << QString::number(frame.channels());*/
    //frame= frame.reshape(1);

    if (!is_imagelist){
        if (frame_count >= frames_total)
        {
            frame_count = 0;
            _capture->set(CV_CAP_PROP_POS_FRAMES, 0);
            if (_capture->get(CV_CAP_PROP_POS_FRAMES) != 0){
                qDebug() << "cvWindow::_tick !!! cant rewind";
                //myTimer.stop();
                return;
            }
        }

        // start to copy from PBO to texture object ///////
        // Pixel Buffer Object indices
        index = (index + 1) % 2;
        int nextIndex = (index + 1) % 2;

        // bind the texture and PBO
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[index]);

        // copy pixels from PBO to texture object
        // Use offset instead of pointer.
        if (texture_is_raw){
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.cols, texture.rows,
                        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        }
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.cols, texture.rows,
                        GL_RGBA, GL_UNSIGNED_BYTE, 0);

        // bind PBO to update pixel values
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nextIndex]);

        // map the buffer object into client's memory
        // Note that glMapBufferARB() causes sync issue.
        // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
        // for GPU to finish its job. To avoid waiting (stall), you can call
        // first glBufferDataARB() with NULL pointer before glMapBufferARB().
        // If you do that, the previous data in PBO will be discarded and
        // glMapBufferARB() returns a new allocated pointer immediately
        // even if GPU is still working with the previous data.
        glBufferData(GL_PIXEL_UNPACK_BUFFER, texture.cols * texture.rows * texture.channels(),
                        0, GL_STREAM_DRAW);
        GLubyte* ptr = (GLubyte*)_func330->glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if(ptr)
        {
            texture.data = ptr;
            _func330->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
        }

        // it is good idea to release PBOs with ID 0 after use.
        // Once bound with 0, all pixel operations behave normal ways.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        cv::Mat frame;
        _capture->read(frame); // reads always three channels

        if (texture_is_raw){
            int from_to[] = { 0,0 };
            cv::mixChannels( &frame, 1, &pretexture, 1, from_to,1 );
            cv::flip(pretexture, texture, 0);

        }
        else{
            cv::cvtColor(frame, pretexture, CV_BGR2RGBA); // has to be RGBA ... -.-
            cv::flip(pretexture, texture, 0);
        }
    }
    else{
        if (frame_count >= frames_total)
            frame_count = 0;

        // start to copy from PBO to texture object ///////
        // Pixel Buffer Object indices
        index = (index + 1) % 2;
        int nextIndex = (index + 1) % 2;

        // bind the texture and PBO
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[index]);

        // copy pixels from PBO to texture object
        // Use offset instead of ponter.
        if (texture_is_raw)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.cols, texture.rows,
                        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.cols, texture.rows,
                        GL_RGBA, GL_UNSIGNED_BYTE, 0);

        // bind PBO to update pixel values
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nextIndex]);

        // map the buffer object into client's memory
        // Note that glMapBufferARB() causes sync issue.
        // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
        // for GPU to finish its job. To avoid waiting (stall), you can call
        // first glBufferDataARB() with NULL pointer before glMapBufferARB().
        // If you do that, the previous data in PBO will be discarded and
        // glMapBufferARB() returns a new allocated pointer immediately
        // even if GPU is still working with the previous data.
        glBufferData(GL_PIXEL_UNPACK_BUFFER, texture.cols * texture.rows * texture.channels(),
                        0, GL_STREAM_DRAW);
        GLubyte* ptr = (GLubyte*)_func330->glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if(ptr)
        {
            texture.data = ptr;
            _func330->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
        }

        // it is good idea to release PBOs with ID 0 after use.
        // Once bound with 0, all pixel operations behave normal ways.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        texture_list[frame_count].copyTo(texture);
    }

    // Trigger paint event to redraw the window
    update();
    frame_count++;
}

void QOpenGL_LFViewer::stop_video()
{
    video_playing = false;
    // reset to position 0
    _capture->set(CV_CAP_PROP_POS_FRAMES, 0);

}

void QOpenGL_LFViewer::initializeGL(){
    initializeOpenGLFunctions();

    // SHADER
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    bool compiled = vshader->compileSourceFile(":/lightfield_raw.vert");

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    compiled = compiled && fshader->compileSourceFile(":/lightfield_raw.fsh");

    QOpenGLShader* vfocusshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    compiled = compiled && vfocusshader->compileSourceFile(":/uvlightfield_focus.vert");

    QOpenGLShader* ffocusshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    compiled = compiled && ffocusshader->compileSourceFile(":/uvlightfield_focus.fsh");

    if(!compiled)
        QMessageBox::warning(this, tr("Warning!"),
        QString(QString("Could not compile shaders!\n")),
        QMessageBox::Ok);

    _func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_2_Core>();
    //_func330 = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_1>();
    if (_func330)
        _func330->initializeOpenGLFunctions();
    else
    {
        QMessageBox::warning(this, tr("Warning!"),
        QString(QString("Could not obtain required OpenGL context version\nNot able to display!")),
        QMessageBox::Ok);
        return;
    }
    qWarning() << "Creating Object";
    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);


#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

    QMatrix3x3 ccm(meta_infos.cc);
    double exact_uv = (meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch);
    double centerx = meta_infos.mla_centerOffset_x / meta_infos.mla_pixelPitch;
    double centery = meta_infos.mla_centerOffset_y / meta_infos.mla_pixelPitch;
    double radiusx = exact_uv * meta_infos.mla_scale_x;
    double radiusy = exact_uv * meta_infos.mla_scale_y;
    double lenslet_rotation = meta_infos.mla_rotation;

    QPointF centerLens_pos = QPointF(meta_infos.width/2.0f + centerx, meta_infos.height/2.0f - centery);
    double dy_PerLenslet_row = sqrt(pow(radiusx,2) - pow(radiusx/2.0f,2))* meta_infos.mla_scale_y;
    //double dy_PerLenslet_row = sqrt(pow(15.0,2) - pow(15.0/2.0f,2))* meta_infos.mla_scale_y;

    float lenslet_matrix[] = {radiusx, 0, 0, dy_PerLenslet_row* meta_infos.mla_scale_y };
    //float lenslet_matrix[] = {1, 0, 0, dy_PerLenslet_row/15.0};

    QMatrix2x2 lenslet_m(lenslet_matrix);

    float R_matrix[] = {std::cos(lenslet_rotation), std::sin(lenslet_rotation),
                       -std::sin(lenslet_rotation), std::cos(lenslet_rotation)};
    R_m = QMatrix2x2(R_matrix);
    //lenslet_m = R_m * lenslet_m;

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 0.0f, 10.0f);
    m.translate(0.0f, 0.0f, -1.0f);


    double pix_size = meta_infos.mla_pixelPitch; // in m
    double lens_size = meta_infos.mla_lensPitch; // in m
    float N = 15;//lens_size / pix_size; // 15 number of pixels per lenslet
    int c_pix = 7; // translational pixel offset ?!
    double c_Mx = meta_infos.mla_centerOffset_x / pix_size;   // optical center offset in mm
    double c_My = -meta_infos.mla_centerOffset_y / pix_size;    // optical center offset in mm
    double c_mux = -7.3299932479858395e-6;  // mla offset in mm
    double c_muy = 5.5686492919921878e-6;   // mla offset in mm
    double d_mu = 3.6999999999999998e-5;  // mla offset in mm
    double f_M = 0.011542153404246169;    // focal length
    double exitPupilOffset = 0.11559105682373047; // distance from the pupil to microlens plane
    double H_data[25];
    DepthCostAware::calcH(pix_size, lens_size, N, c_pix, c_Mx, c_My,
                          c_mux, c_muy, d_mu, f_M, exitPupilOffset, D, H_data);

    cv::Mat H_mat = cv::Mat(5,5, cv::DataType<double>::type, H_data);

    float H_dat[16] = {0, static_cast<float>(H_mat.at<double>(0,0)), static_cast<float>(H_mat.at<double>(0,2)), static_cast<float>(H_mat.at<double>(0,4)),
                       0, static_cast<float>(H_mat.at<double>(1,1)), static_cast<float>(H_mat.at<double>(1,3)), static_cast<float>(H_mat.at<double>(1,4)),
                       0, static_cast<float>(H_mat.at<double>(2,0)), static_cast<float>(H_mat.at<double>(2,2)), static_cast<float>(H_mat.at<double>(2,4)),
                       0, static_cast<float>(H_mat.at<double>(3,1)), static_cast<float>(H_mat.at<double>(3,3)), static_cast<float>(H_mat.at<double>(3,4))};
    H = QMatrix4x4(H_dat);

    cv::Mat H_inv_mat = H_mat.inv();
    float H_inv_dat[16] = {0, static_cast<float>(H_inv_mat.at<double>(0,0)), static_cast<float>(H_inv_mat.at<double>(0,2)), static_cast<float>(H_inv_mat.at<double>(0,4)),
                       0, static_cast<float>(H_inv_mat.at<double>(1,1)), static_cast<float>(H_inv_mat.at<double>(1,3)), static_cast<float>(H_inv_mat.at<double>(1,4)),
                       0, static_cast<float>(H_inv_mat.at<double>(2,0)), static_cast<float>(H_inv_mat.at<double>(2,2)), static_cast<float>(H_inv_mat.at<double>(2,4)),
                       0, static_cast<float>(H_inv_mat.at<double>(3,1)), static_cast<float>(H_inv_mat.at<double>(3,3)), static_cast<float>(H_inv_mat.at<double>(3,4))};
    QMatrix4x4 H_inv = QMatrix4x4(H_inv_dat);
    //H=H.transposed();


    // FRAMEBUFFER

    framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment
    // The texture we're going to render to
    renderedTexture_id;
    glGenTextures(1, &renderedTexture_id);
    glBindTexture(GL_TEXTURE_2D, renderedTexture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, meta_infos.width,
                 meta_infos.height, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _func330->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture_id, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    _func330->glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers



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
    program->setUniformValue("R_m", R_m);
    program->setUniformValue("ccm", ccm.transposed());
    program->setUniformValue("modulationExposureBias", float(meta_infos.modulationExposureBias));
    program->setUniformValue("lenslet_dim", QPointF(radiusx, radiusy));
    program->setUniformValue("lenslet_coord", QPointF(radiusx, dy_PerLenslet_row));
    //program->setUniformValue("size_st", QPoint(meta_infos.width/15, meta_infos.height/15));
    program->setUniformValue("size_st", QPointF(meta_infos.width/radiusx, meta_infos.height/dy_PerLenslet_row * meta_infos.mla_scale_y));
    program->setUniformValue("centerLens_pos", centerLens_pos);
    program->setUniformValue("tex_dim", QPointF(meta_infos.width, meta_infos.height));
    program->setUniformValue("white_balance", QVector3D(meta_infos.r_bal,1.0f,meta_infos.b_bal));
    program->setUniformValue("H", H);
    program->setUniformValue("H_inv", H_inv);

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    glGenTextures(1, &texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); // scale linearly when image smalled than texture

    if (texture_is_raw)
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, meta_infos.width,
                    meta_infos.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, ((is_video) ? NULL : texture.data));
    else
        _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.cols,
                    texture.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, ((is_video) ? NULL : texture.data));

    glBindTexture(GL_TEXTURE_2D, 0);

    _func330->glGenBuffers(2, pboIds);
    _func330->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
    _func330->glBufferData(GL_PIXEL_UNPACK_BUFFER, texture.cols * texture.rows * texture.channels(), 0, GL_STREAM_DRAW);
    _func330->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    _func330->glBufferData(GL_PIXEL_UNPACK_BUFFER, texture.cols * texture.rows * texture.channels(), 0, GL_STREAM_DRAW);
    _func330->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

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

    fps_frames_elapsed = 0;
    fps_time_elapsed = 0;
    fps_timer.start();
}

void QOpenGL_LFViewer::paintGL(){

    QMatrix4x4 m_result;
    if (opengl_view_mode != 4){
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
    focusprogram->setUniformValue("focus", focus);
    focusprogram->setUniformValue("view_mode", opengl_view_mode);
    focusprogram->setUniformValue("focus_spread", focus_spread);
    focusprogram->setUniformValue("decode_mode", opengl_decode_mode);

    glViewport(0,0,width(),height()); // Render on the screen
    glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT );

    if (texture_is_raw || !opengl_option_is_demosaicked)
        glBindTexture(GL_TEXTURE_2D, renderedTexture_id);
    else{
        glBindTexture(GL_TEXTURE_2D, texture_id);
    }

    glUniform1i(focusprogram->uniformLocation("renderedTexture"), 0);
    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }

    // unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    fps_frames_elapsed++;
    fps_time_elapsed += fps_timer.elapsed();
    fps_timer.restart();
    if (fps_time_elapsed >= 1000){
        double fps = fps_time_elapsed/1000.0f * fps_frames_elapsed;
        //qDebug() << QString::number(fps);
        emit refreshFPS("FPS: " + QString::number(fps));
        fps_time_elapsed = 0;
        fps_frames_elapsed = 0;
    }
    if (is_video && video_playing){
        _tick();
    }
    else if (opengl_option_render_frames)
        update();
}

void QOpenGL_LFViewer::restructureImageToUVST(){
    QMatrix4x4 m;
    m.ortho(-1, +1, +1, -1, 0.0f, 10.0f);
    m.translate(0,0, -1.0f);
    program->bind();
    //qDebug() << "program bind : " << program->bind();
    program->setUniformValue("decode_mode", opengl_decode_mode);
    program->setUniformValue("view_mode", opengl_view_mode);
    program->setUniformValue("option_wb", opengl_option_wb);
    program->setUniformValue("option_ccm", opengl_option_ccm);
    program->setUniformValue("option_gamma", opengl_option_gamma);
    program->setUniformValue("option_superresolution", opengl_option_superresolution);
    program->setUniformValue("option_display_mode", opengl_option_display_mode);
    program->setUniformValue("demosaicking_mode", opengl_option_demosaicking_mode);
    program->setUniformValue("H", H);
    program->setUniformValue("R_m", R_m);

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

    glClearColor(0.7,0.8,0.2,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, texture_id);
    glUniform1i(program->uniformLocation("lightfield"), 0);
    if (texture_is_raw)
        program->setUniformValue("is_raw", true);
    else
        program->setUniformValue("is_raw", false);

    for (int i = 0; i < 1; ++i) {
      glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }

    if (opengl_save_current_image){
        glReadPixels(0,0, save_img.cols, save_img.rows, GL_BGR, GL_UNSIGNED_BYTE, save_img.data);
        opengl_save_current_image = false;
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
        /*cur_u += dx * 0.04f;
        cur_v += dy * 0.04f;
        cur_u = std::min(std::max(cur_u, -7.0f), 7.0f);
        cur_v = std::min(std::max(cur_v, -7.0f), 7.0f);*/
        lens_pos_view = QPointF(lens_pos_view.x() + dx * 0.01f, lens_pos_view.y() - dy * 0.01f);
    }
    else if(currentButton == Qt::RightButton){
        focus -= dy * 0.01f;
    }
    else if(currentButton == Qt::MiddleButton){
        D -= dy * 0.001f;
        //focus_spread -= dy * 0.01f;

        double pix_size = meta_infos.mla_pixelPitch; // in m
        double lens_size = meta_infos.mla_lensPitch; // in m
        float N = 15;//lens_size / pix_size; // 15 number of pixels per lenslet
        int c_pix = 7; // translational pixel offset ?!
        double c_Mx = meta_infos.mla_centerOffset_x / pix_size;   // optical center offset in mm
        double c_My = -meta_infos.mla_centerOffset_y / pix_size;// * meta_infos.mla_scale_y;    // optical center offset in mm
        double c_mux = -7.3299932479858395e-6;// / pix_size;  // mla offset in mm
        double c_muy = 5.5686492919921878e-6;// / pix_size;   // mla offset in mm
        double d_mu = 3.6999999999999998e-5;  // mla offset in mm
        double f_M = 0.011542153404246169;    // focal length
        double exitPupilOffset = 0.11559105682373047; // distance from the pupil to microlens plane
        double H_data[25];
        DepthCostAware::calcH(pix_size, lens_size, N, c_pix, c_Mx, c_My,
                              c_mux, c_muy, d_mu, f_M, exitPupilOffset, D, H_data);

        cv::Mat H_inv = cv::Mat(5,5, cv::DataType<double>::type, H_data);
        //cv::transpose(H_inv, H_inv);
        //H_inv = H_inv.inv();


        float H_dat[16] = {0, static_cast<float>(H_inv.at<double>(0,0)), static_cast<float>(H_inv.at<double>(0,2)), static_cast<float>(H_inv.at<double>(0,4)),
                           0, static_cast<float>(H_inv.at<double>(1,1)), static_cast<float>(H_inv.at<double>(1,3)), static_cast<float>(H_inv.at<double>(1,4)),
                           0, static_cast<float>(H_inv.at<double>(2,0)), static_cast<float>(H_inv.at<double>(2,2)), static_cast<float>(H_inv.at<double>(2,4)),
                           0, static_cast<float>(H_inv.at<double>(3,1)), static_cast<float>(H_inv.at<double>(3,3)), static_cast<float>(H_inv.at<double>(3,4))};

        H = QMatrix4x4(H_dat);

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
    focus = value / 100.0f / 2;

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

    QImage retImg(texture.cols,texture.rows,QImage::Format_Indexed8);
    QVector<QRgb> table( 256 );
    for( int i = 0; i < 256; ++i )
    {
        table[i] = qRgb(i,i,i);
    }
    retImg.setColorTable(table);
    for(int i =0; i< texture.cols;i++)
    {
        for(int j=0; j< texture.rows;j++)
        {
            QRgb value = texture.at<int>(i,j);
            retImg.setPixel(i,j,qRed(value));
        }

    }

    if (filename != ""){
        retImg.save(filename);
    }

    //saveMetaInfo(filename);
}

cv::Mat QOpenGL_LFViewer::getDemosaicedImage(int mode){
    opengl_option_demosaicking_mode = mode;
    opengl_save_current_image = true;
    opengl_view_mode = 2;
    save_img.create(meta_infos.height, meta_infos.width, CV_8UC3);
    update();
    cv::flip(save_img, save_img, 0);
    cv::imwrite("bla.png", save_img);
    return save_img.clone();
}

void QOpenGL_LFViewer::saveImage(){
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
    opengl_save_current_image = true;
    save_img.create(meta_infos.height, meta_infos.width, CV_8UC3);
    update();
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");
    if (filename != ""){
        cv::flip(save_img, save_img, 0);
        cv::imwrite(filename.toStdString(), save_img);
        //this->grabFramebuffer().save(filename);
    }

    //saveMetaInfo(filename);
}

