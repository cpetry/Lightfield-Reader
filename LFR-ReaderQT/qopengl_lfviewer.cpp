#include "qopengl_lfviewer.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QGenericMatrix>

QOpenGL_LFViewer::QOpenGL_LFViewer(QWidget *parent, QImage image)
    : QOpenGLWidget(parent),
      clearColor(Qt::black),
      program(0)
{
    texture = image.mirrored();
}

void QOpenGL_LFViewer::setTextures(QList<QImage*> images){
    //textures[0] = new QImage(*images[0]);
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
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "uniform mediump vec2 tex_dim;\n"
        "uniform mediump vec2 lenslet_dim;\n"
        "uniform mediump vec2 centerLens_pos;\n"
        "uniform mediump vec2 lens_pos_view;\n"
        "uniform mediump mat2 lenslet_m;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    vec2 diff_from_raw_center = -(centerLens_pos - (texc.st * tex_dim));\n"
        "    vec2 currentLens = inverse(lenslet_m) * diff_from_raw_center;\n"
        "    currentLens = vec2(int(currentLens.x + 0.5), int(currentLens.y + 0.5));\n"     // Now we have the correct lense
        "    vec2 pixel_in_lense = currentLens + lens_pos_view;\n"
        "    if(int(currentLens.y) % 2 == 1)\n"
        "       pixel_in_lense.x += 0.5;\n"
        "    vec2 pixel_in_lense_pos = lenslet_m * pixel_in_lense;\n"
        "    vec2 texel_pos = (centerLens_pos + pixel_in_lense_pos) / tex_dim;\n"
        "    gl_FragColor = texture2D(texture, texel_pos);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

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

    double centerx = -7.3299 / 1.399;
    double centery = 5.5686 / 1.399;
    double radiusx = 14.2959;
    double radiusy = 14.2959 * 1.0001299;
    double lenslet_rotation = 0.001277;


    float dy_rot_PerLenslet_hori = -tan(lenslet_rotation) * radiusx;
    float dx_rot_PerLenslet_vert = tan(lenslet_rotation) * radiusy;
    QPointF centerLens_pos = QPointF(texture.width()/2.0f + centerx, texture.height()/2.0f - centery);
    float dy_PerLenslet_row = sqrt(pow(radiusx,2) - pow(radiusx/2.0f,2));
    float dx_row_odd = 0.5f*radiusx;
    float matrix[] = {radiusx, dx_rot_PerLenslet_vert,
                    dy_rot_PerLenslet_hori, dy_PerLenslet_row};
    QMatrix2x2 lenslet_m(matrix);
    program->setUniformValue("lenslet_m", lenslet_m);
    program->setUniformValue("dy_rot_PerLenslet_hori", dy_rot_PerLenslet_hori);
    program->setUniformValue("dx_rot_PerLenslet_vert", dx_rot_PerLenslet_vert);
    program->setUniformValue("dy_PerLenslet_row", dy_PerLenslet_row);
    program->setUniformValue("dx_row_odd", dx_row_odd);
    program->setUniformValue("lenslet_dim", QPoint(radiusx, radiusy));
    program->setUniformValue("centerLens_pos", centerLens_pos);
    program->setUniformValue("tex_dim", QPoint(texture.width(), texture.height()));


    glGenTextures(1, &texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    _func330->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width(),
                texture.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, texture.bits());
    glEnable(GL_TEXTURE_2D);
}

void QOpenGL_LFViewer::paintGL()
{
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
    QMatrix4x4 m;
    m.ortho(-orthosize, +orthosize, +orthosize, -orthosize, 0.0f, 10.0f);
    m.translate(translation.x(), translation.y(), -1.0f);

    program->setUniformValue("matrix", m);
    program->setUniformValue("lens_pos_view", lens_pos_view);

    /*program->setUniformValue("u_coord", cur_u);
    program->setUniformValue("v_coord", cur_v);
    program->setUniformValue("focus", focus);
    program->setUniformValue("focus_radius", focus_radius);
    program->setUniformValue("frame", frame_current);*/

    glActiveTexture(texture_id);
    glBindTexture( GL_TEXTURE_2D, texture_id);

    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
}


void QOpenGL_LFViewer::getNextFrame(){
    // leeres Texture-Array-Objekt im Grafikspeicher anlegen:
    frame_current++;
    if (frame_current > frame_max)
        frame_current = 0;
    qWarning() << "current frame: " + QString::number(frame_current);
    update();
}

void QOpenGL_LFViewer::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void QOpenGL_LFViewer::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void QOpenGL_LFViewer::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    //translation = QPointF(translation.x() + dx * 0.004f * orthosize, translation.y() + dy * 0.004f * orthosize);
    lens_pos_view = QPointF(lens_pos_view.x() + dx * 0.001f, lens_pos_view.y() - dy * 0.001f);
    lastPos = event->pos();
    update();
}

void QOpenGL_LFViewer::focus_changed(int value){
    focus = value / 100.0f;
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

    update();
}

void QOpenGL_LFViewer::mouseReleaseEvent(QMouseEvent * /* event */)
{
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

    //QString filepath = "../frenchguy/frenchguy";
    int y_per_frame = 15, x_per_frame = 15;

    //texture = QImage(QString("../raw_decoded_demosaiced_cc.png")).mirrored();
    //if (texture.isNull())
    //    qWarning() << "Could not find image!";

    //this->resize(textures[0].width(), textures[0].height());

    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));

}

