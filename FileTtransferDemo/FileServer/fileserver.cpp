#include "fileserver.h"
#include "ui_fileserver.h"

#include <QTcpSocket>
#include <QFileInfo>
#include <QFileDialog>
#include <QThread>

FileServer::FileServer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileServer)
{
    ui->setupUi(this);

    m_tcpServer = new QTcpServer(this);
    connectSigSlots();
}

FileServer::~FileServer()
{
    delete ui;
}

void FileServer::connectSigSlots()
{
    connect(ui->clearMsg, &QPushButton::clicked, [=]() {
        ui->textBrowser->clear();
    });
    connect(ui->resetProgress, &QPushButton::clicked, [=]() {
        ui->sendProgressBar->setValue(0);
    });
    connect(ui->Serverbtn, &QPushButton::clicked, [=]() {
        if(ui->Serverbtn->text() == "启动服务") {
            if(!startServer()) {
                return;
            }
            ui->Serverbtn->setText("关闭服务");
        }
        else if(ui->Serverbtn->text() == "关闭服务") {
            m_tcpServer->close();
            ui->Serverbtn->setText("启动服务");
            ui->textBrowser->append(QString("服务器已关闭..."));
        }
    });
    connect(ui->selectFilebtn, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "./");
        if(filePath=="") return;
        ui->fileEdit->setText(filePath);
        QFile file(filePath);
        QString msg;
        if(file.size()>1024*1024) {
            msg = QString("大小：%1M").arg(file.size()/1024/1024.0);
        }
        else {
            msg = QString("大小：%1KB").arg(file.size()/1024.0);
        }

        ui->fileSizeLabel->setText(msg);
    });}


bool FileServer::startServer()
{
    if(!checkFile(ui->fileEdit->text())) {
        return false;
    }
    ui->textBrowser->append(QString("服务器已启动，监听端口：%1").arg(ui->portEdit->text()));

    m_tcpServer->listen(QHostAddress::Any, ui->portEdit->text().toInt());
    connect(m_tcpServer, &QTcpServer::newConnection, this, &FileServer::delNewConnect);
    return true;
}

void FileServer::delNewConnect()
{
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
    ui->textBrowser->append(QString("客户端[%1]已连接！").arg(socket->peerAddress().toString()));

    connect(socket, &QTcpSocket::readyRead, [=]() {
        dealMsg(socket);
    });
}

void FileServer::dealMsg(QTcpSocket *socket)
{

    QDataStream in(socket);
    int typeMsg;
    in>>typeMsg;
    ui->textBrowser->append(QString("收到客户端发来的消息：%1").arg(typeMsg));

    if(typeMsg == MsgType::FileInfo) {
        // 发送文件信息
        transferFileInfo(socket);
    }
    else if(typeMsg == MsgType::FileData) {
        // 发送文件数据
        transferFileData(socket);
    }
}

void FileServer::transferFileData(QTcpSocket *socket)
{

    qint64 payloadSize = 1024*1; //每一帧发送1024*64个字节，控制每次读取文件的大小，从而传输速度

    double progressByte= 0;//发送进度
    qint64 bytesWritten=0;//已经发送的字节数

    while(bytesWritten != m_sendFileSize) {
        double temp = bytesWritten/1.0/m_sendFileSize*100;
        int  progress = static_cast<int>(bytesWritten/1.0/m_sendFileSize*100);
        if(bytesWritten<m_sendFileSize){

            QByteArray DataInfoBlock = m_localFile.read(qMin(m_sendFileSize,payloadSize));

            qint64 WriteBolockSize = socket->write(DataInfoBlock, DataInfoBlock.size());

            //QThread::msleep(1); //添加延时，防止服务端发送文件帧过快，否则发送过快，客户端接收不过来，导致丢包
            QThread::usleep(3); //添加延时，防止服务端发送文件帧过快，否则发送过快，客户端接收不过来，导致丢包
            //等待发送完成，才能继续下次发送，
            if(!socket->waitForBytesWritten(3*1000)) {
                ui->textBrowser->append("网络请求超时");
                return;
            }
            bytesWritten += WriteBolockSize;
            ui->sendProgressBar->setValue(progress);
        }

        if(bytesWritten==m_sendFileSize){
            //LogWrite::LOG_DEBUG(QString("当前更新进度：100%,发送总次数:%1").arg(count), "server_"+socket->localAddress().toString());
            ui->textBrowser->append(QString("当前上传进度：%1/%2 -> %3%").arg(bytesWritten).arg(m_sendFileSize).arg(progress));
            ui->textBrowser->append(QString("-------------对[%1]的文件传输完成！------------------").arg(socket->peerAddress().toString()));
            ui->sendProgressBar->setValue(100);
            m_localFile.close();
            return;
        }
        if(bytesWritten > m_sendFileSize) {
            ui->textBrowser->append("意外情况！！！");
            return;
        }

        if(bytesWritten/1.0/m_sendFileSize > progressByte) {
            ui->textBrowser->append(QString("当前上传进度：%1/%2 -> %3%").arg(bytesWritten).arg(m_sendFileSize).arg(progress));
            progressByte+=0.1;
        }

    }

}

void FileServer::transferFileInfo(QTcpSocket *socket)
{
    //获取文件数据，准备发送
    QByteArray  DataInfoBlock = getFileContent(ui->fileEdit->text());

    QThread::msleep(10); //添加延时
    m_fileInfoWriteBytes = socket->write(DataInfoBlock) - typeMsgSize;
    qDebug()<< "传输文件信息，大小："<< m_sendFileSize;
    //等待发送完成，才能继续下次发送，否则发送过快，对方无法接收
    if(!socket->waitForBytesWritten(10*1000)) {
        ui->textBrowser->append(QString("网络请求超时,原因：%1").arg(socket->errorString()));
        return;
    }

    ui->textBrowser->append(QString("文件信息发送完成，开始对[%1]进行文件传输------------------")
                        .arg(socket->localAddress().toString()));
    qDebug()<<"当前文件传输线程id:"<<QThread::currentThreadId();

    m_localFile.setFileName(m_sendFilePath);
    if(!m_localFile.open(QFile::ReadOnly)){
        ui->textBrowser->append(QString("文件[%1]打开失败！").arg(m_sendFilePath));
        return;
    }
}

QByteArray FileServer::getFileContent(QString filePath)
{
    if(!QFile::exists(filePath)) {
        ui->textBrowser->append(QString("没有要传输的文件！" + filePath));
        return "";
    }
    m_sendFilePath = filePath;
    ui->textBrowser->append(QString("正在获取文件信息[%1]......").arg(filePath));
    QFileInfo info(filePath);

    //获取要发送的文件大小
    m_sendFileSize = info.size();

    ui->textBrowser->append(QString("要发送的文件大小：%1字节，%2M").arg(m_sendFileSize).arg(m_sendFileSize/1024/1024.0));

    //获取发送的文件名
    QString currentFileName=filePath.right(filePath.size()-filePath.lastIndexOf('/')-1);
    QByteArray DataInfoBlock;

    QDataStream sendOut(&DataInfoBlock,QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_5_12);
    int type = MsgType::FileInfo;
    //封装发送的信息到DataInfoBlock中
        //消息类型             文件名                  //文件大小
    sendOut<<int(type)<<QString(currentFileName)<<qint64(m_sendFileSize);

    ui->textBrowser->append(QString("文件[%1]信息获取完成！").arg(currentFileName));
    //发送的文件总大小中，信息类型不计入
    QString msg;
    if(m_sendFileSize>1024*1024) {
        msg = QString("%1M").arg(m_sendFileSize/1024/1024.0);
    }
    else {
        msg = QString("%1KB").arg(m_sendFileSize/1024.0);
    }
    ui->textBrowser->append(QString("发送的文件名：%1，文件大小：%2").arg(currentFileName).arg(msg));

    return DataInfoBlock;
}

bool FileServer::checkFile(QString filePath)
{
    QFile file(filePath);
    if(!file.exists(filePath)){
        ui->textBrowser->append("上传文件不存在！");
        return false;
    }
    if(file.size()==0) {
        ui->textBrowser->append("上传文件为空文件！");
        return false;
    }
    return true;
}


