#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QFontMetrics>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QSettings>

#include "fluffeltimer.h"
#include "splitdata.h"

#include "qxt/qxtglobalshortcut.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
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

    /* Timer event (for painting) */
    void timerEvent(QTimerEvent *event) override;

  public slots:
    void onSplit();
    void onPause();
    void onReset();

    void onOpen();
    void onSave();
    void onSaveAs();

    void onExit();

  private:
    /* User interface definitions and setting files */
    Ui::MainWindow *ui;
    QSettings *settings = nullptr;

    void readSettings();

    /* Window movement on client */
    bool isMoving = false;
    QPoint movingStartPos;

    /* Data and timer objects */
    SplitData data;
    FluffelTimer timerReal;
    FluffelTimer timerAdjusted;

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

    QFont fontSegmentTitle;
    QPen penSegmentTitle;
    QFont fontSegmentTime;
    QPen penSegmentTime;

    QFont fontSegmentDifference;
    QPen penSegmentCurrent;
    QPen penSegmentGained;
    QPen penSegmentLost;
    QPen penSegmentNewRecord;

    int marginSize;
    int iconSize;

    /* Status icons */
    enum icons {
        iconFluffel = 0,
        iconLoading = 1,
        iconSavegame = 2,
        iconCinema = 3,
        iconDead = 4,
        iconCOUNT = 5
    };
    QIcon iconStates[iconCOUNT];
    bool gameStates[iconCOUNT] = {false};

    int segmentLines;
    QSize segmentSize;
    int segmentColumnSizes[3];
    QList<SplitData::segment> displaySegments;

    void paintSegmentLine(QPainter &painter, QRect rect, SplitData::segment &segment);

    /* Region functions */
    QRect regionTitle;
    QRect regionTimeList;
    QRect regionStatus;
    void calculateRegionSizes();
};

#endif // MAINWINDOW_H
