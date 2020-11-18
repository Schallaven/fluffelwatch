#ifndef FLUFFELIPCTHREAD_H
#define FLUFFELIPCTHREAD_H

#include <QLocalServer>
#include <QLocalSocket>
#include <QMutex>
#include <QThread>

class FluffelIPCThread : public QThread
{
    public:
        FluffelIPCThread();
        ~FluffelIPCThread();

        void run() override;

        static const int timeout;
        static const QString listenerName;

        /* Data received by sockets. The pragma packing is important here,
         * otherwise the compiler will align the structure (first member)
         * and it will be hard to read from the socket the exact number of
         * bytes. */
#pragma pack(push, 1)
        struct listenerData {
                quint8  timercontrol = 0;
                quint32 section = 0;
                quint32 iconstates = 0;
        };
#pragma pack(pop)
        /* This enum describes the possible values of the timercontrol in
         * the data struct above. All other values should be ignored.
         * Note: These values only have effect if the respective options
         * (autosplit, etc.) are set in Fluffelwatch by the user! */
        enum control {
            timeControlNone = 0,        /* Ingame timer should run normally (or continue if paused) */
            timeControlPause = 1,       /* Ingame timer should be paused (usually done during loading times, etc.) */
            timeControlStart = 200,     /* Both timers should be reset and started (usually done at the very beginning of a run) */
            timeControlStop = 201,      /* Both timers should stopped (usually done at the very end of a run) */
        };

        /* Getter function will set the state changed to false */
        listenerData getData();

        /* Call to find out if the internal data changed since the last read */
        bool dataChanged() const;

    private:
        /* This is the local socket (or named pipe on other platforms) that will be used
         * for interprocess communication (IPC). */
        QLocalServer *server = nullptr;
        QLocalSocket *client = nullptr;

        /* Open and close the server socket */
        bool openListener();
        void closeListener();

        /* Internal data */
        QMutex accessMutex;

        bool changed;
        listenerData internalData;
        void updateData(const listenerData &newdata);
};

bool operator==(const FluffelIPCThread::listenerData& lhs, const FluffelIPCThread::listenerData& rhs);

#endif // FLUFFELIPCTHREAD_H
