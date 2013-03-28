/**
 * @file sam.h
 * Streaming Audio Manager (SAM) interface
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2013
 */

#ifndef SAM_H
#define	SAM_H

#include <QCoreApplication>
#include <QTcpServer>
#include <QUdpSocket>
#include <QVector>

#include "jack/jack.h"
#include "sam_client.h"
#include "osc.h"

static const int MAX_APPS = 100;
static const int MAX_DELAY_MILLIS = 2000;

/**
 * @struct SamParams
 * This struct contains the parameters needed to intialize SAM
 */
struct SamParams
{
    int sampleRate;        ///< The sampling rate for JACK
    int bufferSize;        ///< The buffer size for JACK
    int numBasicChannels;  ///< The number of basic (non-spatialized) channels
    char* jackDriver;      ///< The driver for JACK to use
    quint16 oscPort;       ///< OSC server port
    quint16 rtpPort;       ///< Base JackTrip port
    int outputPortOffset;  ///< offset for starting output port/channel
    int maxOutputChannels; ///< the maximum number of output channels to use
    float volume;          ///< initial global volume
    float delayMillis;     ///< initial global delay in milliseconds
    float maxDelayMillis;  ///< maximum delay in milliseconds (half allocated for global delay, half for per-clientd delay)
    char* renderHost;      ///< host for the renderer
    quint16 renderPort;    ///< port for the renderer
    quint32 packetQueueSize; ///< default client packet queue size
    char* outputJackClientName; ///< jack client name to which SAM will connect outputs
    char* outputJackPortBase;   ///< base jack port name to which SAM will connect outputs
};

/**
 * @struct SamAppPosition
 * This struct contains position information for a SAGE app
 */
struct SamAppPosition
{
    int x;        ///< x-coordinate of top left corner of window
    int y;        ///< y-coordinate of top left corner of window
    int width;    ///< width of corresponding SAGE window
    int height;   ///< height of corresponding SAGE window
    int depth;    ///< depth of corresponding SAGE window
};

/**
 * @enum SamAppState
 * The possible states for an app.
 */
enum SamAppState
{
    AVAILABLE = 0,
    INITIALIZING,
    ACTIVE,
    CLOSING,
    NUM_APP_STATES
};

class StreamingAudioApp;

/**
 * @class StreamingAudioManager
 * @author Michelle Daniels
 * @date 2011
 * 
 * The StreamingAudioManager is responsible for coordinating all SAGE-related audio.
 */
class StreamingAudioManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * @param params the struct of SAM parameters
     */
    StreamingAudioManager(const SamParams& params);

    /**
     * Destructor.
     */
    ~StreamingAudioManager();
    
    /**
     * Copy constructor (not used).
     */
    StreamingAudioManager(const StreamingAudioManager&);

    /**
     * Assignment operator (not used).
     */
    StreamingAudioManager& operator=(const StreamingAudioManager&);

    /**
     * Start this StreamingAudioManager.
     * @return true on success, false on failure
     * @see stop()
     */
    bool start();

    /**
     * Stop this StreamingAudioManager.
     * If this is not called, any forked processes might not be killed properly.
     * @return true on success, false on failure
     * @see start();
     */
    bool stop();

    /**
     * Check if an app id is valid (in range and initialized).
     * @param id the unique id in question
     * @return true if valid, false otherwise
     */
    bool idIsValid(int id);

    /** 
     * Get the number of registered apps.
     * @return the number of registered apps
     */
    int getNumApps();
    
    /**
     * Register a new app.
     * @param name the name of the app, to be used for displaying in a UI
     * @param channels number of output audio channels for this app
     * @param x initial x-coordinate of top left (?) corner of window
     * @param y initial y-coordinate of top left (?) corner of window
     * @param width initial width of corresponding SAGE window
     * @param height initial height of corresponding SAGE window
     * @param depth initial depth of corresponding SAGE window
     * @param type the app's streaming type
     * @param socket the TCP socket through which the app/client connected to SAM
     * @return unique port for this stream or -1 on error
     */
    int registerApp(const char* name, int channels, int x, int y, int width, int height, int depth, sam::StreamingAudioType type, QTcpSocket* socket);

    /**
     * Unregister an app
     * @param port the unique port for the app to be unregistered
     * @return true on success, false on failure
     */
    bool unregisterApp(int port);
    
    /**
     * Register a new UI.
     * @param host the host address of the UI
     * @param port the OSC port of the UI
     * @return true on success, false on failure
     */
    bool registerUI(const char* host, quint16 port);

    /**
     * Unregister a UI.
     * @param host the host address of the UI
     * @param port the OSC port of the UI
     * @return true on success, false on failure
     */
    bool unregisterUI(const char* host, quint16 port);
    
    /**
     * Register a new renderer.
     * @param hostname the hostname of the renderer
     * @param port the OSC port of the renderer
     * @return true on success, false on failure
     */
    bool registerRenderer(const char* hostname, quint16 port);

    /**
     * Unregister a renderer.
     * @return true on success, false on failure
     */
    bool unregisterRenderer();

    /**
     * Set the global volume level.
     * @param volume the volume level to be set, in the range [0.0, 1.0]
     */
    void setVolume(float volume);

    /**
     * Set the global mute status.
     * @param mute true if SAM is to be muted, false otherwise
     */
    void setMute(bool mute);

    /**
     * Set the global delay.
     * @param delay to be set (non-negative), in milliseconds
     */
    void setDelay(float delay);

    /**
     * Enable/disable an output channel
     * @param ch the channel (1-indexed) to be enabled/disabled
     * @param enabled true if the channel is to be enabled, false otherwise
     */
    void setOutEnabled(int ch, bool enabled);
    
    /**
     * Set the volume level for an app.
     * @param port the port/unique ID of the app to be updated
     * @param volume the volume level to be set, in the range [0.0, 1.0]
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppVolume(int port, float volume);

    /**
     * Set the mute status for an app.
     * @param port the port/unique ID of the app to be updated
     * @param isMuted true if the app is to be muted, false otherwise
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppMute(int port, bool isMuted);

    /**
     * Set the solo status for an app.
     * @param port the port/unique ID of the app to be updated
     * @param isSolo true if the app is to be solo'd, false otherwise
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppSolo(int port, bool isSolo);

    /**
     * Set the delay for an app.
     * @param port the port/unique ID of the app to be updated
     * @param delay delay in milliseconds
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppDelay(int port, float delay);

    /**
     * Set the position of an app.
     * @param port the port/unique ID of the app to be updated
     * @param x the x-coordinate of the upper-left corner of the app's window
     * @param y the y-coordinate of the upper-left corner of the app's window
     * @param width the width of the app's window
     * @param height the height of the app's window
     * @param depth the depth of the app's window
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppPosition(int port, int x, int y, int width, int height, int depth);

    /**
     * Set the type for an app.
     * @param port the port/unique ID of the app to be updated
     * @param type the type to be set
     * @return -1 on success, or a non-zero error code on failure
     */
    int setAppType(int port, sam::StreamingAudioType type);

    /**
     * JACK buffer size changed callback.
     * JACK will call this callback if the system buffer size changes.
     */
    static int jackBufferSizeChanged(jack_nframes_t nframes, void* sam);

    /**
     * JACK process callback.
     * The process callback for this JACK application is called in a
     * special realtime thread once for each audio cycle.  
     * This is where muting, volume control, delay, and panning occurs.
     */
    static int jackProcess(jack_nframes_t nframes, void* sam);

    /**
     * JACK sample rate changed callback.
     * JACK will call this callback if the system sample rate changes.
     */
    static int jackSampleRateChanged(jack_nframes_t nframes, void* sam);

    /**
     * JACK xrun callback.
     * JACK will call this callback if an xrun occurs.
     */
    static int jackXrun(void* sam);

    /**
     * JACK shutdown callback.
     * JACK calls this callback if the server ever shuts down or
     * decides to disconnect the client.
     */
    static void jackShutdown(void* sam);

    /**
     * Query if SAM is running.
     * @return true if SAM is running, false otherwise
     */
    bool isRunning() { return m_isRunning; }


public slots:

    /**
     * Clean up before quitting the application.
     * This will shutdown the JACK server.
     */
    void doBeforeQuit();

    /**
     * Handle an OSC message.
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     * @param socket the socket the message was received through
     */
    void handleOscMessage(OscMessage* msg, const char* sender, QAbstractSocket* socket);
    
    /**
     * Handle a new incoming TCP connection.
     */
    void handleTcpConnection();

    /**
     * Read a newly-received UDP datagram.
     */
    void readPendingDatagrams();
    
    /**
     *  Close an app
     */
    void closeApp(int port);

    /**
     * Clean up an app that has unregistered.
     * @param port the unique port/ID for the app that unregistered
     * @param type the type of app that unregistered
     */
    void cleanupApp(int port, int type);

    /**
     * Send meter updates.
     */
    void notifyMeter();

signals:
    /**
     * Announce when it's time for meter updates to be sent.
     */
    void meterTick();
    
private:

    /**
     * Handle requests to register or unregister apps.
     * @param address the part of the OSC address string following "/sam/app"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     * @param socket the socket the message was received through
     */
    void handle_app_message(const char* address, OscMessage*msg, const char* sender, QAbstractSocket* socket);

    /**
     * Handle requests to register or unregister UIs.
     * @param address the part of the OSC address string following "/sam/ui"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_ui_message(const char* address, OscMessage*msg, const char* sender);

    /**
     * Handle requests to register or unregister a renderer.
     * @param address the part of the OSC address string following "/sam/render"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_render_message(const char* address, OscMessage* msg, const char* sender);

    /**
     * Handle requests to set parameter values.
     * @param address the part of the OSC address string following "/sam/set"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     * @param socket the socket the message was received through
     */
    void handle_set_message(const char* address, OscMessage* msg, const char* sender, QAbstractSocket* socket);

    /**
     * Handle requests to get parameter values.
     * @param address the part of the OSC address string following "/sam/get"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_get_message(const char* address, OscMessage* msg, const char* sender);

    /**
     * Handle requests to subscribe to parameters.
     * @param address the part of the OSC address string following "/sam/subscribe"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_subscribe_message(const char* address, OscMessage* msg, const char* sender);

    /**
     * Handle requests to unsubscribe from parameters.
     * @param address the part of the OSC address string following "/sam/unsubscribe"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_unsubscribe_message(const char* address, OscMessage* msg, const char* sender);

    /**
     * Start the JACK server.
     * @param sampleRate the audio sampling rate for JACK
     * @param bufferSize the audio buffer size for JACK
     * @param outChannels the max number of output channels
     * @param driver driver to use for JACK
     * @return true on success, false on failure
     * @see stop_jack
     */
    bool start_jack(int sampleRate, int bufferSize, int outChannels, const char* driver);

    /**
     * Stop the JACK server.
     * @return true on success, false on failure
     * @see start_jack
     */
    bool stop_jack();

    /**
     * Open the jack client.
     * @return true on success, false on failure
     * @see close_jack_client
     */
    bool open_jack_client();

    /**
     * Close the jack client.
     * @return true on success, false on failure
     * @see open_jack_client
     */
    bool close_jack_client();

    /**
     * Handle requests to set volume parameter
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_volume(OscMessage* msg, const char* sender);

    /**
     * Handle requests to set mute parameter (T/F)
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_mute(OscMessage* msg, const char* sender);

    /**
     * Handle requests to get solo parameter
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_get_solo(OscMessage* msg, const char* sender);

    /**
     * Handle requests to set solo parameter
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_solo(OscMessage* msg, const char* sender);

    /**
     * Handle requests to set delay parameter (in ms)
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_delay(OscMessage* msg, const char* sender);

    /**
     * Handle requests to set position parameters (x y w h d)
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_position(OscMessage* msg, const char* sender);

    /**
     * Handle requests to set type
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     * @param socket the socket the OSC message was sent to
     */
    void osc_set_type(OscMessage* msg, const char* sender, QAbstractSocket* socket);

    /**
     * Handle requests to set outputs enabled/disabled
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_outenabled(OscMessage* msg, const char* sender);

    /**
     * Handle /sam/register OSC message.
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_register(OscMessage* msg, QTcpSocket* socket);

    /**
     * JACK process callback
     * @param nframes the number of sample frames to process
     * @return 0 on success, non-zero on failure
     */
    int jack_process(jack_nframes_t nframes);
    
    /**
     * initialize physical output ports 
     * @return true on success, false on failure
     */
    bool init_physical_ports();
    
    /** 
     * send a /sam/stream/add message.
     * @param app the app representing the stream to be added
     * @param address the address to send the message to
     * @return true if sending is successful, false otherwise
     */
    bool send_stream_added(StreamingAudioApp* app, OscAddress* address);
    
    /**
     * Allocate physical output ports.
     * @param port the unique port for the app to be connected
     * @param channels the number of channels the app wants to connect 
     * @param the type of app
     * @return true on success, false on failure
     */
    bool allocate_output_ports(int port, int channels, sam::StreamingAudioType type);
    
    /**
     * Connect app ports to physical outputs.
     * @param port the unique port for the app to be connected
     * @param outputPorts an array of the output ports to connect this app to (one for each app channel)
     * @return true on success, false on failure
     */
    bool connect_app_ports(int port, const int* outputPorts);
    
    /**
     * Disconnect app ports from physical outputs.
     * @param port the unique port for the app to be connected
     * @return true on success, false on failure
     */
    bool disconnect_app_ports(int port);
    
    int m_sampleRate;                 ///< sample rate for JACK
    int m_bufferSize;                 ///< buffer size for JACK
    int m_numBasicChannels;           ///< number of basic channels
    int m_maxOutputChannels;          ///< max number of output channels to use
    char* m_jackDriver;               ///< driver for JACK to use
    pid_t m_jackPID;                  ///< PID for the jackd process
    jack_client_t* m_client;          ///< local JACK client
    StreamingAudioApp* m_apps[MAX_APPS];   ///< pointers to active StreamingAudioApps
    SamAppState m_appState[MAX_APPS]; ///< the state for all StreamingAudioApps in m_apps
    bool m_isRunning;                 ///< flag to see if SAM is running

    int m_numPhysicalPortsOut;        ///< number of physical output ports
    int* m_outputUsed;                ///< which physical output ports are in use (by which app)
    quint16 m_rtpPort;                ///< base port to use for RTP streaming
    int m_outputPortOffset;           ///< the first channel will be offset by this amount
    char* m_outputJackClientName;     ///< jack client name to which SAM will connect outputs
    char* m_outputJackPortBase;       ///< base jack port name to which SAM will connect outputs
    quint32 m_packetQueueSize;        ///< default client packet queue size

    // subscribers
    QVector<OscAddress*> m_uiSubscribers;   ///< list of subscribers to UI parameters
    OscAddress* m_renderer;                 ///< external audio renderer
    quint32 m_meterInterval;                ///< number of samples between meter updates
    qint64 m_nextMeterNotify;               ///< time when next meter updates should be sent (in samples)
    qint64 m_samplesElapsed;                ///< number of samples elapsed since audio callbacks started running

    // control parameters
    float m_volumeCurrent;          ///< the current volume
    float m_volumeNext;             ///< the requested volume
    bool m_muteCurrent;             ///< the current mute status
    bool m_muteNext;                ///< the requested mute status
    bool m_soloCurrent;             ///< the current solo status (true if any app is currently solo'd)
    int m_delayCurrent;             ///< the current delay (in samples)
    int m_delayNext;                ///< the requested delay (in samples)
    int m_delayMax;                 ///< the maximum support delay (in samples)

    // for OSC
    quint16 m_oscServerPort;        ///< port the OSC server will listen for messages on
    QUdpSocket m_udpSocket;         ///< UDP socket for receiving OSC messages
    QTcpServer m_tcpServer;         ///< The server listening for incoming TCP connections
}; 

#endif // SAM_H

