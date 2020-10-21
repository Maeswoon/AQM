#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    last_sent = 0;
    last_recv = 0;
    timer = new QTimer(this);
    timer->setInterval(200);
    this->connect(timer, SIGNAL(timeout()), this, SLOT(comm_loop()));
    qc = (char *) malloc(16);
    send_flag = true;
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::comm_loop() {
    if (send_flag) {
        q = rand() % 255;
        sprintf_s(qc, 16, "<1,0,%d>\n", q);
        last_sent = q;
        s_send(qc);
        send_flag = false;
    }

    if (s_recv() > 0) proc_telem();
}


int MainWindow::init_comms() {
    s_port = CreateFile("\\\\.\\COM9", GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (s_port == INVALID_HANDLE_VALUE) return -1;

    DCB dcb_params = { 0 };
    dcb_params.DCBlength = sizeof(dcb_params);
    dcb_params.BaudRate = CBR_9600;
    dcb_params.ByteSize = 8;
    dcb_params.StopBits = ONESTOPBIT;
    dcb_params.Parity = NOPARITY;

    SetCommState(s_port, &dcb_params);
    SetCommMask(s_port, EV_RXCHAR | EV_ERR);

    return 0;
}

int MainWindow::s_send(char buf[]) {
    DWORD bytes_written = 0;

    if (!WriteFile(s_port, buf, strlen(buf), &bytes_written, NULL)) return -1;

    return bytes_written;
}

int MainWindow::s_recv() {
    DWORD bytes_read;
    memset(&(s_buf), 0, sizeof(s_buf));
    memset(&(payload), 0, sizeof(payload));
    char p_cont[16], temp_char;
    int i = 0, j = 0, p = 0;

    do {
        ReadFile(s_port, &temp_char, sizeof(temp_char), &bytes_read, NULL);

        switch (temp_char) {
            case '\n':
                send_flag = true;
                return 1;
            case '>':
            case ',':
                payload[p++] = std::stod(p_cont);
                memset(&p_cont, 0, sizeof(p_cont));
                j = 0;
                break;
            case '<':
                break;
            default:
                p_cont[j++] = temp_char;
                s_buf[i++] = temp_char;
                break;
        }

    } while (bytes_read > 0 && temp_char != '\n');

    return 1;
}

void MainWindow::proc_telem() {
    b.roll = payload[4];
    b.pitch = payload[3];
    last_recv = (int) payload[2];
    sprintf(s_buf, "Temp: %d\nHumidity: %d\nRoll: %d\nPitch: %d", (int) payload[5],
                (int) payload[6], (int) payload[4], (int) payload[3]);
    output->setText(s_buf);
}

long double gc_dist(long double a_lat, long double b_lat,
                    long double a_lon, long double b_lon) {
    long double lat_a = a_lat * (M_PI / 180.0);
    long double lon_a = a_lon * (M_PI / 180.0);
    long double lat_b = b_lat * (M_PI / 180.0);
    long double lon_b = b_lon * (M_PI / 180.0);
    long double dist, d_lat, d_lon;

    d_lat = lat_b - lat_a;
    d_lon = lon_b - lon_a;
    dist = pow(sin(d_lat / 2), 2) +
              cos(lat_a) * cos(lat_b) *
              pow(sin(d_lon / 2), 2);
    dist = 2 * asin(sqrt(dist));

    return dist;
}
