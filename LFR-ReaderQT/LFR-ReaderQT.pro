#-------------------------------------------------
#
# Project created by QtCreator 2015-04-16T13:49:43
#
#-------------------------------------------------

QT       += core gui

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
