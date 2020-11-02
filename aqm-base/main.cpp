#include "mainwindow.h"

#include <QApplication>
#define _CRT_SECURE_NO_WARNINGS

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    if (w.init_comms() < 0) return -1;

    w.setFixedSize(400, 300);
    w.output = new QLabel(&w);
    w.output->setFixedSize(400, 300);
    QFont font = w.output->font();
    font.setPointSize(16);
    font.setBold(true);
    w.output->setFont(font);
    w.output->show();

    w.timer->start();

    w.show();
    return a.exec();
}
