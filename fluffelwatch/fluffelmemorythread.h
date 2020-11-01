#ifndef FLUFFELMEMORYTHREAD_H
#define FLUFFELMEMORYTHREAD_H

#include <QMutex>
#include <QFile>
#include <QThread>

class FluffelMemoryThread : public QThread {
  public:
    FluffelMemoryThread();
    ~FluffelMemoryThread();

    void run() override;

    int getProcessID() const;
    void setProcessID(int value);

    struct gameData {
        int gamestate = 0;
        int mission = 0;
        int loading = 0;
    };

    gameData getData() const;

    /* Some static function for convenience and interpretation */


  private:
    /* The process ID for Alien Isolation. An ID of zero means
     * simulation mode. */
    int processID = 0;

    /* Mutex for accessing the data */
    QMutex accessMutex;

    /* Memory file */
    QFile memFile;

    /* Memory addresses we need */
    enum tokens {
        tokenGamestate1 = 0,
        tokenGamestate2 = 1,
        tokenLoadingicon = 2,
        tokenPause = 3,
        tokenMission = 4,
        tokenNUMBER = 5
    };

    /* Only addresses for Pause and Mission number are fixed. Gamestate and loading icon
     * need to be read out manually */
    int addresses[tokenNUMBER] = {0, 0, 0, 0x383f571, 0x402896f };
    void initAddresses();

    /* Memory functions */
    bool openMemory();
    qint64 readMemory(qint64 address, int length);

    /* Gamedata */
    gameData internalData;
    void updateData(const gameData &newdata);
};

#endif // FLUFFELMEMORYTHREAD_H
