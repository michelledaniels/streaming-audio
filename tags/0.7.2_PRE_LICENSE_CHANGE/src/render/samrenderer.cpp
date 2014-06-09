/**
 * @file samrenderer.cpp
 * SAM renderer library implementation
 * @author Michelle Daniels
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "samrenderer.h"
#include "../sam_shared.h" // for version numbers

namespace sam
{

SamRenderer::SamRenderer() :
    QObject(),
    m_samIP(NULL),
    m_samPort(0),
    m_registered(false),
    m_replyIP(NULL),
    m_replyPort(0),
    m_responseReceived(false),
    m_oscReader(NULL),
    m_streamAddedCallback(NULL),
    m_streamAddedCallbackArg(NULL),
    m_streamRemovedCallback(NULL),
    m_streamRemovedCallbackArg(NULL),
    m_positionCallback(NULL),
    m_positionCallbackArg(NULL),
    m_typeCallback(NULL),
    m_typeCallbackArg(NULL),
    m_disconnectCallback(NULL),
    m_disconnectCallbackArg(NULL)
{
}

SamRenderer::~SamRenderer()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState)
    {
        // unregister with SAM
        if (m_registered)
        {
            OscMessage msg;
            msg.init("/sam/render/unregister");
            if (!OscClient::sendFromSocket(&msg, &m_socket))
            {
                qWarning("SamRenderer::~SamRenderer() Couldn't send OSC message");
            }
            m_registered = false;
        }

        m_socket.disconnect();
    }
    else
    {
        qWarning("SamRenderer::~SamRenderer() couldn't unregister from SAM because socket was already disconnected");
    }

    m_socket.close();

    if (m_oscReader)
    {
        delete m_oscReader;
        m_oscReader = NULL;
    }

    if (m_samIP)
    {
        delete[] m_samIP;
        m_samIP = NULL;
    }

    if (m_replyIP)
    {
        delete[] m_replyIP;
        m_replyIP = NULL;
    }
}

int SamRenderer::init(const SamRenderParams& params)
{
    if (!params.samIP)
    {
        qWarning("SamRenderer::init samIP must be specified");
        return SAMRENDER_ERROR;
    }

    // copy SAM IP address
    if (m_samIP)
    {
        delete[] m_samIP;
        m_samIP = NULL;
    }
    size_t len = strlen(params.samIP);
    m_samIP = new char[len + 1];
    strncpy(m_samIP, params.samIP, len + 1);

    // copy reply IP address
    if (params.replyIP)
    {
        if (m_replyIP)
        {
            delete[] m_replyIP;
            m_replyIP = NULL;
        }
        size_t len = strlen(params.replyIP);
        m_replyIP = new char[len + 1];
        strncpy(m_replyIP, params.replyIP, len + 1);
    }

    m_samPort = params.samPort;
    m_replyPort = params.replyPort;

    // need an instance of QCoreApplication for the event loop to run - check if one already exists
    if (!qApp)
    {
        int argc = 0;
        char ** argv = {0};
        (void) new QCoreApplication(argc, argv);
        qDebug("Instantiated QCoreApplication for event loop");
    }
    return SAMRENDER_SUCCESS;
}

int SamRenderer::start(unsigned int timeout)
{
    if (m_registered) return SAMRENDER_ERROR; // already registered
    if (!m_samIP) return SAMRENDER_ERROR; // not initialized yet

    printf("\n");
    printf("--------------------------------------\n");
    printf("SAM Renderer version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("Built on %s at %s\n", __DATE__, __TIME__);
    printf("Copyright UCSD 2014\n");
    printf("Connecting to SAM at IP %s, port %d\n", m_samIP, m_samPort);
    printf("--------------------------------------\n\n");

    // bind to local IP and port (Qt 5+ only)
    if (m_replyIP || m_replyPort > 0)
    {
#if QT_VERSION >= 0x050000
        if (m_replyIP)
        {
            QString replyIPStr(m_replyIP);
            QHostAddress replyIPAddr(replyIPStr);
            m_socket.bind(replyIPAddr, m_replyPort);
        }
        else
        {
            m_socket.bind(QHostAddress::Any, m_replyPort);
        }
#else
        qWarning("Couldn't bind to specified local IP and port.  Qt version 5.0+ required.");
        return SAMRENDER_ERROR;
#endif
    }

    QString samIPStr(m_samIP);
    m_socket.connectToHost(samIPStr, m_samPort);
    if (!m_socket.waitForConnected(timeout))
    {
        qWarning("SamRenderer::start() couldn't connect to SAM");
        return SAMRENDER_TIMEOUT;
    }

    m_replyPort = m_socket.localPort();

    // set up listening for OSC messages
    m_oscReader = new OscTcpSocketReader(&m_socket);
    connect(&m_socket, SIGNAL(readyRead()), m_oscReader, SLOT(readFromSocket()));
    connect(m_oscReader, SIGNAL(messageReady(OscMessage*, const char*, QAbstractSocket*)), this, SLOT(handleOscMessage(OscMessage*, const char*, QAbstractSocket*)));
    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(samDisconnected()));

    OscMessage msg;
    msg.init("/sam/render/register", "iiii", VERSION_MAJOR,
                                             VERSION_MINOR,
                                             VERSION_PATCH,
                                             m_replyPort);

    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("SamRenderer::start() Couldn't send OSC message");
        return SAMRENDER_OSC_ERROR;
    }

    // wait on response from SAM
    QEventLoop loop;
    connect(this, SIGNAL(responseReceived()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();

    if (!m_responseReceived)
    {
        qWarning("SamRenderer::start() timed out waiting for response to register request");
        return SAMRENDER_TIMEOUT;
    }
    else if (!m_registered)
    {
       return SAMRENDER_REQUEST_DENIED;
    }

    return SAMRENDER_SUCCESS;
}

int SamRenderer::addType(int id, const char* name, int numPresets, int* presetIds, const char** presetNames)
{
    OscMessage msg;
    msg.init("/sam/type/add", "isi", id, name, numPresets);
    for (int preset = 0; preset < numPresets; preset++)
    {
        msg.addIntArg(presetIds[preset]);
        msg.addStringArg(presetNames[preset]);
    }

    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("SamRenderer::addType() Couldn't send OSC message");
        return SAMRENDER_OSC_ERROR;
    }
    return SAMRENDER_SUCCESS;
}

int SamRenderer::subscribeToPosition(int id)
{
    OscMessage msg;
    msg.init("/sam/subscribe/position", "ii", id, m_replyPort);

    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("SamRenderer::subscribeToPosition() Couldn't send OSC message");
        return SAMRENDER_OSC_ERROR;
    }
    return SAMRENDER_SUCCESS;
}

int SamRenderer::unsubscribeToPosition(int id)
{
    OscMessage msg;
    msg.init("/sam/unsubscribe/position", "ii", id, m_replyPort);

    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("SamRenderer::unsubscribeToPosition() Couldn't send OSC message");
        return SAMRENDER_OSC_ERROR;
    }
    return SAMRENDER_SUCCESS;
}

int SamRenderer::setStreamAddedCallback(StreamAddedCallback callback, void* arg)
{
    if (m_streamAddedCallback) return SAMRENDER_ERROR; // callback already set
    m_streamAddedCallback = callback;
    m_streamAddedCallbackArg = arg;

    return SAMRENDER_SUCCESS;
}

int SamRenderer::setStreamRemovedCallback(StreamRemovedCallback callback, void* arg)
{
    if (m_streamRemovedCallback) return SAMRENDER_ERROR; // callback already set
    m_streamRemovedCallback = callback;
    m_streamRemovedCallbackArg = arg;

    return SAMRENDER_SUCCESS;
}

int SamRenderer::setPositionCallback(RenderPositionCallback callback, void* arg)
{
    if (m_positionCallback) return SAMRENDER_ERROR; // callback already set
    m_positionCallback = callback;
    m_positionCallbackArg = arg;

    return SAMRENDER_SUCCESS;
}

int SamRenderer::setTypeCallback(RenderTypeCallback callback, void* arg)
{
    if (m_typeCallback) return SAMRENDER_ERROR; // callback already set
    m_typeCallback = callback;
    m_typeCallbackArg = arg;

    return SAMRENDER_SUCCESS;
}

int SamRenderer::setDisconnectCallback(RenderDisconnectCallback callback, void* arg)
{
    if (m_disconnectCallback) return SAMRENDER_ERROR; // callback already set
    m_disconnectCallback = callback;
    m_disconnectCallbackArg = arg;

    return SAMRENDER_SUCCESS;
}

void SamRenderer::samDisconnected()
{
    qWarning("SamRenderer SAM was disconnected");
    if (m_disconnectCallback)
    {
        m_disconnectCallback(m_disconnectCallbackArg);
    }
}

void SamRenderer::handleOscMessage(OscMessage* msg, const char*, QAbstractSocket*)
{
    // check prefix
    int prefixLen = 5; // prefix is "/sam/"
    const char* address = msg->getAddress();
    if (qstrncmp(address, "/sam/", prefixLen) != 0)
    {
        printf("Unknown OSC message:\n");
        msg->print();
        delete msg;
        return;
    }

    // check second level of address
    if (qstrncmp(address + prefixLen, "render", 6) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 6, "/regconfirm") == 0) // /sam/render/regconfirm
        {
            if (msg->typeMatches(""))
            {
                qDebug("Renderer received regconfirm from SAM");
                handle_regconfirm();
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 6, "/regdeny") == 0) // /sam/render/regdeny)
        {
            if (msg->typeMatches("i"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int errorCode = arg.val.i;
                qWarning("Renderer SAM registration DENIED: error code = %d", errorCode);
                handle_regdeny(errorCode);
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrncmp(address + prefixLen, "stream", 6) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 6, "/add") == 0) // /sam/stream/add
        {
            if (msg->typeStartsWith("iiii"))
            {
                SamRenderStream stream;
                OscArg arg;
                msg->getArg(0, arg);
                stream.id = arg.val.i;
                msg->getArg(1, arg);
                stream.renderType = arg.val.i;
                msg->getArg(2, arg);
                stream.renderPreset = arg.val.i;
                msg->getArg(3, arg);
                stream.numChannels = arg.val.i;
                stream.channels = new int[stream.numChannels];
                for (int ch = 0; ch < stream.numChannels; ch++)
                {
                    if (!msg->getArg(ch + 4, arg))
                    {
                        qWarning("Couldn't parse channel %d from stream added OSC message:", ch);
                        msg->print();
                        delete msg;
                        return;
                    }
                    else if (arg.type != 'i')
                    {
                        qWarning("Channel %d from stream added OSC message had type %c instead of i", ch, arg.type);
                        msg->print();
                        delete msg;
                        return;
                    }
                    stream.channels[ch] = arg.val.i;
                }
                qDebug("Received request to add stream with id = %d, type = %d, preset = %d, numChannels = %d", stream.id, stream.renderType, stream.renderPreset, stream.numChannels);

                // call stream added callback
                if (m_streamAddedCallback)
                {
                    m_streamAddedCallback(stream, m_streamAddedCallbackArg);
                }

                if (stream.channels)
                {
                    delete[] stream.channels;
                    stream.channels = NULL;
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 6, "/remove") == 0) // /sam/stream/remove)
        {
            if (msg->typeMatches("i"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int id = arg.val.i;
                qDebug("Received request to remove stream with id = %d", id);

                // call stream removed callback
                if (m_streamRemovedCallback)
                {
                    m_streamRemovedCallback(id, m_streamRemovedCallbackArg);
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrncmp(address + prefixLen, "val", 3) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 3, "/position") == 0) // /sam/val/position
        {
            if (msg->typeMatches("iiiiii"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int id = arg.val.i;
                msg->getArg(1, arg);
                int x = arg.val.i;
                msg->getArg(2, arg);
                int y = arg.val.i;
                msg->getArg(3, arg);
                int width = arg.val.i;
                msg->getArg(4, arg);
                int height = arg.val.i;
                msg->getArg(5, arg);
                int depth = arg.val.i;
                qDebug("Received message from SAM that position of stream with ID %d changed.\nx = %d, y = %d, width = %d, height = %d, depth = %d", id, x, y, width, height, depth);

                // call position callback
                if (m_positionCallback)
                {
                    m_positionCallback(id, x, y, width, height, depth, m_positionCallbackArg);
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 3, "/type") == 0) // /sam/val/type
        {
            if (msg->typeMatches("iii"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int id = arg.val.i;
                msg->getArg(1, arg);
                int type = arg.val.i;
                msg->getArg(2, arg);
                int preset = arg.val.i;
                qDebug("Received message from SAM that stream %d has type %d, preset %d", id, type, preset);

                // call type callback
                if (m_typeCallback)
                {
                    m_typeCallback(id, type, preset, m_typeCallbackArg);
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
    delete msg;
}

void SamRenderer::handle_regconfirm()
{
    printf("SamRenderer registration confirmed");

    m_responseReceived = true;
    m_registered = true;
    emit responseReceived();
}

void SamRenderer::handle_regdeny(int errorCode)
{
    qWarning("SamRenderer registration DENIED: error = %d", errorCode);

    m_responseReceived = true;
    emit responseReceived();
}

} // end of namespace sam
