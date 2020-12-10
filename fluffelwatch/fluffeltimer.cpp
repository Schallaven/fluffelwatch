#include "fluffeltimer.h"

FluffelTimer::FluffelTimer() {
    refPauseTime = -1;
}

FluffelTimer::~FluffelTimer() {

}

void FluffelTimer::start() {
    /* Reset pause and start timer */
    pausedTime = 0;
    refPauseTime = -1;

    QElapsedTimer::start();
}

void FluffelTimer::pause() {
    if (refPauseTime > -1) {
        return;
    }

    /* Save the elapsed time as a reference */
    refPauseTime = elapsed();
}

bool FluffelTimer::isPaused() const {
    return (refPauseTime != -1);
}

void FluffelTimer::resume() {
    if ((refPauseTime == -1) || (!isValid())) {
        return;
    }

    /* Add the paused time to the total time paused */
    pausedTime += elapsed() - refPauseTime;
    refPauseTime = -1;
}

qint64 FluffelTimer::restart() {
    /* Save the time with pause and start again */
    qint64 totalTime = elapsed_with_pause();
    start();

    return totalTime;
}

qint64 FluffelTimer::elapsed_with_pause() const {
    /* Simply subtract the total time paused from the
     * absolute elapsed time. If we are currently in
     * a pause then we need to account for this. */
    if (refPauseTime != -1) {
        return refPauseTime - pausedTime;
    }

    return elapsed() - pausedTime;
}

QString FluffelTimer::toString() const {
    /* Invalidate times */
    if (!isValid()) {
        return QString("00:00:00.00");
    }

    /* Transfer the milliseconds qint64 into a
     * string. */
    return FluffelTimer::getStringFromTime(elapsed_with_pause());
}

QString FluffelTimer::getStringFromTime(qint64 time) {
    /* Transfer the milliseconds qint64 into a string. */
    int hours = time / 3600000;                                                      /* 1 hour has 3 600 000 msec */
    int minutes = (time - hours * 3600000) / 60000;                                  /* 1 minutes has 60 000 msec */
    int seconds = (time - hours * 3600000 - minutes * 60000) / 1000;                 /* 1 second has 1 000 msec */
    int per_sec = (time - hours * 3600000 - minutes * 60000 - seconds * 1000) / 10;  /* a 100th of a second has 10 msecs */

    return QString::asprintf("%02d:%02d:%02d.%02d", hours, minutes, seconds, per_sec);
}

QString FluffelTimer::getStringFromTimeDiff(qint64 timediff) {
    /* Transfer a time difference into a string. First we convert it to
     * a normal time string */
    QString timestr = FluffelTimer::getStringFromTime(qAbs(timediff));

    /* Remove hours and/or minutes if the timediff is too low */
    int remove = 0;
    if (qAbs(timediff) < 60000) {
        remove = 6;
    } else if (qAbs(timediff < 3600000)) {
        remove = 3;
    }
    /* The minus here is a real minus (U+2212) not a dash! */
    return (timediff >= 0 ? "+" : "−") + timestr.mid(remove);
}

qint64 FluffelTimer::elapsed() const {
    return QElapsedTimer::elapsed();
}
