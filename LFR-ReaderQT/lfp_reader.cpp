#include "lfp_reader.h"
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include <streambuf>

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include "mainwindow.h"

LFP_Reader::LFP_Reader(){};

struct membuf : std::streambuf
{
    membuf(std::vector<char> &v) {
        this->setg(v.data(), v.data(), v.data() + v.size());
    }
};

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

static std::string getValueOf(std::string searchstr, std::string heystack, int from){
    int pos = heystack.find(":", heystack.find(searchstr, from));
    int str_size = std::min(heystack.find(",", pos), heystack.find("}", pos)) - pos;
    return trim(heystack.substr(pos+1, str_size-1));
}


char LFP[12] = {0x89, 0x4C, 0x46, 0x50, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x01};
char LFM[12] = {0x89, 0x4C, 0x46, 0x4D, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x00};
char LFC[12] = {0x89, 0x4C, 0x46, 0x43, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x00};

bool checkForByteSequence(std::basic_ifstream<unsigned char> &input, int len, char* sequence, char* alternative = {}){
    for( int byte = 0; byte < len; ++byte){
        char c = input.get();
        if(sequence[byte] != c
        && (sizeof(alternative) > 0 && alternative[byte] != c))
            return false;
    }
    return true;
}

std::vector<char> LFP_Reader::readBytes(std::basic_ifstream<unsigned char> &input, int nmb_bytes){
    std::vector<char> c_array;
    while (!input.eof()){
        c_array.push_back(input.get());
        if (nmb_bytes > 0){
            nmb_bytes--;
            if (nmb_bytes == 0)
                break;
        }
    }
    return c_array;
}

std::string readBytesUntilNextHeader(std::basic_ifstream<unsigned char> &input){
    std::vector<char> c_array;
    while(!input.eof() && input.peek() != 0x89){
        input.get();
    }
    return std::string(c_array.begin(),c_array.end());
}

int getSectionLength(std::basic_ifstream<unsigned char> &input){
    int len=0;
    for( int byte = 0; byte < 4; ++byte){
        len=len << 8;
        len += input.get();
    }
    return len;
}


QImage LFP_Reader::readRawPixelData(MainWindow* main, std::basic_ifstream<unsigned char> &input, int section_length){

    // Illum: 1640 * 1640 * 4 * 10 / 8 / 1024 / 1024
    // 1640 * 1640 * 4 * 12 / 8 / 1024 / 1024
    // 3280 * 3280     * 12 / 8 / 1024 / 1024
    /*
     * switch( BitPacking )
    case '8bit'
        ImgSize = LFDefaultVal( 'ImgSize', [3280,3280] );
        BuffSize = prod(ImgSize);

    case '12bit'
        ImgSize = LFDefaultVal( 'ImgSize', [3280,3280] );
        BuffSize = prod(ImgSize) * 12/8;

    case '10bit'
        ImgSize = LFDefaultVal( 'ImgSize', [7728, 5368] );
        BuffSize = prod(ImgSize) * 10/8;

    case '16bit'
        ImgSize = LFDefaultVal( 'ImgSize', [3280,3280] );
        BuffSize = prod(ImgSize) * 16/8;

    otherwise
        error('Unrecognized bit packing format');
    end
      */

    int col=0, row=0;

    uint8_t byte[5];
    int bit = this->meta_infos.bits;
    QImage image = QImage(this->meta_infos.width, this->meta_infos.height, QImage::Format_RGB32);
    int nmb_pixel = 0;
    int read_byte = 0;
    int nmb_byte = 0;

    while(row < image.height()){
        QRgb *scL = reinterpret_cast< QRgb *>( image.scanLine( row ) );
        while (col < image.width()){
            byte[nmb_byte] = input.get();
            read_byte++;
            nmb_byte++;
            if (bit == 12 && nmb_byte == 3){
                int val = byte[0] << 4;
                val += byte[1] & 0xF0 >> 4;
                val = val >> 4;
                scL[col] = qRgb(val, val, val);

                val = (byte[1] & 0x0F) << 8;
                val += byte[2];
                val = val >> 4;
                scL[col+1] = qRgb(val, val, val);

                col+=2;
                nmb_pixel+=2;
                nmb_byte = 0;
            }
            if (bit == 10 && nmb_byte == 5){

                /*uint16_t t0  = uint16_t(byte[0]);
                uint16_t t1  = uint16_t(byte[1]);
                uint16_t t2  = uint16_t(byte[2]);
                uint16_t t3  = uint16_t(byte[3]);
                uint16_t lsb = uint16_t(byte[4]);*/

                uint16_t lsb = uint16_t(byte[0]);
                uint16_t t0  = uint16_t(byte[1]) << 2;
                uint16_t t1  = uint16_t(byte[2]) << 2;
                uint16_t t2  = uint16_t(byte[3]) << 2;
                uint16_t t3  = uint16_t(byte[4]) << 2;
                /*uint16_t t0  = uint16_t(byte[0]) << 2;
                uint16_t t1  = uint16_t(byte[1]) << 2;
                uint16_t t2  = uint16_t(byte[2]) << 2;
                uint16_t t3  = uint16_t(byte[3]) << 2;
                uint16_t lsb = uint16_t(byte[4]);*/

                t3 = t3 + ((lsb & 0x03)     ) - 64;
                t2 = t2 + ((lsb & 0x0C) >> 2) - 64;
                t1 = t1 + ((lsb & 0x30) >> 4) - 64;
                t0 = t0 + ((lsb & 0xC0) >> 6) - 64;

                float multipl = 1.0f / (1023.0f - 64.0f) * 255;
                // now everything is in 16bit representation
                // -> to 8 bit
                t0 = int((t0 * multipl)+0.5f);
                t1 = int((t1 * multipl)+0.5f);
                t2 = int((t2 * multipl)+0.5f);
                t3 = int((t3 * multipl)+0.5f);

                scL[col] = qRgb(t0, t0, t0);
                scL[col+1] = qRgb(t1, t1, t1);
                scL[col+2] = qRgb(t2, t2, t2);
                scL[col+3] = qRgb(t3, t3, t3);

                col+=4;
                nmb_pixel+=4;
                nmb_byte = 0;
            }
            if (read_byte >= section_length)
                break;
        }
        if (read_byte >= section_length)
            break;
        row++;
        col=0;
    }

    // just finish the section
    if (read_byte < section_length)
        readBytes(input, section_length - read_byte);

    return image;
}

bool LFP_Reader::readSection(MainWindow* main, std::basic_ifstream<unsigned char> &input, HEADER_TYPE section_type){

    // magic 12 byte Header
    std::vector<char> bytes = readBytes(input, 12);
    QString byte_header = QString::fromStdString(std::string(bytes.begin(), bytes.end()));

    // Section length in 4 bytes
    int sec_length = getSectionLength(input);
    // 45 bytes of sha1 hash as hex in ascii
    bytes = readBytes(input, 45);
    QString sha1_hash = QString::fromStdString(std::string(bytes.begin(), bytes.end()));
    // 35 bytes of null padding
    readBytes(input, 35);

    bool correct = true;
    if (section_type == HEADER_TYPE::TYPE_TEXT){
        std::string text = readText(input, sec_length);
        QString meta_info = QString::fromStdString(text);
        main->setupMetaInfos(byte_header, sha1_hash, sec_length, meta_info, "MetaInfo");
        parseLFMetaInfo(meta_info);
    }
    else if (section_type == HEADER_TYPE::TYPE_PICTURE){
        uchar* pic = new uchar[sec_length];
        input.read(pic, sec_length);
        QImage image = QImage::fromData(pic, sec_length);
        if (image.isNull()){ // retry with JPG
            image = QImage::fromData(pic, sec_length, "JPG");
        }
        main->setupView(byte_header, sha1_hash, sec_length, image, false); // is a normal image
        delete(pic);
    }
    else if (section_type == HEADER_TYPE::TYPE_RAWPICTURE){
        QImage image = readRawPixelData(main, input, sec_length); // 10 bit per pixel
        main->setupView(byte_header, sha1_hash, sec_length, image, true); // is a raw image
    }
    else if (section_type == HEADER_TYPE::TYPE_IGNORE){
        // just finish the section
        readBytes(input, sec_length);
        QString info = "Unknown Type";
        main->setupMetaInfos(byte_header, sha1_hash, sec_length, "Unknown Type", "X");
        correct = true;
    }
    else
        return false;
    // 0 or more bytes of null padding
    readBytesUntilNextHeader(input);

    return correct;
}

std::string LFP_Reader::readText(std::basic_ifstream<unsigned char> &input, int nmb_bytes){
    std::string text = "";
    std::vector<char> buffer = readBytes(input, nmb_bytes);
    membuf sbuf(buffer);
    std::istream in(&sbuf);
    std::string line;
    while (std::getline(in, line)) {
        text += line + "\n";
    }
    return text;
}

void LFP_Reader::parseLFMetaInfo(QString meta_info){
    int width = 20;
    int height = 20;
    int bits = 10;
    if (meta_info.contains(QString("image"), Qt::CaseInsensitive)
      && meta_info.contains(QString("ILLUM"), Qt::CaseInsensitive)){

        std::string smeta = meta_info.toStdString();
        int pos = smeta.find("image");

        bits = atoi(getValueOf("bitsPerPixel", smeta, pos).c_str());
        width = atoi(getValueOf("width", smeta, pos).c_str());
        height = atoi(getValueOf("height", smeta, pos).c_str());

        std::cout << bits << std::endl;
        std::cout << width << std::endl;
        std::cout << height << std::endl;
        this->meta_infos = LFP_Reader::lf_meta(bits, width, height);
    }
}



bool LFP_Reader::read_RAWFile(MainWindow* main, std::string file ){
    std::basic_ifstream<unsigned char> input(file, std::ifstream::binary);

    int sec_length = getSectionLength(input);

    QImage image = readRawPixelData(main, input, sec_length);
    main->setupView("No Header", "No SHA1", sec_length, image, true); // is a raw image

    return true;
}

bool LFP_Reader::read_lfp(MainWindow* main, std::string file){
    std::basic_ifstream<unsigned char> input(file, std::ifstream::binary);

    // magic 12 byte File Header
    // LFP (89 4C 46 50 0D 0A 1A 0A 00 00 00 01)
    // LFM (89 4C 46 4D 0D 0A 1A 0A 00 00 00 00)
    // LFC (89 4C 46 43 0D 0A 1A 0A 00 00 00 00)

    // magic 12 byte File Header LFP
    if(!checkForByteSequence(input, 12, LFP))
        return false;

    // 4byte nothing (00 00 00 00)
    char nothing[4] = {0x00, 0x00, 0x00, 0x00};
    if(!checkForByteSequence(input, 4, nothing))
        return false;

    bool is_lytro_illum = true;

    HEADER_TYPE* ptypes;
    /*HEADER_TYPE types_illum[12] = {TYPE_TEXT, TYPE_TEXT, TYPE_TEXT, TYPE_TEXT,
                         TYPE_PICTURE, TYPE_TEXT, TYPE_PICTURE, TYPE_PICTURE,
                         TYPE_PICTURE, TYPE_PICTURE, TYPE_PICTURE, TYPE_TEXT};*/
    HEADER_TYPE types_illum[12] = {TYPE_IGNORE, TYPE_IGNORE, TYPE_IGNORE, TYPE_IGNORE,
                         TYPE_TEXT, TYPE_PICTURE, TYPE_TEXT, TYPE_TEXT,
                         TYPE_TEXT, TYPE_PICTURE, TYPE_RAWPICTURE, TYPE_TEXT};
    HEADER_TYPE types_p01[4] = {TYPE_TEXT, TYPE_PICTURE, TYPE_TEXT, TYPE_TEXT};
    ptypes = types_illum;


    std::cout << "( " << sizeof(types_illum)/sizeof(HEADER_TYPE) << " Sections )" << std::endl;
    for(int i = 0; i < sizeof(types_illum)/sizeof(HEADER_TYPE); i++)
        if(!readSection(main, input, ptypes[i]))
            return false;

    return true;
}
