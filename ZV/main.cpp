#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow wind;

    app.setWindowIcon(QIcon("icons/program.png"));


    wind.show();
    return app.exec();
}
