#pragma once

#include <string>

#include "mainwindow.h"

#include <QImage>
#include <QTextBrowser>
#include <QPushButton>
#include "aspectratiopixmaplabel.h"

class LFP_Reader
{
private:

    enum HEADER_TYPE{
        NONE, TYPE_LFP, TYPE_TEXT, TYPE_PICTURE, TYPE_RAWPICTURE, TYPE_IGNORE
    };

    static bool readSection(MainWindow *main, std::basic_ifstream<unsigned char> &input, HEADER_TYPE section_type);
    static QImage readRawPixelData(MainWindow *main, std::basic_ifstream<unsigned char> &input, int section_length, int bit);


public:
    static bool read_lfp(MainWindow *main, std::string file);
    static bool read_RAWFile(MainWindow* main, std::string file);
};
