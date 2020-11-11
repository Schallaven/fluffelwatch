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

        bool isListening() const;

        static const int timeout;
        static const QString listenerName;

        /* Data received by sockets */
        struct listenerData {
                quint32 section;
                quint32 iconstates;
        };

        listenerData getData() const;

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
        listenerData internalData;
        void updateData(const listenerData &newdata);
};

#endif // FLUFFELIPCTHREAD_H
