/**
 * @file sac_audio_interface.h
 * Interface for different audio interfaces for StreamingAudioClients
 * @author Michelle Daniels
 * @date September 2012
 * @copyright UCSD 2012
 */

#ifndef SACAUDIOINTERFACE_H
#define	SACAUDIOINTERFACE_H

#ifndef SAC_NO_JACK
#include "jack/jack.h"
#include <errno.h>
#endif

#include <QElapsedTimer>
#include <QThread>

namespace sam
{
    /**
     * @typedef AudioInterfaceCallback
     * Prototype for a SacAudioInterface audio callback function.
     * IMPORTANT: as per the JACK audio process callback: the code in this function must be
     * suitable for real-time execution. That means that it cannot call functions that might block
     * for a long time. This includes all I/O functions (disk, TTY, network), malloc, free, printf,
     * pthread_mutex_lock, sleep, wait, poll, select, pthread_join, pthread_cond_wait, etc, etc.
     * @param nchannels the number of channels of audio to write
     * @param nframes the number of sample frames to write
     * @param in pointer to input data in[nchannels][nframes]
     * @param out pointer to output data out[nchannels][nframes]
     * @param arg pointer to client-supplied data
     * @return true on success, false on failure
     */
    typedef bool(*AudioInterfaceCallback)(unsigned int nchannels, unsigned int nframes, float** in, float** out, void* arg);
    
    
    /**
     * @class SacAudioInterface
     * @author Michelle Daniels
     * @date 2012
     *
     * This is a virtual base class/abstraction of different interfaces to sound cards.
     */
    class SacAudioInterface
    {
    public:
        /**
         * SacAudioInterface constructor.
         * @param channels number of output audio channels this interface will support
         * @param bufferSamples number of samples per audio buffer/clock tick
         * @param sampleRate sampling rate the interface will run at
         * @see ~SacAudioInterface
         */
        SacAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate);
        
        /**
         * SacAudioInterface destructor.
         * @see SacAudioInterface
         */
        virtual ~SacAudioInterface();
        
        /**
         * Start running this interface.
         * @return true on success, false on failure
         */
        virtual bool go() = 0;
        
        /**
         * Stop running this interface.
         * @return true on success, false on failure
         */
        virtual bool stop() = 0;
        
        /**
         * Set audio callback.
         * IMPORTANT: as per the JACK audio process callback: the code in the supplied function must be
         * suitable for real-time execution. That means that it cannot call functions that might block
         * for a long time. This includes all I/O functions (disk, TTY, network), malloc, free, printf,
         * pthread_mutex_lock, sleep, wait, poll, select, pthread_join, pthread_cond_wait, etc, etc.
         * @param callback the function this client will call when audio samples are needed
         * @param arg each time the callback is called it will pass this as an argument
         * @return true on success, false on failure
         */
        bool setAudioCallback(AudioInterfaceCallback callback, void* arg);
        
        /**
         * Get the sample rate for this interface.
         * @return the sample rate for this interface.
         */
        unsigned int getSampleRate() { return m_sampleRate; }
        
    protected:
        unsigned int m_channels;       ///< number of input and output channels this interface supports
        unsigned int m_bufferSamples;  ///< interface buffer size
        unsigned int m_sampleRate;     ///< interface sampling rate
        
        // for audio callback
        AudioInterfaceCallback m_audioCallback; ///< the audio callback
        void* m_audioCallbackArg;               ///< user-supplied data for audio callback
        float** m_audioIn;                      ///< input audio buffer
        float** m_audioOut;                     ///< output audio buffer
    };
    
    /**
     * @class VirtualAudioInterface
     * @author Michelle Daniels
     * @date 2012
     *
     * This SacAudioInterface encapsulates a virtual sound card where an internal clock drives audio.
     */
    class VirtualAudioInterface : public SacAudioInterface, public QThread
    {
    public:
        /**
         * VirtualAudioInterface constructor.
         * @param channels number of output audio channels this interface will support
         * @param bufferSamples number of samples per audio buffer/clock tick
         * @param sampleRate sampling rate the interface will run at
         * @see ~VirtualAudioInterface
         */
        VirtualAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate);
        
        /**
         * VirtualAudioInterface destructor.
         * @see VirtualAudioInterface
         */
        virtual ~VirtualAudioInterface();
        
        /**
         * Copy constructor (not used).
         */
        VirtualAudioInterface(const VirtualAudioInterface&);
        
        /**
         * Assignment operator (not used).
         */
        VirtualAudioInterface& operator=(const VirtualAudioInterface);
        
        /**
         * Start running this interface.
         * @return true on success, false on failure
         */
        virtual bool go();
        
        /**
         * Stop running this interface.
         * @return true on success, false on failure
         */
        virtual bool stop();
        
        /**
         * Run the qthread.
         */
        virtual void run();
        
    protected:
        
        QElapsedTimer m_timer;      ///< virtual clock
        double m_audioInterval;     ///< time between clock ticks, in milliseconds
        quint64 m_nextAudioTick;    ///< time of next clock tick, in samples
        
        bool m_shouldQuit;          ///< flag: true if the interface thread should stop running
    };
    
#ifndef SAC_NO_JACK
    
    /**
     * @class JackAudioInterface
     * @author Michelle Daniels
     * @date 2012
     *
     * This SacAudioInterface encapsulates a JACK Audio Connection Kit interface.
     * A JackAudioInterface supports input audio channels as well as output.
     */
    class JackAudioInterface : public SacAudioInterface
    {
    public:
        /**
         * JackAudioInterface constructor.
         * @param channels number of input and output audio channels this interface will support
         * @param bufferSamples number of samples per audio buffer/clock tick
         * @param sampleRate sampling rate the interface will run at
         * @param clientName desired JACK client name
         * @see ~JackAudioInterface
         */
        JackAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate, const char* clientName);
        
        /**
         * JackAudioInterface destructor.
         * @see JackAudioInterface
         */
        virtual ~JackAudioInterface();
        
        /**
         * Copy constructor (not used).
         */
        JackAudioInterface(const JackAudioInterface&);
        
        /**
         * Assignment operator (not used).
         */
        JackAudioInterface& operator=(const JackAudioInterface);
        
        /**
         * Start running this interface.
         * @return true on success, false on failure
         */
        virtual bool go();
        
        /**
         * Stop running this interface.
         * @return true on success, false on failure
         */
        virtual bool stop();
        
        /**
         * Set physical audio inputs for this client.
         * @param inputChannels an array (length numInputs) listing which input channel indices should
         * be used.  The length of inputChannels should be equal to the number of channels specified in
         * the constructor for this SAC.  The channel indices should be 1-indexed.
         * NOTE: this method must be called AFTER registering otherwise it will fail.
         * @return true on success, false on failure
         */
        bool setPhysicalInputs(unsigned int* inputChannels);
        
        /**
         * Get audio latency (in sec?)
         * TODO: not yet implemented
         * @return the estimated audio latency
         */
        float getLatency() { return 0.0f; }
        
        /**
         * Get audio sample rate.
         * @return the sampling rate
         */
        virtual int getSampleRate() { if (m_client) return jack_get_sample_rate(m_client); else return 0; }
        
    protected:
        
        /**
         * JACK process callback
         * @param nframes the number of sample frames to process
         * @return 0 on success, non-zero on failure
         */
        int process_audio(jack_nframes_t nframes);
        
        /**
         * Start the JACK server.
         * @param sampleRate the audio sampling rate for JACK
         * @param bufferSize the audio buffer size for JACK
         * @return the PID of the started JACK process
         * @see stop_jack
         */
        pid_t start_jack(int sampleRate, int bufferSize);
        
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
         * Check if the JACK server is alreay running.
         * @return true if running, false otherwise
         */
        bool jack_server_is_running();
        
        /**
         * JACK buffer size changed callback.
         * JACK will call this callback if the system buffer size changes.
         */
        static int jack_buffer_size_changed(jack_nframes_t nframes, void* sam);
        
        /**
         * JACK process callback.
         * The process callback for this JACK application is called in a
         * special realtime thread once for each audio cycle.  
         * This is where muting, volume control, delay, and panning occurs.
         */
        static int jack_process(jack_nframes_t nframes, void* sam);
        
        /**
         * JACK sample rate changed callback.
         * JACK will call this callback if the system sample rate changes.
         */
        static int jack_sample_rate_changed(jack_nframes_t nframes, void* sam);
        
        /**
         * JACK shutdown callback.
         * JACK calls this callback if the server ever shuts down or
         * decides to disconnect the client.
         */
        static void jack_shutdown(void* sam);
        
        // for JACK
        jack_client_t* m_client;          ///< local JACK client
        jack_port_t** m_inputPorts;       ///< array of JACK input ports for this app
        char* m_clientName;               ///< JACK client name
        
    };
#endif // SAC_NO_JACK
    
} // end of namespace sam

#endif // SACAUDIOINTERFACE_H
