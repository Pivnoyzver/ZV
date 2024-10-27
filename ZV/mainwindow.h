#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
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
    void reloadDevices();        // слот кнопки перезагрузки списка устройств
    void streamFromMicrophone();  // Трансляция с микрофона

    void reload();         // Перезагрузка списка устройств

    void eoscheck();              // Проверка конца файла

signals:
    void eos();  // Сигнал

private:
    QPushButton *startButton;
    QPushButton *skipButton;
    QPushButton *pauseButton;
    QPushButton *microphoneButton;
    QPushButton *stopButton;

    QPushButton *reloadDevicesButton;
    QListWidget *deviceList;

    QPushButton *AddFileButton;
    QPushButton *RemoveFileButton;
    QListWidget *playlistWidget;

    GstElement *pipeline;  // Объявляем pipeline для работы с GStreamer

    QTimer *timer = new QTimer(this);
};

#endif // MAINWINDOW_H
