#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#define _USE_MATH_DEFINES

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <windows.h>
#include <math.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

long double gc_dist(long double a_lat, long double b_lat,
                    long double a_lon, long double b_lon);

struct bearing {
    double yaw;
    double pitch;
    double roll;

    bearing operator-(bearing& a) {
        return bearing{ yaw - a.yaw, pitch - a.pitch, roll - a.roll };
    }

    bearing operator+(bearing& a) {
        return bearing{ yaw + a.yaw, pitch + a.pitch, roll + a.roll };
    }

    bearing operator*(double& a) {
        return bearing{ yaw * a, pitch * a, roll * a };
    }

    bearing operator*(int& a) {
        return bearing{ yaw * a, pitch * a, roll * a };
    }
};

struct coord {
    long double lat;
    long double lon;

    long double operator-(coord& a) {
        return gc_dist(lat, a.lat, lon, a.lon);
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();
        int init_comms();
        QLabel *output;
        QTimer *timer;

    private slots:
        void comm_loop();

    private:
        HANDLE s_port;
        Ui::MainWindow *ui;
        int q;
        int s_recv();
        int s_send(char buf[]);
        void proc_telem();
        char s_buf[256];
        double payload[7];
        bearing b{0.0, 0.0, 0.0};
        int last_sent;
        int last_recv;
        char *qc;
        bool send_flag;

};
#endif // MAINWINDOW_H
