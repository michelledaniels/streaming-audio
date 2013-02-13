/**
 * @file osc.h
 * Interface for Open Sound Control (OSC) functionality
 * @author Michelle Daniels
 * @date May 2012
 * @copyright UCSD 2012
 */

#ifndef OSC_H
#define OSC_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVector>


#define OSC_INT qint32      ///< An OSC integer type
#define OSC_TIME quint64    ///< An OSC time type
#define OSC_FLOAT float     ///< An OSC float type
#define OSC_STRING char*    ///< An OSC string type
#define OSC_BLOB void*      ///< An OSC blob type

static const char SLIP_END = 192;                    ///< Signals the end of a SLIP-encoded sequence
static const char SLIP_ESC = 219;                    ///< SLIP escape character
static const char SLIP_ESC_END[2] = {SLIP_ESC, 220}; ///< the escaped SLIP end character
static const char SLIP_ESC_ESC[2] = {SLIP_ESC, 221}; ///< the escaped SLIP escape character


/**
 * @union OscArgVal
 * This union represents the value of an OSC argument.
 */
union OscArgVal
{
    OSC_INT i;      ///< An OSC 32-bit integer type
    OSC_FLOAT f;    ///< An OSC 32-bit floating point type
    OSC_STRING s;   ///< An OSC string
};


/**
 * @struct OscArg
 * This struct represents an OSC argument.
 */
struct OscArg
{
    char type;      ///< The argument's type
    OscArgVal val;  ///< The argument's value
};


/**
 * @struct OscAddress
 * This struct represents an OSC address.
 */
struct OscAddress
{
    QHostAddress host;  ///< The host address to connect to
    quint16 port;       ///< The OSC port on the remote host
};

/**
 * @class OscMessage
 * @author Michelle Daniels
 * @date 2012
 *
 * An OscMessage encapsulates data related to OSC messages (OSC address,
 * type string, and argument list), and functionality including reading
 * and writing messages to byte arrays.
 *
 */
class OscMessage : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor.
     */
    OscMessage();

    /**
     * Destructor.
     */
    ~OscMessage();

    /**
     * Copy constructor (not used).
     */
    OscMessage(const OscMessage&);

    /**
     * Assignment operator (not used).
     */
    OscMessage& operator=(const OscMessage&);

    /**
     * Clear all of this OscMessage's data.
     */
    void clear();

    /**
     * Initialize this OscMessage.
     * @param address the OSC address for this message
     * @param types the NULL-terminated OSC type string (NULL signals no arguments)
     * @param ... a variable-length list of OSC arguments.
     * The number of arguments must equal the length of the provided type string.
     * @return true on success, false on failure
     */
    bool init(const char* address, const char* types = NULL, ...);

    /**
     * Add an OSC 32-bit int argument to this OscMessage.
     * @param val the value of the argument to be added
     * @return true on success, false on failure
     */
    bool addIntArg(OSC_INT val);

    /**
     * Add an OSC 32-bit float argument to this OscMessage.
     * @param val the value of the argument to be added
     * @return true on success, false on failure
     */
    bool addFloatArg(OSC_FLOAT val);

    /**
     * Add an OSC string argument to this OscMessage.
     * Makes a deep copy of the given string
     * @param val the value of the argument to be added
     * @return true on success, false on failure
     */
    bool addStringArg(OSC_STRING val);

    /**
     * Read an OSC message from a byte array.
     * @param data the array of bytes containing the OscMessage data
     * @return true on success, false on failure
     * @see read
     */
    bool read(QByteArray& data);

    /**
     * Write an OSC message to a byte array.
     * @param data the array of bytes to which the message data will be written
     * @return true on success, false on failure
     * @see write
     */
    bool write(QByteArray& data);

    /**
     * Get this OscMessage's OSC address string.
     * @return A pointer to the OSC address string (must not be modified!)
     */
    const char* getAddress() { return m_address.data(); }

    /**
     * Get the number of OSC arguments this message contains.
     * @return the number of OSC arguments in this message
     */
    int getNumArgs() { return m_args.size(); }

    /**
     * Get one of this message's OSC arguments.
     * @param index the index of the desired argument
     * @param arg the OscArg struct that will be populated
     * Note - no deep copy is made of OSC string data, so the caller
     * must not free a returned OSC_STRING argument.
     * @return true on success, false on failure
     */
    bool getArg(int index, OscArg& arg); // TODO: caller must not free string!!

    /**
     * Print the contents of the OSC message to the console.
     */
    void print();

    /**
     * Check if a type string matches this OscMessage's types.
     * A match occurs if both type strings are identical (same values and number of arguments).
     * @param type the type string to compare against
     * @return true if the type strings match, false otherwise.
     */
    bool typeMatches(const char* type);

    /**
     * Encodes a byte array using double-ended SLIP (RFC 1055).
     * SLIP is used to signify the end of a serially-transmitted message
     * @param data the data to be encoded and replaced
     */
    static void slipEncode(QByteArray& data);
    
    /**
     * Decodes a byte array encoded using SLIP (RFC 1055).
     * SLIP is used to specify the end of a serial message.
     * @param data the encoded data to be replaced by decoded data
     */
    static void slipDecode(QByteArray& data);
    
private:
    QByteArray m_address;   ///< The OSC address string
    QByteArray m_type;      ///< The OSC type string (provided for convenience only, since the types are also represented in the OscArgs)
    QVector<OscArg> m_args; ///< The OSC arguments
};

/**
 * @class OscClient
 * An OscClient sends OSC messages using UDP or TCP.
 */
class OscClient : public QObject
{
    Q_OBJECT
public:
    
    /**
     * Constructor.
     */
    OscClient(QObject* parent = NULL);
    
    /**
     * Destructor.
     */
    ~OscClient();

    /**
     * Copy constructor (not used).
     */
    OscClient(const OscClient&);

    /**
     * Assignment operator (not used).
     */
    OscClient& operator=(const OscClient&);

    /**
     * Initialize this client and connect to remote host.
     * Once initialized, this OscClient can send messages until disconnected.
     * @param host the remote host to connect to
     * @param port the OSC port for the remote host
     * @param socketType the type of connection to use (QAbstractSocket::TcpSocket or QAbstractSocket::UdpSocket)
     * @return true on success, false otherwise
     */
    bool init(QString& host, quint16 port, QAbstractSocket::SocketType socketType);
    
    /**
     * Disconnect this client from the remote host.
     * @return true on success, false otherwise
     */
    bool disconnect();
    
    /**
     * Send an OSC message to the remote host.
     * @param msg the OscMessage to send
     * @return true on success, false otherwise (not connected, etc.)
     */
    bool send(OscMessage* msg);
    
    /**
     * Send an OSC message to the remote host using TCP.
     * This method connects to the remote host, sends the message, and disconnects.
     * Calling it multiple times results in new connections each time.  To maintain
     * a connection across multiple messages, use the non-static functionality of OscClient.
     * @param msg the OscMessage to send
     * @param dest the remote host and port to send to
     * @return true on success, false otherwise
     * @see sendUdp
     */
    static bool sendTcp(OscMessage* msg, OscAddress* dest);
    
    /**
     * Send an OSC message to the remote host using UDP.
     * @param msg the OscMessage to send
     * @param dest the remote host and port to send to
     * @return true on success, false otherwise
     * @see sendTcp
     */
    static bool sendUdp(OscMessage* msg, OscAddress* dest);
    
    /**
     * Send an OSC message to the remote host.
     * @param msg the OscMessage to send
     * @param socket the socket to send from (must already be connected to the remote host)
     * @return true on success, false otherwise
     */
    static bool sendFromSocket(OscMessage* msg, QAbstractSocket* socket);
    
private:

    QAbstractSocket* m_socket; ///< a TCP or UDP socket used to send messages
};

/**
 * @class OscTcpSocketReader
 * An OscTcpSocketReader reads OCS messages from a TCP socket.
 */
class OscTcpSocketReader : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    OscTcpSocketReader(QTcpSocket* socket);

    /**
     * Destructor.
     */
    virtual ~OscTcpSocketReader();

    /**
     * Copy constructor (not used).
     */
    OscTcpSocketReader(const OscTcpSocketReader&);

    /**
     * Assignment operator (not used).
     */
    OscTcpSocketReader& operator=(const OscTcpSocketReader);

signals:
    /**
     * Signals when a complete OSC message has been received.
     * @param msg the OscMessage received
     * @param sender the remote host that sent the message
     * @param socket the socket the message was received through
     */
    void messageReady(OscMessage* msg, const char* sender, QAbstractSocket* socket);

public slots:
    /**
     * Reads data from a QTcpSocket.
     */
    void readFromSocket();

protected:

    QTcpSocket* m_socket; ///< The TCP socket to read from
    QByteArray m_data;    ///< temporary storage for received data
};

/**
 * @class OscServer
 * An OscServer listens for OSC messages using both UDP and TCP.
 */
class OscServer : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    OscServer(quint16 port, QObject *parent = 0);

    /**
     * Destructor.  
     * Stops the server if running.
     */
    virtual ~OscServer();

    /**
     * Copy constructor (not used).
     */
    OscServer(const OscServer&);

    /**
     * Assignment operator (not used).
     */
    OscServer& operator=(const OscServer);

    /**
     * Get the server's port.
     * @return the server's port
     */
    quint16 getPort() const { return m_port; }
    
    /**
     * Start the server.
     * @return true if successfully started, false on error
     */
    bool start();
    
    /**
     * Stop the server.
     */
    void stop();
    
signals:
    /**
     * Signals when a complete OSC message has been received.
     * @param msg the OscMessage received
     * @param sender the remote host that sent the message
     */
    void messageReady(OscMessage* msg, const char* sender);

protected slots:
    /**
     * Handle a complete OSC message received from a QTcpSocketReader.
     * @param msg the OscMessage received
     * @param sender the remote host that sent the message
     */
    void handleOscMessage(OscMessage* msg, const char* sender);

    /**
     * Handle a new incoming TCP connection.
     */
    void handleTcpConnection();

    /**
     * Read a newly-received UDP datagram.
     */
    void readPendingDatagrams();

protected:

    QTcpServer* m_tcpServer; ///< The server listening for incoming TCP connections
    QUdpSocket* m_udpSocket; ///< The socket receiving incoming UDP datagrams
    quint16 m_port;          ///< The port to listen on
};

#endif // OSC_H
