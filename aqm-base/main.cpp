#include "mainwindow.h"

#include <QApplication>

static std::string q;
static char *qc;
static const char nl = '\n';
static const char gt = '>';

int main(int argc, char *argv[])
{
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

    std::function<void()> f = [&w]() {
        if (w.last_recv == w.last_sent) {
            q = std::to_string(rand() % 255);
            strcpy(qc, "<1,0,");
            strcat(qc, const_cast<char *>(q.c_str()));
            strcat(qc, &gt);
            strcat(qc, &nl);
            w.last_sent = std::stod(q);
            w.s_send(qc);
        }

        while(w.s_recv() == 0) w.proc_telem();

        return;
    };

    QTimer *timer = new QTimer(&w);
    QTimer::connect(timer, &QTimer::timeout, f);
    timer->start(500);

    w.show();
    return a.exec();
}
