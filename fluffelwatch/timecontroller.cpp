#include "timecontroller.h"

TimeController::TimeController() {
    preferredTime = prefTime::prefIngameTime;
}

TimeController::~TimeController() {
}

void TimeController::startBothTimer() {
    timeIngame.start();
    timeReal.start();
}

void TimeController::pauseBothTimer() {
    timeIngame.pause();
    timeReal.pause();
}

void TimeController::resumeBothTimer() {
    timeIngame.resume();
    timeReal.resume();
}

void TimeController::toggleBothTimer() {
    if (timeIngame.isPaused() && timeReal.isPaused()) {
        resumeBothTimer();
    } else {
        pauseBothTimer();
    }
}

void TimeController::restartBothTimer() {
    timeIngame.restart();
    timeReal.restart();
}

void TimeController::resetBothTimer() {
    timeIngame.invalidate();
    timeReal.invalidate();
}

bool TimeController::areBothTimerValid() {
    return timeIngame.isValid() && timeReal.isValid();
}

bool TimeController::areBothTimerRunning() {
    return (!timeIngame.isPaused() && !timeReal.isPaused());
}

bool TimeController::isAnyTimerRunning() {
    return (!timeIngame.isPaused() || !timeReal.isPaused());
}

quint64 TimeController::elapsedRealTime() {
    if (timeReal.isValid()) {
        return timeReal.elapsed_with_pause();
    }

    return 0;
}

QString TimeController::elapsedRealTimeString() {
    return timeReal.toString();
}

quint64 TimeController::elapsedIngameTime() {
    if (timeIngame.isValid()) {
        return timeIngame.elapsed_with_pause();
    }

    return 0;
}

QString TimeController::elapsedIngameTimeString() {
    return timeIngame.toString();
}

void TimeController::setPreferredTimer(TimeController::prefTime value) {
    preferredTime = value;
}

TimeController::prefTime TimeController::getPreferredTimer() {
    return preferredTime;
}

quint64 TimeController::elapsedPreferredTime() {
    if (preferredTime == prefTime::prefIngameTime) {
        return elapsedIngameTime();
    } else {
        return elapsedRealTime();
    }
}

void TimeController::pauseIngameTimer() {
    timeIngame.pause();
}

void TimeController::resumeIngameTimer() {
    timeIngame.resume();
}

bool TimeController::isIngameTimerRunning() {
    return !timeIngame.isPaused();
}

QString TimeController::getStringFromTime(qint64 time) {
    return FluffelTimer::getStringFromTime(time);
}

QString TimeController::getStringFromTimeDiff(qint64 timediff) {
    return FluffelTimer::getStringFromTimeDiff(timediff);
}
