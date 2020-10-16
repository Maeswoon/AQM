#include "lora_aqm.h"
#include <QApplication>

static std::string q;
static char *qc;
static const char nl = '\n';
static const char gt = '>';
int i = 0;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    LoRa_AQM w;

    w.init_comms();
    w.setFixedSize(300, 200);

    w.output = new QLabel(&w);
    w.output->setFixedSize(300, 200);
    QFont font = w.output->font();
    font.setPointSize(16);
    font.setBold(true);
    w.output->setFont(font);

    std::function<void()> f = [&w]() {
        if (w.f_active == true) return;

        w.f_active = true;
        if (w.last_recv == w.last_sent || w.recv_flag == false) {
            q = std::to_string(rand() % 255);
            qc = "<";
            strcat(qc, const_cast<char *>(q.c_str()));
            strcat(qc, &gt);
            strcat(qc, &nl);
            w.last_sent = std::stod(q);
            w.s_send(qc);
        }

        w.recv_flag = false;
        w.s_recv();
        w.proc_telem();
        w.f_active = false;

        return;
    };

    QTimer *timer = new QTimer(&w);
    QTimer::connect(timer, &QTimer::timeout, f);
    timer->start(10);

    w.show();
    return a.exec();
}

