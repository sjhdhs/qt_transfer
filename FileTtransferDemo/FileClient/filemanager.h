#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QWidget>
#include <QTcpSocket>

#include "myfileinfo.h"

namespace Ui {
class FileManager;
}

//消息类型
enum MsgType{
    FileInfo,   //文件信息，如文件名，文件大小等信息
    FileData,   //文件数据，即文件内容
};

class FileManager : public QWidget
{
    Q_OBJECT

public:
    explicit FileManager(QWidget *parent = nullptr);
    ~FileManager();

    void downLoadFile();
    void readServerMsg();
    void fileInfoRead();
    void fileDataRead();

private:
    void connectSigSlots();
    void connectToServer();
    bool connectToServer(QTcpSocket *socket);
private:
    Ui::FileManager *ui;
    QTcpSocket* m_tcpSocket;
    MyFileInfo* myFile;
    QString m_downloadPath;//下载路径
    bool isDownloading; //是否正在下载标识
};

#endif // FILEMANAGER_H
