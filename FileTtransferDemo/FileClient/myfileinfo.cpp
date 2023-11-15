#include "myfileinfo.h"

MyFileInfo::MyFileInfo(QObject *parent) : QObject(parent)
{
    initReadData();
}

MyFileInfo::MyFileInfo(const MyFileInfo &other) :QObject()
{
    *this = other;
}

MyFileInfo::~MyFileInfo()
{

}

void MyFileInfo::initReadData()
{
    this->No = 0;
    this->fileName = "";
    this->fileSize = 0;
    this->filePath = "";
    this->progressStr = "0";
    this->progressByte = 0;
    this->bytesReceived = 0;
}

MyFileInfo& MyFileInfo::operator=(const MyFileInfo &other)
{
    this->No = other.No;
    this->fileName = other.fileName;
    this->fileSize = other.fileSize;
    this->filePath = other.filePath;
    this->progressStr = other.progressStr;
    this->progressByte = other.progressByte;
    this->bytesReceived = other.bytesReceived;
    return *this;
}
