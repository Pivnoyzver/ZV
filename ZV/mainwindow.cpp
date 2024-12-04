#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent), pipeline(nullptr){

    // Инициализация GStreamer
    GError *error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &error)) {
        qDebug() << "Ошибка инициализации GStreamer:" << error->message;
        g_error_free(error);
    } else {
        qDebug() << "GStreamer инициализирован";
    }

    // Создаем иконки
    QIcon pauseIcon("icons/pause.png");
    QIcon skipdIcon("icons/skip.png");
    QIcon addFileIcon("icons/addfile.png");
    QIcon removeFileIcon("icons/removefile.png");
    QIcon microphoneIcon("icons/microphone.png");

    // Создаём кнопки и списоки
    startButton = new QPushButton("Начать трансляцию", this);
    stopButton = new QPushButton("Остановить трансляцию", this);
    skipButton = new QPushButton("", this);
    pauseButton = new QPushButton("", this);
    microphoneButton = new QPushButton("", this);

    reloadDevicesButton = new QPushButton("Перезагрузить устройства", this);
    deviceList = new QListWidget(this);

    addFileButton = new QPushButton("", this);
    removeFileButton = new QPushButton("", this);
    playlistWidget = new QListWidget(this);

    statlamp = new QLabel(this);

    //создаем лампочку
    statlamp->setStyleSheet("background-color: red; border-radius: 7px; width: 15px; height: 15px;");

    // Подключаем иконки к кнопкам
    skipButton->setIcon(skipdIcon);
    pauseButton->setIcon(pauseIcon);
    addFileButton->setIcon(addFileIcon);
    removeFileButton->setIcon(removeFileIcon);
    microphoneButton->setIcon(microphoneIcon);

    // Устанавливаем размер иконок
    addFileButton->setIconSize(QSize(19, 19));
    removeFileButton->setIconSize(QSize(19, 19));
    microphoneButton->setIconSize(QSize(19, 19));

    // Устанавливаем режим множественного выбора для списка
    deviceList->setSelectionMode(QAbstractItemView::MultiSelection);

    // Создаём вертикальный макет (layout)
    QGridLayout *gridLayout = new QGridLayout();

    // Добавляем кнопки и список в макет
    gridLayout->addWidget(statlamp, 0, 0, 1, 1);

    gridLayout->addWidget(startButton, 0, 1, 1, 3);
    gridLayout->addWidget(skipButton, 1, 2, 1, 2);
    gridLayout->addWidget(pauseButton, 1, 0, 1, 2);
    gridLayout->addWidget(microphoneButton, 2, 0, 1, 4);
    gridLayout->addWidget(stopButton, 3, 0, 1, 4);

    gridLayout->addWidget(reloadDevicesButton, 5, 0, 1, 4);
    gridLayout->addWidget(deviceList, 6, 0, 1, 4);

    gridLayout->addWidget(addFileButton, 0, 4, 1, 3);
    gridLayout->addWidget(removeFileButton, 0, 7);
    gridLayout->addWidget(playlistWidget, 1, 4, 6, 4);

    //ставим кастомный размер

    statlamp->setFixedSize(15, 15);

    addFileButton->setFixedHeight(addFileButton->sizeHint().height() * 3);
    removeFileButton->setFixedHeight(removeFileButton->sizeHint().height() * 3);

    startButton->setFixedHeight(addFileButton->sizeHint().height() * 3);
    skipButton->setFixedSize(125, 48);
    pauseButton->setFixedSize(125, 48);;
    microphoneButton->setFixedHeight(microphoneButton->sizeHint().height() * 3);
    stopButton->setFixedHeight(stopButton->sizeHint().height() * 3);

    // Создаём центральный виджет и устанавливаем для него макет
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(gridLayout);
    setCentralWidget(centralWidget);

    reload();

    // Подключаем сигналы кнопок к слотам
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startStreaming);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopStreaming);
    connect(addFileButton, &QPushButton::clicked, this, &MainWindow::AddFile);
    connect(reloadDevicesButton, &QPushButton::clicked, this, &MainWindow::reloadDevices);
    connect(microphoneButton, &QPushButton::clicked, this, &MainWindow::streamFromMicrophone);
    connect(removeFileButton, &QPushButton::clicked, this, &MainWindow::RemoveFile);
    connect(skipButton, &QPushButton::clicked, this, &MainWindow::skipFile);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseStreaming);

    // Сигналы для работы таймера и очередного воспроизведения
    connect(timer, &QTimer::timeout, this, &MainWindow::eoscheck);
    connect(this, &MainWindow::eos, this, &MainWindow::startStreaming);

}

MainWindow::~MainWindow()
{
}

QStringList playlist;  // Список для хранения путей к аудиофайлу

bool firststart = 1;
bool streammic = 0;

void MainWindow::AddFile()
{
    // Открываем диалог для выбора аудиофайла
    QString audioFilePath = QFileDialog::getOpenFileName(this, "Выберите аудиофайл", "", "Audio Files (*.mp3 *.wav)");

    if (!audioFilePath.isEmpty()) {
        playlist.append(audioFilePath);

        int lastSlash = audioFilePath.lastIndexOf('/');
        if(lastSlash == -1){
            lastSlash = audioFilePath.lastIndexOf('\\');
        }
        if(lastSlash == -1){
            playlistWidget->addItem(audioFilePath);
        } else {
            playlistWidget->addItem(audioFilePath.mid(lastSlash + 1));
        }

        qDebug() << "Выбранный аудиофайл:" << audioFilePath;
        QMessageBox::information(this, "Очередь", "Аудиофайл добавлен в очередь!");
    } else {
        qDebug() << "Файл не был выбран.";
    }
}

void MainWindow::RemoveFile()
{
    if (!playlist.isEmpty()) {
        // Создаём диалоговое окно
        QMessageBox confirmRemove;
        confirmRemove.setWindowTitle("Очередь");
        confirmRemove.setText("Вы действительно хотите удалить последний аудиофайл?");
        confirmRemove.setIcon(QMessageBox::Question);

        // Устанавливаем кнопки "Да" и "Нет" на русском языке
        QPushButton *yesButton = confirmRemove.addButton(tr("Да"), QMessageBox::YesRole);
        QPushButton *noButton = confirmRemove.addButton(tr("Нет"), QMessageBox::NoRole);

        // Показываем диалоговое окно
        confirmRemove.exec();

        if (confirmRemove.clickedButton() == yesButton) {
            playlist.removeLast();
            delete playlistWidget->takeItem(playlistWidget->count() - 1);
            qDebug() << "Последний аудиофайл удален";
            QMessageBox::information(this, "Очередь", "Последний аудиофайл удален!");
        }
    } else {
        qDebug() << "Очередь уже пуста";
        QMessageBox::information(this, "Очередь", "Очередь уже пуста!");
    }
}

void MainWindow::startStreaming()
{
    // Получаем выбранные устройства из списка
    QList<QListWidgetItem*> selectedDevices = deviceList->selectedItems();

    if (selectedDevices.isEmpty()) {
        qDebug() << "Не выбрано ни одного устройства";
        QMessageBox::information(this, "Трансляция", "Не выбрано ни одного устройства!");
        return;
    }

    if (playlist.isEmpty()) {
        qDebug() << "Очередь пуста";
        setlamp(0);
        QMessageBox::information(this, "Очередь", "Очередь пуста!");
        firststart = 1;
        return;
    }

    // Проверяем, что pipeline не был ранее создан
    if (pipeline) {
        qDebug() << "Трансляция уже запущена";
        QMessageBox::information(this, "Трансляция", "Трансляция уже запущена!");
        return;
    }

    QString filepath = playlist[0];

    qDebug() << "\nСоздаём GStreamer pipeline...";

    pipeline = gst_pipeline_new("audio-pipeline");
    GstElement *source = gst_element_factory_make("filesrc", "source");
    GstElement *decoder = gst_element_factory_make("decodebin", "decoder");
    GstElement *convert = gst_element_factory_make("audioconvert", "convert");
    GstElement *tee = gst_element_factory_make("tee", "tee");

    if (!pipeline || !source || !decoder || !convert || !tee) {
        qDebug() << "Ошибка: Не удалось создать один или несколько элементов GStreamer";
        QMessageBox::warning(this, "Ошибка", "Не удалось создать один или несколько элементов GStreamer!");
        return;
    }

    // Устанавливаем путь к выбранному аудиофайлу
    g_object_set(source, "location", filepath.toStdString().c_str(), NULL);
    qDebug() << "Путь к аудиофайлу установлен:" << filepath;

    // Добавляем элементы в pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, convert, tee, NULL);

    // Связываем source с decoder
    if (gst_element_link(source, decoder) != TRUE) {
        qDebug() << "Ошибка: Не удалось связать source и decoder";
        QMessageBox::warning(this, "Ошибка", "Не удалось связать source и decoder!");
        return;
    }

    // Когда decoder готов, связываем его с audioconvert
    g_signal_connect(decoder, "pad-added", G_CALLBACK(+[](GstElement *, GstPad *pad, GstElement *convert) {
        GstPad *sinkpad = gst_element_get_static_pad(convert, "sink");
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            qDebug() << "Ошибка: Не удалось связать decoder и audioconvert";
        }
        gst_object_unref(sinkpad);
    }), convert);

    // Связываем convert и tee
    if (gst_element_link_many(convert, tee, NULL) != TRUE) {
        qDebug() << "Ошибка: Не удалось связать convert и tee";
        QMessageBox::warning(this, "Ошибка", "Не удалось связать convert и tee!");
        return;
    }

    qDebug() << "Элементы GStreamer созданы успешно";

    // Настраиваем трансляцию на выбранные устройства
    for (auto *item : selectedDevices) {
        QString deviceIP = item->text();
        qDebug() << "Настраиваем трансляцию на устройство:" << deviceIP;

        GstElement *queue = gst_element_factory_make("queue", nullptr);
        GstElement *rtpPay = gst_element_factory_make("rtpL16pay", nullptr);
        GstElement *udpSink = gst_element_factory_make("udpsink", nullptr);

        if (!queue || !rtpPay || !udpSink) {
            qDebug() << "Ошибка: Не удалось создать элементы для RTP трансляции";
            QMessageBox::warning(this, "Ошибка", "Не удалось создать элементы для RTP трансляции!");
            return;
        }

        // Добавляем элементы в pipeline и связываем tee с queue, rtpL16pay и udpSink
        gst_bin_add_many(GST_BIN(pipeline), queue, rtpPay, udpSink, NULL);

        // Связываем tee -> queue -> rtpL16pay -> udpsink
        if (!gst_element_link_many(tee, queue, rtpPay, udpSink, NULL)) {
            qDebug() << "Ошибка: Не удалось связать tee, queue, rtpL16pay и udpsink";
            QMessageBox::warning(this, "Ошибка", "Не удалось связать tee, queue, rtpL16pay и udpsink!");
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
        qDebug() << "Ошибка при запуске трансляции";
        QMessageBox::warning(this, "Ошибка", "Ошибка при запуске трансляции!");
        return;
    } else if (ret == GST_STATE_CHANGE_ASYNC) {
        qDebug() << "Трансляция запускается асинхронно";
        const char* res1 = gst_element_state_get_name(state);
        const char* res2 = gst_element_state_get_name(pending);
        qDebug() << "Текущее состояние:" << res1;
        qDebug() << "Ожидаемое состояние:" << res2;
        QMessageBox::warning(this, "Ошибка", QString("Трансляция запускается асинхронно!\nТекущее состояние: %1\nОжидаемое состояние: %2").arg(QString(res1),QString(res2)));
        return;
    } else if (ret == GST_STATE_CHANGE_SUCCESS) {
        qDebug() << "Трансляция начата для файла:" << filepath;

        if(firststart){
            setlamp(1);
            QMessageBox::information(this, "Трансляция", "Трансляция успешно начата!");
            firststart = 0;
        }
    }

    timer->start(100);
}

void MainWindow::stopStreaming()
{
    if (pipeline) {

        // Создаём диалоговое окно
        QMessageBox confirmStop;
        confirmStop.setWindowTitle("Подтверждение");
        confirmStop.setText("Вы действительно хотите остановить трансляцию?");
        confirmStop.setIcon(QMessageBox::Question);

        // Устанавливаем кнопки "Да" и "Нет" на русском языке
        QPushButton *yesButton = confirmStop.addButton(tr("Да"), QMessageBox::YesRole);
        QPushButton *noButton = confirmStop.addButton(tr("Нет"), QMessageBox::NoRole);

        // Показываем диалоговое окно
        confirmStop.exec();

        if (confirmStop.clickedButton() == yesButton) {
            timer->stop();
            gst_element_set_state(pipeline, GST_STATE_NULL);  // Останавливаем pipeline
            gst_object_unref(pipeline);  // Освобождаем ресурсы
            pipeline = nullptr;  // Сбрасываем pipeline

            setlamp(0);
            firststart = 1;
            streammic = 0;

            qDebug() << "Трансляция остановлена";
            QMessageBox::information(this, "Трансляция", "Трансляция остановлена!");
        }
    }

    else {
        qDebug() << "Трансляция не была запущена";
        QMessageBox::information(this, "Трансляция", "Трансляция не была запущена!");
    }
}

void MainWindow::pauseStreaming()
{
    if (streammic) {

        QMessageBox::information(this, "Трансляция", "Невозможно применить к трансляции с микрофона!");
        return;
    }

    if (!pipeline) {
            qDebug() << "Трансляция не была запущена";
            QMessageBox::information(this, "Трансляция", "Трансляция не была запущена!");
            return;
        }

    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PAUSED) {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            qDebug() << "Воспроизведение возобновлено.";
        } else {
            gst_element_set_state(pipeline, GST_STATE_PAUSED);
            qDebug() << "Трансляция приостановлена.";
        }
}

void MainWindow::skipFile()
{
    if (streammic) {

        QMessageBox::information(this, "Трансляция", "Невозможно применить к трансляции с микрофона!");
        return;
    }

    if (pipeline) {

        timer->stop();

        gst_element_set_state(pipeline, GST_STATE_NULL);  // Останавливаем pipeline
        gst_object_unref(pipeline);  // Освобождаем ресурсы
        pipeline = nullptr;  // Сбрасываем pipeline

        playlist.removeFirst();
        delete playlistWidget->takeItem(0);

        qDebug() << "Воспроизведение аудиофайла завершено";

        emit eos();
    }

    else {
        qDebug() << "Трансляция не была запущена";
        QMessageBox::information(this, "Трансляция", "Трансляция не была запущена!");
    }
}

void MainWindow::reloadDevices()
{
    reload();

    qDebug() << "Список устройств обновлён";
    QMessageBox::information(this, "Файл", "Список устройств обновлён!");
}

void MainWindow::streamFromMicrophone()
{
    // Получаем выбранные устройства из списка
    QList<QListWidgetItem*> selectedDevices = deviceList->selectedItems();

    if (!selectedDevices.isEmpty()) {

        // Останавливаем предидущюю трансляцию
        if (pipeline) {
            timer->stop();
            setlamp(0);
            gst_element_set_state(pipeline, GST_STATE_NULL);  // Останавливаем pipeline
            gst_object_unref(pipeline);  // Освобождаем ресурсы
            pipeline = nullptr;  // Сбрасываем pipeline
            qDebug() << "Трансляция остановлена";
        }

        qDebug() << "Создаём GStreamer pipeline для микрофона...";

        pipeline = gst_pipeline_new("mic-pipeline");
        GstElement *micSource = gst_element_factory_make("autoaudiosrc", "mic-source");
        GstElement *convert = gst_element_factory_make("audioconvert", "convert");
        GstElement *resample = gst_element_factory_make("audioresample", "resample");
        GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter"); // Фильтр для явной установки параметров
        GstElement *tee = gst_element_factory_make("tee", "tee");

        if (!pipeline || !micSource || !convert || !resample || !capsfilter || !tee) {
            qDebug() << "Ошибка: Не удалось создать один или несколько элементов GStreamer";
            QMessageBox::warning(this, "Ошибка", "Не удалось создать один или несколько элементов GStreamer!");
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
            qDebug() << "Ошибка: Не удалось связать micSource, convert, resample, capsfilter и tee";
            QMessageBox::warning(this, "Ошибка", "Не удалось связать micSource, convert, resample, capsfilter и tee!");
            return;
        }

        qDebug() << "Элементы GStreamer созданы успешно";

        // Настраиваем трансляцию на выбранные устройства
        for (auto *item : selectedDevices) {
            QString deviceIP = item->text();
            qDebug() << "Настраиваем трансляцию на устройство:" << deviceIP;

            GstElement *queue = gst_element_factory_make("queue", nullptr);
            GstElement *rtpPay = gst_element_factory_make("rtpL16pay", nullptr);
            GstElement *udpSink = gst_element_factory_make("udpsink", nullptr);

            if (!queue || !rtpPay || !udpSink) {
                qDebug() << "Ошибка: Не удалось создать элементы для RTP трансляции";
                QMessageBox::warning(this, "Ошибка", "Не удалось создать элементы для RTP трансляции!");
                return;
            }

            // Добавляем элементы в pipeline и связываем tee с queue, rtpL16pay и udpSink
            gst_bin_add_many(GST_BIN(pipeline), queue, rtpPay, udpSink, NULL);

            // Связываем tee -> queue -> rtpL16pay -> udpsink
            if (!gst_element_link_many(tee, queue, rtpPay, udpSink, NULL)) {
                qDebug() << "Ошибка: Не удалось связать tee, queue, rtpL16pay и udpsink";
                QMessageBox::warning(this, "Ошибка", "Не удалось связать tee, queue, rtpL16pay и udpsink!");
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
            qDebug() << "Ошибка при запуске трансляции";
            QMessageBox::warning(this, "Ошибка", "Ошибка при запуске трансляции!");
        } else if (ret == GST_STATE_CHANGE_ASYNC) {
            qDebug() << "Трансляция запускается асинхронно";
            const char* res1 = gst_element_state_get_name(state);
            const char* res2 = gst_element_state_get_name(pending);
            qDebug() << "Текущее состояние:" << res1;
            qDebug() << "Ожидаемое состояние:" << res2;
            QMessageBox::warning(this, "Ошибка", QString("Трансляция запускается асинхронно!\nТекущее состояние: %1\nОжидаемое состояние: %2").arg(QString(res1),QString(res2)));
        } else if (ret == GST_STATE_CHANGE_SUCCESS) {
            qDebug() << "Трансляция успешно начата";

            setlamp(1);
            streammic = 1;

            QMessageBox::information(this, "Трансляция с микрофона", "Трансляция с микрофона успешно начата!\nВсе предыдущие трансляции были остановлены");
        }

    } else {
        qDebug() << "Не выбрано ни одного устройства";
        QMessageBox::information(this, "Трансляция", "Не выбрано ни одного устройства!");
    }
}

void MainWindow::reload()
{
    // Очищаем текущий список устройств
    deviceList->clear();

    // Открываем JSON-файл с устройствами
    QFile file("devices.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть файл devices.json";
        QMessageBox::warning(this, "Файл", "Не удалось открыть файл devices.json!");
        return;
    }

    // Читаем содержимое файла
    QByteArray fileData = file.readAll();
    file.close();

    // Парсим JSON
    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isObject()) {
        qDebug() << "Неверный формат JSON";
        QMessageBox::warning(this, "Файл", "Неверный формат JSON!");
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

}

void MainWindow::setlamp(bool f)
{
    if (f) {
        statlamp->setStyleSheet("background-color: green; border-radius: 7px; width: 15px; height: 15px;");
    } else {
        statlamp->setStyleSheet("background-color: red; border-radius: 7px; width: 15px; height: 15px;");
    }
}


void MainWindow::eoscheck()
{
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_pop(bus);

    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS){

        timer->stop();

        gst_element_set_state(pipeline, GST_STATE_NULL);  // Останавливаем pipeline
        gst_object_unref(pipeline);  // Освобождаем ресурсы
        pipeline = nullptr;  // Сбрасываем pipeline

        playlist.removeFirst();
        delete playlistWidget->takeItem(0);

        qDebug() << "Воспроизведение аудиофайла завершено";

        emit eos();
    }
}
