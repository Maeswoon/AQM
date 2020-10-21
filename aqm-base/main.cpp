#include "mainwindow.h"

#include <QApplication>
#define _CRT_SECURE_NO_WARNINGS

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    w.init_comms();
    w.setFixedSize(300, 200);

    w.output = new QLabel(&w);
    w.output->setFixedSize(300, 200);
    QFont font = w.output->font();
    font.setPointSize(16);
    font.setBold(true);
    w.output->setFont(font);
    w.output->setText("Loading...");
    w.output->show();

    w.timer->start();

    w.show();
    return a.exec();
}
