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
#include <QSettings>

#include "icondisplay.h"
#include "fluffelipcthread.h"
#include "splitdata.h"
#include "timecontroller.h"

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
    void onToggleAutosave(bool enable);
    void onToggleAutostartstop(bool enable);

    void onExit();

  private:
    /* User interface definitions and setup */
    Ui::MainWindow *ui;    
    IconDisplay icons;

    void setupContextMenu();
    void setupGlobalShortcuts();

    /* Timer id */
    int timerID;

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
    QMap<QString, QColor> userColors;

    bool autosplit = false;
    bool autosave = false;
    bool autostartstop = false;

    int marginSize;
    int segmentLines;

    /* Thread that handles the IPC with external programs, i.e. the actual
     * autosplitters (also controlling icon display, etc.) */
    FluffelIPCThread ipcthread;

    /* Object to control the real and ingame timer */
    TimeController timeControl;

    /* Split data object and the segments that are shown in the main window */
    SplitData data;
    QList<SplitData::segment> displaySegments;

    /* Painting functions and tools */
    void paintAllElements(QPainter &painter);

    void paintText(QPainter &painter, const QRect &rect, const QFont &font, const QColor &color, const QString &text, int flags);
    void paintSeparator(QPainter &painter, const QPoint& start, const QPoint &end);

    void paintSegmentLine(QPainter &painter, const QRect &rect, SplitData::segment &segment);
    void paintSegmentLinePast(QPainter &painter, const QRect& rect, SplitData::segment &segment);
    void paintSegmentLineCurrent(QPainter &painter, const QRect& rect, SplitData::segment &segment);
    void paintSegmentLineFuture(QPainter &painter, const QRect& rect, SplitData::segment &segment);





    QSize adjustedTimerSize;
    QSize mainTimerSize;


    QSize segmentSize;
    int segmentColumnSizes[3];





    /* Region functions */
    QRect regionTitle;
    QRect regionTimeList;
    QRect regionStatus;
    void calculateRegionSizes();
};

#endif // MAINWINDOW_H
