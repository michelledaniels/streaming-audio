/**
 * @file sam_client.h
 * Interface for applications connecting to the Streaming Audio Manager
 * @author Michelle Daniels
 * @date 2012-2013
 * @copyright UCSD 2012-2013
 */

#ifndef SAM_CLIENT_H
#define	SAM_CLIENT_H

#include <QObject>
#include "osc.h"
#include "rtpsender.h"
#include "sac_audio_interface.h"

/**
 * @namespace sam
 * Namespace for SAM-related functionality.
 */
namespace sam
{
    
static const int VERSION_MAJOR = 0;
static const int VERSION_MINOR = 5;
static const int VERSION_PATCH = 10;
static const unsigned int SAC_DEFAULT_TIMEOUT = 10000; // in milliseconds

/**
 * @enum SamErrorCode
 * Possible error codes returned by SAM in OSC messages to clients, renderers, and UIs.
 */
enum SamErrorCode
{
    SAM_ERR_DEFAULT = 0,        ///< non-specific error
    SAM_ERR_VERSION_MISMATCH,   ///< the client, renderer, or UI version didn't match SAM's version
    SAM_ERR_MAX_CLIENTS,        ///< the maximum number of clients has been reached
    SAM_ERR_NO_FREE_OUTPUT,     ///< there are no more output channels (JACK ports) available in SAM
    SAM_ERR_INVALID_ID,         ///< invalid client id
    SAM_ERR_INVALID_TYPE        ///< invalid rendering type
};

/**
 * @enum StreamingAudioType
 * The possible types of audio streams for an app.
 */
enum StreamingAudioType
{
    TYPE_BASIC = 0
};

/**
 * @enum SACReturn
 * The possible return codes for StreamingAudioClient methods.
 */
enum SACReturn
{
    SAC_SUCCESS = 0,    ///< Success
    SAC_REQUEST_DENIED, ///< A SAM request was denied (i.e. registration or changing type failed)
    SAC_NOT_REGISTERED, ///< Attempted to send a request to SAM before registering
    SAC_OSC_ERROR,      ///< An error occurred trying to send an OSC message to SAM or receive one from SAM
    SAC_TIMEOUT,        ///< A request to SAM timed out waiting for a response
    SAC_ERROR,          ///< An error occurred that doesn't fit one of the above codes
    SAC_NUM_RETURN_CODES
};

/**
 * @struct SacParams
 * This struct contains the parameters needed to intialize a SAC
 */
struct SacParams
{
    SacParams() :
        numChannels(0),
        type(TYPE_BASIC),
        preset(0),
        name(NULL),
        samIP(NULL),
        samPort(0),
        replyIP(NULL),
        replyPort(0),
        payloadType(PAYLOAD_PCM_16),
        driveExternally(false),
        packetQueueSize(-1)
    {}

    unsigned int numChannels;   ///< number of channels of audio to send to SAM
    StreamingAudioType type;    ///< rendering type
    unsigned int preset;        ///< rendering preset
    const char* name;           ///< human-readable client name (for UIs)
    const char* samIP;          ///< IP address of SAM to connect to
    quint16 samPort;            ///< Port on which SAM receives OSC messages
    const char* replyIP;        ///< local IP address from which to send and receive OSC messages
    quint16 replyPort;          ///< Local port for receiving OSC message replies (or 0 to have port assigned internally)
    quint8 payloadType;         ///< RTP payload type (16, 24, or 32-bit PCM)
    bool driveExternally;       ///< whether audio sending will be driven by external clock
    int packetQueueSize;        ///< number of packets that will be queued on SAM's end before playback, or -1 to use SAM's internal default
};

/**
 * @typedef SACAudioCallback
 * Prototype for a StreamingAudioClient audio callback function.
 * This function is called whenever more audio data is needed.
 * IMPORTANT: as per the JACK audio process callback: the code in this function must be 
 * suitable for real-time execution. That means that it cannot call functions that might block 
 * for a long time. This includes all I/O functions (disk, TTY, network), malloc, free, printf, 
 * pthread_mutex_lock, sleep, wait, poll, select, pthread_join, pthread_cond_wait, etc, etc.
 * @param numChannels the number of channels of audio to write
 * @param nframes the number of sample frames to write
 * @param out pointer to output data out[numChannels][nframes]
 * @param arg pointer to user-supplied data
 * @return true on success, false on failure
 */
typedef bool(*SACAudioCallback)(unsigned int numChannels, unsigned int nframes, float** out, void* arg);

/**
 * @typedef SACMuteCallback
 * Prototype for a StreamingAudioClient mute callback function.
 * This function is called whenever the client is muted or unmuted.
 * @param mute true if client is muted, false otherwise
 * @param arg pointer to user-supplied data
 */
typedef void(*SACMuteCallback)(bool mute, void* arg);

/**
 * @typedef SACSoloCallback
 * Prototype for a StreamingAudioClient solo callback function.
 * This function is called whenever the client solo status changes.
 * @param solo true if client is solo'd, false otherwise
 * @param arg pointer to user-supplied data
 */
typedef void(*SACSoloCallback)(bool solo, void* arg);

/**
 * @typedef SACDisconnectCallback
 * Prototype for a StreamingAudioClient disconect callback function.
 * This function is called when the connection with SAM is broken.
 * @param arg pointer to user-supplied data
 */
typedef void(*SACDisconnectCallback)(void* arg);

/**
 * @class StreamingAudioClient
 * @author Michelle Daniels
 * @date 2012
 *
 * This class encapsulates the functionality required for an application to
 * connect to the Streaming Audio Manager (SAM).
 */
class StreamingAudioClient : public QObject
{
    Q_OBJECT
public:

    /**
      * StreamingAudioClient default constructor.
      * After construction, initialize using init()
      */
    StreamingAudioClient();

    /**
     * StreamingAudioClient destructor.
     * @see StreamingAudioClient
     */
    ~StreamingAudioClient();
    
    /**
     * Copy constructor (not used).
     */
    StreamingAudioClient(const StreamingAudioClient&);

    /**
     * Assignment operator (not used).
     */
    StreamingAudioClient& operator=(const StreamingAudioClient);

    /**
     * Init this StreamingAudioClient.
     * This must be called before calling start().
     * @param params SacParams struct of parameters
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int init(const SacParams& params);

    /**
     * Init this StreamingAudioClient.
     * This must be called before calling start().
     * @param numChannels number of audio channels this client will send to SAM
     * @param type the type of audio stream this client will send
     * @param name the name of this client
     * @param samIP the IP address of SAM
     * @param samPort SAM's OSC port
     * @param replyPort this client's port to listen on (if NULL, a port will be randomly chosen)
     * @param payloadType PAYLOAD_PCM_16, PAYLOAD_PCM_24 or PAYLOAD_PCM_32
     * @param driveExternally false to allow SAC to drive the audio sending or true to drive the
     *      audio sending externally.
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int init(unsigned int numChannels,
             StreamingAudioType type,
             const char* name,
             const char* samIP,
             quint16 samPort,
             quint16 replyPort = 0,
             quint8 payloadType = PAYLOAD_PCM_16,
             bool driveExternally = false);

    /**
     * Register this client with SAM and block until response is received.
     * @param x the initial x position of the app window
     * @param y the initial y position of the app window
     * @param width the initial width of the app window
     * @param height the initial height of the app window
     * @param depth the initial depth of the app window
     * @param timeout response timeout time in milliseconds
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int start(int x, int y, int width, int height, int depth, unsigned int timeout = SAC_DEFAULT_TIMEOUT);
    
    /**
     * Send audio to SAM.
     * Should only be called if driving the sending from outside SAC.
     * @param in a buffer of input audio samples to send (NULL if no input).
     *      Format: in[ch][frame] using the number of channels specified at initialization time
     *      and buffer size (number of sample "frames" per channel) returned from getBufferSize()
     *      after successfully starting SAC.
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int sendAudio(float** in);

    /**
     * Get the buffer size that should be used when driving audio sending from outside SAC.
     * This can only be called after start() has returned successfully.
     * @return buffer size (number of samples per channel sent in each packet) or 0 if unitialized
     */
    unsigned int getBufferSize() { return m_bufferSize; }

    /**
     * Get the sample rate that should be used when driving audio sending from outside SAC.
     * This can only be called after start() has returned successfully.
     * @return the current sampling rate or zero if uninitialized
     */
    unsigned int getSampleRate() { return m_sampleRate; }

    /**
     * Find out how many channels of physical audio inputs are available to this client.
     * @return the number of physical audio inputs available
     */
    unsigned int getNumPhysicalInputs();
    
    /**
     * Send SAM a mute message.
     * @param isMuted whether this client should be muted or not
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setMute(bool isMuted);
    
    /**
     * Send SAM a global mute message.
     * @param isMuted whether all clients should be muted or not
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setGlobalMute(bool isMuted);

    /**
     * Send SAM a solo message.
     * @param isSolo whether this client should be soloed or not
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setSolo(bool isSolo);

    /**
     * Tell SAM the position of this client's parent app.
     * @param x the x position of the app window
     * @param y the y position of the app window
     * @param width the width of the app window
     * @param height the height of the app window
     * @param depth the depth of the app window
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setPosition(int x, int y, int width, int height, int depth);
    
    /**
     * Tell SAM the audio type for this client and block until a response is received.
     * @param type the type of audio stream this client will send
     * @param timeout the timeout time in milliseconds after which this method will return false if no
     * response has been received from SAM.
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setType(StreamingAudioType type, unsigned int timeout = SAC_DEFAULT_TIMEOUT);
    
    /**
     * Tell SAM the audio type for this client and block until a response is received.
     * @param type the type of audio stream this client will send
     * @param preset the rendering preset this client is requesting
     * @param timeout the timeout time in milliseconds after which this method will return false if no
     * response has been received from SAM.
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setTypeWithPreset(StreamingAudioType type, int preset, unsigned int timeout = SAC_DEFAULT_TIMEOUT);

    /**
     * Send SAM a volume message.
     * @param volume the volume for the client
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setVolume(float volume);
    
    /**
     * Send SAM a global volume message.
     * @param volume the global volume for all clients
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setGlobalVolume(float volume);
    
    /**
     * Send SAM a delay message.
     * @param delay the delay for the client (in milliseconds)
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setDelay(float delay);

    /**
     * Set audio callback.
     * IMPORTANT: as per the JACK audio process callback: the code in the supplied function must be 
     * suitable for real-time execution. That means that it cannot call functions that might block 
     * for a long time. This includes all I/O functions (disk, TTY, network), malloc, free, printf, 
     * pthread_mutex_lock, sleep, wait, poll, select, pthread_join, pthread_cond_wait, etc, etc.
     * @param callback the function this client will call when audio samples are needed
     * @param arg each time the callback is called it will pass this as an argument
     * @return 0 on success, a non-zero ::SACReturn code on failure (will fail if a callback has already been set)
     */
    int setAudioCallback(SACAudioCallback callback, void* arg);
    
    /**
     * Set mute callback.
     * @param callback the function this client will call when muted or unmuted
     * @param arg each time the callback is called it will pass this as an argument
     * @return 0 on success, a non-zero ::SACReturn code on failure (will fail if a callback has already been set)
     */
    int setMuteCallback(SACMuteCallback callback, void* arg);

    /**
     * Set solo callback.
     * @param callback the function this client will call when solo status changes
     * @param arg each time the callback is called it will pass this as an argument
     * @return 0 on success, a non-zero ::SACReturn code on failure (will fail if a callback has already been set)
     */
    int setSoloCallback(SACSoloCallback callback, void* arg);

    /**
     * Set disconnect callback.
     * @param callback the function this client will call when disconnected
     * @param arg each time the callback is called it will pass this as an argument
     * @return 0 on success, a non-zero ::SACReturn code on failure (will fail if a callback has already been set)
     */
    int setDisconnectCallback(SACDisconnectCallback callback, void* arg);

    /**
     * Set physical audio inputs for this client.
     * NOTE: this method must be called AFTER registering otherwise it will fail.
     * @param inputChannels an array (length numInputs) listing which input channel indices should
     * be used.  The length of inputChannels should be equal to the number of channels specified in
     * the constructor for this SAC.  The channel indices should be 1-indexed.
     * @return 0 on success, a non-zero ::SACReturn code on failure
     */
    int setPhysicalInputs(unsigned int* inputChannels);
            
    /** 
     * Get audio latency (in sec?)
     * TODO: not yet implemented
     * @return the estimated audio latency
     */
    float getLatency() { return 0.0f; }
    
    /**
     * Check if this StreamingAudioClient is running.
     * @return true if running, false otherwise
     */
    bool isRunning() { return (m_port >= 0); }

public slots:
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

signals:
    /**
     * Emitted when a response from SAM is received.
     * This signal is for use internal to the SAC library, and clients don't need to (and shouldn't) connect it to any slots.
     */
    void responseReceived();

private:
    
    /**
     * Handle a /sam/regconfirm OSC message.
     */
    void handle_regconfirm(int port, unsigned int sampleRate, unsigned int bufferSize, quint16 rtpBasePort);

    /**
     * Handle a /sam/regdeny OSC message.
     */
    void handle_regdeny(int errorCode);
    
    /**
     * Handle a /sam/typeconfirm OSC message.
     */
    void handle_typeconfirm(int type, int preset);

    /**
     * Handle a /sam/typedeny OSC message.
     */
    void handle_typedeny(int errorCode);
    
    /**
     * Audio interface process callback.
     */
    static bool interface_callback(unsigned int nchannels, unsigned int nframes, float** in, float** out, void* sac);

    unsigned int m_channels;
    unsigned int m_bufferSize;
    unsigned int m_sampleRate;
    StreamingAudioType m_type;
    unsigned int m_preset;
    volatile int m_port;
    char* m_name;
    char* m_samIP;
    quint16 m_samPort;
    quint8 m_payloadType;
    int m_packetQueueSize;

    // for OSC
    char* m_replyIP;
    quint16 m_replyPort;              ///< OSC port for this client
    QTcpSocket m_socket;              ///< Socket for sending and receiving OSC messages
    QByteArray m_data;                ///< temporary storage for received data
    bool m_responseReceived;          ///< whether or not SAM responded to register request
    OscTcpSocketReader* m_oscReader;  ///< OSC-reading helper

    // for audio interface
    bool m_driveExternally;           ///< true to drive with an external clock tick, false to use internal SAC driver.
    SacAudioInterface* m_interface;   ///< audio interface
    
    // for RTP
    RtpSender* m_sender;              ///< RTP streamer

    // for audio callback
    SACAudioCallback m_audioCallback; ///< the audio callback function
    void* m_audioCallbackArg;         ///< user-supplied data for audio callback
    float** m_audioOut;               ///< output audio buffer

    // for other callbacks
    SACMuteCallback m_muteCallback;             ///< the mute callback function
    void* m_muteCallbackArg;                    ///< user-supplied data for mute callback
    SACSoloCallback m_soloCallback;             ///< the solo callback function
    void* m_soloCallbackArg;                    ///< user-supplied data for solo callback
    SACDisconnectCallback m_disconnectCallback; ///< the disconnect callback function
    void* m_disconnectCallbackArg;              ///< user-supplied data for disconnect callback

};

} // end of namespace sam

#endif // SAM_CLIENT_H

