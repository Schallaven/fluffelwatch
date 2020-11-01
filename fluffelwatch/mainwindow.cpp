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

    /* Prepare icons */
    iconStates[iconFluffel].addFile(":/res/fluffelicon.png", QSize(), QIcon::Normal, QIcon::On);
    iconStates[iconFluffel].addFile(":/res/fluffelicon_disabled.png", QSize(), QIcon::Disabled, QIcon::On);
    iconStates[iconLoading].addFile(":/res/loadingicon.png", QSize(), QIcon::Normal, QIcon::On);
    iconStates[iconLoading].addFile(":/res/loadingicon_disabled.png", QSize(), QIcon::Disabled, QIcon::On);
    iconStates[iconSavegame].addFile(":/res/savegameicon.png", QSize(), QIcon::Normal, QIcon::On);
    iconStates[iconSavegame].addFile(":/res/savegameicon_disabled.png", QSize(), QIcon::Disabled, QIcon::On);
    iconStates[iconCinema].addFile(":/res/cinemaicon.png", QSize(), QIcon::Normal, QIcon::On);
    iconStates[iconCinema].addFile(":/res/cinemaicon_disabled.png", QSize(), QIcon::Disabled, QIcon::On);
    iconStates[iconDead].addFile(":/res/deadicon.png", QSize(), QIcon::Normal, QIcon::On);
    iconStates[iconDead].addFile(":/res/deadicon_disabled.png", QSize(), QIcon::Disabled, QIcon::On);

    /* Load split data */
    data.loadData(settings->value("segmentdata").toString());
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Got %d segments from data object", segments);

    /* Calculate the region and window size */
    calculateRegionSizes();

    /* Start timer every 100 msec (can do more, but it costs CPU time!) */
    startTimer(100, Qt::PreciseTimer);
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
    painter.drawText(regionTitle, Qt::AlignHCenter | Qt::AlignVCenter, data.getTitle());

    painter.setPen(penSeparator);
    painter.drawLine(regionTitle.bottomLeft(), regionTitle.bottomRight());

    /* Time list */
    for (int i = 0; i < displaySegments.size(); ++i) {
        paintSegmentLine(painter, QRect(marginSize,
                                        regionTimeList.top() + marginSize + i * segmentSize.height(),
                                        segmentSize.width(),
                                        segmentSize.height()), displaySegments[i]);
    }

    painter.setPen(penSeparator);
    painter.drawLine(regionTimeList.bottomLeft(), regionTimeList.bottomRight());

    /* Status bar: 5 icons + two timers */
    for(int i = 0; i < iconCOUNT; ++i) {
        iconStates[i].paint(&painter, QRect(i * iconSize + marginSize, regionStatus.bottom() - iconSize - marginSize, iconSize, iconSize),
                            Qt::AlignCenter, gameStates[i] ? QIcon::Normal : QIcon::Disabled);
    }

    painter.setFont(fontMainTimer);
    painter.setPen(penMainTimer);
    painter.drawText(QRect(regionStatus.right() - mainTimerSize.width() - marginSize,
                           regionStatus.top() + marginSize,
                           mainTimerSize.width(),
                           mainTimerSize.height()), Qt::AlignRight | Qt::AlignVCenter, timerReal.toString());

    painter.setFont(fontAdjustedTimer);
    painter.setPen(penAdjustedTimer);
    painter.drawText(QRect(regionStatus.right() - adjustedTimerSize.width() - marginSize,
                           regionStatus.bottom() - adjustedTimerSize.height() - marginSize,
                           adjustedTimerSize.width(),
                           adjustedTimerSize.height()), Qt::AlignRight | Qt::AlignVCenter, timerAdjusted.toString());

}

void MainWindow::timerEvent(QTimerEvent* event) {
    Q_UNUSED(event)

    /* Update the display */
    update();
}

void MainWindow::onSplit() {
    qDebug("Start/Split");
    timerReal.start();
    timerAdjusted.start();
}

void MainWindow::onPause() {
    if (timerReal.isPaused()) {
        qDebug("resume");
        timerReal.resume();
        timerAdjusted.resume();
    } else {
        qDebug("pause");
        timerReal.pause();
        timerAdjusted.pause();
    }
}

void MainWindow::onReset() {
    qDebug("Reset");
    timerReal.invalidate();
    timerAdjusted.invalidate();
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

    /* Segment settings */
    segmentLines = qMax(2, settings->value("segmentLines").toInt());
    fontSegmentTitle.fromString(settings->value("segmentTitleFont", QFont("Arial", 18, QFont::Bold).toString()).toString());
    penSegmentTitle = QPen(QColor(settings->value("segmentTitleColor", "#c0c0c0").toString()));
    fontSegmentTime.fromString(settings->value("segmentTimeFont", QFont("Arial", 18, QFont::Bold).toString()).toString());
    penSegmentTime = QPen(QColor(settings->value("segmentTimeColor", "#ffffff").toString()));

    fontSegmentDifference.fromString(settings->value("segmentDifferenceFont", QFont("Arial", 14, QFont::Bold).toString()).toString());
    penSegmentCurrent = QPen(QColor(settings->value("segmentCurrentColor", "#33ff00").toString()));
    penSegmentGained = QPen(QColor(settings->value("segmentGainedColor", "#6295fc").toString()));
    penSegmentLost = QPen(QColor(settings->value("segmentLostColor", "#e82323").toString()));
    penSegmentNewRecord = QPen(QColor(settings->value("segmentNewRecordColor", "#ffff99").toString()));

    /* Timer/Status settings */
    fontMainTimer.fromString(settings->value("mainTimerFont", QFont("Arial", 28, QFont::Bold).toString()).toString());
    penMainTimer = QPen(QColor(settings->value("mainTimerColor", "#22cc22").toString()));
    fontAdjustedTimer.fromString(settings->value("adjustedTimerFont", QFont("Arial", 22, QFont::Bold).toString()).toString());
    penAdjustedTimer = QPen(QColor(settings->value("adjustedTimerColor", "#22cc22").toString()));
    iconSize = settings->value("iconSize", 40).toInt();
}

void MainWindow::paintSegmentLine(QPainter& painter, QRect rect, SplitData::segment& segment) {
    /* Segment title */
    painter.setFont(fontSegmentTitle);
    painter.setPen(penSegmentTitle);
    if (segment.current && timerReal.isValid()) {
        painter.setPen(penSegmentCurrent);
    }
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, segment.title);

    /* Segment diff time (if it exists) */
    if (segment.ran) {
        painter.setFont(fontSegmentDifference);
        if (segment.improtime < 0) {
            painter.setPen(penSegmentGained);
        } else if (segment.improtime > 0 ) {
            painter.setPen(penSegmentLost);
        }

        painter.drawText(QRect(rect.right() - segmentColumnSizes[2] - segmentColumnSizes[1] - marginSize * 2,
                               rect.top(),
                               segmentColumnSizes[1],
                               rect.height()), Qt::AlignRight | Qt::AlignVCenter,
                         FluffelTimer::getStringFromTimeDiff(segment.improtime));
    }

    /* Segment time (or improvement) */
    painter.setFont(fontSegmentTime);
    if (segment.ran && (segment.runtime < segment.besttime)) {
        painter.setPen(penSegmentNewRecord);
    } else if (!segment.ran && !segment.current) {
        painter.setPen(penSegmentTime);
    }
    QString time = FluffelTimer::getStringFromTime(segment.runtime) + " ";
    painter.drawText(QRect(rect.right() - segmentColumnSizes[2] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[2],
                           rect.height()), Qt::AlignRight | Qt::AlignVCenter,
                     FluffelTimer::getStringFromTime(segment.runtime) + " ");
}

void MainWindow::calculateRegionSizes() {
    /* Calculate title region */
    QFontMetrics fm(fontTitle);
    regionTitle = QRect(QPoint(0, 0), fm.size(Qt::TextSingleLine, data.getTitle()));
    regionTitle.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate time list region */
    QFontMetrics segTitle(fontSegmentTitle);
    QFontMetrics segDiff(fontSegmentDifference);
    QFontMetrics segTime(fontSegmentTime);
    QSize sizeTitle = segTitle.size(Qt::TextSingleLine, data.getLongestSegmentTitle());
    segmentColumnSizes[0] = sizeTitle.width();
    QSize sizeDiff = segDiff.size(Qt::TextSingleLine, " −00:00:00.0 ");
    segmentColumnSizes[1] = sizeDiff.width();
    QSize sizeTime = segTime.size(Qt::TextSingleLine, " 00:00:00.0 ");
    segmentColumnSizes[2] = sizeTime.width();
    segmentSize.setWidth(sizeTitle.width() + marginSize + sizeDiff.width() + marginSize + sizeTime.width());
    segmentSize.setHeight(qMax(qMax(sizeTitle.height(), sizeDiff.height()), sizeTime.height()));

    regionTimeList = QRect(regionTitle.bottomLeft(), QSize(segmentSize.width(), segmentSize.height() * segmentLines));
    regionTimeList.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate status bar (with the two timers) */
    QFontMetrics mtFm(fontMainTimer);
    mainTimerSize = mtFm.size(Qt::TextSingleLine, "00:00:00.0");

    QFontMetrics atFm(fontAdjustedTimer);
    adjustedTimerSize = atFm.size(Qt::TextSingleLine, "00:00:00.0");

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
    segmentSize.setWidth(maxWidth);
    regionStatus.setWidth(maxWidth);
}
