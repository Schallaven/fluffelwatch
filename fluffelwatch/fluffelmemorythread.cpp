#include "fluffelmemorythread.h"

#include <sys/uio.h>

FluffelMemoryThread::FluffelMemoryThread() {

}

FluffelMemoryThread::~FluffelMemoryThread() {

}

void FluffelMemoryThread::run() {
    /* First open memory then init addresses! */
    if (!openMemory()) {
        qDebug("Could not open memory. Error: %s", memFile.errorString().toStdString().c_str());
        return;
    }

    /* First let's init everything */
    initAddresses();

    /* Read constantly the memory and update if necessary */
    gameData temp;
    while(true) {
        /* Read temporary values */
        temp.gamestate = readMemory(addresses[tokenGamestate1], 4);
        temp.mission = readMemory(addresses[tokenMission], 2);
        temp.loading = readMemory(addresses[tokenLoadingicon], 2);

        /* Update internal structure */
        updateData(temp);

        /* Someone wants to shut us down or there were errors while
         * reading from memory. Let's just leave in these cases. */
        if (isInterruptionRequested() || (memFile.error() != QFileDevice::NoError)) {
            break;
        }

        /* Sleep a little bit to not overload the CPU */
        msleep(50);
    }

    qDebug("Thread exit");

    /* Close memory file */
    memFile.close();
}

int FluffelMemoryThread::getProcessID() const {
    return processID;
}

void FluffelMemoryThread::setProcessID(int value) {
    processID = value;
}

bool FluffelMemoryThread::isConnected() const {
    return (memFile.isOpen() && QFileDevice::NoError == memFile.error());
}

void FluffelMemoryThread::initAddresses() {
    /* Initialize gamestate addresses. Both are directly behind each other. There is even more
     * data behind that - but could not figure out what the data actually is. */
    addresses[tokenGamestate1] = readMemory(0x4024f40, 4);
    addresses[tokenGamestate2] = addresses[tokenGamestate1] + 0x04;
    qDebug("Addresses for gamestate1 and gamestate2: 0x%08llX and 0x%08llX", addresses[tokenGamestate1], addresses[tokenGamestate2]);

    /* Get the address for the loading icon. Notice that the address here is different of that in
     * the Windows binary. However, we still need to add 0x1c suggesting that the internal data
     * structure is the same. */
    addresses[tokenLoadingicon] = readMemory(0x4088510, 8) + 0x1c;
    qDebug("Address for the loading icon: 0x%016llX", addresses[tokenLoadingicon]);
    qDebug("Address for the loading icon: %llu", addresses[tokenLoadingicon]);

    /* Add all the constant addresses we have */
    addresses[tokenMission] = 0x402896f;
    addresses[tokenPause] = 0x383f571;
}

bool FluffelMemoryThread::openMemory() {
    /* Process ID = 0, so just simulate successfully opening the memory */
    if (processID == 0) {
        qDebug("Simulation memory");
        return true;
    }

    /* Close file if already open */
    if (memFile.isOpen()) {
        memFile.close();
    }

    /* Open the memory of the process */
    memFile.setFileName("/proc/" + QString::number(processID) + "/mem");

    return (memFile.open(QIODevice::ReadOnly));
}

qint64 FluffelMemoryThread::readMemory(qint64 address, int length) {
    /* Process ID = 0, so we are randomly generating data here */
    if (processID == 0) {
        return random() % 257;
    }

    /* Length should be not exceeding our "buffer" value */
    if (length > sizeof(qint64)) {
        return 0;
    }

    /* No file opened? */
    if (!memFile.isOpen()) {
        qDebug("Memory is not opened");
        return 0;
    }

    /* Otherwise, read from memory address given */
    if (!memFile.seek(address)) {
        qDebug("Memory could not be seeked");
        return 0;
    }

    qint64 value = 0;
    int bytesRead = 0;


    bytesRead = memFile.read((char*)(&value), length);

    if (bytesRead != length) {
        qDebug("read %d bytes instead of %d. value: %llu", bytesRead, length, value);
        return 0;
    }

    /* Only return value if fully read */
    return value;
}

void FluffelMemoryThread::updateData(const gameData& newdata) {
    accessMutex.lock();
    internalData.gamestate = newdata.gamestate;
    internalData.mission = newdata.mission;
    internalData.loading = newdata.loading;
    accessMutex.unlock();
}

FluffelMemoryThread::gameData FluffelMemoryThread::getData() const {
    return internalData;
}

