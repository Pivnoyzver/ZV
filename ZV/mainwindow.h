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
    void startStreaming();        // Начало трансляции
    void stopStreaming();         // Остановка трансляции
    void AddFile();               // Загрузка аудиофайла
    void RemoveFile();            // Удаление последнего аудиофайла
    void reloadDevices();         // Перезагрузка списка устройств
    void streamFromMicrophone();  // Трансляция с микрофона

private:
    QPushButton *startButton;
    QPushButton *microphoneButton;
    QPushButton *stopButton;

    QPushButton *AddFileButton;
    QPushButton *RemoveFileButton;

    QPushButton *reloadDevicesButton;
    QListWidget *deviceList;

    QListWidget *playlistWidget;

    GstElement *pipeline;  // Объявляем pipeline для работы с GStreamer
};

#endif // MAINWINDOW_H
