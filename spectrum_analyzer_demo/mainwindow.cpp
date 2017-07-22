#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <inttypes.h>
#include "iio_utils.h"
#include "generic_buffer.h"

#ifdef __cplusplus
}
#endif

#define FFT_SIZE        512
#define SAMPLING_FREQ   300000

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(0, 0, 800, 480);

    setupRealtimeData(ui->plot);
    statusBar()->clearMessage();
    ui->plot->replot();
}

void MainWindow::setupRealtimeData(QCustomPlot *plot)
{
    // Fully disable antialiasing for higher performance:
#if 1
    plot->setNotAntialiasedElements(QCP::aeAll);
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    plot->xAxis->setTickLabelFont(font);
    plot->yAxis->setTickLabelFont(font);
    plot->legend->setFont(font);
#endif

    plot->setBackground(Qt::black);
    plot->axisRect()->setBackground(Qt::black);

    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::blue));

    // give the axes some labels:
    plot->xAxis->setLabel("Hz");
    plot->yAxis->setLabel("dB");
    plot->graph(0)->rescaleAxes();

    plot->graph()->setLineStyle((QCPGraph::LineStyle)5);

    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    plot->axisRect()->setupFullAxesBox();
    plot->xAxis->setRange(0, FFT_SIZE - 1); /* FFT bin values */

    // make left and bottom axes tranfer their ranges to right and top axes:
    //connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    //connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));

    qDebug("IIO Buffer init\n");
    sa_gb.buf_len = FFT_SIZE;
    sa_gb.device_name = "rpmsg-openamp-demo-channel";
    sa_gb.trigger_name = "sysfstrig0";
    sa_gb.notrigger = false;
    sa_gb.noevents = false;
    sa_gb.timedelay = 1000000;
    sa_gb.done_init = false;

    int ret = iio_buf_init(&sa_gb);
    if (ret) {
        qDebug("IIO Buf init failed, Ret:%d\n", ret);
        exit(ret);
    }

    // setup a timer that repeatedly calls MainWindow::realtimeDummyDataSlot:
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeFFTDataSlot()));
}

void MainWindow::realtimeFFTDataSlot()
{
    QVector<double> freq(FFT_SIZE), fft(FFT_SIZE);
    int ret, toread, i, f_gap;
    ssize_t read_size;

    f_gap = SAMPLING_FREQ / FFT_SIZE;

    for (i = 0; i < 512; ++i) {
      //freq[i] = (SAMPLING_FREQ / FFT_SIZE) * i;
        freq[i] = i;
    }

    if (!sa_gb.noevents) {
        struct pollfd pfd = {
            .fd = sa_gb.fp,
            .events = POLLIN,
        };

        ret = poll(&pfd, 1, 1000);
        if (ret < 0)
            ret = -errno;
        toread = sa_gb.buf_len;
    } else {
        usleep(sa_gb.timedelay);
        toread = 64;
    }
    read_size = read(sa_gb.fp, sa_gb.data, toread * sa_gb.scan_size);
    if (read_size < 0) {
        if (errno == EAGAIN)
            qDebug("nothing available\n");
    }

    struct iio_channel_info *info = &sa_gb.channels[0];
    uint16_t input;

    for (i = 0; i < read_size / sa_gb.scan_size; i++) {
        input = *(uint16_t *)(sa_gb.data + sa_gb.scan_size * i + sa_gb.channels[0].location);
        /* First swap if incorrect endian */
        if (info->be)
            input = be16toh(input);
        else
            input = le16toh(input);

        /*
         * Shift before conversion to avoid sign extension
         * of left aligned data
         */
        input >>= info->shift;
        input &= info->mask;

        if (info->is_signed) {
            int16_t val = (int16_t)(input << (16 - info->bits_used)) >>
                      (16 - info->bits_used);
            fft[i] = ((float)val + info->offset) * info->scale;
        } else {
            //fft[i] = 10 * log10((input + info->offset) * info->scale);
            fft[i] = ((float)input + info->offset) * info->scale;
            //qDebug("%f %f\n", fft[i], freq[i]);
        }
    }

    ui->plot->xAxis->setRange(0, 511);
    ui->plot->yAxis->setRange(0, 5000);

    ui->plot->graph(0)->setData(freq, fft);
    ui->plot->replot();
}

void MainWindow::realtimeADCDataSlot()
{
    // generate some data:
    QVector<double> freq(100), fft(100); // initialize with entries 0..100
    for (int i=0; i<100; ++i)
    {
      freq[i] = i/50.0 - 1; // x goes from -1 to 1
      fft[i] = freq[i]*freq[i]; // let's plot a quadratic function
    }

    ui->plot->xAxis->setRange(-1, 1);
    ui->plot->yAxis->setRange(0, 1);

    ui->plot->graph(0)->setData(freq, fft);
    ui->plot->replot();
}

void MainWindow::setupPlayground(QCustomPlot *plot)
{
    Q_UNUSED(plot)
}

MainWindow::~MainWindow()
{
    iio_buf_free(&sa_gb);
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    dataTimer.start(0);
}

void MainWindow::on_pushButton_2_clicked()
{
    dataTimer.stop();
}
