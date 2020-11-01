#ifndef SPLITDATA_H
#define SPLITDATA_H

#include <QFile>
#include <QList>
#include <QQueue>
#include <QString>
#include <QTextStream>

class SplitData {
  public:
    SplitData();
    ~SplitData();

    /* Loading/Saving split data */
    void loadData(const QString &filename);
    void saveData(const QString &filename) const;

    /* Segment */
    struct segment {
        QString title;
        bool ran = false;
        bool current = false;
        qint64 runtime = 0;
        qint64 besttime = 0;
        qint64 improtime = 0;
        int mission = 0;
    };

    /* Title */
    QString getTitle() const;
    void setTitle(const QString& value);

    /* Get longest segment title */
    QString getLongestSegmentTitle() const;

    /* Gets n lines of segments around the current one
     * plus the last one. Returns the real number of
     * lines added (if less). */
    int getCurrentSegments(QList<segment>& list, int lines) const;

  private:
    /* Segment lists: all segments contains all segments in a list.
     * futureSegments contains all segments that still have to be
     * run in normal order. pastSegments contains all past segments
     * in reversed order (i.e. the latest segment run is at the front
     * of the list) */
    QList<segment> allSegments;
    QQueue<segment> futureSegments;
    QQueue<segment> pastSegments;

    /* Title of the run */
    QString title;

    /* Gets n lines or less from a segment queue Returns
     * the real number of lines added (if less). */
    int getSegments(QList<segment>& toList, const QList<segment> &fromList, int lines) const;

};

#endif // SPLITDATA_H
