#ifndef FLUFFELTIMER_H
#define FLUFFELTIMER_H

#include <QElapsedTimer>
#include <QString>

class FluffelTimer : public QElapsedTimer {
  public:
    FluffelTimer();
    ~FluffelTimer();

    /* Pause/Continue functions */
    void start();
    void pause();
    bool isPaused() const;
    void resume();
    qint64 restart();

    qint64 elapsed_with_pause() const;
    QString toString() const;

    /* Static function to convert a qint64 into a string */
    static QString getStringFromTime(qint64 time);
    static QString getStringFromTimeDiff(qint64 timediff);


  private:
    /* This holds the total amount of pause we did */
    qint64 pausedTime;
    qint64 refPauseTime;

    /* Make elapsed private so it cannot be called from the outside */
    qint64 elapsed() const;
};

#endif // FLUFFELTIMER_H
