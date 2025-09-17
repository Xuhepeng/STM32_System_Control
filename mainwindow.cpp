#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建状态栏
    //this->setStatusBar(statbar);
    //statbar->addWidget(ui->status_label);
    //初始化串口
    serial = new QSerialPort(this);
    //连接信号槽
    connect(serial,&QSerialPort::readyRead,this,&MainWindow::readSerialData);
    // 更新串口列表
    updateSerialPortList();
    //设置下拉框选项
    ui->baud_boBox->addItems({"9600", "19200", "38400", "57600", "115200",
                            "230400", "460800", "921600"});
    ui->data_boBox->addItems({"5", "6", "7", "8"});
    ui->vali_boBox->addItems({"无", "奇", "偶"});
    ui->stop_boBox->addItems({"1", "1.5", "2"});
    //设置默认参数
    ui->baud_boBox->setCurrentText("115200");
    ui->data_boBox->setCurrentText("8");
    ui->vali_boBox->setCurrentText("无");
    ui->stop_boBox->setCurrentText("1");


    //设置默认电机参数
    ui->step_spinBox->setValue(1000);
    ui->speed_spinBox->setValue(500);
    ui->forward_rdbtn->setChecked(true);
    ui->enable_checkBox->setChecked(false);

    // 正转勾选 → 取消反转
    connect(ui->forward_rdbtn, &QCheckBox::clicked, this, [=](bool checked) {
        if (checked) {
            ui->reverse_rdbtn->setChecked(false);
        }
    });
    // 反转勾选 → 取消正转
    connect(ui->reverse_rdbtn, &QCheckBox::clicked, this, [=](bool checked) {
        if (checked) {
            ui->forward_rdbtn->setChecked(false);
        }
    });

    // 初始禁用groupBox（未连接串口时不可操作）
    ui->groupBox->setEnabled(false);  // 添加此行，明确初始状态
    //滑块信号连接
    connect(ui->horizontalSlider, &QSlider::valueChanged, ui->speed_spinBox, &QSpinBox::setValue);
    connect(ui->speed_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->horizontalSlider, &QSlider::setValue);
    // 设置滑块范围（根据实际需求调整）
    ui->horizontalSlider->setRange(0, 1000);
    ui->speed_spinBox->setRange(0, 1000);
}

MainWindow::~MainWindow()
{
    if(serial->isOpen())
    {
        serial->close();
    }
    delete ui;
    delete serial;
}

void MainWindow::updateSerialPortList()
{
    ui->comboBox->clear();
    //获取端口的具体信息列表
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBox->addItem(info.portName() + " (" + info.description() + ")");
    }
}

bool MainWindow::isConnected()
{
    return serial->isOpen();
}



void MainWindow::readSerialData()
{
    QByteArray data = serial->readAll();
    if(!data.isEmpty())
    {
        ui->logtextEdit->append("接收: " + data);
    }
}


void MainWindow::on_connect_btn_clicked()
{
    if(isConnected())
    {
        //断开连接
        serial->close();
        ui->connect_btn->setText("connect");
        ui->groupBox->setEnabled(false);
        ui->statusbar->showMessage("已断开连接");
    }
    else
    {
        //连接串口
        if(ui->comboBox->count() == 0)
        {
            QMessageBox::warning(this,"警告","未发现可用串口！");
            return;
        }
        //获取选择的端口名
        QString PortName = ui->comboBox->currentText().split(" ")[0];
        //配置串口参数
        serial->setPortName(PortName);
        //设置串口的波特率
        serial->setBaudRate(ui->baud_boBox->currentText().toInt());
        //设置数据位
        switch (ui->data_boBox->currentText().toInt()) {
        case 5:
            serial->setDataBits(QSerialPort::Data5);
            break;
        case 6:
            serial->setDataBits(QSerialPort::Data6);
            break;
        case 7:
            serial->setDataBits(QSerialPort::Data7);
            break;
        case 8:
            serial->setDataBits(QSerialPort::Data8);
            break;
        default:
            serial->setDataBits(QSerialPort::Data8);
            break;
        }

        //设置校验位
        if(ui->vali_boBox->currentText() == "无")
            serial->setParity(QSerialPort::NoParity);
        else if(ui->vali_boBox->currentText() == "奇")
            serial->setParity(QSerialPort::OddParity);
        else if(ui->vali_boBox->currentText() == "偶")
            serial->setParity(QSerialPort::EvenParity);

        //设置停止位
        if(ui->stop_boBox->currentText() == "1")
            serial->setStopBits(QSerialPort::OneStop);
        else if(ui->stop_boBox->currentText() == "1.5")
            serial->setStopBits(QSerialPort::OneAndHalfStop);
        else if(ui->stop_boBox->currentText() == "2")
            serial->setStopBits(QSerialPort::TwoStop);

        serial->setFlowControl(QSerialPort::NoFlowControl);

        if(serial->open(QIODevice::ReadWrite))
        {
            ui->connect_btn->setText("disconnect");
            ui->groupBox->setEnabled(true);
            ui->statusbar->showMessage("已连接到"+PortName);
            foreach (QWidget *widget, ui->groupBox->findChildren<QWidget*>()) {
                widget->setEnabled(true);
            }
        }
        else
        {
            QMessageBox::critical(this,"错误","无法打开串口："+serial->errorString());
        }
    }
}


void MainWindow::on_sendcmd_btn_clicked()
{
    if(!isConnected())
    {
        QMessageBox::warning(this,"警告","请先连接串口！");
        return;
    }

    //获取参数
    int steps = ui->step_spinBox->value();


    int dir;
    if(ui->forward_rdbtn->isChecked())
        dir = 1;
    else if(ui->reverse_rdbtn->isChecked())
        dir = 0;
    else
    {
        dir = 1;
        QMessageBox::warning(this,"警告","未选择方向，默认正转");
    }
    int enable;
    if(ui->enable_checkBox->isChecked())
        enable = 1;
    else
        enable = 0;

    int speed = ui->speed_spinBox->value();
    // 构建指令格式: CMD:steps,dir,enable,speed;
    QString cmd = QString("CMD:%1,%2,%3,%4;\0")
                      .arg(steps)
                      .arg(dir)
                      .arg(enable)
                      .arg(speed);
    // 发送指令
    qint64 bytesWritten = serial->write(cmd.toUtf8());
    if (bytesWritten == -1) {
        QMessageBox::critical(this, "错误", "发送失败: " + serial->errorString());
    } else {
        ui->logtextEdit->append("发送: " + cmd);
    }
}


void MainWindow::on_stop_btn_clicked()
{
    if (!isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口!");
        return;
    }

    // 发送停止指令 (steps=0)
    // 停止指令：steps=0（停止运动），保持当前enable状态（避免禁用）
    int currentEnable = ui->enable_checkBox->isChecked() ? 1 : 0;
    QString cmd = QString("CMD:0,0,%1,0;").arg(currentEnable);
    if (serial->write(cmd.toUtf8()) != -1) {
        ui->logtextEdit->append("发送: " + cmd + " (停止电机)");
    }
}

