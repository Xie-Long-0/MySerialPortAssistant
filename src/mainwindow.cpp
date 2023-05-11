#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>

static QString DateTimeString()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

static QString FmtString(const QString &arg1, const QString &arg2, const QString &arg3,
                         const QColor &color = QColor("#88555555"))
{
    return QString("<span style='color: %5;'>[%4] %1 %2 %3</span>")
        .arg(arg1, arg2, arg3, DateTimeString(), color.name(QColor::HexArgb));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_config("config.ini", QSettings::IniFormat)
{
    ui->setupUi(this);

    setWindowTitle(tr("串口助手"));

    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::dataReceived);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &MainWindow::onPortErrorOccurred);

    updatePorts();

    ui->openPortBtn->setIcon(createBtnIcon(Qt::gray));
    connect(ui->openPortBtn, &QPushButton::clicked, this, &MainWindow::onOpenPortBtnClicked);
    connect(ui->clearBtn, &QPushButton::clicked, this, [this] { ui->sendTextEdit->clear(); });
    connect(ui->sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    // 切换面板
    connect(ui->switchBtn, &QPushButton::clicked, this, [this] {
        int index = ui->sendStackedPanel->currentIndex() + 1;
        ui->sendStackedPanel->setCurrentIndex(index % 2);
        });

    // 加载配置
    ui->hexShowChkBox->setChecked(m_config.value("RecvHex", false).toBool());
    ui->hexSendChkBox->setChecked(m_config.value("SendHex", false).toBool());
    ui->addNewlineChkBox->setChecked(m_config.value("SendNewLine", false).toBool());
    ui->newlineComboBox->setCurrentIndex(m_config.value("NewLine", 0).toInt());
    ui->sendStackedPanel->setCurrentIndex(m_config.value("SendPanel", 0).toInt());
}

MainWindow::~MainWindow()
{
    // 保存配置
    m_config.setValue("RecvHex", ui->hexShowChkBox->isChecked());
    m_config.setValue("SendHex", ui->hexSendChkBox->isChecked());
    m_config.setValue("SendNewLine", ui->addNewlineChkBox->isChecked());
    m_config.setValue("NewLine", ui->newlineComboBox->currentIndex());
    m_config.setValue("SendPanel", ui->sendStackedPanel->currentIndex());

    delete ui;
}

void MainWindow::updatePorts()
{
    auto ports = QSerialPortInfo::availablePorts();
    ui->portComboBox->clear();
    for (auto &p : ports)
    {
        if (!p.isNull())
            ui->portComboBox->addItem(p.portName());
    }

    if (ui->portComboBox->count() <= 0)
    {
        ui->openPortBtn->setEnabled(false);
        return;
    }
    ui->openPortBtn->setEnabled(true);

    auto port_name = m_config.value("PortName", "COM1").toString();
    auto port_baud = m_config.value("Baud", 115200).toInt();
    auto port_databits = m_config.value("DataBits", 3).toInt();
    auto port_parity = m_config.value("Parity", 0).toInt();
    auto port_stopbits = m_config.value("StopBits", 0).toInt();
    auto port_flowctrl = m_config.value("FlowCtrl", 0).toInt();

    // 使用上次的配置
    // 端口号
    int index = ui->portComboBox->findText(port_name);
    ui->portComboBox->setCurrentIndex(index < 0 ? 0 : index);
    // 波特率
    auto str_baud = QString::number(port_baud);
    index = ui->baudComboBox->findText(str_baud);
    if (index < 0)
    {
        ui->baudComboBox->setCurrentText(str_baud);
    }
    else
    {
        ui->baudComboBox->setCurrentIndex(index);
    }
    // 数据位
    ui->databitsComboBox->setCurrentIndex(port_databits);
    // 校验位
    ui->parityComboBox->setCurrentIndex(port_parity);
    // 停止位
    ui->stopbitsComboBox->setCurrentIndex(port_stopbits);
    // 流控制
    ui->flowCtrlComboBox->setCurrentIndex(port_flowctrl);
}

void MainWindow::dataReceived()
{
    auto data = m_serialPort->readAll();
    if (ui->hexShowChkBox->isChecked())
        data = data.toHex();

    ui->textEdit->append(FmtString(m_serialPort->portName(), "RECV",
                                   ui->hexShowChkBox->isChecked() ? "HEX" : "ASCII", "#880000ff"));
    ui->textEdit->append(QString::fromLocal8Bit(data));
}

void MainWindow::onOpenPortBtnClicked()
{
    // 关闭串口
    if (m_serialPort->isOpen())
    {
        m_serialPort->close();
        ui->openPortBtn->setText(tr("打开串口"));
        ui->openPortBtn->setIcon(createBtnIcon(Qt::gray));
        setWidgetsEnable(true);
        ui->textEdit->append(FmtString(m_serialPort->portName(), "CLOSE", "", "#88ff0000"));
        return;
    }
    // 打开串口
    else
    {
        m_serialPort->setPortName(ui->portComboBox->currentText());

        QStringList errors;
        if (!m_serialPort->setBaudRate(ui->baudComboBox->currentText().toInt()))
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
        if (!m_serialPort->setDataBits(databit))
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
        if (!m_serialPort->setParity(parity))
            errors.append(tr("校验位 %1 设置失败").arg(ui->parityComboBox->currentText()));

        QSerialPort::StopBits stopbit;
        switch (ui->stopbitsComboBox->currentIndex())
        {
        case 0: stopbit = QSerialPort::OneStop; break;
        case 1: stopbit = QSerialPort::OneAndHalfStop; break;
        case 2: stopbit = QSerialPort::TwoStop; break;
        default: stopbit = QSerialPort::OneStop; break;
        }
        if (!m_serialPort->setStopBits(stopbit))
            errors.append(tr("停止位 %1 设置失败").arg(ui->stopbitsComboBox->currentText()));

        QSerialPort::FlowControl flowctrl;
        switch (ui->flowCtrlComboBox->currentIndex())
        {
        case 0: flowctrl = QSerialPort::NoFlowControl; break;
        case 1:
        case 2: flowctrl = QSerialPort::SoftwareControl; break;
        case 3:
        case 4: flowctrl = QSerialPort::HardwareControl; break;
        default: flowctrl = QSerialPort::NoFlowControl; break;
        }
        if (!m_serialPort->setFlowControl(flowctrl))
            errors.append(tr("流控制 %1 设置失败").arg(ui->flowCtrlComboBox->currentText()));

        if (!errors.isEmpty())
            QMessageBox::warning(this, tr("串口设置"), errors.join("\r\n"));

        if (!m_serialPort->open(QIODevice::ReadWrite))
        {
            QMessageBox::critical(this, tr("打开串口"), tr("无法打开串口“%1”！").arg(ui->portComboBox->currentText()));
            return;
        }

        // 保存配置
        m_config.setValue("PortName", ui->portComboBox->currentText());
        m_config.setValue("Baud", ui->baudComboBox->currentText().toInt());
        m_config.setValue("DataBits", ui->databitsComboBox->currentIndex());
        m_config.setValue("Parity", ui->parityComboBox->currentIndex());
        m_config.setValue("StopBits", ui->stopbitsComboBox->currentIndex());
        m_config.setValue("FlowCtrl", ui->flowCtrlComboBox->currentIndex());

        ui->openPortBtn->setText(tr("关闭串口"));
        ui->openPortBtn->setIcon(createBtnIcon(Qt::green));
        setWidgetsEnable(false);
        ui->textEdit->append(FmtString(m_serialPort->portName(), "OPEN", "", "#88008800"));
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
    ui->portGroupBox->setEnabled(enable);
}

void MainWindow::onSendBtnClicked()
{
    if (m_serialPort->isOpen())
    {
        auto data = ui->sendTextEdit->toPlainText().toLocal8Bit();

        if (data.isEmpty())
            return;

        auto newline = "\r\n";
        if (ui->newlineComboBox->currentIndex() == 1)
            newline = "\r";
        else if (ui->newlineComboBox->currentIndex() == 2)
            newline = "\n";
        if (ui->addNewlineChkBox->isChecked())
            data.append(newline);
        if (ui->hexSendChkBox->isChecked())
            data = data.toHex();

        if (m_serialPort->write(data) && m_serialPort->waitForBytesWritten(1000))
        {
            ui->textEdit->append(FmtString(m_serialPort->portName(), "SEND",
                                           ui->hexSendChkBox->isChecked() ? "HEX" : "ASCII"));
            ui->textEdit->append(data);
        }
        else
        {
            ui->textEdit->append(FmtString(m_serialPort->portName(), "SEND", "Failed", "#88ff0000"));
        }
    }
}

void MainWindow::onPortErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    QMessageBox::critical(this, tr("发生错误"), tr("串口错误：%1").arg(m_serialPort->errorString()));
}
