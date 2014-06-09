/**
 * @file sam_app.h
 * Interface for functionality related to audio for a single client application
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2012
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef SAM_APP_H
#define	SAM_APP_H

#include "jack/jack.h"

#include "sam.h"
#include "rtpreceiver.h"

namespace sam
{
/**
 * @enum SamClientSubscription
 * The possible client parameters to subscribe/unsubscribe to.
 */
enum SamClientSubscription
{
    SUBSCRIPTION_VOLUME = 0,
    SUBSCRIPTION_MUTE,
    SUBSCRIPTION_SOLO,
    SUBSCRIPTION_DELAY,
    SUBSCRIPTION_POSITION,
    SUBSCRIPTION_TYPE,
    SUBSCRIPTION_METER,
    NUM_SUBSCRIPTIONS
};

/**
 * @class StreamingAudioApp
 * @author Michelle Daniels
 * @date 2011
 *
 * This class encapsulates the functionality required for a single SAGE application to play audio
 */
class StreamingAudioApp : public QObject
{
    Q_OBJECT
public:

    /** Constructor.
     */
    StreamingAudioApp(const char* name, 
                      int port, 
                      int channels, 
                      const SamAppPosition& pos, 
                      StreamingAudioType type, 
                      int preset,
                      jack_client_t* client, 
                      QTcpSocket* socket, 
                      quint16 rtpBasePort, 
                      int maxDelay, 
                      quint32 m_packetQueueSize, 
                      qint32 clockSkewThreshold,
                      StreamingAudioManager* sam, 
                      QObject* parent = 0);

    /**
     * Destructor.
     */
    ~StreamingAudioApp();

    /**
     * Copy constructor (not used).
     */
    StreamingAudioApp(const StreamingAudioApp&);

    /**
     * Assignment operator (not used).
     */
    StreamingAudioApp& operator=(const StreamingAudioApp&);

    /**
     * Initialize the app.
     */
    bool init();

    /** 
     * Activate the app.
     * Called from the SamAppStartThread
     */
    bool activate();

    /**
     * Kill the app.
     */
    bool kill();

    /**
     * Start the thread that will start jacktrip/make connections, etc.
     * @param sam pointer to the parent StreamingAudioManager
     */
    void startThread(StreamingAudioManager* sam);

    /**
     * Set the number of actual channels used.
     * @param the number of channels used
     */
    void setChannelsUsed(int channels) { m_channelsUsed = channels; }

    /**
     * Set the volume.
     * @param volume the volume to be set, in the range [0.0, 1.0]
     */
    void setVolume(float volume);

    /**
     * Get the volume level.
     * @return the volume level
     */
    float getVolume() const { return m_volumeNext; }

    /**
     * Set the mute status.
     * @param isMuted true if this app is to be muted, false otherwise
     */
    void setMute(bool isMuted);

    /**
     * Get the mute status.
     * @return true if this app is muted, false otherwise
     */
    bool getMute() const { return m_isMutedNext; }

    /**
     * Set the solo status.
     * @param isSolo true if this app is to be solo'd, false otherwise
     */
    void setSolo(bool isSolo);

    /**
     * Get the solo status.
     * @return true if this app is solo'd, false otherwise
     */
    bool getSolo() const { return m_isSoloNext; }

    /**
     * Set the delay.
     * @param delay delay in milliseconds
     */
    void setDelay(float delay);

    /**
     * Get the delay.
     * @return the delay in milliseconds
     */
    float getDelay() const { return  ((m_delayNext * 1000.0f) / (float)m_sampleRate); }

    /**
     * Set the position.
     * @param pos the new position
     */
    void setPosition(const SamAppPosition& pos);

    /**
     * Get the app window's position.
     * @return the app window's position
     */
    SamAppPosition getPosition() const { return m_position; }

    /**
     * Set the rendering type and preset.
     * @param type the rendering type to be set
     * @param preset the rendering preset to be set
     */
    void setType(StreamingAudioType type, int preset);

    /**
     * Get the rendering type.
     * @return the rendering type
     */
    StreamingAudioType getType() const { return m_type; }

    /**
     * Get the rendering preset.
     * @return the type level
     */
    int getPreset() const { return m_preset; }

    /**
     * Set a channel assignment.
     * @param appChannel the app's channel
     * @param assignChannel the output channel assigned
     */
    void setChannelAssignment(int appChannel, int assignChannel);

    /**
     * Get channel assignments.
     * @return the array of channel assignments (not to be modified)
     */
    const int* getChannelAssignments() const { return m_channelAssign; }
    
    /**
     * Subscribe to changes for the given parameter.
     * @param param the SamClientSubscription parameter to subscribe to
     * @param host the host to be subscribed
     * @param port the port on the host to be subscribed
     * @return true on success, false on failure
     */
    bool subscribe(const char* host, quint16 port, int param);

    /**
     * Unsubscribe from changes for the given parameter.
     * @param param the SamClientSubscription parameter to unsubscribe from
     * @param host the host to be unsubscribed
     * @param port the port on the host to be unsubscribed
     * @return true on success, false on failure
     */
    bool unsubscribe(const char* host, quint16 port, int param);

    /**
     * Subscribe to changes for the given parameter using TCP.
     * @param param the SamClientSubscription parameter to subscribe to
     * @param socket the socket to be subscribed
     * @return true on success, false on failure
     */
    bool subscribeTcp(QTcpSocket* socket, int param);

    /**
     * Unsubscribe from changes for the given parameter using TCP.
     * @param param the SamClientSubscription parameter to unsubscribe from
     * @param socket the socket to be unsubscribed
     * @return true on success, false on failure
     */
    bool unsubscribeTcp(QTcpSocket* socket, int param);

    /**
     * Subscribe to all changes.
     * @param host the host to be subscribed
     * @param port the port on the host to be subscribed
     * @return true on success, false on failure
     */
    bool subscribeAll(const char* host, quint16 port);

    /**
     * Unsubscribe from all changes.
     * @param host the host to be unsubscribed
     * @param port the port on the host to be unsubscribed
     * @return true on success, false on failure
     */
    bool unsubscribeAll(const char* host, quint16 port);

    /**
     * Subscribe to all changes using TCP.
     * @param host the host to be subscribed
     * @param port the port on the host to be subscribed
     * @return true on success, false on failure
     */
    bool subscribeAllTcp(QTcpSocket* socket);

    /**
     * Unsubscribe from all changes using TCP.
     * @param host the host to be unsubscribed
     * @param port the port on the host to be unsubscribed
     * @return true on success, false on failure
     */
    bool unsubscribeAllTcp(QTcpSocket* socket);

    /**
     * Notify subscribers of current meter levels.
     * @return true on success, false on failure
     */
    bool notifyMeter();

    /**
     * Process a buffer of audio.
     * @param nframes the number of sample frames to process
     * @param volumeCurrent the current global volume (volume at end of previous frame)
     * @param volumeNext the desired global volume (target volume for this frame)
     * @param muteCurrent the current global mute status (status at end of previous frame)
     * @param muteNext the desired global mute status (target status for this frame)
     * @param soloCurrent true if any app was solo'd at the end of the previous frame
     * @param soloNext true if any app will be solo'd in this frame
     * @param delayCurrent the current global delay in samples (delay at end of previous frame)
     * @param delayNext the desired global delay (target delay for this frame)
     * @return 0 on success, non-zero on failure
     */
    int process(jack_nframes_t nframes, float volumeCurrent, float volumeNext, bool muteCurrent, bool muteNext, bool soloCurrent, bool soloNext, int delayCurrent, int delayNext);
    
    /**
     * Get output port name.
     * @param index the index of the port
     * @return the name of the port, or null if the index is invalid
     */
    const char* getOutputPortName(unsigned int index);

    /**
     * Get this app's number of channels.
     * @return the number of channels
     */
    int getNumChannels() const { return m_channels; }

    /**
     * Get this app's port.
     * @return the port
     */
    int getPort() const { return m_port; }
    
    /**
     * Get this app's name
     * @return non-editable name
     */
    const char* getName() const { return m_name; }
    
    /**
     * Get meter levels for a particular channel of this app.
     * @ch channel to get level info for
     * @rmsIn storage for RMS level of input signal (what SAM receives from client)
     * @peakIn storage for Peak level of input signal
     * @rmsOut storage for RMS level of output signal (SAM output after volume/mute/etc.)
     * @peakOut storage for RMS level of output signal
     * @return true on success, false otherwise
     */
    bool getMeters(int ch, float& rmsIn, float& peakIn, float& rmsOut, float& peakOut);

    /**
     * Subscribe to a parameter.
     * @param subscribers a vector of subscriber addresses to which the specified address will be added
     * @param host the host to be subscribed
     * @param port the port to be subscribed
     * @return true on success, false on failure (address is already subscribed)
     */
    static bool subscribe_helper(QVector<OscAddress*> &subscribers, const char* host, quint16 port);

    /**
     * Unsubscribe from a parameter.
     * @param subscribers a vector of subscriber addresses from which to remove the specified address
     * @param host the host to be unsubscribed
     * @param port the port to be unsubscribed
     * @return true on success, false on failure (address was not subscribed in the first place)
     */
    static bool unsubscribe_helper(QVector<OscAddress*> &subscribers, const char* host, quint16 port);

    /**
     * Subscribe to a parameter.
     * @param subscribers a vector of subscriber sockets to which the specified socket will be added
     * @param socket the TCP socket to be subscribed
     * @return true on success, false on failure (address is already subscribed)
     */
    static bool subscribe_tcp_helper(QVector<QTcpSocket*> &subscribers, QTcpSocket* socket);

    /**
     * Unsubscribe from a parameter.
     * @param subscribers a vector of subscriber sockets from which to remove the specified socket
     * @param socket the socket to be unsubscribed
     * @return true on success, false on failure (address was not subscribed in the first place)
     */
    static bool unsubscribe_tcp_helper(QVector<QTcpSocket*> &subscribers, QTcpSocket* socket);

    /**
     * Flag this app for deletion.
     */
    void flagForDelete() { qDebug("StreamingAudioApp::flagForDelete app %d", m_port); m_deleteMe = true; }

    /**
     * Query if this app is flagged for deletion.
     * @return true if flagged for deletion, false otherwise
     */
    bool shouldDelete() const { return m_deleteMe; }
    
signals:
    /**
     * Signals when an app is disconnected.
     */
    void appDisconnected(int);

    /**
     * Emit that this app was closed.
     * @param port the unique port/ID for the app that unregistered
     * @param type the type of app that unregistered
     */
    void appClosed(int port, int type);

public slots:
    /**
     * Shutdown a client because its TCP connection with SAM has been disconnected.
     */
    void disconnectApp();
    
private:

    char* m_name;               ///< the name of this app, to be used for UI displays
    int m_port;                 ///< the port (offset from default 4464) to be used for jacktrip (also serves as unique ID)
    int m_channels;             ///< number of audio channels
    int m_channelsUsed;         ///< number of audio channels actually used by SAM
    int m_sampleRate;           ///< audio sample rate
    SamAppPosition m_position;  ///< app window position
    StreamingAudioType m_type;  ///< audio type
    int m_preset;               ///< rendering preset
    bool m_deleteMe;            ///< indicates whether this app is ready to be deleted
    StreamingAudioManager* m_sam; ///< SAM

    // JACK ports, etc.
    int* m_channelAssign;        ///< channel assignments (which physical output channels this app will be connected to)
    jack_client_t* m_jackClient; ///< pointer to the parent SAM's JACK client, needed to register/unregister ports
    jack_port_t** m_outputPorts; ///< array of JACK output ports for this app
    
    // control parameters
    float m_volumeCurrent;  ///< current volume level in the range [0.0, 1.0]
    float m_volumeNext;     ///< next target/requested volume level in the range [0.0, 1.0]
    bool m_isMutedCurrent;  ///< current mute status
    bool m_isMutedNext;     ///< next target/requested mute status
    bool m_isSoloCurrent;   ///< current solo status
    bool m_isSoloNext;      ///< next target/requested solo status
    int m_delayCurrent;     ///< current delay in samples
    int m_delayNext;        ///< next target/requested delay in samples
    int m_delayMax;         ///< maximum number of samples for delay
    float** m_delayBuffer;  ///< buffer for delayed samples
    int* m_delayRead;       ///< index into delay buffer for reading samples (per channel)
    int* m_delayWrite;      ///< index into delay buffer for writing samples (per channel)
    float* m_rmsOut;        ///< output RMS levels for metering (per channel)
    float* m_peakOut;       ///< output peak levels for metering (per channel)
    float* m_rmsIn;         ///< input RMS levels for metering (per channel)
    float* m_peakIn;        ///< input peak levels for metering (per channel)

    // UDP subscribers
    QVector<OscAddress*> m_volumeSubscribers;   ///< OSC addresses subscribed to volume changes
    QVector<OscAddress*> m_muteSubscribers;     ///< OSC addresses subscribed to mute changes
    QVector<OscAddress*> m_soloSubscribers;     ///< OSC addresses subscribed to solo changes
    QVector<OscAddress*> m_delaySubscribers;    ///< OSC addresses subscribed to delay changes
    QVector<OscAddress*> m_positionSubscribers; ///< OSC addresses subscribed to position changes
    QVector<OscAddress*> m_typeSubscribers;     ///< OSC addresses subscribed to type changes
    QVector<OscAddress*> m_meterSubscribers;    ///< OSC addresses subscribed to meter updates

    // TCP subscribers
    QVector<QTcpSocket*> m_volumeSubscribersTcp;   ///< TCP sockets subscribed to volume changes
    QVector<QTcpSocket*> m_muteSubscribersTcp;     ///< TCP sockets subscribed to mute changes
    QVector<QTcpSocket*> m_soloSubscribersTcp;     ///< TCP sockets subscribed to solo changes
    QVector<QTcpSocket*> m_delaySubscribersTcp;    ///< TCP sockets subscribed to delay changes
    QVector<QTcpSocket*> m_positionSubscribersTcp; ///< TCP sockets subscribed to position changes
    QVector<QTcpSocket*> m_typeSubscribersTcp;     ///< TCP sockets subscribed to type changes
    QVector<QTcpSocket*> m_meterSubscribersTcp;    ///< TCP sockets subscribed to meter updates

    // RTP-related parameters
    RtpReceiver* m_receiver;     ///< RTP receiver for this app/client
    float** m_audioData;         ///< temp buffer for received audio data
    quint16 m_rtpBasePort;       ///< base RTP and RTCP port for this app/client
    quint32 m_packetQueueSize;   ///< packet queue size
    qint32 m_clockSkewThreshold; ///< number of samples of clock skew required before compensation
    
    // For OSC
    QTcpSocket* m_socket;       ///< TCP socket for sending and listening to OSC messages to/from this app/client
};

} // end of namespace SAM

#endif // SAM_APP_H

