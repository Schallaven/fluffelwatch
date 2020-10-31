#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

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
        /* User interface definitions */
        Ui::MainWindow *ui;

        /* Window movement on client */
        bool isMoving = false;
        QPoint movingStartPos;

        /* Painting brushes, pens, and fonts */
        QBrush backgroundBrush;
        QFont fontTitle;
        QPen penTitle;
};

#endif // MAINWINDOW_H
