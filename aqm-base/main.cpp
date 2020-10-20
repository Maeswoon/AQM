#include "mainwindow.h"

#include <QApplication>
#define _CRT_SECURE_NO_WARNINGS

int q;
char *qc = (char *) malloc(16);

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


    std::function<void()> f = [&w]() {
        if (w.last_recv == w.last_sent) {
            q = rand() % 255;
            sprintf_s(qc, 16, "<1,0,%d>\n", q);
            w.last_sent = q;
            w.s_send(qc);
        }

        if (w.s_recv() > 0) w.proc_telem();

        return;
    };

    QTimer *timer = new QTimer(&w);
    QTimer::connect(timer, &QTimer::timeout, f);
    timer->start(200);

    w.show();
    return a.exec();
}
