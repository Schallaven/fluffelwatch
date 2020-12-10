#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    /* Setup UI with a border less window and an action context menu */
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    setupContextMenu();
    setupGlobalShortcuts();

    /* Read in settings from an conf-file */
    settings = new QSettings("fluffelwatch.conf", QSettings::NativeFormat);
    readSettings();

    /* Calculate the region and window size */
    calculateRegionSizes();

    /* Start timer every 10 msec (can do faster timers, but it costs CPU load!) */
    timerID = startTimer(10, Qt::PreciseTimer);

    /* Start the thread for managing IPC to allow external programs to
     * change section number and iconstates */
    ipcthread.start();
}

MainWindow::~MainWindow() {
    /* Request exit of the thread and give it some time to exit */
    ipcthread.requestInterruption();
    QThread::msleep(ipcthread.timeout * 2);

    /* Kill the timer we started */
    killTimer(timerID);

    /* Destroy everything */
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

    QMainWindow::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.testRenderHint(QPainter::TextAntialiasing);

    /* Fill background and draw all elements */
    painter.setBrush(backgroundBrush);
    painter.drawRect(this->rect());

    paintAllElements(painter);
}

void MainWindow::timerEvent(QTimerEvent* event) {
    Q_UNUSED(event)

    /* Process new information from thread if available */
    if (ipcthread.dataChanged()) {
        /* Get the newest data */
        FluffelIPCThread::listenerData tempData = ipcthread.getData();

        /* Send icon states to the icon manager */
        icons.setStates(tempData.iconstates);

        qDebug("New state: section = %d, states = 0x%08X, timercontrol = %d", tempData.section, tempData.iconstates, tempData.timercontrol);

        /* If the timers are NOT running yet, then only react to the autostart signal and only if the user wants that */
        if (autostartstop && !timeControl.areBothTimerValid() && tempData.timercontrol == FluffelIPCThread::timeControlStart) {
            qDebug("Got start signal, resetting and starting both timers.");
            timeControl.resetBothTimer();
            timeControl.restartBothTimer();
        }

        /* If the timers are running and the user wants it, react to the autostop signal */
        else if (autostartstop && timeControl.isAnyTimerRunning() && tempData.timercontrol == FluffelIPCThread::timeControlStop) {
            qDebug("Got stop signal. Stopping both timers and do a split.");
            timeControl.pauseBothTimer();

            displaySegments.clear();
            int remains = data.split(timeControl.elapsedPreferredTime());
            int segments = data.getCurrentSegments(displaySegments, segmentLines);
            qDebug("Got %d segments from data object. %d remaining segments.", segments, remains);
        }

        /* Autosplits enabled, so split if the section number changes */
        if (autosplit && (tempData.section > data.getCurrentSection())) {
            qDebug("Do an autosplit to section %d", tempData.section);

            displaySegments.clear();
            int remains = data.splitToSection(tempData.section, timeControl.elapsedPreferredTime());
            int segments = data.getCurrentSegments(displaySegments, segmentLines);
            qDebug("Got %d segments from data object. %d remaining segments.", segments, remains);
        }

        /* Pause the ingame timer whenever requested */
        if (timeControl.areBothTimerValid() && timeControl.isIngameTimerRunning()
                && tempData.timercontrol == FluffelIPCThread::timeControlPause) {
            qDebug("Pausing ingame timer.");
            timeControl.pauseIngameTimer();
        }

        /* Resume ingame timer whenever requested */
        else if (timeControl.areBothTimerValid() && !timeControl.isIngameTimerRunning()
                 && tempData.timercontrol == FluffelIPCThread::timeControlNone) {
            qDebug("Continuing ingame timer");
            timeControl.resumeIngameTimer();
        }
    }

    /* Update the display */
    update();
}

void MainWindow::onSplit() {
    /* If timers are not started yet, start them */
    if (!timeControl.areBothTimerValid()) {
        qDebug("Start");

        timeControl.startBothTimer();
        return;
    }

    /* Otherwise, split the time here */
    qDebug("Split");
    displaySegments.clear();
    int remains = data.split(timeControl.elapsedPreferredTime());
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Got %d segments from data object. %d remaining segments.", segments, remains);

    /* Stop the timer if that was the last split */
    if (remains == 0) {
        timeControl.pauseBothTimer();
    }
}

void MainWindow::onPause() {
    /* Doesn't do anything if there are no more splits to do */
    if (!data.canSplit()) {
        qDebug("Cannot do any more splits");
        return;
    }

    /* Check if paused or not */
    qDebug("Toggle timer");
    timeControl.toggleBothTimer();
}

void MainWindow::onReset() {
    bool merge = false;

    /* Pause timer so they do not continue running */
    timeControl.pauseBothTimer();

    /* Check if there have been splits and ask user about data */
    if (data.hasSplit()) {
        int ret = QMessageBox::warning(this, "Segment times changed", "Do you want to merge your times before reset?",
                                       QMessageBox::Save | QMessageBox::No, QMessageBox::Save);

        merge = (ret == QMessageBox::Save);
    }

    qDebug("Reset");
    timeControl.resetBothTimer();
    displaySegments.clear();

    /* Reset data */
    data.reset(merge);
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Got %d segments from data object", segments);
}

void MainWindow::onOpen() {
    qDebug("open");

    /* Pause timer so they do not continue running */
    timeControl.pauseBothTimer();

    /* Check if there have been splits and ask user about data */
    if (data.hasSplit()) {
        int ret = QMessageBox::warning(this, "Segment times changed", "Do you want to discard this data?",
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (ret == QMessageBox::No) {
            return;
        }
    }

    qDebug("open new file");

    /* Let the user select a filename and try to open this file */
    QString filename = QFileDialog::getOpenFileName(this, "Open file with data segments", "", "All files (*.*)");

    if (filename.length() == 0)
        return;

    /* Load data */
    data.loadData(filename);

    /* Set back timers, display, etc. */
    timeControl.resetBothTimer();

    displaySegments.clear();
    data.getCurrentSegments(displaySegments, segmentLines);

    calculateRegionSizes();
}

void MainWindow::onSave() {
    qDebug("save");

    /* Pause timer so they do not continue running */
    timeControl.pauseBothTimer();

    /* Merging unsaved data */
    data.reset(true);

    /* Check for filename */
    if (data.getFilename().size() == 0) {
        onSaveAs();
    } else {
        data.saveData(data.getFilename());
    }
}

void MainWindow::onSaveAs() {
    qDebug("saveas");

    /* Pause timer so they do not continue running */
    timeControl.pauseBothTimer();

    /* Let the user select a filename to save the segment data */
    QString filename = QFileDialog::getSaveFileName(this, "Save segment data", data.getFilename(), "All files (*.*)");

    if (filename.length() == 0)
        return;

    data.saveData(filename);
}

void MainWindow::onToggleAutosplit(bool enable) {    
    if (enable) {
        qDebug("Enabled autosplit");
    } else {
        qDebug("Disabled autosplit");
    }

    autosplit = enable;
}

void MainWindow::onToggleAutosave(bool enable) {
    if (enable) {
        qDebug("Enabled autosave");
    } else {
        qDebug("Disabled autosave");
    }

    autosave = enable;
}

void MainWindow::onToggleAutostartstop(bool enable) {
    if (enable) {
        qDebug("Enabled autostart/stop");
    } else {
        qDebug("Disabled autostart/stop");
    }

    autostartstop = enable;
}

void MainWindow::onExit() {
    /* Whatever is in the split data, save it as a temporary file with a time stamp
     * and set it as last filename used. Of course, only if the split data changed
     * (i.e. has splits already). Set this file as the last split data file so that
     * it will be loaded next time Fluffelwatch is opened. */
    if (autosave && data.hasSplit()) {
        QString filename = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") + " " + data.getTitle() + ".conf";
        data.reset(true);
        data.saveData(filename);
        settings->setValue("Data/segmentData", filename);
    }

    /* Destroy the settings here to sync it and write everything to the file. */
    delete settings;

    /* Close the window */
    this->close();
}

void MainWindow::setupContextMenu() {
    QAction *separator1 = new QAction(this);
    QAction *separator2 = new QAction(this);
    separator1->setSeparator(true);
    separator2->setSeparator(true);

    this->addAction(ui->action_Start_Split);
    this->addAction(ui->action_Pause);
    this->addAction(ui->action_Reset);
    this->addAction(ui->actionAutosplit_between_missions);
    this->addAction(ui->actionAutostart_stop_the_timers);
    this->addAction(separator1);
    this->addAction(ui->action_Open);
    this->addAction(ui->actionS_ave);
    this->addAction(ui->actionSave_as);
    this->addAction(ui->actionAutosave_at_exit);
    this->addAction(separator2);
    this->addAction(ui->action_Exit);
}

void MainWindow::setupGlobalShortcuts() {
    /* Register global hotkeys for start, pause/resume, reset */
    QxtGlobalShortcut* shortcutSplit = new QxtGlobalShortcut(this);
    shortcutSplit->setShortcut(QKeySequence("Ctrl+Shift+F1"));

    QxtGlobalShortcut* shortcutPause = new QxtGlobalShortcut(this);
    shortcutPause->setShortcut(QKeySequence("Ctrl+Shift+F2"));

    QxtGlobalShortcut* shortcutReset = new QxtGlobalShortcut(this);
    shortcutReset->setShortcut(QKeySequence("Ctrl+Shift+F3"));

    connect(shortcutSplit, &QxtGlobalShortcut::activated, this, &MainWindow::onSplit);
    connect(shortcutPause, &QxtGlobalShortcut::activated, this, &MainWindow::onPause);
    connect(shortcutReset, &QxtGlobalShortcut::activated, this, &MainWindow::onReset);
}

void MainWindow::readSettings() {
    /* Read font and color settings into maps for convenient access */
    readSettingsFonts();
    readSettingsColors();
    backgroundBrush = QBrush(userColors["background"]);

    /* General settings */
    marginSize = settings->value("marginSize", 0).toInt();
    segmentLines = qMax(2, settings->value("segmentLines").toInt());

    /* Autosplit, Autosave, Autostart/stop (will automatically set the boolean through the toggle slot) */
    ui->actionAutosplit_between_missions->setChecked(settings->value("autosplit", false).toBool());
    ui->actionAutosave_at_exit->setChecked(settings->value("autosave", false).toBool());
    ui->actionAutostart_stop_the_timers->setChecked(settings->value("autostartstop", false).toBool());

    /* Read the segment and food data if available */
    readSettingsData();
}

void MainWindow::readSettingsFonts() {
    /* Get all keys from the fonts group */
    settings->beginGroup("Fonts");
    QStringList keys = settings->allKeys();

    /* Iterate over each key and create an entry in the map. The value is read into a string
     * and then converted to a QFont by fromString. */
    for(int i = 0; i < keys.size(); ++i) {
        QFont value;

        if (value.fromString(settings->value(keys[i], QFont("Arial", 18, QFont::Bold).toString()).toString())) {
            userFonts.insert(keys[i], value);
        }
    }

    /* Reset the group */
    settings->endGroup();
}

void MainWindow::readSettingsColors() {
    /* Read the keys and settings from the colors group */
    settings->beginGroup("Colors");
    QStringList keys = settings->allKeys();

    /* Iterate over each key and create an entry in the map. The value is read into a string
     * and then converted to QColor. */
    for(int i = 0; i < keys.size(); ++i) {
        QColor value = QColor(settings->value(keys[i], "#fefefe").toString());

        userColors.insert(keys[i], value);
    }

    /* Reset the group */
    settings->endGroup();
}

void MainWindow::readSettingsData() {
    /* Read the segment data as well as the current fluffelfood data in */
    settings->beginGroup("Data");

    QString segmentData = settings->value("segmentData").toString();
    QString foodData = settings->value("foodData").toString();

    /* Load segment data */
    data.loadData(segmentData);
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Segment data loaded from... %s", segmentData.toStdString().c_str());
    qDebug("Got %d segments from data object", segments);

    /* Load food data */
    icons.loadFromFile(foodData);
    icons.showAllIcons();
    qDebug("Food data loaded from... %s", foodData.toStdString().c_str());

    settings->endGroup();
}

void MainWindow::paintAllElements(QPainter& painter) {
    /* Main title (taken from split data file) */
    paintText(painter, regionTitle, userFonts["mainTitle"], userColors["mainTitle"], data.getTitle(), Qt::AlignCenter);

    /* Separators */
    paintSeparator(painter, regionTitle.bottomLeft(), regionTitle.bottomRight());
    paintSeparator(painter, regionTimeList.bottomLeft(), regionTimeList.bottomRight());

    /* Paint all segments (middle part) */
    int lines = qMin(segmentLines, displaySegments.size());
    for (int i = 0; i < lines; ++i) {
        paintSegmentLine(painter, QRect(marginSize,
                                        regionTimeList.top() + marginSize + i * segmentSize.height(),
                                        segmentSize.width(),
                                        segmentSize.height()), displaySegments[i]);
    }

    /* Status area: the icons + the ingame and real timer */
    QRect rectReal = QRect(regionStatus.right() - mainTimerSize.width() - marginSize,
                           regionStatus.top() + marginSize,
                           mainTimerSize.width(),
                           mainTimerSize.height());
    QRect rectIngame = QRect(regionStatus.right() - adjustedTimerSize.width() - marginSize,
                             regionStatus.bottom() - adjustedTimerSize.height() - marginSize,
                             adjustedTimerSize.width(),
                             adjustedTimerSize.height());

    icons.paint(painter, regionStatus);
    paintText(painter, rectReal, userFonts["realTimer"], userColors["realTimer"],
              timeControl.elapsedRealTimeString(), Qt::AlignRight | Qt::AlignVCenter);
    paintText(painter, rectIngame, userFonts["ingameTimer"], userColors["ingameTimer"],
              timeControl.elapsedIngameTimeString(), Qt::AlignRight | Qt::AlignVCenter);
}

void MainWindow::paintText(QPainter& painter, const QRect& rect, const QFont& font, const QColor& color, const QString& text, int flags) {
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, flags, text);
}

void MainWindow::paintSeparator(QPainter& painter, const QPoint& start, const QPoint& end) {
    painter.setPen(userColors["separatorLine"]);
    painter.drawLine(start, end);
}

void MainWindow::paintSegmentLine(QPainter& painter, const QRect& rect, SplitData::segment& segment) {
    /* Decide which state we want to draw: past segments, current segment (when timer are on), and
     * future segments. */
    if (segment.ran) {
        paintSegmentLinePast(painter, rect, segment);
    } else if (segment.current && timeControl.areBothTimerValid()) {
        paintSegmentLineCurrent(painter, rect, segment);
    } else {
        paintSegmentLineFuture(painter, rect, segment);
    }
}

void MainWindow::paintSegmentLinePast(QPainter& painter, const QRect& rect, SplitData::segment& segment) {
    /* Draw a past/ran segment */

    /* Segment title */
    paintText(painter, rect, userFonts["segmentTitle"], userColors["segmentTitle"],
              segment.title, Qt::AlignLeft | Qt::AlignVCenter);

    /* Segment difference time; display only if the segment was ran or it is the current segment */
    QRect rectDiff = QRect(rect.right() - segmentColumnSizes[2] - segmentColumnSizes[1] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[1],
                           rect.height());

    QColor textColor = userColors["segmentTitle"];
    if (segment.improtime < 0) {
        textColor = userColors["gainedTime"];
    } else if (segment.improtime > 0 ) {
        textColor = userColors["lostTime"];
    }

    paintText(painter, rectDiff, userFonts["segmentDiff"], textColor,
              TimeController::getStringFromTimeDiff(segment.totalimprotime), Qt::AlignRight | Qt::AlignVCenter);


    /* Segment time (lost or improvement) */
    QRect rectTime = QRect(rect.right() - segmentColumnSizes[2] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[2],
                           rect.height());

    textColor = userColors["segmentTime"];
    if (segment.runtime < segment.besttime) {
        textColor = userColors["newRecord"];
    } else if (segment.improtime > 0) {
        textColor = userColors["lostTime"];
    } else if (segment.improtime < 0) {
        textColor = userColors["gainedTime"];
    }

    paintText(painter, rectTime, userFonts["segmentTime"], textColor,
              TimeController::getStringFromTime(segment.totaltime), Qt::AlignRight | Qt::AlignVCenter);

}

void MainWindow::paintSegmentLineCurrent(QPainter& painter, const QRect& rect, SplitData::segment& segment) {
    /* Draw the current segment with the timers on */

    /* Highlighted segment title */
    paintText(painter, rect, userFonts["segmentTitle"], userColors["currentSegment"],
              segment.title, Qt::AlignLeft | Qt::AlignVCenter);

    /* Segment difference time; display only if the segment was ran or it is the current segment */
    QRect rectDiff = QRect(rect.right() - segmentColumnSizes[2] - segmentColumnSizes[1] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[1],
                           rect.height());

    qint64 improtime = timeControl.elapsedPreferredTime() - segment.totaltime;

    /* Display with normal text color */
    paintText(painter, rectDiff, userFonts["segmentDiff"], userColors["segmentTitle"],
              TimeController::getStringFromTimeDiff(improtime), Qt::AlignRight | Qt::AlignVCenter);


    /* Segment time (or improvement) */
    QRect rectTime = QRect(rect.right() - segmentColumnSizes[2] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[2],
                           rect.height());

    QColor textcolor = userColors["currentSegment"];
    if (improtime > 0) {
        textcolor = userColors["lostTime"];
    }

    paintText(painter, rectTime, userFonts["segmentTime"], textcolor,
              TimeController::getStringFromTime(timeControl.elapsedPreferredTime()), Qt::AlignRight | Qt::AlignVCenter);

}

void MainWindow::paintSegmentLineFuture(QPainter& painter, const QRect& rect, SplitData::segment& segment) {
    /* Draw a future segment */

    /* Segment title */
    paintText(painter, rect, userFonts["segmentTitle"], userColors["segmentTitle"],
              segment.title, Qt::AlignLeft | Qt::AlignVCenter);

    /* Segment time */
    QRect rectTime = QRect(rect.right() - segmentColumnSizes[2] - marginSize * 2,
                           rect.top(),
                           segmentColumnSizes[2],
                           rect.height());
    paintText(painter, rectTime, userFonts["segmentTime"], userColors["segmentTime"],
              TimeController::getStringFromTime(segment.totaltime), Qt::AlignRight | Qt::AlignVCenter);
}

void MainWindow::calculateRegionSizes() {
    /* Calculate title region */
    QFontMetrics fm(userFonts["mainTitle"]);
    regionTitle = QRect(QPoint(0, 0), fm.size(Qt::TextSingleLine, data.getTitle()));
    regionTitle.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate time list region */
    QFontMetrics segTitle(userFonts["segmentTitle"]);
    QFontMetrics segDiff(userFonts["segmentDiff"]);
    QFontMetrics segTime(userFonts["segmentTime"]);
    QSize sizeTitle = segTitle.size(Qt::TextSingleLine, data.getLongestSegmentTitle());
    segmentColumnSizes[0] = sizeTitle.width();
    QSize sizeDiff = segDiff.size(Qt::TextSingleLine, " −00:00:00.00 ");
    segmentColumnSizes[1] = sizeDiff.width();
    QSize sizeTime = segTime.size(Qt::TextSingleLine, " 00:00:00.00 ");
    segmentColumnSizes[2] = sizeTime.width();
    segmentSize.setWidth(sizeTitle.width() + marginSize + sizeDiff.width() + marginSize + sizeTime.width());
    segmentSize.setHeight(qMax(qMax(sizeTitle.height(), sizeDiff.height()), sizeTime.height()));

    regionTimeList = QRect(regionTitle.bottomLeft(), QSize(segmentSize.width(), segmentSize.height() * segmentLines));
    regionTimeList.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate status bar (with the two timers) */
    QFontMetrics mtFm(userFonts["realTimer"]);
    mainTimerSize = mtFm.size(Qt::TextSingleLine, "00:00:00.00");

    QFontMetrics atFm(userFonts["ingameTimer"]);
    adjustedTimerSize = atFm.size(Qt::TextSingleLine, "00:00:00.00");

    QSize iconArea = adjustedTimerSize;

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


