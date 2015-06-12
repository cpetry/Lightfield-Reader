#include "lfp_reader.h"
#include "mainwindow.h"


unsigned char LFP[] = {0x89, 0x4C, 0x46, 0x50, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x01};
unsigned char LFM[] = {0x89, 0x4C, 0x46, 0x4D, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x00};
unsigned char LFC[] = {0x89, 0x4C, 0x46, 0x43, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x00};

bool checkForByteSequence(std::basic_ifstream<unsigned char> &input, int len, unsigned char* sequence){
    for( int byte = 0; byte < len; ++byte){
        unsigned char c = input.get();
        if(sequence[byte] != c)
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


QImage LFP_Reader::readRawPixelData(std::basic_ifstream<unsigned char> &input, int section_length){

    int col=0, row=0;

    uint8_t byte[5];
    int bit = this->meta_infos.bits;
    QImage image = QImage(this->meta_infos.width, this->meta_infos.height, QImage::Format_RGB32);
    int nmb_pixel = 0;
    int read_byte = 0;
    int nmb_byte = 0;

    float multipl = 1.0f / (1023.0f - 64.0f) * 255.0f;

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
                // 01010101 01011011 01011101 11011011 11|01|10|11
                /*uint16_t lsb = uint16_t(byte[0]);
                uint16_t t0  = uint16_t(byte[1]) << 2;
                uint16_t t1  = uint16_t(byte[2]) << 2;
                uint16_t t2  = uint16_t(byte[3]) << 2;
                uint16_t t3  = uint16_t(byte[4]) << 2;*/
                uint16_t t0  = uint16_t(byte[0]) << 2;
                uint16_t t1  = uint16_t(byte[1]) << 2;
                uint16_t t2  = uint16_t(byte[2]) << 2;
                uint16_t t3  = uint16_t(byte[3]) << 2;
                uint16_t lsb = uint16_t(byte[4]);


                /*t0 = t0 + ((lsb & 0x03)     ) - 64;
                t1 = t1 + ((lsb & 0x0C) >> 2) - 64;
                t2 = t2 + ((lsb & 0x30) >> 4) - 64;
                t3 = t3 + ((lsb & 0xC0) >> 6) - 64;*/

                // black seems to be a little less noisy
                t3 = t3 + ((lsb & 0x03)     ) - 64;
                t2 = t2 + ((lsb & 0x0C) >> 2) - 64;
                t1 = t1 + ((lsb & 0x30) >> 4) - 64;
                t0 = t0 + ((lsb & 0xC0) >> 6) - 64;


                // now everything is in 16bit representation
                // -> to 8 bit
                t0 = int((t0 * multipl)+0.5f);
                t1 = int((t1 * multipl)+0.5f);
                t2 = int((t2 * multipl)+0.5f);
                t3 = int((t3 * multipl)+0.5f);

                scL[col]   = qRgb(t0, t0, t0);
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

    // Convert it to a smaller image in grayscale!
    if (image.allGray())
        image = image.convertToFormat(QImage::Format_Indexed8);

    return image;
}

bool LFP_Reader::readSection(MainWindow* main, std::basic_ifstream<unsigned char> &input,
                             HEADER_TYPE section_type, std::string save_file_name){

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
        // if we want to save several images, we dont need tabs
        if(save_file_name.empty()){
            main->addTabMetaInfos(byte_header, sha1_hash, sec_length, meta_info, "MetaInfo");
        }
        parseLFMetaInfo(meta_info);
    }
    else if (section_type == HEADER_TYPE::TYPE_PICTURE){
        uchar* pic = new uchar[sec_length];
        input.read(pic, sec_length);
        QImage image = QImage::fromData(pic, sec_length);
        if (image.isNull()){ // retry with JPG
            image = QImage::fromData(pic, sec_length, "JPG");
        }
        if(save_file_name.empty()){
            main->addTabImage(byte_header, sha1_hash, sec_length, image, false, meta_infos); // is a normal image
        }
        delete(pic);
    }
    else if (section_type == HEADER_TYPE::TYPE_RAWPICTURE){
        QImage image = readRawPixelData(input, sec_length); // 10 bit per pixel

        // if we want to save several images, we dont need tabs
        if(!save_file_name.empty()){
            MyGraphicsView* mgv = new MyGraphicsView(NULL, image, meta_infos);
            image = mgv->demosaic(3);
            /*QImage retImg(image.width(),image.height(),QImage::Format_Indexed8);
            QVector<QRgb> table( 256 );
            for( int i = 0; i < 256; ++i )
                table[i] = qRgb(i,i,i);

            retImg.setColorTable(table);
            for(int i =0; i< image.width();i++)
                for(int j=0; j< image.height();j++)
                    retImg.setPixel(i,j,qRed(image.pixel(i,j)));*/

            //bool saved = retImg.save(QString::fromStdString(save_file_name));
            bool saved = image.save(QString::fromStdString(save_file_name));
            if (saved)
                qDebug() << "Saved raw to " << QString::fromStdString(save_file_name);
            else
                qDebug() << "Could not save to " << QString::fromStdString(save_file_name);
        }
        else
            main->addTabImage(byte_header, sha1_hash, sec_length, image, true, meta_infos ); // is a raw image
    }
    else if (section_type == HEADER_TYPE::TYPE_IGNORE){
        // just finish the section
        readBytes(input, sec_length);
        QString info = "Unknown Type";
        if (save_file_name.empty()){
            main->addTabMetaInfos(byte_header, sha1_hash, sec_length, "Unknown Type", "X");
        }
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

    if (meta_info.contains(QString("\"image\""), Qt::CaseInsensitive)){

        std::string smeta = meta_info.toStdString();
        size_t pos = smeta.find("image");

        this->meta_infos.bits = atoi(getValueOf("bitsPerPixel", smeta, int(pos)).c_str());
        this->meta_infos.width = atoi(getValueOf("width", smeta, int(pos)).c_str());
        this->meta_infos.modulationExposureBias = atof(getValueOf("modulationExposureBias", smeta, int(pos)).c_str());
        this->meta_infos.height = atoi(getValueOf("height", smeta, int(pos)).c_str());

        std::string cc = getValueOf("ccm", smeta, int(pos)).c_str();
        std::vector<std::string> cc_split = split(cc, ',');
        for (int i = 0; i < 9; i++)
            meta_infos.cc[i] = atof(cc_split[i].c_str());

        pos = smeta.find("whiteBalanceGain");
        meta_infos.r_bal = atof(getValueOf("\"r\"", smeta, int(pos)).c_str());
        meta_infos.b_bal = atof(getValueOf("\"b\"", smeta, int(pos)).c_str());

        pos = smeta.find("sensor");
        meta_infos.mla_pixelPitch = atof(getValueOf("\"pixelPitch\"", smeta, int(pos)).c_str());

        pos = smeta.find("mla");
        meta_infos.mla_rotation = atof(getValueOf("rotation", smeta, int(pos)).c_str());
        pos = smeta.find("scaleFactor");
        meta_infos.mla_scale_x = atof(getValueOf("x", smeta, int(pos)).c_str());
        meta_infos.mla_scale_y = atof(getValueOf("y", smeta, int(pos)).c_str());
        meta_infos.mla_lensPitch = atof(getValueOf("lensPitch", smeta, int(pos)).c_str());
        pos = smeta.find("sensorOffset");
        meta_infos.mla_centerOffset_x = atof(getValueOf("x", smeta, int(pos)).c_str());;
        meta_infos.mla_centerOffset_y = atof(getValueOf("y", smeta, int(pos)).c_str());;

        /*std::cout << meta_infos.bits << std::endl;
        std::cout << meta_infos.width << std::endl;
        std::cout << meta_infos.height << std::endl;
        std::cout << meta_infos.cc[0] << std::endl;
        std::cout << meta_infos.r_bal << std::endl;
        std::cout << meta_infos.b_bal << std::endl;
        std::cout << meta_infos.mla_rotation << std::endl;
        std::cout << meta_infos.mla_scale_x << std::endl;
        std::cout << meta_infos.mla_scale_y << std::endl;
        std::cout << meta_infos.mla_lensPitch << std::endl;
        std::cout << meta_infos.mla_centerOffset_x << std::endl;
        std::cout << meta_infos.mla_centerOffset_y << std::endl;*/

    }
}



bool LFP_Reader::read_RAWFile(MainWindow* main, std::string file ){
    std::basic_ifstream<unsigned char> input(file, std::ifstream::binary);

    int sec_length = getSectionLength(input);

    QImage image = readRawPixelData(input, sec_length);
    main->addTabImage("No Header", "No SHA1", sec_length, image, true, this->meta_infos); // is a raw image

    return true;
}

bool LFP_Reader::read_lfp(MainWindow* main, std::string file, bool save_raw_to_file, std::string raw_file_name){
    std::basic_ifstream<unsigned char> input(file, std::ifstream::binary);

    // magic 12 byte File Header
    // LFP (89 4C 46 50 0D 0A 1A 0A 00 00 00 01)
    // LFM (89 4C 46 4D 0D 0A 1A 0A 00 00 00 00)
    // LFC (89 4C 46 43 0D 0A 1A 0A 00 00 00 00)

    // magic 12 byte File Header LFP
    if(!checkForByteSequence(input, 12, LFP))
        return false;

    // 4byte nothing (00 00 00 00)
    unsigned char nothing[4] = {0x00, 0x00, 0x00, 0x00};
    if(!checkForByteSequence(input, 4, nothing))
        return false;

    HEADER_TYPE* ptypes;
    /*HEADER_TYPE types_illum[12] = {TYPE_TEXT, TYPE_TEXT, TYPE_TEXT, TYPE_TEXT,
                         TYPE_PICTURE, TYPE_TEXT, TYPE_PICTURE, TYPE_PICTURE,
                         TYPE_PICTURE, TYPE_PICTURE, TYPE_PICTURE, TYPE_TEXT};*/
    HEADER_TYPE types_illum[12] = {TYPE_IGNORE, TYPE_IGNORE, TYPE_IGNORE, TYPE_IGNORE,
                         TYPE_TEXT, TYPE_PICTURE, TYPE_TEXT, TYPE_TEXT,
                         TYPE_TEXT, TYPE_PICTURE, TYPE_RAWPICTURE, TYPE_TEXT};
    //HEADER_TYPE types_p01[4] = {TYPE_TEXT, TYPE_PICTURE, TYPE_TEXT, TYPE_TEXT};
    ptypes = types_illum;


    std::cout << "( " << sizeof(types_illum)/sizeof(HEADER_TYPE) << " Sections )" << std::endl;
    for(int i = 0; i < sizeof(types_illum)/sizeof(HEADER_TYPE); i++)
        if(!readSection(main, input, ptypes[i], raw_file_name))
            return false;

    return true;
}
