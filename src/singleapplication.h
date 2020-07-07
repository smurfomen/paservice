#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H
#include <QtCore/QtGlobal>
#include <QtNetwork/QLocalSocket>

#ifndef QAPPLICATION_CLASS
  #define QAPPLICATION_CLASS QCoreApplication
#endif

#include QT_STRINGIFY(QAPPLICATION_CLASS)

class SingleApplicationPrivate;

/**
 * @brief The SingleApplication class handles multiple instances of the same
 * Application
 * @see QCoreApplication
 */
class SingleApplication : public QAPPLICATION_CLASS
{
    Q_OBJECT

    using app_t = QAPPLICATION_CLASS;

public:
    /**
     * @brief Mode of operation of SingleApplication.
     * Whether the block should be user-wide or system-wide and whether the
     * primary instance should be notified when a secondary instance had been
     * started.
     * @note Operating system can restrict the shared memory blocks to the same
     * user, in which case the User/System modes will have no effect and the
     * block will be user wide.
     * @enum
     */
    enum Mode {
        User                    = 1 << 0,
        System                  = 1 << 1,
        SecondaryNotification   = 1 << 2,
        ExcludeAppVersion       = 1 << 3,
        ExcludeAppPath          = 1 << 4
    };
    Q_DECLARE_FLAGS(Options, Mode)

    /**
     * @brief Intitializes a SingleApplication instance with argc command line
     * arguments in argv
     * @arg {int &} argc - Number of arguments in argv
     * @arg {const char *[]} argv - Supplied command line arguments
     * @arg {bool} allowSecondary - Whether to start the instance as secondary
     * if there is already a primary instance.
     * @arg {Mode} mode - Whether for the SingleApplication block to be applied
     * User wide or System wide.
     * @arg {int} timeout - Timeout to wait in milliseconds.
     * @note argc and argv may be changed as Qt removes arguments that it
     * recognizes
     * @note Mode::SecondaryNotification only works if set on both the primary
     * instance and the secondary instance.
     * @note The timeout is just a hint for the maximum time of blocking
     * operations. It does not guarantee that the SingleApplication
     * initialisation will be completed in given time, though is a good hint.
     * Usually 4*timeout would be the worst case (fail) scenario.
     * @see See the corresponding QAPPLICATION_CLASS constructor for reference
     */
    explicit SingleApplication( int &argc, char *argv[], bool allowSecondary = false, Options options = Mode::User, int timeout = 1000 );
    ~SingleApplication() override;

    /**
     * @brief Returns if the instance is the primary instance
     * @returns {bool}
     */
    bool isPrimary();

    /**
     * @brief Returns if the instance is a secondary instance
     * @returns {bool}
     */
    bool isSecondary();

    /**
     * @brief Returns a unique identifier for the current instance
     * @returns {qint32}
     */
    quint32 instanceId();

    /**
     * @brief Returns the process ID (PID) of the primary instance
     * @returns {qint64}
     */
    qint64 primaryPid();

    /**
     * @brief Returns the username of the user running the primary instance
     * @returns {QString}
     */
    QString primaryUser();

    /**
     * @brief Returns the username of the current user
     * @returns {QString}
     */
    QString currentUser();

    /**
     * @brief Sends a message to the primary instance. Returns true on success.
     * @param {int} timeout - Timeout for connecting
     * @returns {bool}
     * @note sendMessage() will return false if invoked from the primary
     * instance.
     */
    bool sendMessage( const QByteArray &message, int timeout = 100 );

Q_SIGNALS:
    void instanceStarted();
    void receivedMessage( quint32 instanceId, QByteArray message );

private:
    SingleApplicationPrivate *d_ptr;
    Q_DECLARE_PRIVATE(SingleApplication)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SingleApplication::Options)

#include <QSharedMemory>
#include <QLocalServer>
struct InstancesInfo {
    bool primary;
    quint32 secondary;
    qint64 primaryPid;
    quint16 checksum;
    char primaryUser[128];
};

struct ConnectionInfo {
    qint64 msgLen = 0;
    quint32 instanceId = 0;
    quint8 stage = 0;
};

class SingleApplicationPrivate : public QObject {
Q_OBJECT
public:
    enum ConnectionType : quint8 {
        InvalidConnection = 0,
        NewInstance = 1,
        SecondaryInstance = 2,
        Reconnect = 3
    };
    enum ConnectionStage : quint8 {
        StageHeader = 0,
        StageBody = 1,
        StageConnected = 2,
    };
    Q_DECLARE_PUBLIC(SingleApplication)

    SingleApplicationPrivate( class SingleApplication *q_ptr );
    ~SingleApplicationPrivate() override;

    QString getUsername();
    void genBlockServerName();
    void initializeMemoryBlock();
    void startPrimary();
    void startSecondary();
    void connectToPrimary(int msecs, ConnectionType connectionType );
    quint16 blockChecksum();
    qint64 primaryPid();
    QString primaryUser();
    void readInitMessageHeader(QLocalSocket *socket);
    void readInitMessageBody(QLocalSocket *socket);

    SingleApplication *q_ptr;
    QSharedMemory *memory;
    QLocalSocket *socket;
    QLocalServer *server;
    quint32 instanceNumber;
    QString blockServerName;
    SingleApplication::Options options;
    QMap<QLocalSocket*, ConnectionInfo> connectionMap;

public Q_SLOTS:
    void slotConnectionEstablished();
    void slotDataAvailable( QLocalSocket*, quint32 );
    void slotClientConnectionClosed( QLocalSocket*, quint32 );
};

#endif // SINGLEAPPLICATION_H
