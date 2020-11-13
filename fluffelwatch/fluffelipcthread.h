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
                quint8  stoptimer;
                quint32 section;
                quint32 iconstates;
        };
#pragma pack(pop)

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
