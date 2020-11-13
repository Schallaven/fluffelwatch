#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>

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

    /* Start timer every 100 msec (can do faster timers, but it costs CPU load!) */
    startTimer(100, Qt::PreciseTimer);

    /* Start the thread for managing IPC to allow external programs to
     * change section number and iconstates */
    ipcthread.start();
}

MainWindow::~MainWindow() {
    /* Request exit of the thread and give it some time to exit */
    ipcthread.requestInterruption();
    QThread::msleep(ipcthread.timeout * 2);

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

        qDebug("New state: section = %d, states = 0x%08X, stoptimer = %d", tempData.section, tempData.iconstates, tempData.stoptimer);
    }

    /* Process new information */
//    if (memoryReaderThread.isRunning()) {
//        /* Let's get the data into a temporary buffer */
//        FluffelMemoryThread::gameData tempGameData = memoryReaderThread.getData();

//        /* Add measurement if neccessary */
//        if ((tempGameData.gamestate != currentGameData.gamestate) || (tempGameData.loading != currentGameData.loading) ||
//                (tempGameData.mission != currentGameData.mission)) {
//            measureList.append(QString("%1\t%2\t%3\t%4\t%5")
//                               .arg(FluffelTimer::getStringFromTime(timerReal.elapsed_with_pause()))
//                               .arg(timerReal.elapsed_with_pause())
//                               .arg(tempGameData.mission)
//                               .arg(tempGameData.loading)
//                               .arg(tempGameData.gamestate));
//        }

//        /* Check if gamestate and/or loading changed to adapt the icon states */
//        if ((tempGameData.gamestate != currentGameData.gamestate) || (tempGameData.loading != currentGameData.loading)) {
//            updateIcons(tempGameData);
//        }

//        /* If loading changed, then we might want to either pause or resume the adjusted timer */
//        if (tempGameData.loading != currentGameData.loading) {
//            if (tempGameData.loading == FluffelMemoryThread::loadingCircle) {
//                timerAdjusted.pause();
//            } else if (tempGameData.loading != FluffelMemoryThread::loadingCircle) {
//                timerAdjusted.resume();
//            }
//        }

//        /* Mission number changed, we skip forward into our split data until the first entry with
//         * this mission id is visible */
//        if (tempGameData.mission != currentGameData.mission) {
//            /* This can happen sometimes if we save during a mission. Ignore the reset of the mission number */
//            if ((tempGameData.mission == 0) && (currentGameData.mission > 0)) {
//                tempGameData.mission = currentGameData.mission;
//            } else {
//                qDebug("Mission changed from %d to %d", currentGameData.mission, tempGameData.mission);
//            }

//            /* Autosplit only if the new mission is larger then the previous one. */
//            if (autosplit && (tempGameData.mission > currentGameData.mission)) {
//                displaySegments.clear();
//                int remains = data.splitToMission(tempGameData.mission, timerReal.elapsed_with_pause());
//                int segments = data.getCurrentSegments(displaySegments, segmentLines);
//                qDebug("Got %d segments from data object. %d remaining segments.", segments, remains);

//                /* Stop the timer if that was the last split */
//                if (remains == 0) {
//                    timerReal.pause();
//                    timerAdjusted.pause();
//                }
//            }
//        }

//        /* Save game data */
//        currentGameData = tempGameData;
//    }

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

    measureList.clear();
    qDebug("Cleared measurement list");
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
    /*displaySegments.clear();
    int remains = data.splitToMission(20, timeControl.elapsedPreferredTime());
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Got %d segments from data object. %d remaining segments.", segments, remains);
    */

    /* Stop the timer if that was the last split */
    /*if (remains == 0) {
        timeControl.pauseBothTimer();
    }*/

    if (enable) {
        qDebug("Enabled autosplit");
    } else {
        qDebug("Disabled autosplit");
    }

    autosplit = enable;
}

void MainWindow::onExit() {
    this->close();
    return;

    /* Whatever is in the split data, save it as a temporary file with a time stamp
     * and set it as last filename used */
    QString filename = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") + " " + data.getTitle() + ".conf";
    data.saveData(filename);
    settings->setValue("segmentdata", filename);

    /* Destroy the settings here to sync it and write everything to the file. This
     * allows us to set permissions/ownership right if needed. */
    QString settingsname = settings->fileName();
    delete settings;

    /* Make sure that if we run Fluffelwatch as su that the ownership is still given
     * to the current user */
    if (geteuid() == 0) {
        /* Get the user id that invoked sudo */
        int uid = atoi(getenv("SUDO_UID"));
        int gid = atoi(getenv("SUDO_GID"));

        qDebug("UID %d and GID %d", uid, gid);

        if (uid != 0 && gid != 0) {
            int ret = chown(filename.toStdString().c_str(), uid, gid);
            qDebug("Chown ret %d. %d:%d", ret, uid, gid);

            ret = chown(settingsname.toStdString().c_str(), uid, gid);
            qDebug("Chown ret %d. %d:%d", ret, uid, gid);
        }
    }

    /* Save measurements */
    qDebug("Saving %d measurements", measureList.size());
    QFile measurements("measurements.txt");
    measurements.open(QFile::WriteOnly|QFile::Truncate|QFile::Text);
    QTextStream out(&measurements);
    QString toWrite = measureList.join("\n");
    measurements.write(toWrite.toUtf8());
    measurements.close();

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
    this->addAction(separator1);
    this->addAction(ui->action_Open);
    this->addAction(ui->actionS_ave);
    this->addAction(ui->actionSave_as);
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

    /* Autosplit (will automatically set the boolean through the toggle slot) */
    ui->actionAutosplit_between_missions->setChecked(settings->value("autosplit", false).toBool());

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

    qDebug("Segment data loaded from... %s", segmentData.toStdString().c_str());
    qDebug("Food data loaded from... %s", foodData.toStdString().c_str());

    /* Load segment data */
    data.loadData(segmentData);
    int segments = data.getCurrentSegments(displaySegments, segmentLines);
    qDebug("Got %d segments from data object", segments);

    /* Load food data */
    icons.loadFromFile(foodData);
    icons.showAllIcons();

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

void MainWindow::paintSegmentLine(QPainter& painter, QRect rect, SplitData::segment& segment) {
    /* Use this variable to set and vary the current text color */
    QColor textColor = userColors["segmentTitle"];

    /* Segment title: highlighted if it is current segment */
    if (segment.current && timeControl.areBothTimerValid()) {
        textColor = userColors["currentSegment"];
    }
    paintText(painter, rect, userFonts["segmentTitle"], textColor, segment.title, Qt::AlignLeft | Qt::AlignVCenter);

    /* Segment difference time; display only if the segment was ran or it is the current segment */
    QRect rectDiff = QRect(rect.right() - segmentColumnSizes[2] - segmentColumnSizes[1] - marginSize * 2,
        rect.top(),
        segmentColumnSizes[1],
        rect.height());

    textColor = userColors["segmentTitle"];
    if (segment.current && timeControl.areBothTimerValid()) {
        /* Calculate a temporary improvement time here */
        quint64 improtime = timeControl.elapsedPreferredTime() - segment.totaltime;

        /* Display with normal text color */
        paintText(painter, rectDiff, userFonts["segmentDiff"], textColor,
                           TimeController::getStringFromTimeDiff(improtime), Qt::AlignRight | Qt::AlignVCenter);
    } else if (segment.ran) {
        if (segment.improtime < 0) {
            textColor = userColors["gainedTime"];
        } else if (segment.improtime > 0 ) {
            textColor = userColors["lostTime"];
        }

        paintText(painter, rectDiff, userFonts["segmentDiff"], textColor,
                           TimeController::getStringFromTimeDiff(segment.totalimprotime), Qt::AlignRight | Qt::AlignVCenter);
    }

    /* Segment time (or improvement) */
    QRect rectTime = QRect(rect.right() - segmentColumnSizes[2] - marginSize * 2,
            rect.top(),
            segmentColumnSizes[2],
            rect.height());

    textColor = userColors["segmentTime"];
    if (segment.ran) {
        if (segment.runtime < segment.besttime) {
            textColor = userColors["newRecord"];
        } else if (segment.improtime > 0) {
            textColor = userColors["lostTime"];
        } else if (segment.improtime < 0) {
            textColor = userColors["gainedTime"];
        }

        paintText(painter, rectTime, userFonts["segmentTime"], textColor,
                           TimeController::getStringFromTime(segment.totaltime), Qt::AlignRight | Qt::AlignVCenter);
    } else if (segment.current && timeControl.areBothTimerValid()) {
        textColor = userColors["currentSegment"];

        paintText(painter, rectTime, userFonts["segmentTime"], textColor,
                           TimeController::getStringFromTime(timeControl.elapsedPreferredTime()), Qt::AlignRight | Qt::AlignVCenter);
    } else {
        paintText(painter, rectTime, userFonts["segmentTime"], textColor,
                           TimeController::getStringFromTime(segment.totaltime), Qt::AlignRight | Qt::AlignVCenter);
    }
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
    QSize sizeDiff = segDiff.size(Qt::TextSingleLine, " −00:00:00.0 ");
    segmentColumnSizes[1] = sizeDiff.width();
    QSize sizeTime = segTime.size(Qt::TextSingleLine, " 00:00:00.0 ");
    segmentColumnSizes[2] = sizeTime.width();
    segmentSize.setWidth(sizeTitle.width() + marginSize + sizeDiff.width() + marginSize + sizeTime.width());
    segmentSize.setHeight(qMax(qMax(sizeTitle.height(), sizeDiff.height()), sizeTime.height()));

    regionTimeList = QRect(regionTitle.bottomLeft(), QSize(segmentSize.width(), segmentSize.height() * segmentLines));
    regionTimeList.adjust(0, 0, marginSize * 2, marginSize * 2);

    /* Calculate status bar (with the two timers) */
    QFontMetrics mtFm(userFonts["realTimer"]);
    mainTimerSize = mtFm.size(Qt::TextSingleLine, "00:00:00.0");

    QFontMetrics atFm(userFonts["ingameTimer"]);
    adjustedTimerSize = atFm.size(Qt::TextSingleLine, "00:00:00.0");

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


