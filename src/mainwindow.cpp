#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 串口线程
    m_portThread = new QThread;
    m_mySerialPort = new SerialPort();
    // 先移动线程再连接信号槽，因为连接信号时会判断信号与槽是否在同一线程，以决定信号连接方式，除非手动指定连接方式
    m_mySerialPort->moveToThread(m_portThread);
    connect(m_portThread, &QThread::finished, m_portThread, &QObject::deleteLater);
    connect(m_portThread, &QThread::finished, m_mySerialPort, &QObject::deleteLater);
    connect(m_portThread, &QThread::started, m_mySerialPort, &SerialPort::startProtThread);
    connect(this, &MainWindow::dataSends, m_mySerialPort, &SerialPort::sendData);
    connect(m_mySerialPort, &SerialPort::dataReceived, this, &MainWindow::dataReceived);
    // 启动线程，QThread默认会开启线程中的事件循环
    m_portThread->start();
    updatePorts();

    // TODO: 使用上次的配置
    ui->portComboBox->setCurrentIndex(ui->portComboBox->findText("COM11"));
    ui->baudComboBox->setCurrentIndex(ui->baudComboBox->findText("115200"));
    ui->databitsComboBox->setCurrentIndex(3);
    ui->parityComboBox->setCurrentIndex(0);
    ui->stopbitsComboBox->setCurrentIndex(0);
    ui->flowCtrlComboBox->setCurrentIndex(0);

    ui->openPortBtn->setIcon(createBtnIcon(Qt::red));
    ui->showSentDataChkBox->setChecked(true);
}

MainWindow::~MainWindow()
{
    m_portThread->quit();
    m_portThread->wait();
    delete ui;
}

void MainWindow::updatePorts()
{
    auto ports = m_mySerialPort->availablePorts();
    ui->portComboBox->clear();
    ui->portComboBox->addItems(ports);
}

void MainWindow::dataReceived(const QByteArray &data)
{
    // 使用HTML在TextEdit中设置格式化文本
    ui->textEdit->append(QString("<span style='color: #66555555;'>[%1] %2 %3 &lt;&lt;</span>")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(ui->portComboBox->currentText(), "ASCII"));
    ui->textEdit->append(QString().fromLocal8Bit(data) + "\r\n");
}

void MainWindow::on_openPortBtn_clicked()
{
    if (m_mySerialPort->isOpen())
    {
        m_mySerialPort->close();
        ui->openPortBtn->setText(tr("打开串口"));
        ui->openPortBtn->setIcon(createBtnIcon(Qt::red));
        setWidgetsEnable(true);
        return;
    }
    else
    {
        QStringList errors;
        if (!m_mySerialPort->setBaudRate(ui->baudComboBox->currentText().toInt()))
            errors.append(tr("波特率 %1 设置失败").arg(ui->baudComboBox->currentText()));

        QSerialPort::DataBits databit;
        switch (ui->databitsComboBox->currentIndex())
        {
        case 0: databit = QSerialPort::Data5; break;
        case 1: databit = QSerialPort::Data6; break;
        case 2: databit = QSerialPort::Data7; break;
        case 3: databit = QSerialPort::Data8; break;
        default: databit = QSerialPort::Data8; break;
        }
        if (!m_mySerialPort->setDataBits(databit))
            errors.append(tr("数据位 %1 设置失败").arg(ui->databitsComboBox->currentText()));

        QSerialPort::Parity parity;
        switch (ui->parityComboBox->currentIndex())
        {
        case 0: parity = QSerialPort::NoParity; break;
        case 1: parity = QSerialPort::OddParity; break;
        case 2: parity = QSerialPort::EvenParity; break;
        case 3: parity = QSerialPort::MarkParity; break;
        case 4: parity = QSerialPort::SpaceParity; break;
        default: parity = QSerialPort::NoParity; break;
        }
        if (!m_mySerialPort->setParity(parity))
            errors.append(tr("校验位 %1 设置失败").arg(ui->parityComboBox->currentText()));

        QSerialPort::StopBits stopbit;
        switch (ui->stopbitsComboBox->currentIndex())
        {
        case 0: stopbit = QSerialPort::OneStop; break;
        case 1: stopbit = QSerialPort::OneAndHalfStop; break;
        case 2: stopbit = QSerialPort::TwoStop; break;
        default: stopbit = QSerialPort::OneStop; break;
        }
        if (!m_mySerialPort->setStopBits(stopbit))
            errors.append(tr("停止位 %1 设置失败").arg(ui->stopbitsComboBox->currentText()));

        // TODO: 设置流控制

        if (!errors.isEmpty())
            QMessageBox::warning(this, tr("串口设置"), tr("某些串口设置无法应用：\r\n") + errors.join("\r\n"));

        if (!m_mySerialPort->open())
        {
            QMessageBox::critical(this, tr("打开串口"), tr("无法打开串口！"));
            return;
        }
    }
    ui->textEdit->append(QString("<span style='color: #66555555;'>[%1] %3: %2</span>")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(m_mySerialPort->portName(), tr("串口打开成功")));
    ui->openPortBtn->setText(tr("关闭串口"));
    ui->openPortBtn->setIcon(createBtnIcon(Qt::green));
    setWidgetsEnable(false);
}

void MainWindow::on_portComboBox_currentTextChanged(const QString &port)
{
    if (m_mySerialPort->isOpen())
    {
        return;
    }

    if (!m_mySerialPort->setPortName(port))
    {
        QMessageBox::warning(this, tr("设置串口"), tr("无法设置串口%1").arg(port));
        return;
    }
}

QPixmap MainWindow::createBtnIcon(const QColor &color)
{
    QImage icon(16, 16, QImage::Format_ARGB32);
    icon.fill(QColor(0, 0, 0, 0));
    QPainter p(&icon);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath pp;
    pp.addEllipse(QPoint(8, 8), 5, 5);
    p.fillPath(pp, color);
    p.setPen(QPen(Qt::black, 1));
    p.drawEllipse(QPoint(8, 8), 7, 7);
    p.end();
    return QPixmap().fromImage(icon);
}

void MainWindow::setWidgetsEnable(bool enable)
{
    ui->portComboBox->setEnabled(enable);
    ui->baudComboBox->setEnabled(enable);
    ui->databitsComboBox->setEnabled(enable);
    ui->parityComboBox->setEnabled(enable);
    ui->stopbitsComboBox->setEnabled(enable);
    ui->flowCtrlComboBox->setEnabled(enable);
}

void MainWindow::on_clearBtn_clicked()
{
    ui->sendTextEdit->clear();
}

void MainWindow::on_sendBtn_clicked()
{
    auto data = ui->sendTextEdit->toPlainText();

    if (data.isEmpty())
        return;

    if (m_mySerialPort->isOpen())
    {
        emit dataSends(data.toLocal8Bit());

        if (ui->showSentDataChkBox->isChecked())
        {
            ui->textEdit->append(QString("<span style='color: #66555555;'>[%1] %2 %3 &gt;&gt;</span>")
                                 .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                                 .arg(ui->portComboBox->currentText(), "ASCII"));
            ui->textEdit->append(data + "\r\n");
        }
    }
}
