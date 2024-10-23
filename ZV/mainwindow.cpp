#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStringList>
#include <QDebug>
#include <gst/gst.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

#include <gst/gstbin.h>  // Для отладки pipeline

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), pipeline(nullptr) //Иницивлизация pipiline как nullptr
{

    // Инициализация GStreamer
        GError *error = nullptr;
        if (!gst_init_check(nullptr, nullptr, &error)) {
            qDebug() << "Ошибка инициализации GStreamer:" << error->message;
            g_error_free(error);
        } else {
            qDebug() << "GStreamer initialized successfully!";
        }

    // Создаём кнопки и список
    startButton = new QPushButton("Начать трансляцию", this);
    stopButton = new QPushButton("Остановить трансляцию", this);
    loadPlaylistButton = new QPushButton("Загрузить плейлист", this);
    reloadDevicesButton = new QPushButton("Перезагрузить устройства", this);
    microphoneButton = new QPushButton("Трансляция с микрофона", this);
    deviceList = new QListWidget(this);

    // Устанавливаем режим множественного выбора для списка
    deviceList->setSelectionMode(QAbstractItemView::MultiSelection);

    // Создаём вертикальный макет (layout)
    QVBoxLayout *layout = new QVBoxLayout();

    // Добавляем кнопки и список в макет
    layout->addWidget(loadPlaylistButton);
    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(microphoneButton);
    layout->addWidget(reloadDevicesButton);
    layout->addWidget(deviceList);

    // Создаём центральный виджет и устанавливаем для него макет
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // Добавляем устройства в список (например, IP-адреса)
    reloadDevices();

    // Подключаем сигналы кнопок к слотам
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startStreaming);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopStreaming);
    connect(loadPlaylistButton, &QPushButton::clicked, this, &MainWindow::loadPlaylist);
    connect(reloadDevicesButton, &QPushButton::clicked, this, &MainWindow::reloadDevices);
    connect(microphoneButton, &QPushButton::clicked, this, &MainWindow::streamFromMicrophone);
}

MainWindow::~MainWindow()
{
}

QString audioFilePath; // Глобальная переменная для хранения пути к аудиофайлу

void MainWindow::loadPlaylist()
{
    // Открываем диалог для выбора аудиофайла
    audioFilePath = QFileDialog::getOpenFileName(this, "Выберите аудиофайл", "", "Audio Files (*.mp3 *.wav)");

    if (!audioFilePath.isEmpty()) {
        qDebug() << "Выбранный аудиофайл:" << audioFilePath;
    } else {
        qDebug() << "Файл не был выбран.";
    }
}

void MainWindow::startStreaming()
{
    if (audioFilePath.isEmpty()) {
        qDebug() << "Аудиофайл не был выбран!";
        return;
    }

    // Получаем выбранные устройства из списка
    QList<QListWidgetItem*> selectedDevices = deviceList->selectedItems();

    if (!selectedDevices.isEmpty()) {
        // Проверяем, что pipeline не был ранее создан
        if (pipeline) {
            qDebug() << "Трансляция уже запущена!";
            return;
        }

        qDebug() << "Создаём GStreamer pipeline...";

        pipeline = gst_pipeline_new("audio-pipeline");
        GstElement *source = gst_element_factory_make("filesrc", "source");
        GstElement *decoder = gst_element_factory_make("decodebin", "decoder");
        GstElement *convert = gst_element_factory_make("audioconvert", "convert");
        GstElement *tee = gst_element_factory_make("tee", "tee");

        if (!pipeline || !source || !decoder || !convert || !tee) {
            qDebug() << "Ошибка: Не удалось создать один или несколько элементов GStreamer!";
            return;
        }

        // Устанавливаем путь к выбранному аудиофайлу
        g_object_set(source, "location", audioFilePath.toStdString().c_str(), NULL);
        qDebug() << "Путь к аудиофайлу установлен:" << audioFilePath;

        // Добавляем элементы в pipeline
        gst_bin_add_many(GST_BIN(pipeline), source, decoder, convert, tee, NULL);

        // Связываем source с decoder
        if (gst_element_link(source, decoder) != TRUE) {
            qDebug() << "Ошибка: Не удалось связать source и decoder!";
            return;
        }

        // Когда decoder готов, связываем его с audioconvert
        g_signal_connect(decoder, "pad-added", G_CALLBACK(+[](GstElement *, GstPad *pad, GstElement *convert) {
            GstPad *sinkpad = gst_element_get_static_pad(convert, "sink");
            if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
                qDebug() << "Ошибка: Не удалось связать decoder и audioconvert!";
            }
            gst_object_unref(sinkpad);
        }), convert);

        // Связываем convert и tee
        if (gst_element_link_many(convert, tee, NULL) != TRUE) {
            qDebug() << "Ошибка: Не удалось связать convert и tee!";
            return;
        }

        qDebug() << "Элементы GStreamer созданы успешно.";

        // Настраиваем трансляцию на выбранные устройства
        for (auto *item : selectedDevices) {
            QString deviceIP = item->text();
            qDebug() << "Настраиваем трансляцию на устройство:" << deviceIP;

            GstElement *queue = gst_element_factory_make("queue", nullptr);
            GstElement *rtpPay = gst_element_factory_make("rtpL16pay", nullptr);
            GstElement *udpSink = gst_element_factory_make("udpsink", nullptr);

            if (!queue || !rtpPay || !udpSink) {
                qDebug() << "Ошибка: Не удалось создать элементы для RTP трансляции!";
                return;
            }

            // Добавляем элементы в pipeline и связываем tee с queue, rtpL16pay и udpSink
            gst_bin_add_many(GST_BIN(pipeline), queue, rtpPay, udpSink, NULL);

            // Связываем tee -> queue -> rtpL16pay -> udpsink
            if (!gst_element_link_many(tee, queue, rtpPay, udpSink, NULL)) {
                qDebug() << "Ошибка: Не удалось связать tee, queue, rtpL16pay и udpsink!";
                return;
            }

            // Устанавливаем параметры для RTP-трансляции (host, port, pt)
            g_object_set(udpSink,"host", deviceIP.toStdString().c_str(),"port", 5004,"pt", 10,NULL);
        }

        // Запускаем pipeline
        qDebug() << "Запуск pipeline...";
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        // Проверка состояния pipeline
        GstState state;
        GstState pending;
        GstStateChangeReturn ret = gst_element_get_state(pipeline, &state, &pending, 5 * GST_SECOND);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            qDebug() << "Ошибка при запуске трансляции.";
        } else if (ret == GST_STATE_CHANGE_ASYNC) {
            qDebug() << "Трансляция запускается асинхронно.";
            qDebug() << "Текущее состояние:" << gst_element_state_get_name(state);
            qDebug() << "Ожидаемое состояние:" << gst_element_state_get_name(pending);
        } else if (ret == GST_STATE_CHANGE_SUCCESS) {
            qDebug() << "Трансляция успешно начата!";
        }

        // Дополнительная проверка состояния элементов
//        GstStateChangeReturn srcState = gst_element_set_state(source, GST_STATE_PLAYING);
//        qDebug() << "Source state:" << srcState;

//        GstStateChangeReturn decState = gst_element_set_state(decoder, GST_STATE_PLAYING);
//        qDebug() << "Decoder state:" << decState;

//        for (auto *item : selectedDevices) {
//            GstStateChangeReturn payState = gst_element_set_state(gst_bin_get_by_name(GST_BIN(pipeline), "rtpL16pay"), GST_STATE_PLAYING);
//            qDebug() << "RTP Payloader state:" << payState;

//            GstStateChangeReturn udpState = gst_element_set_state(gst_bin_get_by_name(GST_BIN(pipeline), "udpsink"), GST_STATE_PLAYING);
//            qDebug() << "UDP Sink state:" << udpState;
//        }

    } else {
        qDebug() << "Не выбрано ни одного устройства.";
    }
}

void MainWindow::stopStreaming()
{
    // Останавливаем текущий GStreamer pipeline, если он существует
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);  // Останавливаем pipeline
        gst_object_unref(pipeline);  // Освобождаем ресурсы
        pipeline = nullptr;  // Сбрасываем pipeline
        qDebug() << "Трансляция остановлена.";
    } else {
//        QMessageBox::information(this, "Трансляция", "Трансляция не была запущена.");
        qDebug() << "Трансляция не была запущена.";
    }
}

void MainWindow::reloadDevices()
{
    // Очищаем текущий список устройств
    deviceList->clear();

    // Открываем JSON-файл с устройствами
    QFile file("devices.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть файл devices.json";
        return;
    }

    // Читаем содержимое файла
    QByteArray fileData = file.readAll();
    file.close();

    // Парсим JSON
    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isObject()) {
        qDebug() << "Неверный формат JSON";
        return;
    }

    QJsonObject jsonObj = doc.object();
    QJsonArray devicesArray = jsonObj["devices"].toArray();

    // Добавляем устройства в список
    foreach (const QJsonValue &value, devicesArray) {
        QString deviceIP = value.toString();
        deviceList->addItem(deviceIP);
        qDebug() << "Добавлено устройство:" << deviceIP;
    }

    qDebug() << "Список устройств обновлён.";
}

void MainWindow::streamFromMicrophone()
{
    // Получаем выбранные устройства из списка
    QList<QListWidgetItem*> selectedDevices = deviceList->selectedItems();

    if (!selectedDevices.isEmpty()) {
        // Проверяем, что pipeline не был ранее создан
        if (pipeline) {
            stopStreaming();
        }

        qDebug() << "Создаём GStreamer pipeline для микрофона...";

        pipeline = gst_pipeline_new("mic-pipeline");
        GstElement *micSource = gst_element_factory_make("autoaudiosrc", "mic-source");
        GstElement *convert = gst_element_factory_make("audioconvert", "convert");
        GstElement *resample = gst_element_factory_make("audioresample", "resample");
        GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter"); // Фильтр для явной установки параметров
        GstElement *tee = gst_element_factory_make("tee", "tee");

        if (!pipeline || !micSource || !convert || !resample || !capsfilter || !tee) {
            qDebug() << "Ошибка: Не удалось создать один или несколько элементов GStreamer!";
            return;
        }

        // Устанавливаем caps для rtpL16pay с частотой дискретизации 44100 и 2 каналами
        GstCaps *caps = gst_caps_new_simple("audio/x-raw","rate", G_TYPE_INT, 44100,"channels", G_TYPE_INT, 2,NULL);

        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        // Добавляем элементы в pipeline
        gst_bin_add_many(GST_BIN(pipeline), micSource, convert, resample, capsfilter, tee, NULL);

        // Связываем micSource -> convert -> resample -> capsfilter -> tee
        if (!gst_element_link_many(micSource, convert, resample, capsfilter, tee, NULL)) {
            qDebug() << "Ошибка: Не удалось связать micSource, convert, resample, capsfilter и tee!";
            return;
        }

        qDebug() << "Элементы GStreamer созданы успешно.";

        // Настраиваем трансляцию на выбранные устройства
        for (auto *item : selectedDevices) {
            QString deviceIP = item->text();
            qDebug() << "Настраиваем трансляцию на устройство:" << deviceIP;

            GstElement *queue = gst_element_factory_make("queue", nullptr);
            GstElement *rtpPay = gst_element_factory_make("rtpL16pay", nullptr);
            GstElement *udpSink = gst_element_factory_make("udpsink", nullptr);

            if (!queue || !rtpPay || !udpSink) {
                qDebug() << "Ошибка: Не удалось создать элементы для RTP трансляции!";
                return;
            }

            // Добавляем элементы в pipeline и связываем tee с queue, rtpL16pay и udpSink
            gst_bin_add_many(GST_BIN(pipeline), queue, rtpPay, udpSink, NULL);

            // Связываем tee -> queue -> rtpL16pay -> udpsink
            if (!gst_element_link_many(tee, queue, rtpPay, udpSink, NULL)) {
                qDebug() << "Ошибка: Не удалось связать tee, queue, rtpL16pay и udpsink!";
                return;
            }

            // Устанавливаем параметры для RTP-трансляции (host, port, pt)
            g_object_set(udpSink,"host", deviceIP.toStdString().c_str(),"port", 5004,"pt", 10,NULL);
        }

        // Запускаем pipeline
        qDebug() << "Запуск pipeline для микрофона...";
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        // Проверка состояния pipeline
        GstState state;
        GstState pending;
        GstStateChangeReturn ret = gst_element_get_state(pipeline, &state, &pending, 5 * GST_SECOND);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            qDebug() << "Ошибка при запуске трансляции.";
        } else if (ret == GST_STATE_CHANGE_ASYNC) {
            qDebug() << "Трансляция запускается асинхронно.";
            qDebug() << "Текущее состояние:" << gst_element_state_get_name(state);
            qDebug() << "Ожидаемое состояние:" << gst_element_state_get_name(pending);
        } else if (ret == GST_STATE_CHANGE_SUCCESS) {
            qDebug() << "Трансляция успешно начата!";
        }

    } else {
        qDebug() << "Не выбрано ни одного устройства.";
    }
}