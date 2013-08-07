/**
 * @file sam_client.h
 * Interface for applications connecting to the Streaming Audio Manager
 * @author Michelle Daniels
 * @date January 2012
 * @copyright UCSD 2012
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
    static const int VERSION_PATCH = 6;
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
        SAM_ERR_INVALID_ID          ///< invalid client id
    };
    
    /**
     * @enum StreamingAudioType
     * The possible types of audio streams for an app.
     */
    enum StreamingAudioType
    {
        TYPE_BASIC = 0,     ///< mono sources placed in center channel or equally in left/right, stereo sources played in front left and right, surround played in surround system
        TYPE_SPATIAL,       ///< for mono and stereo sources that should be spatialized/tracked
        TYPE_ARRAY,         ///< for sources that should be rendered by a speaker array
        NUM_TYPES
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
     * @param arg pointer to client-supplied data
     * @return true on success, false on failure
     */
    typedef bool(*SACAudioCallback)(unsigned int numChannels, unsigned int nframes, float** out, void* arg);
    
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
         * Initialization must be done separately with init().  This is the preferred
         * method, since it allows the caller to handle any errors within init().
         */
        StreamingAudioClient();
        
        /**
         * StreamingAudioClient constructor.
         * Includes initialization, but because a constructor has no return value
         * to report initialization errors, the preferred approach is to use the
         * default constructor and call init() separately.
         * @param numChannels number of audio channels this client will send to SAM
         * @param type the type of audio stream this client will send
         * @param name the name of this client
         * @param samIP the IP address of SAM
         * @param samPort SAM's OSC port
         * @param replyPort this client's port to listen on (if NULL, a port will be randomly chosen)
         * @param payloadType PAYLOAD_PCM_16, PAYLOAD_PCM_24, or PAYLOAD_PCM_32
         * @param driveExternally false to allow SAC to drive the audio sending or true to drive the
         *      audio sending externally.
         * @see ~StreamingAudioClient
         */
        StreamingAudioClient(unsigned int numChannels, StreamingAudioType type, const char* name, const char* samIP, quint16 samPort, quint16 replyPort = 0, quint8 payloadType = PAYLOAD_PCM_16, bool driveExternally = false);
        
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
         * If you use the default constructor, you must separately initialize with
         * this method before calling start().
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
        int init(unsigned int numChannels, StreamingAudioType type, const char* name, const char* samIP, quint16 samPort, quint16 replyPort = 0, quint8 payloadType = PAYLOAD_PCM_16, bool driveExternally = false);
        
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
         * @return true on success, false on failure
         */
        bool sendAudio(float** in);
        
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
         * @return 0 on success, a non-zero ::SACReturn code on failure
         */
        int setAudioCallback(SACAudioCallback callback, void* arg);
        
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
         * Handle an incoming OSC message
         * @param msg the message to handle
         * @param sender sender IP address/hostname
         */
        void handleOscMessage(OscMessage* msg, const char* sender);
        
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
        void handle_typeconfirm(int type);
        
        /**
         * Handle a /sam/typedeny OSC message.
         */
        void handle_typedeny(int errorCode);
        
        /**
         * Audio interface process callback.
         */
        static bool interface_callback(unsigned int nchannels, unsigned int nframes, float** in, float** out, void* sac);
        
        
        /**
         * Read data from the TCP socket.
         * @return true on success, false on failure (malformed message, etc.)
         */
        bool read_from_socket();
        
        unsigned int m_channels;
        unsigned int m_bufferSize;
        unsigned int m_sampleRate;
        StreamingAudioType m_type;
        volatile int m_port;
        char* m_name;
        char* m_samIP;
        quint16 m_samPort;
        quint8 m_payloadType;
        
        // for OSC
        quint16 m_replyPort;              ///< OSC port for this client
        QTcpSocket m_socket;              ///< Socket for sending and receiving OSC messages
        QByteArray m_data;                ///< temporary storage for received data
        
        // for audio interface
        bool m_driveExternally;           ///< true to drive with an external clock tick, false to use internal SAC driver.
        SacAudioInterface* m_interface;   ///< audio interface
        
        // for RTP
        RtpSender* m_sender;              ///< RTP streamer
        
        // for audio callback
        SACAudioCallback m_audioCallback; ///< the audio callback
        void* m_audioCallbackArg;         ///< user-supplied data for audio callback
        float** m_audioOut;               ///< output audio buffer
    };
    
} // end of namespace sam

#endif // SAM_CLIENT_H
