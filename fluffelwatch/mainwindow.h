#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFontMetrics>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QSettings>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        /* Mouse events */
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        /* Paint event */
        void paintEvent(QPaintEvent *event) override;

    private:
        /* User interface definitions and setting files */
        Ui::MainWindow *ui;
        QSettings *settings = nullptr;

        void readSettings();

        /* Window movement on client */
        bool isMoving = false;
        QPoint movingStartPos;

        /* Data object */


        /* Painting brushes, pens, and fonts */
        QBrush backgroundBrush;
        QPen penSeparator;
        QFont fontTitle;
        QPen penTitle;
        QFont fontMainTimer;
        QPen penMainTimer;
        QSize mainTimerSize;
        QFont fontAdjustedTimer;
        QPen penAdjustedTimer;
        QSize adjustedTimerSize;

        int marginSize;
        int iconSize;

        /* Region functions */
        QRect regionTitle;
        QRect regionTimeList;
        QRect regionStatus;
        void calculateRegionSizes();
};

#endif // MAINWINDOW_H
