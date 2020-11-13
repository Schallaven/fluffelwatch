#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QProcess>
#include <QSettings>

#include "icondisplay.h"
#include "fluffelipcthread.h"
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

    void onToggleAutosplit(bool enable);

    void onExit();

  private:
    /* User interface definitions and setup */
    Ui::MainWindow *ui;    
    IconDisplay icons;

    void setupContextMenu();
    void setupGlobalShortcuts();

    /* Window movement control */
    bool isMoving = false;
    QPoint movingStartPos;

    /* Settings are saved here mostly as maps that can be accessed
     * by a convenient key. */
    QSettings *settings = nullptr;
    void readSettings();
    void readSettingsFonts();
    void readSettingsColors();
    void readSettingsData();

    QBrush backgroundBrush;
    QMap<QString, QFont> userFonts;
    QMap<QString, QPen> userColors;




    FluffelIPCThread ipcthread;

    /* Options */
    bool autosplit = false;



    /* Data and timer objects */
    SplitData data;
    FluffelTimer timerReal;
    FluffelTimer timerAdjusted;

    /* Painting brushes, pens, and fonts */


    QSize adjustedTimerSize;
    QSize mainTimerSize;

    int marginSize;
    int iconSize;

    int segmentLines;
    QSize segmentSize;
    int segmentColumnSizes[3];
    QList<SplitData::segment> displaySegments;

    QStringList measureList;

    void paintSegmentLine(QPainter &painter, QRect rect, SplitData::segment &segment);

    /* Region functions */
    QRect regionTitle;
    QRect regionTimeList;
    QRect regionStatus;
    void calculateRegionSizes();
};

#endif // MAINWINDOW_H
