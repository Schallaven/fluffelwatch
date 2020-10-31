#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    /* Setup UI including action context menu */
    ui->setupUi(this);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);

    this->addAction(ui->action_Start_Split);
    this->addAction(ui->action_Pause);
    this->addAction(ui->action_Reset);
    this->addAction(separator);
    this->addAction(ui->action_Exit);

    /* Read in settings from an conf-file */
    settings = new QSettings("fluffelwatch.conf", QSettings::NativeFormat);
    readSettings();

    /* Borderless window with black background */
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    /* Calculate the region and window size */
    calculateRegionSizes();
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
    painter.setRenderHint(QPainter::Antialiasing);
    painter.testRenderHint(QPainter::TextAntialiasing);

    /* Fill background */
    painter.setBrush(backgroundBrush);
    painter.drawRect(this->rect());

    /* Title */
    painter.setFont(fontTitle);
    painter.setPen(penTitle);
    painter.drawText(regionTitle, Qt::AlignHCenter | Qt::AlignVCenter, settings->value("title").toString());

    painter.setPen(penSeparator);
    painter.drawLine(regionTitle.bottomLeft(), regionTitle.bottomRight());

    /* Time list */

    painter.setPen(penSeparator);
    painter.drawLine(regionTimeList.bottomLeft(), regionTimeList.bottomRight());

    /* Status bar: 5 icons + two timers */
    for(int i = 0; i < 5; ++i) {
        QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0);
        icon.paint(&painter, QRect(i * iconSize + marginSize, regionStatus.bottom() - iconSize - marginSize, iconSize, iconSize));
    }

    painter.setFont(fontMainTimer);
    painter.setPen(penMainTimer);
    painter.drawText(QRect(regionStatus.right() - mainTimerSize.width() - marginSize,
                           regionStatus.top() + marginSize,
                           mainTimerSize.width(),
                           mainTimerSize.height()), Qt::AlignRight | Qt::AlignVCenter, "00:00:00.00");

    painter.setFont(fontAdjustedTimer);
    painter.setPen(penAdjustedTimer);
    painter.drawText(QRect(regionStatus.right() - adjustedTimerSize.width() - marginSize,
                           regionStatus.bottom() - adjustedTimerSize.height() - marginSize,
                           adjustedTimerSize.width(),
                           adjustedTimerSize.height()), Qt::AlignRight | Qt::AlignVCenter, "00:00:00.00");

}

void MainWindow::onSplit() {
    qDebug("Start/Split");
}

void MainWindow::onPause() {
    qDebug("Pause");
}

void MainWindow::onReset() {
    qDebug("Reset");
}

void MainWindow::onExit() {
    this->close();
}

void MainWindow::readSettings() {
    /* Main window / general settings */
    backgroundBrush = QBrush(QColor(settings->value("backgroundColor", "#000000").toString()));
    penSeparator = QPen(QColor(settings->value("separatorColor", "#666666").toString()));
    marginSize = settings->value("marginSize", 0).toInt();

    /* Title settings */
    fontTitle.fromString(settings->value("titleFont", QFont("Arial", 22, QFont::Bold).toString()).toString());
    penTitle = QPen(QColor(settings->value("titleColor", "#f0b012").toString()));

    /* Timer/Status settings */
    fontMainTimer.fromString(settings->value("mainTimerFont", QFont("Arial", 28, QFont::Bold).toString()).toString());
    penMainTimer = QPen(QColor(settings->value("mainTimerColor", "#22cc22").toString()));
    fontAdjustedTimer.fromString(settings->value("adjustedTimerFont", QFont("Arial", 22, QFont::Bold).toString()).toString());
    penAdjustedTimer = QPen(QColor(settings->value("adjustedTimerColor", "#22cc22").toString()));
    iconSize = settings->value("iconSize", 40).toInt();
}

void MainWindow::calculateRegionSizes() {
    /* Calculate title region */
    QFontMetrics fm(fontTitle);
    regionTitle = QRect(QPoint(0, 0), fm.size(Qt::TextSingleLine, settings->value("title").toString()));
    regionTitle.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate time list region */
    regionTimeList = QRect(regionTitle.bottomLeft(), QSize(100, 200));
    regionTimeList.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate status bar (with the two timers) */
    QFontMetrics mtFm(fontMainTimer);
    mainTimerSize = mtFm.size(Qt::TextSingleLine, "00:00:00.00");

    QFontMetrics atFm(fontAdjustedTimer);
    adjustedTimerSize = atFm.size(Qt::TextSingleLine, "00:00:00.00");

    QSize iconArea = QSize(5 * iconSize, iconSize);

    regionStatus = QRect(regionTimeList.bottomLeft(),
                         QSize(iconArea.width() + qMax(mainTimerSize.width(), adjustedTimerSize.width()),
                               qMax(mainTimerSize.height() + adjustedTimerSize.height(), iconArea.height())));
    regionStatus.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate new window size: width is the max width of all regions and
     * height is simply the sum of all regions. */
    int maxWidth = qMax(qMax(regionTitle.width(), regionTimeList.width()), regionStatus.width());
    QSize windowSize;
    windowSize.setWidth(maxWidth);
    windowSize.setHeight(regionTitle.height() + regionTimeList.height() + regionStatus.height());
    resize(windowSize);

    /* Update all regions to have the max width */
    regionTitle.setWidth(maxWidth);
    regionTimeList.setWidth(maxWidth);
    regionStatus.setWidth(maxWidth);
}
