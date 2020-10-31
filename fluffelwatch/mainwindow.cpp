#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    /* Setup UI including action context menu */
    ui->setupUi(this);
    this->addAction(ui->action_Exit);

    /* Borderless window with black background */
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    /* Set up all the painting tools */
    backgroundBrush = QBrush(QColor(0, 0, 0));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    /* Start moving the window with the left mouse button; this avoids accidently moving
     * the window when accesing the context menu. */
    if (event->button() == Qt::LeftButton) {
        isMoving = true;
        movingStartPos = event->pos();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    /* Moving the window: Calculating the difference between the current position of the
     * mouse and the starting position. Then add this to the window position (top-left)
     * corner to move the window the same distance/direction. */
    if (isMoving) {
        QPoint diffPos = event->pos() - movingStartPos;
        window()->move(window()->pos() + diffPos);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event)

    /* Stop moving in any case */
    isMoving = false;
}

void MainWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);

    /* Fill background */
    painter.setBrush(backgroundBrush);
    painter.drawRect(this->rect());
}
