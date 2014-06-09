/**
 * @file samrenderer.h
 * SAM renderer library interface
 * @author Michelle Daniels
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef SAMRENDERER_H
#define SAMRENDERER_H

#include "libsamrender_global.h"
#include "../osc.h"

/**
 * @namespace sam
 * Namespace for SAM-related functionality.
 */
namespace sam
{

static const unsigned int SAMRENDER_DEFAULT_TIMEOUT = 10000; // in milliseconds

/**
 * @enum SamRenderReturn
 * The possible return codes for SamRenderer methods.
 */
enum SamRenderReturn
{
    SAMRENDER_SUCCESS = 0,    ///< Success
    SAMRENDER_REQUEST_DENIED, ///< A SAM request was denied (i.e. registration or changing type failed)
    SAMRENDER_NOT_REGISTERED, ///< Attempted to send a request to SAM before registering
    SAMRENDER_OSC_ERROR,      ///< An error occurred trying to send an OSC message to SAM or receive one from SAM
    SAMRENDER_TIMEOUT,        ///< A request to SAM timed out waiting for a response
    SAMRENDER_ERROR,          ///< An error occurred that doesn't fit one of the above codes
    SAMRENDER_NUM_RETURN_CODES
};

/**
 * @struct SamRenderParams
 * This struct contains the parameters needed to intialize a SamRenderer
 */
struct SamRenderParams
{
    SamRenderParams() :
        samIP(NULL),
        samPort(0),
        replyIP(NULL),
        replyPort(0)
    {}

    char* samIP;          ///< IP address of SAM to connect to
    quint16 samPort;      ///< Port on which SAM receives OSC messages
    char* replyIP;        ///< local IP address from which to send and receive OSC messages
    quint16 replyPort;    ///< Local port for receiving OSC message replies (or 0 to have port assigned internally)
};

/**
 * @struct SamRenderStream
 * This struct contains the parameters describing a stream to be renderered.
 */
struct SamRenderStream
{
    SamRenderStream() :
        id(0),
        renderType(0),
        renderPreset(0),
        numChannels(0),
        channels(NULL)
    {}

    int id;             ///< unique ID of this stream
    int renderType;     ///< rendering type
    int renderPreset;   ///< rendering preset
    int numChannels;    ///< number of channels this stream contains
    int* channels;      ///< array with numChannels elements, containing the indices of the input channels this stream will arrive on
};

/**
 * @typedef StreamAddedCallback
 * Prototype for a StreamAddedCallback function.
 * This function is called whenever a new client stream is added to SAM.
 * @param stream reference to SamRenderStream struct containing stream parameters
 * @param arg pointer to user-supplied data
 */
typedef void(*StreamAddedCallback)(SamRenderStream& stream, void* arg);

/**
 * @typedef StreamRemovedCallback
 * Prototype for a StreamRemovedCallback function.
 * This function is called whenever a client stream is removed from SAM.
 * @param id unique ID for the stream to be removed
 * @param arg pointer to user-supplied data
 */
typedef void(*StreamRemovedCallback)(int id, void* arg);

/**
 * @typedef RenderPositionCallback
 * Prototype for a SamRenderer position change callback function.
 * This function is called when a stream's position changes
 * @param id unique ID for the stream whose position changed
 * @param x x-coordinate of stream
 * @param y y-coordinate of stream
 * @param width width of stream
 * @param height height of stream
 * @param depth depth of stream
 * @param arg pointer to user-supplied data
 */
typedef void(*RenderPositionCallback)(int id, int x, int y, int width, int height, int depth, void* arg);

/**
 * @typedef RenderTypeCallback
 * Prototype for a SamRenderer type change callback function.
 * This function is called when the type or preset for a stream changes
 * @param id the unique ID for the stream whose type/preset changed
 * @param type the stream's type
 * @param the stream's preset
 * @param arg pointer to user-supplied data
 */
typedef void(*RenderTypeCallback)(int id, int type, int preset, void* arg);

/**
 * @typedef RenderDisconnectCallback
 * Prototype for a SamRenderer disconect callback function.
 * This function is called when the connection with SAM is broken.
 * @param arg pointer to user-supplied data
 */
typedef void(*RenderDisconnectCallback)(void* arg);

/**
 * @class SamRenderer
 * @author Michelle Daniels
 * @date 2014
 * This class encapsulates the OSC message-based communication between a
 * Streaming Audio Manager (SAM) and a third-party renderer.
 */
class SamRenderer : public QObject
{
    Q_OBJECT

public:

    /**
      * SamRenderer default constructor.
      * After construction, initialize using init()
      */
    SamRenderer();

    /**
     * SamRenderer destructor.
     * @see SamRenderer
     */
    ~SamRenderer();

    /**
     * Copy constructor (not used).
     */
    SamRenderer(const SamRenderer&);

    /**
     * Assignment operator (not used).
     */
    SamRenderer& operator=(const SamRenderer);

    /**
     * Init this SamRenderer.
     * This must be called before registering.
     * @param params SamRenderParams struct of parameters
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure
     */
    int init(const SamRenderParams& params);

    /**
     * Register this renderer with SAM and block until response is received.
     * The renderer will stay registered with SAM until this SamRenderer object is deleted.
     * @param timeout response timeout time in milliseconds
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure
     */
    int start(unsigned int timeout = SAMRENDER_DEFAULT_TIMEOUT);

    /**
     * Add a rendering type and its presets.
     * This method informs SAM that the type and preset are available for use.
     * The "basic" type (id 0) with "default" preset (id 0) are always available
     * and therefore do not need to be added explcitly.
     * @param id the unique ID for the rendering type being added
     * @param name the user-readable name for the type
     * @param numPresets the number of presets this type has (including the required default)
     * @param presetIds an array of length numPresets containing the unique ID for each preset (including the required default)
     * @param presetNames an array of length numPresets containing the user-readable name for each preset (including the required default)
     */
    int addType(int id, const char* name, int numPresets, int* presetIds, const char** presetNames);

    /**
     * Subscribe to changes in position information for the stream with given ID.
     * @param id the unique identifier for the stream
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure
     */
    int subscribeToPosition(int id);

    /**
     * Unsubscribe to changes in position information for the stream with given ID.
     * The renderer is automatically unsubscribed when a stream is removed, so this is only
     * needed if the renderer wishes to unsubscribe while a stream is still active.
     * @param id the unique identifier for the stream
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure
     */
    int unsubscribeToPosition(int id);

    /**
     * Set stream added callback.
     * @param callback the function this renderer will call when a stream is added
     * @param arg each time the callback is called it will pass this as an argument
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure (will fail if a callback has already been set)
     */
    int setStreamAddedCallback(StreamAddedCallback callback, void* arg);

    /**
     * Set stream removed callback.
     * @param callback the function this renderer will call when a stream is removed
     * @param arg each time the callback is called it will pass this as an argument
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure (will fail if a callback has already been set)
     */
    int setStreamRemovedCallback(StreamRemovedCallback callback, void* arg);

    /**
     * Set position change callback.
     * @param callback the function this renderer will call when a stream's position changes
     * @param arg each time the callback is called it will pass this as an argument
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure (will fail if a callback has already been set)
     */
    int setPositionCallback(RenderPositionCallback callback, void* arg);

    /**
     * Set type change callback.
     * @param callback the function this renderer will call when a stream's type or preset changes
     * @param arg each time the callback is called it will pass this as an argument
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure (will fail if a callback has already been set)
     */
    int setTypeCallback(RenderTypeCallback callback, void* arg);

    /**
     * Set disconnect callback.
     * @param callback the function this renderer will call if the connection with SAM is lost
     * @param arg each time the callback is called it will pass this as an argument
     * @return SAMRENDER_SUCCESS on success, a non-zero ::SamRenderReturn code on failure (will fail if a callback has already been set)
     */
    int setDisconnectCallback(RenderDisconnectCallback callback, void* arg);

signals:
    /**
     * Emitted when a response from SAM is received.
     * This signal is for use internal to the SamRenderer library, and users don't need to (and shouldn't) connect it to any slots.
     */
    void responseReceived();

private slots:
    /**
     * Handle an OSC message.
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     * @param socket the socket the message was received through
     */
    void handleOscMessage(OscMessage* msg, const char* sender, QAbstractSocket* socket);

    /**
     * Handle the case where we get disconnected from SAM.
     */
    void samDisconnected();

private:

    /**
     * Handle a /sam/regconfirm OSC message.
     */
    void handle_regconfirm();

    /**
     * Handle a /sam/regdeny OSC message.
     */
    void handle_regdeny(int errorCode);

    char* m_samIP;                    ///< SAM IP address
    quint16 m_samPort;                ///< port on which SAM listens for OSC messages
    bool m_registered;                ///< whether or not we've successfully registered with SAM

    // for OSC
    char* m_replyIP;
    quint16 m_replyPort;              ///< OSC port for this renderer
    QTcpSocket m_socket;              ///< Socket for sending and receiving OSC messages
    bool m_responseReceived;          ///< whether or not SAM responded to request
    OscTcpSocketReader* m_oscReader;  ///< OSC-reading helper

    // for callbacks
    StreamAddedCallback m_streamAddedCallback;      ///< the /sam/stream/add callback function
    void* m_streamAddedCallbackArg;                 ///< user-supplied data for /sam/stream/add callback
    StreamRemovedCallback m_streamRemovedCallback;  ///< the /sam/stream/remove callback function
    void* m_streamRemovedCallbackArg;               ///< user-supplied data for /sam/stream/remove callback
    RenderPositionCallback m_positionCallback;      ///< the /sam/val/position callback function
    void* m_positionCallbackArg;                    ///< user-supplied data for /sam/val/position callback
    RenderTypeCallback m_typeCallback;              ///< the /sam/val/type callback function
    void* m_typeCallbackArg;                        ///< user-supplied data for /sam/val/type callback
    RenderDisconnectCallback m_disconnectCallback;  ///< the disconnect callback function
    void* m_disconnectCallbackArg;                  ///< user-supplied data for disconnect callback

};

} // end of namespace sam

#endif // SAMRENDERER_H
