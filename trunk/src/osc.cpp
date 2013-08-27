/**
 * @file osc.cpp
 * Implementation of Open Sound Control (OSC) functionality
 * @author Michelle Daniels
 * @date May 2012
 * @copyright UCSD 2012
 */

#include <QDebug>

#include <osc.h>

namespace sam
{

static const int CONNECT_TIMEOUT_MILLIS = 5000;
static const int DISCONNECT_TIMEOUT_MILLIS = 1000;

OscMessage::OscMessage() :
    QObject()
{

}

OscMessage::~OscMessage()
{
    clear();
}

void OscMessage::clear()
{
    m_address.clear();
    m_type.clear();
    for (int n = 0; n < m_args.size(); n++)
    {
        if (m_args[n].type == 's')
        {
            delete[] m_args[n].val.s;
            m_args[n].val.s = NULL;
        }
    }
    m_args.clear();
}

bool OscMessage::init(const char* address, const char* types, ...)
{
    if (!address) return false;
    clear();

    m_address.append(address);

    if (types)
    {
        int numArgs = strlen(types);

        va_list args;
        va_start(args, types);

        for (int n = 0; n < numArgs; n++)
        {

            OscArg arg;
            arg.type = types[n];
            switch (arg.type)
            {
            case 'i':
            {
                qint32 i = (qint32)va_arg(args, int);
                arg.val.i = i;
                break;
            }
            case 'f':
            {
                float f = (float)va_arg(args, double);
                arg.val.f = f;
                break;
            }
            case 's':
            {
                const char* s = va_arg(args, char*);
                int len = strlen(s);
                arg.val.s = new char[len + 1];
                strncpy(arg.val.s, s, len);
                arg.val.s[len] = '\0';
                break;
            }
            default:
                qDebug("Unrecognized argument type %c", arg.type);
                return false;
            }

            m_args.push_back(arg);
            m_type.append(arg.type);
        }

        va_end(args);
    }
    return true;
}

bool OscMessage::addIntArg(OSC_INT val)
{
    OscArg arg;
    arg.type = 'i';
    arg.val.i = val;
    m_args.push_back(arg);
    m_type.append(arg.type);
    return true;
}

bool OscMessage::addFloatArg(OSC_FLOAT val)
{
    OscArg arg;
    arg.type = 'f';
    arg.val.f = val;
    m_args.push_back(arg);
    m_type.append(arg.type);
    return true;
}

bool OscMessage::addStringArg(OSC_STRING val)
{
    OscArg arg;
    arg.type = 's';
    int len = strlen(val);
    arg.val.s = new char[len + 1];
    strncpy(arg.val.s, val, len);
    arg.val.s[len] = '\0';
    m_args.push_back(arg);
    m_type.append(arg.type);
    return true;
}

bool OscMessage::read(QByteArray& data)
{
    if (data.isEmpty()) return false;
    if (!data.startsWith('/')) return false;

    clear();

    int addrEnd = data.indexOf('\0');
    m_address.append(data.left(addrEnd));
    //qDebug() << "OscMessage::Read address = " << m_address;

    int delim = data.indexOf(',', addrEnd);
    if (delim < 0)
    {
        //qDebug() << "OscMessage::Read no arguments";
        return true; // no arguments
    }
    int typesStart = delim + 1;
    int typesEnd = data.indexOf('\0', typesStart);

    // advance to start of args
    int argStart = typesEnd + (4 - typesEnd % 4);

    int len = data.length();
    QDataStream in(&data, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::BigEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);
    // advance to start of args
    in.skipRawData(argStart);
    for (int n = typesStart; n < typesEnd; n++)
    {
        if (argStart >= len)
        {
            qWarning("OscMessage::Read Invalid OSC message: missing argument %d", n - typesStart);
            return false;
        }
        OscArg arg;
        arg.type = data[n];
        switch (arg.type)
        {
        case 'i': // int32
            if (argStart + 4 > len)
            {
                qWarning("OscMessage::Read Invalid OSC message: missing part of int argument %d", n);
                return false;
            }
            in >> arg.val.i;
            argStart += 4;
            //qDebug("Argument %d has type %c, value %d", n - typesStart, arg.type, arg.val.i);
            break;

        case 'f': // float32
            if (argStart + 4 > len)
            {
                qWarning("OscMessage::Read Invalid OSC message: missing part of float argument %d", n);
                return false;
            }
            in >> arg.val.f;
            argStart += 4;
            //qDebug("Argument %d has type %c, value %f", n - typesStart, arg.type, arg.val.f);
            break;

        case 's': // string
        {
            int strEnd = data.indexOf('\0', argStart);
            if (strEnd < 0)
            {
                qWarning("OscMessage::Read Invalid OSC message: string argument %d not null-terminated", n);
                return false;
            }
            arg.val.s = new char[strEnd - argStart + 1];
            in.readRawData(arg.val.s, strEnd - argStart + 1);
            while (strEnd % 4 != 3)
            {
                if (data[strEnd] != '\0')
                {
                    qWarning("OscMessage::Read Invalid OSC message: string argument %d not null-padded", n);
                    delete[] arg.val.s;
                    return false;
                }
                strEnd++;
                in.skipRawData(1);
            }
            argStart = strEnd + 1;
            //qDebug("Argument %d has type %c, value %s", n - typesStart, arg.type, arg.val.s);
            break;
        }
        case 'b':
            qWarning("OscMessage::Read OSC blob type not supported at this time");
            return false;
        }
        m_args.push_back(arg);
        m_type.append(arg.type);
    }
    if (argStart < data.length())
    {
        qWarning("OscMessage::Read more arguments than there are types in the type string");
        return false;
    }

    return true;
}

bool OscMessage::write(QByteArray& data)
{
    if (m_address.isEmpty()) return false;

    data.clear();

    // write address
    data.append(m_address);
    data.append('\0');
    // pad with zeros
    while (data.length() % 4 != 0)
    {
        data.append('\0');
    }

    // write type tag
    data.append(',');
    data.append(m_type);
    data.append('\0');

    // pad with zeros
    while (data.length() % 4 != 0)
    {
        data.append('\0');
    }

    if (m_args.isEmpty()) return true; // no arguments

    // write args
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);
    out.skipRawData(data.length());
    int numArgs = m_args.size();
    for (int n = 0; n < numArgs; n++)
    {
        switch (m_args[n].type)
        {
        case 'i':
            out << m_args[n].val.i;
            break;

        case 'f':
            out << m_args[n].val.f;
            break;

        case 's':
        {
            // TODO: check that s isn't NULL?
            int stringLength = strlen(m_args[n].val.s);
            int result = out.writeRawData(m_args[n].val.s, stringLength);
            if (result != stringLength)
            {
                qDebug("could only write %d out of %d bytes", result, stringLength);
            }
            result = out.writeRawData("\0", 1);
            // pad with zeros
            while (data.length() % 4 != 0)
            {
                result = out.writeRawData("\0", 1);
            }
            break;
        }
        default:
            qWarning("Unknown argument type %c", m_args[n].type);
            return false;
        }
    }

    return true;
}

bool OscMessage::getArg(int index, OscArg& arg) // TODO: caller must not free string!!
{
    if (index < 0 || index >= m_args.size()) return false;
    arg = m_args[index];
    return true;
}

void OscMessage::print()
{
    printf("OscMessage: address = %s, %d arguments\n", m_address.data(), m_args.size());
    for (int n = 0; n < m_args.size(); n++)
    {
        switch (m_args[n].type)
        {
        case 'i':
            printf("Argument %d has type %c, value %d\n", n, m_args[n].type, m_args[n].val.i);
            break;

        case 'f':
            printf("Argument %d has type %c, value %f\n", n, m_args[n].type, m_args[n].val.f);
            break;

        case 's':
        {
            printf("Argument %d has type %c, value %s\n", n, m_args[n].type, m_args[n].val.s);
            break;

        }
        default:
            printf("Argument %d has type %c, value unknown (unrecognized type)\n", n, m_args[n].type);
            break;
        }
    }
}

bool OscMessage::typeMatches(const char* type)
{
    if ((int)qstrlen(type) != m_type.size()) return false;
    return m_type.startsWith(type);
}

void OscMessage::slipEncode(QByteArray& data)
{
    data.replace(&SLIP_ESC, 1, SLIP_ESC_ESC, 2);
    data.replace(&SLIP_END, 1, SLIP_ESC_END, 2);
}

void OscMessage::slipDecode(QByteArray& data)
{
    data.replace(SLIP_ESC_END, 2, &SLIP_END, 1);
    data.replace(SLIP_ESC_ESC, 2, &SLIP_ESC, 1);
}

OscClient::OscClient(QObject* parent) :
    QObject(parent),
    m_socket(NULL)
{
    
}

OscClient::~OscClient()
{
    if (m_socket)
    {
        delete m_socket;
        m_socket = NULL;
    }
}
    
bool OscClient::init(QString& host, quint16 port, QAbstractSocket::SocketType socketType)
{
    m_socket = new QAbstractSocket(socketType, NULL);
    m_socket->connectToHost(host, port, QIODevice::WriteOnly);
    if (!m_socket->waitForConnected(CONNECT_TIMEOUT_MILLIS))
    {
        qWarning("Couldn't connect to host: %s", m_socket->errorString().toAscii().data());
        return false;
    }
    return true;
}

bool OscClient::disconnect()
{
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState && !m_socket->waitForDisconnected(DISCONNECT_TIMEOUT_MILLIS))
    {
        qWarning("Couldn't disconnect from host: %s", m_socket->errorString().toAscii().data());
        return false;
    }
    return true;
}

bool OscClient::send(OscMessage* msg)
{
    return sendFromSocket(msg, m_socket);
}

bool OscClient::sendTcp(OscMessage* msg, OscAddress* dest)
{
    QTcpSocket socket;
    socket.connectToHost(dest->host, dest->port, QIODevice::WriteOnly);
    if (socket.state() != QAbstractSocket::ConnectedState && !socket.waitForConnected(CONNECT_TIMEOUT_MILLIS))
    {
        qWarning("OscClient::SendTcp Couldn't connect to host: %s", socket.errorString().toAscii().data());
        return false;
    }

    if (!sendFromSocket(msg, &socket))
    {
        qWarning("OscClient::SendTcp Couldn't send message");
        return false;
    }

    socket.disconnectFromHost();

    if (socket.state() != QAbstractSocket::UnconnectedState && !socket.waitForDisconnected(DISCONNECT_TIMEOUT_MILLIS))
    {
        qWarning("OscClient::SendTcp Couldn't disconnect from host: %s", socket.errorString().toAscii().data());
        return false;
    }
    return true;
}

bool OscClient::sendUdp(OscMessage* msg, OscAddress* dest)
{
    QUdpSocket socket;
    socket.connectToHost(dest->host, dest->port, QIODevice::WriteOnly);
    if (!socket.waitForConnected(CONNECT_TIMEOUT_MILLIS))
    {
        qWarning("OscClient::SendUdp Couldn't connect to host: %s", socket.errorString().toAscii().data());
        return false;
    }
    
    if (!sendFromSocket(msg, &socket))
    {
        qWarning("OscClient::SendUdp Couldn't send message");
        return false;
    }
    
    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState && !socket.waitForDisconnected(DISCONNECT_TIMEOUT_MILLIS))
    {
        qWarning("OscClient::SendUdp Couldn't disconnect from host: %s", socket.errorString().toAscii().data());
        return false;
    }
    return true;
}

bool OscClient::sendFromSocket(OscMessage* msg, QAbstractSocket* socket)
{
    if (!socket || !msg) return false;
    
    QByteArray msgBytes;
    bool success = msg->write(msgBytes);
    if (!success)
    {
        qWarning("OscClient::sendFromSocket Couldn't write OSC message");
        return false;
    }
    //qDebug() << "OscClient::sendFromSocket Message to send has " << msgBytes.length() << " bytes: " << msgBytes;

    /*printf("message bytes:\n");
    for (int i = 0; i < msgBytes.length(); i++)
    {
        printf("char %d = %c\n", i, msgBytes.at(i));
    }*/

    if (socket->socketType() == QAbstractSocket::TcpSocket)
    {
        //qDebug("OscClient::sendFromSocket slipifying TCP message");
        OscMessage::slipEncode(msgBytes);
        msgBytes.prepend(SLIP_END);
        msgBytes.append(SLIP_END);
    }
    qint64 n = socket->write(msgBytes);
    if (n != msgBytes.length())
    {
        qWarning("OscClient::sendFromSocket Only %lld out of %d bytes written", n, msgBytes.length());
        return false;
    }
    /*else
    {
        qDebug("OscClient::sendFromSocket Message written successfully");
    }*/
    socket->flush();  
    return true;
}

OscTcpSocketReader::OscTcpSocketReader(QTcpSocket* socket) :
    QObject(),
    m_socket(socket)
{
}

OscTcpSocketReader::~OscTcpSocketReader()
{
    /*if (m_socket)
    {
        m_socket->deleteLater();
        m_socket = NULL;
    }*/
}

void OscTcpSocketReader::readFromSocket()
{
    qWarning("OscTcpSocketReader::readFromSocket");
    QTcpSocket* socket = (QTcpSocket*)QObject::sender();
    if (socket != m_socket)
    {
        qWarning("OscTcpSocketReader::readFromSocket socket mismatch!");
    }
    //int available = m_socket->bytesAvailable();
    //qDebug() << "reading from TCP socket " << m_socket << ", " << available << " bytes available";

    QByteArray block = m_socket->readAll();
    int start = block.startsWith(SLIP_END) ? 1 : 0;
    if (m_data.isEmpty() && (start == 0))
    {
        qWarning("Invalid message fragment received: %s", block.constData());
        return; // valid OSC messages must start with SLIP_END
    }

    if (start > 0)
    {
        m_data.clear();
    }

    int len = block.length();
    while (start < len)
    {
        int end = block.indexOf(SLIP_END, start);
        //qDebug() << "start = " << start << ", end = " << end;
        if (start == end)
        {
            start++;
            continue;
        }
        if (end < 0)
        {
            // partial OSC message
            QByteArray msg = block.right(len - start);
            OscMessage::slipDecode(msg);
            m_data.append(msg);
            qDebug() << "TCP partial message received = " << QString(m_data);
            break;
        }
        else
        {
            // end of an OSC message
            QByteArray msg = block.mid(start, end - start);
            OscMessage::slipDecode(msg);
            m_data.append(msg);
            
            OscMessage* oscMsg = new OscMessage();
            bool success = oscMsg->read(m_data);
            if (!success)
            {
                qDebug("Couldn't read OSC message");
            }
            else
            {
                emit messageReady(oscMsg, m_socket->peerAddress().toString().toAscii().data(), socket);
            }

            start = end + 1;
            m_data.clear(); // prepare for new message
        }
    }
}

OscServer::OscServer(quint16 port, QObject *parent) :
    QObject(parent),
    m_tcpServer(NULL),
    m_udpSocket(NULL),
    m_port(port)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(handleTcpConnection()));

    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

OscServer::~OscServer()
{
    stop();
    
    if (m_tcpServer)
    {
        delete m_tcpServer;
        m_tcpServer = NULL;
    }

    if (m_udpSocket)
    {
        delete m_udpSocket;
        m_udpSocket = NULL;
    }
}


bool OscServer::start()
{
    if (!m_tcpServer->listen(QHostAddress::Any, m_port))
    {
        qWarning("OscServer::start(): TCP server couldn't listen on port %d", m_port);
        return false;
    }
    m_port = m_tcpServer->serverPort();
    qDebug("TCP server listening on port %d", m_port);

    if (!m_udpSocket->bind(m_port))
    {
        qWarning("OscServer::start(): UDP socket couldn't bind to port %d: %s", m_port, m_udpSocket->errorString().toAscii().data());
        return false;
    }
    qDebug("OscServer::start(): UDP socket binded successfully to port %d", m_port);
    
    return true;
}

void OscServer::stop()
{
    if (m_tcpServer) m_tcpServer->close();
    if (m_udpSocket) m_udpSocket->close();
}

void OscServer::handleTcpConnection()
{
    qDebug("OscServer::handleTcpConnection");
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
    int available = socket->bytesAvailable();
    qDebug() << "handling connection for socket " << socket << ", " << available << " bytesAvailable";
    OscTcpSocketReader* reader = new OscTcpSocketReader(socket);
    connect(socket, SIGNAL(readyRead()), reader, SLOT(readFromSocket()));
    connect(socket, SIGNAL(disconnected()), reader, SLOT(deleteLater())); // TODO: need to make sure socket and reader get deleted somewhere!
    connect(reader, SIGNAL(messageReady(OscMessage*, const char*)), this, SLOT(handleOscMessage(OscMessage*, const char*)));
}

void OscServer::readPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        qDebug() << "UDP message = " << QString(datagram);

        OscMessage* oscMsg = new OscMessage();
        bool success = oscMsg->read(datagram);
        if (!success)
        {
            qDebug("Couldn't read OSC message");
        }
        else
        {
            emit messageReady(oscMsg, sender.toString().toAscii().data());
        }
    }
}

void OscServer::handleOscMessage(OscMessage* msg, const char* sender)
{
    emit messageReady(msg, sender);
}

} // end of namespace SAM

