#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define ROLL_INC 0.5
#define PITCH_INC 0.5
#define YAW_INC 0.5
#define INC_INT 100
#define REFRESH 50
#define S_TIMEOUT 200

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    qc = (char *) malloc(32);
    send_flag = true;

    timer = new QTimer(this);
    timer->setInterval(REFRESH);
    this->connect(timer, SIGNAL(timeout()), this, SLOT(comm_loop()));

    reconnect_timeout = new QTimer(this);
    reconnect_timeout->setInterval(S_TIMEOUT);
    //this->connect(reconnect_timeout, SIGNAL(timeout()), this, SLOT(reconnect()));

    pitch_inc = new QTimer(this);
    pitch_inc->setInterval(INC_INT);
    pitch_dec = new QTimer(this);
    pitch_dec->setInterval(INC_INT);
    this->connect(pitch_inc, SIGNAL(timeout()), this, SLOT(inc_pitch()));
    this->connect(pitch_dec, SIGNAL(timeout()), this, SLOT(dec_pitch()));

    roll_inc = new QTimer(this);
    roll_inc->setInterval(INC_INT);
    roll_dec = new QTimer(this);
    roll_dec->setInterval(INC_INT);
    this->connect(roll_inc, SIGNAL(timeout()), this, SLOT(inc_roll()));
    this->connect(roll_dec, SIGNAL(timeout()), this, SLOT(dec_roll()));

    yaw_inc = new QTimer(this);
    yaw_inc->setInterval(INC_INT);
    yaw_dec = new QTimer(this);
    yaw_dec->setInterval(INC_INT);
    this->connect(yaw_inc, SIGNAL(timeout()), this, SLOT(inc_yaw()));
    this->connect(yaw_dec, SIGNAL(timeout()), this, SLOT(dec_yaw()));

    last_send = GetTickCount();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_W:
            inc_pitch();
            pitch_inc->start();
            break;
        case Qt::Key_S:
            dec_pitch();
            pitch_dec->start();
            break;
        case Qt::Key_A:
            dec_roll();
            roll_dec->start();
            break;
        case Qt::Key_D:
            inc_roll();
            roll_inc->start();
        case Qt::Key_Q:
            dec_yaw();
            yaw_dec->start();
            break;
        case Qt::Key_E:
            inc_yaw();
            yaw_inc->start();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_W:
            pitch_inc->stop();
            break;
        case Qt::Key_S:
            pitch_dec->stop();
            break;
        case Qt::Key_A:
            roll_dec->stop();
            break;
        case Qt::Key_D:
            roll_inc->stop();
        case Qt::Key_Q:
            yaw_dec->stop();
            break;
        case Qt::Key_E:
            yaw_inc->stop();
    }
}

void MainWindow::inc_pitch() { exp.pitch += PITCH_INC; }

void MainWindow::dec_pitch() { exp.pitch -= PITCH_INC; }

void MainWindow::inc_roll() { exp.roll += ROLL_INC; }

void MainWindow::dec_roll() { exp.roll -= ROLL_INC; }

void MainWindow::inc_yaw() { exp.yaw += YAW_INC; }

void MainWindow::dec_yaw() { exp.yaw -= YAW_INC; }

void MainWindow::comm_loop() {
    if (GetTickCount() - last_send > S_TIMEOUT) {
        sprintf(s_buf, "TIMEOUT");
        output->setText(s_buf);
    }
    if (send_flag == true || GetTickCount() - last_send > S_TIMEOUT) {
        sprintf_s(qc, 32, "<1,0,%d,%d>\n", (int) exp.roll, (int) exp.pitch);
        s_send(qc);
        send_flag = false;
        last_send = GetTickCount();
    }
    if (s_recv() > 0) proc_telem();
}

void MainWindow::reconnect() {
    sprintf_s(qc, 32, "<1,0>\n");
    s_send(qc);
}

int MainWindow::init_comms() {
    s_port = CreateFile("\\\\.\\COM9", GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (s_port == INVALID_HANDLE_VALUE) return -1;

    DCB dcb_params;
    SecureZeroMemory(&dcb_params, sizeof(DCB));
    dcb_params.DCBlength = sizeof(DCB);
    dcb_params.BaudRate = CBR_56000;
    dcb_params.ByteSize = 8;
    dcb_params.StopBits = ONESTOPBIT;
    dcb_params.Parity = NOPARITY;

    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = S_TIMEOUT;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(s_port, &timeouts)) return -1;

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
        ReadFile(s_port, &temp_char, 1, &bytes_read, NULL);

        if (bytes_read < 1) return 0;
        switch (temp_char) {
            case '\n':
                send_flag = true;
                return 1;
            case '>':
            case ',':
                payload[p++] = std::stod(p_cont);
                memset(&p_cont, 0, sizeof(p_cont));
                s_buf[i++] = temp_char;
                j = 0;
                break;
            case '<':
                s_buf[i++] = temp_char;
                break;
            default:
                p_cont[j++] = temp_char;
                s_buf[i++] = temp_char;
                break;
        }

    } while (temp_char != '\n');

    return 0;
}

void MainWindow::proc_telem() {
    act.roll = payload[3];
    act.pitch = payload[2];
    sprintf(s_buf, "Temp: %d C\nHumidity: %d%%\nRoll: %.1fdeg\nPitch: %.1fdeg\nRoll (exp.): %.1fdeg\nPitch (exp.): %.1fdeg\nRTT: %lums",
            (int) payload[4], (int) payload[5], act.roll, act.pitch, exp.roll, exp.pitch, GetTickCount() - last_send);
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
