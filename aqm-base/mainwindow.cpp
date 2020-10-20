#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


int MainWindow::init_comms() {
    s_port = CreateFileA("\\\\.\\COM9", GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);
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

    if(!WriteFile(s_port, buf, strlen(buf), &bytes_written, NULL)) return -1;

    return bytes_written;
}

int MainWindow::s_recv() {
    DWORD bytes_read;
    memset(&(this->s_buf), 0, sizeof(this->s_buf));
    char p_cont[16], temp_char;
    int i = 0, j = 0, p = 0;

    do {
        ReadFile(s_port, &temp_char, sizeof(temp_char), &bytes_read, NULL);

        switch (temp_char) {
            case '\n':
                return 1;
            case '>':
            case ',':
                this->payload[p++] = std::stod(p_cont);
                memset(&p_cont, 0, sizeof(p_cont));
                j = 0;
                break;
            case '<':
                break;
            default:
                p_cont[j++] = temp_char;
                ++i;
                break;
        }

        ++i;
    } while (bytes_read > 0 && i < 255);

    return 0;
}

void MainWindow::proc_telem() {
    this->b.roll = this->payload[4];
    this->b.pitch = this->payload[3];
    this->last_recv = this->payload[2];
    sprintf_s(this->s_buf, "Temp: %d\nHumidity: %d\nRoll: %d\nPitch: %d", (int) this->payload[5],
                (int) this->payload[6], (int) this->payload[4], (int) this->payload[3]);
    this->output->setText(this->s_buf);
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
