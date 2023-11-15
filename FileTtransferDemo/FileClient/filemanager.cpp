#include "filemanager.h"
#include "ui_filemanager.h"
#include <QMessageBox>
#include <QDataStream>
#include <QDir>
#include <QDesktopServices>

FileManager::FileManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileManager)
{
    ui->setupUi(this);
    myFile = new MyFileInfo(this);
    m_downloadPath = QCoreApplication::applicationDirPath() + "/../下载";
    isDownloading = false;
    QDir dir;
    if(!dir.exists(m_downloadPath)) {
        dir.mkdir(m_downloadPath);
    }
    connectToServer();
    connectSigSlots();
}

FileManager::~FileManager()
{
    delete ui;
}

void FileManager::downLoadFile()
{
    if(m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        if(!connectToServer(m_tcpSocket)) {
            return;
        }
    }
    QByteArray data;
    int typeMsg = MsgType::FileInfo;
    QDataStream out(&data, QIODevice::WriteOnly);
    //m_loadProgressBar->show();
    //发送要下载的文件列表路径
    out << typeMsg;
    ui->textBrowser->append(QString("发送消息：%1").arg(typeMsg));
    m_tcpSocket->write(data);
}

void FileManager::readServerMsg()
{
    //如果正在下载，则收到的全是文件数据，读取即可
    if(isDownloading) {
        fileDataRead();
        return;
    }
    qDebug()<< ".............readServerMsg................";

    QDataStream in(m_tcpSocket);
    in.setVersion(QDataStream::Qt_5_12);
    int type;
    in >> type; //判断消息类型

    if(type == MsgType::FileInfo) {
        fileInfoRead();
        isDownloading = true;
    }
    else {
        qDebug()<<"收到其他消息类型！！！type："<<type;
    }
}

void FileManager::fileInfoRead()
{
    QDataStream in(m_tcpSocket);
    in.setVersion(QDataStream::Qt_5_12);

    qDebug()<<"文件信息读取on_fileInfoRead......";
    // 接收文件大小，数据总大小信息和文件名大小,文件名信息
    in >> myFile->fileName >> myFile->fileSize;

    // 获取文件名，建立文件
    ui->textBrowser->append(QString("下载文件 %1, 文件大小：%2").arg(myFile->fileName).arg(myFile->fileSize));
    QString filePath = m_downloadPath + "/" + myFile->fileName;
    myFile->localFile.setFileName(filePath);
    // 打开文件，准备写入
    if(!myFile->localFile.open(QIODevice::WriteOnly)) {
        qDebug()<<"文件打开失败！";
    }
    //文件信息获取完成，接着获取文件数据
    QByteArray data;
    int typeMsg = MsgType::FileData;
    QDataStream out(&data, QIODevice::WriteOnly);
    //m_loadProgressBar->show();
    //发送要下载的文件列表路径
    out << typeMsg;
    m_tcpSocket->write(data);
}

void FileManager::fileDataRead()
{
    qint64 readBytes = m_tcpSocket->bytesAvailable();
    if(readBytes <0) return;

    int progress = 0;
    // 如果接收的数据大小小于要接收的文件大小，那么继续写入文件
    if(myFile->bytesReceived < myFile->fileSize) {
        // 返回等待读取的传入字节数
        QByteArray data = m_tcpSocket->read(readBytes);
        myFile->bytesReceived+=readBytes;
        ui->textBrowser->append(QString("接收进度：%1/%2(字节)").arg(myFile->bytesReceived).arg(myFile->fileSize));
        progress =static_cast<int>(myFile->bytesReceived*100/myFile->fileSize);
        myFile->progressByte = myFile->bytesReceived;
        myFile->progressStr = QString("%1").arg(progress);
        ui->progressBar->setValue(progress);
        myFile->localFile.write(data);
    }

    // 接收数据完成时
    if (myFile->bytesReceived==myFile->fileSize){
        ui->textBrowser->append(tr("接收文件[%1]成功！").arg(myFile->fileName));
        progress = 100;
        myFile->localFile.close();

        ui->textBrowser->append(QString("接收进度：%1/%2（字节）").arg(myFile->bytesReceived).arg(myFile->fileSize));
        myFile->progressByte = myFile->bytesReceived;
        ui->progressBar->setValue(progress);
        isDownloading = false;
        myFile->initReadData();
    }

    if (myFile->bytesReceived > myFile->fileSize){
        qDebug()<<"myFile->bytesReceived > m_fileSize";
    }
}

void FileManager::connectSigSlots()
{
    //点击下载文件
    connect(ui->downloadBtn, &QPushButton::clicked, this, &FileManager::downLoadFile);
    connect(ui->ConnectBtn, &QPushButton::clicked, this, [=](){
        connectToServer(m_tcpSocket);
    });
    connect(ui->disConnectbutton, &QPushButton::clicked, this, [=](){
        m_tcpSocket->disconnectFromHost();
        ui->textBrowser->append("与服务器断开连接...");
    });
    connect(ui->openFolder, &QPushButton::clicked, this, [=]() {
        QString path = QCoreApplication::applicationDirPath()+"/../下载";
        QDesktopServices::openUrl(QUrl("file:"+path, QUrl::TolerantMode));
    });
    connect(ui->resetProgress, &QPushButton::clicked, [=]() {
        ui->progressBar->setValue(0);
    });
}

void FileManager::connectToServer()
{
    m_tcpSocket = new QTcpSocket(this);
    connectToServer(m_tcpSocket);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &FileManager::readServerMsg);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, [=]() {
        ui->textBrowser->append(QString("与服务器断开连接：原因：%1").arg(m_tcpSocket->errorString()));
        //isConnected = false;
    });
}

bool FileManager::connectToServer(QTcpSocket *socket)
{
    socket->connectToHost(ui->serverIpEdit_2->text(), ui->serverPortEdit_2->text().toInt());
    if(!socket->waitForConnected(2*1000)) {
        QMessageBox::warning(this, "警告", "服务器连接失败，原因："+m_tcpSocket->errorString());
        return false;
    }
    QMessageBox::information(this, "提示", "服务器连接成功！");
    ui->textBrowser->append("服务器连接成功！");

    return true;
}
