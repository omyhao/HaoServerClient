#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QNetworkProxy>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();

    void on_clearButton_clicked();

    void on_loginButton_clicked();

    void on_disconnectedButton_clicked();
    void on_regeditButton_clicked();

    void on_heartbeatButton_clicked();

    void on_pushButton_clicked();

    void on_floodButton_clicked();

private:
    QString SocketStateString(QAbstractSocket::SocketState);
private:
    Ui::MainWindow *ui;
    QTcpSocket *socket_;
    QNetworkProxy proxy;
    quint32 type = 0;
};
#endif // MAINWINDOW_H
