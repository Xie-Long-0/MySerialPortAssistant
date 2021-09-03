#include "serialport.h"
#include <QSerialPortInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

SerialPort::SerialPort(QObject *parent) : QObject(parent)
{
    m_qSerialPort = new QSerialPort(this);
}

SerialPort::~SerialPort()
{
    if (m_qSerialPort)
    {
        m_qSerialPort->close();
        m_qSerialPort->deleteLater();
        m_qSerialPort = nullptr;
        qDebug() << __FUNCTION__ << "SerialPort quit";
    }
}

bool SerialPort::open()
{
    if (m_qSerialPort->isOpen())
    {
        m_qSerialPort->close();
    }
    return m_qSerialPort->open(QIODevice::ReadWrite);
}

void SerialPort::close()
{
    m_qSerialPort->close();
}

bool SerialPort::setPortName(const QString &name)
{
    if (!m_qSerialPort->isOpen() && !name.isEmpty())
    {
        auto ports_name = this->availablePorts();
        if (ports_name.contains(name, Qt::CaseInsensitive))
        {
            m_qSerialPort->setPortName(name);
            return true;
        }
    }
    return false;
}

bool SerialPort::setBaudRate(qint32 baudrate)
{
    if (!m_qSerialPort->isOpen() && baudrate > 0)
    {
        return m_qSerialPort->setBaudRate(baudrate);
    }
    return false;
}

bool SerialPort::setStopBits(QSerialPort::StopBits stopbit)
{
    if (!m_qSerialPort->isOpen())
    {
        return m_qSerialPort->setStopBits(stopbit);
    }
    return false;
}

bool SerialPort::setDataBits(QSerialPort::DataBits databit)
{
    if (!m_qSerialPort->isOpen())
    {
        return m_qSerialPort->setDataBits(databit);
    }
    return false;
}

bool SerialPort::setParity(QSerialPort::Parity parity)
{
    if (!m_qSerialPort->isOpen())
    {
        return m_qSerialPort->setParity(parity);
    }
    return false;
}

QStringList SerialPort::availablePorts()
{
    QStringList ports_name;
    auto ports = QSerialPortInfo::availablePorts();
    for (auto &p : ports)
    {
        if (!ports_name.contains(p.portName()))
        {
            ports_name.append(p.portName());
        }
    }
    return ports_name;
}

void SerialPort::startProtThread()
{
    connect(m_qSerialPort, &QSerialPort::readyRead, this, &SerialPort::onDataReceived);
    connect(m_qSerialPort, &QSerialPort::errorOccurred, this, &SerialPort::errorOccurred);
}

void SerialPort::sendData(const QByteArray &data)
{
    if (m_qSerialPort->isOpen())
    {
//        qDebug() << __FUNCTION__ << "Send data:" << data;
        m_qSerialPort->write(data);
        m_qSerialPort->waitForBytesWritten();
    }
}

void SerialPort::blockPort(bool block)
{
    QMutex mutex;
    mutex.lock();
    m_block = block;
    mutex.unlock();
}

void SerialPort::onDataReceived()
{
    if (!m_block)
        emit dataReceived(m_qSerialPort->readAll());
}
