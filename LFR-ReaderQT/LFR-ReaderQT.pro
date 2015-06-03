#-------------------------------------------------
#
# Project created by QtCreator 2015-04-16T13:49:43
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LFR-ReaderQT
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    lfp_reader.cpp \
    myqgraphicsview.cpp \
    qopengl_lfviewer.cpp

HEADERS  += mainwindow.h \
    lfp_reader.h \
    myqgraphicsview.h \
    qopengl_lfviewer.h

FORMS    += mainwindow.ui

DISTFILES += \
    lightfield.fsh \
    lightfield_raw.fsh

## OpenCV settings for Windows and OpenCV 2.4.2
win32 {
    # MSVCRT link option (static or dynamic, it must be the same with your Qt SDK link option)
    #MSVCRT_LINK_FLAG_DEBUG = "/MDd"
    #MSVCRT_LINK_FLAG_RELEASE = "/MD"

    message("* Using settings for Windows.")
    INCLUDEPATH += "E:\\Libraries\\open_cv\\build\\include" \
                   "E:\\Libraries\\open_cv\\build\\include\\opencv" \
                   "E:\\Libraries\\open_cv\\build\\include\\opencv2"

    CONFIG(debug, debug | release) {
        LIBS += -L"E:\\Libraries\\open_cv\\build\\x64\\vc12\\lib" \
            -lopencv_core2411d \
            -lopencv_highgui2411d \
            -lopencv_imgproc2411d
    }

    CONFIG(release, debug | release) {
        LIBS += -L"E:\\Libraries\\open_cv\\build\\x64\\vc12\\lib" \
            -lopencv_core2411 \
            -lopencv_highgui2411 \
            -lopencv_imgproc2411
    }
}
