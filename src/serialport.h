#pragma once

#include <QObject>
#include <QSerialPort>

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);
    virtual ~SerialPort();

    bool open();
    bool setPortName(const QString &name);
    bool setBaudRate(qint32 baudrate);
    bool setStopBits(QSerialPort::StopBits stopbit);
    bool setDataBits(QSerialPort::DataBits databit);
    bool setParity(QSerialPort::Parity parity);

    static QStringList availablePorts();
    QString portName() const { return m_qSerialPort->portName(); }
    bool isOpen() const { return m_qSerialPort->isOpen(); }
    qint32 baudRate() const { return m_qSerialPort->baudRate(); }
    QSerialPort::StopBits stopBits() const { return m_qSerialPort->stopBits(); }
    QSerialPort::DataBits dataBits() const { return m_qSerialPort->dataBits(); }

public slots:
    void close();
    void startProtThread(); // 用于启动线程后连接内部信号槽
    void sendData(const QByteArray &data);
    void blockPort(bool block);

protected slots:
    void onDataReceived();

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_qSerialPort = nullptr;
    volatile bool m_block = false;
};

