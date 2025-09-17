#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void readSerialData();

    void on_connect_btn_clicked();

    void on_sendcmd_btn_clicked();

    void on_stop_btn_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    //QStatusBar *statbar = this->statusBar();
    void updateSerialPortList();
    bool isConnected();

};
#endif // MAINWINDOW_H
