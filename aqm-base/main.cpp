#include "mainwindow.h"

#include <QApplication>
#define _CRT_SECURE_NO_WARNINGS

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    if (w.init_comms() < 0) {
        w.output->setText("COMMS INIT FAILED.");
    } else {
        w.timer->start();
    }


    w.show();
    return a.exec();
}
