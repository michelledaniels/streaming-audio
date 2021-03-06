/**
 * @file sam.h
 * Streaming Audio Manager (SAM) interface
 * @author Michelle Daniels
 * @date September 2011
 * @license 
 * This software is Copyright 2011-2014 The Regents of the University of California. 
 * All Rights Reserved.
 *
 * Permission to copy, modify, and distribute this software and its documentation for 
 * educational, research and non-profit purposes by non-profit entities, without fee, 
 * and without a written agreement is hereby granted, provided that the above copyright
 * notice, this paragraph and the following three paragraphs appear in all copies.
 *
 * Permission to make commercial use of this software may be obtained by contacting:
 * Technology Transfer Office
 * 9500 Gilman Drive, Mail Code 0910
 * University of California
 * La Jolla, CA 92093-0910
 * (858) 534-5815
 * invent@ucsd.edu
 *
 * This software program and documentation are copyrighted by The Regents of the 
 * University of California. The software program and documentation are supplied 
 * "as is", without any accompanying services from The Regents. The Regents does 
 * not warrant that the operation of the program will be uninterrupted or error-free. 
 * The end-user understands that the program was developed for research purposes and 
 * is advised not to rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. THE UNIVERSITY OF
 * CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, 
 * AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 */

#ifndef SAM_H
#define	SAM_H

#include <QCoreApplication>
#include <QTcpServer>
#include <QUdpSocket>
#include <QVector>

#include "jack/jack.h"
#include "osc.h"
#include "sam_shared.h"

namespace sam
{

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
 * @struct ClientParams
 * This struct contains the parameters describing a client/app
 */
struct ClientParams
{
    int channels;
    float volume;
    bool mute;
    bool solo;
    float delayMillis;
    SamAppPosition pos;
    StreamingAudioType type;
    int preset;
};

/**
 * @struct RenderingPreset
 * This struct contains the parameters describing a rendering preset
 */
struct RenderingPreset
{
    int id;
    QString name;
};

/**
 * @struct RenderingType
 * This struct contains the parameters describing a rendering type
 */
struct RenderingType
{
    StreamingAudioType id;
    QString name;
    QList<RenderingPreset> presets;
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
class SamParams;

/**
 * @class StreamingAudioManager
 * @author Michelle Daniels
 * @date 2011-2013
 * 
 * The StreamingAudioManager is responsible for coordinating all streaming audio clients.
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
     * Check if an app id is valid (in range and initialized).
     * @param id the unique id in question
     * @return true if valid, false otherwise
     */
    bool idIsValid(int id);

    /**
     * Check if a rendering type is valid (has been added).
     * @param type the rendering type in question
     * @return true if valid, false otherwise
     */
    bool typeIsValid(StreamingAudioType type);

    /**
     * Check if a rendering type is valid (has been added).
     * @param type the rendering type in question
     * @param preset the id for the preset in question
     * @return true if valid, false otherwise
     */
    bool typeIsValidWithPreset(StreamingAudioType type, int preset);

    /**
     * Get the number of registered apps.
     * @return the number of registered apps
     */
    int getNumApps();

    /**
     * Get list of registered rendering types.
     * @return const list of registered rendering types
     */
    const QList<RenderingType>& getRenderingTypes() { return m_renderingTypes; }

    /**
     * Register a new app.
     * @param name the name of the app, to be used for displaying in a UI
     * @param channels number of output audio channels for this app
     * @param x initial x-coordinate of top left (?) corner of window
     * @param y initial y-coordinate of top left (?) corner of window
     * @param width initial width of corresponding SAGE window
     * @param height initial height of corresponding SAGE window
     * @param depth initial depth of corresponding SAGE window
     * @param type the app's rendering type
     * @param preset the rendering preset for this type
     * @param packetQueueSize number of packets to buffer in receiver, or -1 to use SAM default
     * @param socket the TCP socket through which the app/client connected to SAM
     * @param errCode if an error occurs, the SamErrorCode which best describes the error.  Otherwise undefined.
     * @return unique port for this stream or -1 on error
     */
    int registerApp(const char* name, int channels, int x, int y, int width, int height, int depth, sam::StreamingAudioType type, int preset, int packetQueueSize, QTcpSocket* socket, sam::SamErrorCode& errCode);

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
     * @param renderSocket the TCP socket for connection to renderer (or NULL if using UDP)
     * @return true on success, false on failure
     */
    bool registerRenderer(const char* hostname, quint16 port, QTcpSocket* renderSocket);

    /**
     * Unregister a renderer.
     * @return true on success, false on failure
     */
    bool unregisterRenderer();

    /**
     * Get the name of an app.
     * @param id the port/unique ID of the app to be queried
     * @return name on success, NULL on failure
     */
    const char* getAppName(int id);

    /**
     * Get the name of an app.
     * @param id the port/unique ID of the app to be queried
     * @param params the parameter struct to be populated
     * @return true on success, false on failure
     */
    bool getAppParams(int id, ClientParams& params);

    /**
     * Get the string that describes where to send OSC messages.
     */
    QString& getOscMessageString() { return m_oscDirections; }

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

    /**
     * Notify receivers of JACK xruns.
     */
    void notifyXrun();

public slots:

    /**
     * Start this StreamingAudioManager
     */
    int start();

    /**
     * Stop this StreamingAudioManager.
     * If this is not called, any forked processes might not be killed properly.
     * @see start();
     */
    void stop();

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
     * Set the rendering type and preset of an app.
     * @param port the port/unique ID of the app to be updated
     * @param type the unique id of the type to be set
     * @param preset the unique id of the preset to be set
     * @return true on success, false on failure (invalid port, etc.)
     */
    bool setAppType(int port, int type, int preset);

    /**
     * Get details about a rendering type from its id.
     * @param id the unique id for this rendering type
     * @return NULL if no type matching this id is found, otherwise pointer to a RenderingType struct describing this rendering type (caller must not free this - it is still owned by SAM)
     */
    const RenderingType* getType(int id);
     
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
     * Announce when it's time for OSC meter updates to be sent.
     */
    void meterTick();

    /**
     * Signal that meter levels have changed for a particular app
     */
    void appMeterChanged(int, int, float, float, float, float);
    
    /**
     * Signal that an error occurred on startup.
     */
    void startupError();

    /**
     * Signal that an xrun occurred.
     */
    void xrun();

    /**
     * Signal that a stop request has been received.
     */
    void stopConfirmed();

    void started();
    void stopped();

    void volumeChanged(float);
    void muteChanged(bool);
    void delayChanged(float);
    void appAdded(int);
    void appRemoved(int);
    void appVolumeChanged(int, float);
    void appMuteChanged(int, bool);
    void appSoloChanged(int, bool);
    void appDelayChanged(int, float);
    void appPositionChanged(int, int, int, int, int, int);
    void appTypeChanged(int, int, int);

    void typeAdded(int);
    void typeRemoved(int);
    
    void setAppTypeFailed(int, int, int, int, int, int);

private:
    
    /**
     * Set the rendering type for an app.
     * @param port the port/unique ID of the app to be updated
     * @param type the type to be set
     * @param preset the preset to be set
     * @param error code on failure, otherwise undefined
     * @return true on success, false on failure
     */
    bool set_app_type(int port, sam::StreamingAudioType type, int preset, sam::SamErrorCode& errorCode);

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
     * @param socket the socket the message was received through
     */
    void handle_render_message(const char* address, OscMessage* msg, const char* sender, QAbstractSocket* socket);

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
     * @param socket the socket the message was received through
     * @param unsubscribe true to unsubscribe, false to subscribe
     */
    void handle_subscribe_message(const char* address, OscMessage* msg, const char* sender, QAbstractSocket* socket, bool unsubscribe);

    /**
     * Handle requests about rendering types.
     * @param address the part of the OSC address string following "/sam/type"
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void handle_type_message(const char* address, OscMessage* msg, const char* sender);

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
     * Handle requests to set preset parameter
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_set_preset(OscMessage* msg, const char* sender);

    /**
     * Handle /sam/register OSC message.
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_register(OscMessage* msg, QTcpSocket* socket);

    /**
     * Handle requests to add a rendering type.
     * @param msg the OSC message to handle
     * @param sender the name of the host that sent the message
     */
    void osc_add_type(OscMessage* msg, const char* sender);

    /**
     * Handle requests to remove a rendering type.
     * @param msg the OSC message to handle
     */
    void osc_remove_type(OscMessage* msg);

    /**
     * JACK process callback
     * @param nframes the number of sample frames to process
     * @return 0 on success, non-zero on failure
     */
    int jack_process(jack_nframes_t nframes);
    
    /**
     * initialize basic output ports
     * @return true on success, false on failure
     */
    bool init_basic_output_ports();
    
    /**
     * initialize discrete output ports
     * @return true on success, false on failure
     */
    bool init_discrete_output_ports();

    /**
     * send a /sam/stream/add message.
     * @param app the app representing the stream to be added
     * @return true if sending is successful, false otherwise
     */
    bool send_stream_added(StreamingAudioApp* app);
    
    /**
     * send a /sam/type/add message.
     * @param type the rendering type that was added
     * @param address the address to send the message to
     * @return true if sending is successful, false otherwise
     */
    bool send_type_added(RenderingType& type, OscAddress& address);

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
     * @param type the rendering type for the app to be connected (basic type treated differently from discrete types)
     * @return true on success, false on failure
     */
    bool connect_app_ports(int port, const int* outputPorts, int type);
    
    /**
     * Disconnect app ports from physical outputs.
     * @param port the unique port for the app to be connected
     * @return true on success, false on failure
     */
    bool disconnect_app_ports(int port);

    /**
     * Print debug info to console.
     */
    void print_debug();

    /**
     * Check for version match.
     */
    bool version_check(int major, int minor, int patch);
    
    int m_sampleRate;                 ///< sample rate for JACK
    int m_bufferSize;                 ///< buffer size for JACK
    int m_numBasicChannels;           ///< number of basic channels
    int m_maxOutputChannels;          ///< max number of output channels to use
    char* m_jackDriver;               ///< driver for JACK to use
    pid_t m_jackPID;                  ///< PID for the jackd process
    jack_client_t* m_client;          ///< local JACK client
    int m_maxClients;                 ///< max number of clients that can simultaneously connect
    StreamingAudioApp** m_apps;       ///< pointers to active StreamingAudioApps
    SamAppState* m_appState;          ///< the state for all StreamingAudioApps in m_apps

    // TODO: do these flags need to be volatile?
    bool m_isRunning;                 ///< flag to see if SAM is running
    bool m_stopRequested;             ///< flag to see if there is a request to stop processing

    QList<unsigned int> m_basicChannels; ///< list of basic channels to use
    QList<unsigned int> m_discreteChannels; ///< list of discrete channels to use
    QList<RenderingType> m_renderingTypes;  ///< list of available rendering types

    unsigned int m_maxBasicOutputs;    ///< max number of output JACK ports for basic JACK client
    unsigned int m_maxDiscreteOutputs; ///< max number of output JACK ports for discrete JACK client
    int* m_discreteOutputUsed;         ///< which output ports are in use (by which app)
    quint16 m_rtpPort;                 ///< base port to use for RTP streaming
    char* m_outJackClientNameBasic;    ///< jack client name to which SAM will connect outputs
    char* m_outJackPortBaseBasic;      ///< base jack port name to which SAM will connect outputs
    char* m_outJackClientNameDiscrete; ///< jack client name to which SAM will connect outputs
    char* m_outJackPortBaseDiscrete;   ///< base jack port name to which SAM will connect outputs
    quint32 m_packetQueueSize;         ///< default client packet queue size
    qint32 m_clockSkewThreshold;       ///< number of samples of clock skew that must be measured before compensating

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
    int m_delayMaxClient;           ///< the maximum supported delay (in samples)
    int m_delayMaxGlobal;           ///< the maximum supported delay (in samples)

    // for OSC
    quint16 m_oscServerPort;        ///< port the OSC server will listen for messages on
    QUdpSocket* m_udpSocket;        ///< UDP socket for receiving OSC messages
    QTcpServer* m_tcpServer;        ///< The server listening for incoming TCP connections
    QHostAddress m_hostAddress;     ///< Local host address where OSC messages should be sent
    QString m_oscDirections;        ///< string that explains where to send OSC messages for SAM
    QTcpSocket* m_renderSocket;     ///< TCP socket for sending and listening to OSC messages to/from the renderer

    // for version checking
    bool m_verifyPatchVersion;      ///< Whether the patch version must match for version checking
}; 

} // end of namespace SAM

#endif // SAM_H

