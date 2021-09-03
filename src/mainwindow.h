#pragma once

#include <QMainWindow>
#include <QThread>
#include "serialport.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updatePorts();

public slots:
    void dataReceived(const QByteArray &data);

private slots:
    void on_openPortBtn_clicked();
    void on_portComboBox_currentTextChanged(const QString &port);
    void on_clearBtn_clicked();
    void on_sendBtn_clicked();

signals:
    void dataSends(const QByteArray &data);

private:
    QPixmap createBtnIcon(const QColor &color);
    void setWidgetsEnable(bool enable);

private:
    Ui::MainWindow *ui;
    SerialPort *m_mySerialPort = nullptr;
    QThread *m_portThread = nullptr;
};
