#ifndef TIMECONTROLLER_H
#define TIMECONTROLLER_H

#include "fluffeltimer.h"

class TimeController
{
    public:
        TimeController();
        ~TimeController();

        /* These functions control and investigate both timers explicitly. */
        void startBothTimer();
        void pauseBothTimer();
        void resumeBothTimer();
        void toggleBothTimer();
        void restartBothTimer();
        void resetBothTimer();

        bool areBothTimerValid();
        bool areBothTimerRunning();
        bool isAnyTimerRunning();

        quint64 elapsedRealTime();
        QString elapsedRealTimeString();

        quint64 elapsedIngameTime();
        QString elapsedIngameTimeString();

        /* These functions react to a preference that can be set. This is
         * useful if you want to use the real timer instead of the ingame
         * timer for splitting or vice-versa. */
        enum prefTime { prefIngameTime = 0, prefRealTime = 1 };

        void setPreferredTimer(prefTime value);
        prefTime getPreferredTimer();

        quint64 elapsedPreferredTime();

        /* Control the ingame timer. Note that there are no functions
         * to control the real timer on its own. Also this timer can
         * not be started independently. */
        void pauseIngameTimer();
        void resumeIngameTimer();

        bool isIngameTimerRunning();

        /* These are just forward functions to the respective static
         * FluffelTimer functions. */
        static QString getStringFromTime(qint64 time);
        static QString getStringFromTimeDiff(qint64 timediff);

    private:
        /* Define two timers:
         * one for the ingame time that can be paused externally by IPC and
         * one for the real time that will just run from start to the end. */
        FluffelTimer timeIngame;
        FluffelTimer timeReal;

        /* Preferred time defines what time is returned by elapsedPrefTime() */
        prefTime preferredTime;
};

#endif // TIMECONTROLLER_H
