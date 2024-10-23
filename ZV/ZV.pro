QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += D:/gstreamer/1.0/msvc_x86_64/include/gstreamer-1.0 \
               D:/gstreamer/1.0/msvc_x86_64/include/glib-2.0 \
               D:/gstreamer/1.0/msvc_x86_64/lib/glib-2.0/include

LIBS += -LD:/gstreamer/1.0/msvc_x86_64/lib \
        -lgstreamer-1.0 \
        -lgobject-2.0 \
        -lglib-2.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    config.json \
    devices.json
