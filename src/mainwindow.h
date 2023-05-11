#pragma once

#include <QMainWindow>
#include <QSerialPort>
#include <QSettings>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void dataReceived();
    void onOpenPortBtnClicked();
    void onSendBtnClicked();
    void onPortErrorOccurred(QSerialPort::SerialPortError error);

private:
    void updatePorts();
    QPixmap createBtnIcon(const QColor &color);
    void setWidgetsEnable(bool enable);

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serialPort = nullptr;
    QSettings m_config;
};
