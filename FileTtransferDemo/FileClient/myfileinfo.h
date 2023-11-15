#ifndef FILEINFO_H
#define FILEINFO_H

#include <QObject>
#include <QFile>

class QTcpSocket;

class MyFileInfo : public QObject
{
    Q_OBJECT
public:
    explicit MyFileInfo(QObject *parent = nullptr);

    MyFileInfo(const MyFileInfo& other);

    ~MyFileInfo();
    int No;
    QString fileName;
    QString filePath;
    qint64 fileSize; //格式如 1.8M
    QString progressStr; // 0-100 ,上传和下载进度
    long progressByte; // 0-100
    qint64 bytesReceived;//已经接收的字节大小
    QFile localFile;

    void initReadData();
    MyFileInfo& operator=(const MyFileInfo& other);
};

#endif // FILEINFO_H
