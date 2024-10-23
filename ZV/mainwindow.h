#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QListWidget>
#include <gst/gst.h>  // Подключаем GStreamer

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startStreaming();       // Начало трансляции
    void stopStreaming();        // Остановка трансляции
    void ChooseFile();         // Загрузка аудиофайла
    void reloadDevices();        // Перезагрузка списка устройств
    void streamFromMicrophone(); // Трансляция с микрофона

private:
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *ChooseFileButton;
    QPushButton *reloadDevicesButton;
    QPushButton *microphoneButton;
    QListWidget *deviceList;

    GstElement *pipeline;  // Объявляем pipeline для работы с GStreamer
};

#endif // MAINWINDOW_H
