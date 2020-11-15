#include "fluffelipcthread.h"

/* Timeout for the blocking waiting-for-connection function (in ms) */
const int FluffelIPCThread::timeout = 400;

/* Server name for the socket */
const QString FluffelIPCThread::listenerName = "fluffelwatch";


FluffelIPCThread::FluffelIPCThread() {
    server = nullptr;
    client = nullptr;
    changed = false;

    /* Give this thread a good name to be able to find it in process overviews (ps and the like) */
    setObjectName("fluffelwatch socket thread");
}

FluffelIPCThread::~FluffelIPCThread() {
}

void FluffelIPCThread::run() {
    /* Nothing connected */
    server = nullptr;
    client = nullptr;
    changed = false;
    internalData = listenerData();

    if (!openListener()) {
        qDebug("Socket could not be opened. Aborting thread.");
        return;
    }

    /* Main loop for this thread */
    while(!isInterruptionRequested()) {
        /* If there is no client connected, then we will wait simply for one */
        if (client == nullptr) {
            /* Wait for connection (or timeout); this will also reduce the CPU load */
            server->waitForNewConnection(timeout);

            /* Client connected? Will return nullptr if no client is there */
            client = server->nextPendingConnection();

            /* This will refuse all remaining connections */
            if (client != nullptr) {
                server->close();
            }

            continue;
        }

        /* The current client is disconnected, so destroy the object and start listening again */
        if (client->state() == QLocalSocket::UnconnectedState) {
            qDebug("Client disconnected");

            delete client;
            client = nullptr;

            if (!server->listen(listenerName)) {
                qDebug("Could not recreate a fluffelwatch socket (Error %d): %s",
                       server->serverError(), server->errorString().toStdString().c_str());
                break;
            }

            qDebug("Removed client. Recreated listener.");
            continue;
        }

        /* Read data from connected client if available; will also reduce the CPU load */
        if (client->waitForReadyRead(timeout)) {
            /* Read data. The data is 1 byte for the loading/stoptimer byte and another
             * 2 x 32bit integers (8 bytes) in size. The first 32bit integer is the
             * "section number" used for autosplitting. The second 32bit integer is the
             * state of the icons encoded as bits, i.e. 32 icons max (1 = on, 0 = off). */
            listenerData value;
            quint64 readbytes = client->read(reinterpret_cast<char*>(&value), sizeof(value));

            if (readbytes == sizeof(listenerData)) {
                qDebug("Read from socket: timercontrol = %d, section = %d, iconstates = 0x%08X",
                       value.timercontrol, value.section, value.iconstates);
                updateData(value);
            }

            /* Sleep a little bit here to reduce CPU load. */
            QThread::usleep(100);
        }
    }

    /* Close the socket and remove it */
    closeListener();
    if (server != nullptr) {
        delete server;
    }
}

FluffelIPCThread::listenerData FluffelIPCThread::getData() {
    changed = false;
    return internalData;
}

bool FluffelIPCThread::dataChanged() const {
    return changed;
}

bool FluffelIPCThread::openListener() {
    /* Close the socket if it is currently open and remove all potential sockets
     * with this name. This will increase the chances that "listen" will be
     * successfully called. */
    closeListener();

    /* Object not created yet? Important here: do not provide any parent since the parent might be
     * not in the same thread as QLocalServer, which causes some problems. */
    if (server == nullptr) {
        server = new QLocalServer();
        server->moveToThread(this);
    }

    /* Create a socket server and start listening */
    if (!server->listen(listenerName)) {
        qDebug("Could not create a fluffelwatch socket (Error %d): %s", server->serverError(), server->errorString().toStdString().c_str());
        return false;
    }

    qDebug("Starting listening at '%s'.", server->fullServerName().toStdString().c_str());

    return true;
}

void FluffelIPCThread::closeListener() {
    /* Close the socket if it is currently open and remove all potential sockets
     * with this name. */
    if (server != nullptr) {
        server->close();
    }
    QLocalServer::removeServer(listenerName);
}

void FluffelIPCThread::updateData(const FluffelIPCThread::listenerData& newdata) {
    /* Nothing changed, so do nothing here */
    if (newdata == internalData) {
        return;
    }

    accessMutex.lock();
    internalData.timercontrol   = newdata.timercontrol;
    internalData.section        = newdata.section;
    internalData.iconstates     = newdata.iconstates;
    changed = true;
    accessMutex.unlock();
}

bool operator==(const FluffelIPCThread::listenerData& lhs, const FluffelIPCThread::listenerData& rhs) {
    /* Allows to compare listener data simply by == */
    return (lhs.timercontrol == rhs.timercontrol) && (lhs.section == rhs.section) && (lhs.iconstates == rhs.iconstates);
}
