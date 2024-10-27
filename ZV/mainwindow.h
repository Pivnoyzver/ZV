#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QListWidget>
#include <QIcon>

#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <gst/gst.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    void skipFile();
    void pauseStreaming();

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

    QPushButton *addFileButton;
    QPushButton *removeFileButton;
    QListWidget *playlistWidget;

    GstElement *pipeline;  // Объявляем pipeline для работы с GStreamer

    QTimer *timer = new QTimer(this);
};

#endif // MAINWINDOW_H
