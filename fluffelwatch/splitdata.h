#ifndef SPLITDATA_H
#define SPLITDATA_H

#include <QFile>
#include <QList>
#include <QString>
#include <QTextStream>

class SplitData {
  public:
    SplitData();
    ~SplitData();

    /* Loading/Saving split data */
    void loadData(const QString &filename);
    void saveData(const QString &filename);

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

    /* Split, returns the number of segments remaining in futureSegments */
    int split(quint64 curtime);
    int splitToMission(int mission, quint64 curtime);
    bool canSplit() const;
    bool hasSplit() const;

    /* Resets the list and merges times if wanted */
    void reset(bool merge = false);

    QString getFilename() const;

    private:
    /* Segment lists: all segments contains all segments in a list.
     * futureSegments contains all segments that still have to be
     * run in normal order. pastSegments contains all past segments
     * in reversed order (i.e. the latest segment run is at the front
     * of the list) */
    QList<segment> allSegments;
    QList<segment> futureSegments;
    QList<segment> pastSegments;

    /* Title and filename of the run */
    QString title;
    QString filename;

    /* Gets n lines or less from a segment queue Returns
     * the real number of lines added (if less). */
    int getSegments(QList<segment>& toList, const QList<segment> &fromList, int lines, bool backwards = false) const;

};

#endif // SPLITDATA_H
