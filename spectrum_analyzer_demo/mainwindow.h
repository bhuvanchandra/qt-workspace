#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "qcustomplot.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setupRealtimeData(QCustomPlot *customPlot);
    void setupPlayground(QCustomPlot *customPlot);

private slots:
    void realtimeADCDataSlot();
    void realtimeFFTDataSlot();

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    QTimer dataTimer;
};

#endif // MAINWINDOW_H
