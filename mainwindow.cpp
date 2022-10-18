#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "common.h"
#include <QMessageBox>
#include <QtEndian>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new QTcpSocket(this))
{
    ui->setupUi(this);
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(7890);
    socket_->setProxy(proxy);
    ui->infoTextEdit->appendPlainText("初始socket状态:"+SocketStateString(socket_->state()));

    connect(socket_, &QTcpSocket::connected, this, [&](){
        ui->infoTextEdit->appendPlainText("连接成功,此时socket状态:"+SocketStateString(socket_->state()));
    });
    connect(socket_, &QTcpSocket::disconnected, this, [&](){
       ui->infoTextEdit->appendPlainText("连接断开,此时socket状态:"+SocketStateString(socket_->state()));
       socket_->abort();
    });
    connect(socket_, &QTcpSocket::errorOccurred, this, [&](){
       ui->infoTextEdit->appendPlainText(socket_->errorString());
       socket_->abort();
    });
    connect(socket_, &QTcpSocket::readyRead, this, [&]{
        QByteArray ba = socket_->readAll();
        const char* data = ba.data();
        PkgHeader * header = (PkgHeader*)data;
        int msg_code = qToLittleEndian(header->msg_code);
        if(msg_code == 5)
        {
            RegisterInfo *info = (RegisterInfo*)(data + sizeof(PkgHeader));
            ui->infoTextEdit->appendPlainText("crc原始数据校验结果:"+QString::number(GetCRC((unsigned char*)info, sizeof(RegisterInfo))));
            ui->infoTextEdit->appendPlainText("从服务端返回的数据如下:");
            ui->infoTextEdit->appendPlainText("pkg_len  :"+QString::number(qToLittleEndian(header->pkg_len)));
            ui->infoTextEdit->appendPlainText("msg_code :"+QString::number(header->msg_code));
            ui->infoTextEdit->appendPlainText("crc32    :"+QString::number(header->crc32));
            ui->infoTextEdit->appendPlainText("type     :"+QString::number(qToLittleEndian(info->type)));
            ui->infoTextEdit->appendPlainText("username :"+QString(info->username));
            ui->infoTextEdit->appendPlainText("password :"+QString(info->password));
        }
        else if(msg_code == 6)
        {
            LoginInfo *info = (LoginInfo*)(data + sizeof(PkgHeader));
            ui->infoTextEdit->appendPlainText("crc原始数据校验结果:"+QString::number(GetCRC((unsigned char*)info, sizeof(LoginInfo))));
            ui->infoTextEdit->appendPlainText("从服务端返回的数据如下:");
            ui->infoTextEdit->appendPlainText("pkg_len  :"+QString::number(qToLittleEndian(header->pkg_len)));
            ui->infoTextEdit->appendPlainText("msg_code :"+QString::number(header->msg_code));
            ui->infoTextEdit->appendPlainText("crc32    :"+QString::number(header->crc32));
            ui->infoTextEdit->appendPlainText("username :"+QString(info->username));
            ui->infoTextEdit->appendPlainText("password :"+QString(info->password));
        }

        ui->infoTextEdit->appendPlainText("---------------------------------");

    });
}

QString MainWindow::SocketStateString(QAbstractSocket::SocketState state)
{
    QString result;
    switch (state) {
        case QAbstractSocket::UnconnectedState:
            result = "UnconnectedState";
            break;
        case QAbstractSocket::HostLookupState:
            result = "HostLookupState";
            break;
        case QAbstractSocket::ConnectingState:
            result = "ConnectingState";
            break;
        case QAbstractSocket::ConnectedState:
            result = "ConnectedState";
            break;
        case QAbstractSocket::BoundState:
            result = "BoundState";
            break;
        default:
            result = "Unknow";
            break;
    }
    return result;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete socket_;
}


void MainWindow::on_connectButton_clicked()
{
    QString ip = ui->ipEdit->text();
    quint16 port = ui->portEdit->text().toInt();
    socket_->connectToHost(QHostAddress(ip), port);
    ui->infoTextEdit->appendPlainText("准备连接服务器...");
    socket_->waitForConnected();
}


void MainWindow::on_clearButton_clicked()
{
    ui->infoTextEdit->clear();
}


void MainWindow::on_loginButton_clicked()
{
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();
    if(username.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入用户名");
        return;
    }
    if(password.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入密码");
        return;
    }
    if(socket_->state() == QAbstractSocket::ConnectedState)
    {
        char buffer[512];
        memset(buffer, 0, 512);
        LoginInfo info;
        memset(&info, 0, sizeof(LoginInfo));
        strcpy(info.username, "cheng hao");
        strcpy(info.password, "123456");
        PkgHeader header;
        // pkg_Len = 8+100 = 108;
        header.pkg_len = sizeof(PkgHeader) + sizeof(LoginInfo);
        header.msg_code = 6;
        header.crc32 = GetCRC((unsigned char*)&info, sizeof(LoginInfo));
        ui->infoTextEdit->appendPlainText("要发送的数据长度为:"+QString::number(header.pkg_len));
        ui->infoTextEdit->appendPlainText("包体的crc32值为:"+QString::number(header.crc32));
        memcpy(buffer, &header, sizeof(PkgHeader));
        memcpy(buffer+sizeof(PkgHeader), &info, sizeof(LoginInfo));
        int n{0};
        if((n = socket_->write(buffer, sizeof(PkgHeader)+sizeof(LoginInfo))) == -1)
        {
            ui->infoTextEdit->appendPlainText("发送数据时发生错误");
        }
        else if(n == 0)
        {
            ui->infoTextEdit->appendPlainText("返回0，表示服务端关闭了");
            socket_->abort();
        }
        socket_->waitForBytesWritten();
        ui->infoTextEdit->appendPlainText("数据发送完毕");

    }
    else
    {
        ui->infoTextEdit->appendPlainText("连接已经断开，请重新连接服务器");
    }
}


void MainWindow::on_disconnectedButton_clicked()
{
    if(socket_->state() == QTcpSocket::UnconnectedState)
    {
        ui->infoTextEdit->appendPlainText("已经关闭连接了，请重新连接服务器");
        return;
    }
    ui->infoTextEdit->appendPlainText("准备关闭连接");
    socket_->disconnectFromHost();
}


void MainWindow::on_regeditButton_clicked()
{
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();
    if(username.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入用户名");
        return;
    }
    if(password.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入密码");
        return;
    }
    if(socket_->state() == QAbstractSocket::ConnectedState)
    {
        char buffer[512];
        memset(buffer, 0, 512);
        RegisterInfo info;
        memset(&info, 0, sizeof(RegisterInfo));
        info.type = 1;
        strcpy(info.username, "cheng hao");
        strcpy(info.password, "123456");
        PkgHeader header;
        // pkg_Len = 8+100 = 108;
        header.pkg_len = sizeof(PkgHeader) + sizeof(RegisterInfo);
        header.msg_code = 5;
        header.crc32 = GetCRC((unsigned char*)&info, sizeof(RegisterInfo));
        ui->infoTextEdit->appendPlainText("要发送的数据长度为:"+QString::number(header.pkg_len));
        ui->infoTextEdit->appendPlainText("包体的crc32值为:"+QString::number(header.crc32));
        memcpy(buffer, &header, sizeof(PkgHeader));
        memcpy(buffer+sizeof(PkgHeader), &info, sizeof(RegisterInfo));
        int n{0};
        if((n = socket_->write(buffer, sizeof(PkgHeader)+sizeof(RegisterInfo))) == -1)
        {
            ui->infoTextEdit->appendPlainText("发送数据时发生错误");
        }
        else if(n == 0)
        {
            ui->infoTextEdit->appendPlainText("返回0，表示服务端关闭了");
            socket_->abort();
        }
        socket_->waitForBytesWritten();
        ui->infoTextEdit->appendPlainText("数据发送完毕");

    }
    else
    {
        ui->infoTextEdit->appendPlainText("连接已经断开，请重新连接服务器");
    }

}


void MainWindow::on_heartbeatButton_clicked()
{
    if(socket_->state() == QAbstractSocket::ConnectedState)
    {
        PkgHeader header;
        header.pkg_len = sizeof(PkgHeader);
        header.msg_code = 0;
        header.crc32 = 0;
        int n{0};
        if((n = socket_->write((char*)&header, sizeof(PkgHeader))) == -1)
        {
            ui->infoTextEdit->appendPlainText("发送心跳包时发生错误");
        }
        socket_->waitForBytesWritten();
        ui->infoTextEdit->appendPlainText("心跳包发送完毕");
    }
    else
    {
        ui->infoTextEdit->appendPlainText("连接已经断开，请重新连接服务器");
    }

}


void MainWindow::on_pushButton_clicked()
{

    if(socket_->state() == QAbstractSocket::ConnectedState)
    {
        if(type & 1)
        {
            PkgHeader header;
            header.pkg_len = 100;
            header.msg_code = 100;
            header.crc32 = 100;
            int n{0};
            if((n = socket_->write((char*)&header, sizeof(PkgHeader))) == -1)
            {
                ui->infoTextEdit->appendPlainText("发送错误包类型1时发生错误");
            }
            socket_->waitForBytesWritten();
            // 这里也是同理，因为包长度比包头大，所以服务器一直等后续包内容到达，
            // 如果一直没到，就只能等定时器自动踢开
            ui->infoTextEdit->appendPlainText("错误包类型1\n"
                                              "pkg_len = 100\n"
                                              "msg_code = 100\n"
                                              "crc32 = 100\n");
            ui->infoTextEdit->appendPlainText("错误包1发送完毕");
        }
        else
        {
            char buffer[32] = "hello world";
            int n{0};
            if((n = socket_->write(buffer, sizeof(buffer))) == -1)
            {
                ui->infoTextEdit->appendPlainText("发送错误包类型2时发生错误");
            }
            socket_->waitForBytesWritten();
            // 发送错包类型2的时候，并不会造成服务端的主动断开
            // 因为假如服务器识别到要收500个字符，但是实际过去20个，
            // connection 状态一直处于body_recving,
            // 这种情况只能等待定时器自动关闭
            ui->infoTextEdit->appendPlainText("错误包类型2\n"
                                              "buffer[32] = \"hello world\"\n");
            ui->infoTextEdit->appendPlainText("错误包2发送完毕");

        }
        ++type;
    }
    else
    {
        ui->infoTextEdit->appendPlainText("连接已经断开，请重新连接服务器");
    }
}


void MainWindow::on_floodButton_clicked()
{
    ui->infoTextEdit->appendPlainText("开始给服务器发送大量有效垃圾包");
    char buffer[512];
    memset(buffer, 0, 512);
    LoginInfo info;
    memset(&info, 0, sizeof(LoginInfo));
    strcpy(info.username, "cheng hao");
    strcpy(info.password, "123456");
    PkgHeader header;
    // pkg_Len = 8+100 = 108;
    header.pkg_len = sizeof(PkgHeader) + sizeof(LoginInfo);
    header.msg_code = 6;
    header.crc32 = GetCRC((unsigned char*)&info, sizeof(LoginInfo));
    ui->infoTextEdit->appendPlainText("要发送的数据长度为:"+QString::number(header.pkg_len));
    ui->infoTextEdit->appendPlainText("包体的crc32值为:"+QString::number(header.crc32));
    memcpy(buffer, &header, sizeof(PkgHeader));
    memcpy(buffer+sizeof(PkgHeader), &info, sizeof(LoginInfo));
    int n{0};

    for(int i = 0; i < 1000; ++i)
    {
        if(socket_->state() == QAbstractSocket::ConnectedState)
        {
            if((n = socket_->write(buffer, sizeof(PkgHeader)+sizeof(LoginInfo))) == -1)
            {
                ui->infoTextEdit->appendPlainText("发送数据时发生错误");
            }
            else if(n == 0)
            {
                ui->infoTextEdit->appendPlainText("返回0，表示服务端关闭了");
                socket_->abort();
            }
            socket_->waitForBytesWritten();
        }
        else
        {
            ui->infoTextEdit->appendPlainText("服务端断开连接,此时发送了 "+QString::number(i)+" 组数据");
            break;
        }
    }
}

