

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern "C"
{
#include "config.h"
#include "mp_msg.h"
#include "help_mp.h"

#include "audio_out.h"
#include "audio_out_internal.h"
#include "libaf/af_format.h"
#include "osdep/timer.h"
#include "subopt-helper.h"

#include "libavutil/fifo.h"

}

//////////////////////////////////////////////////////////////
#include "sam_client.h"
using namespace sam;
StreamingAudioClient sac;
static const int SAM_OSC_PORT = 7770;
static const StreamingAudioType SAM_DEFAULT_TYPE = TYPE_BASIC;
static const unsigned int SAM_REGISTER_TIMEOUT = 5000; // milliseconds

//////////////////////////////////////////////////////////////


static const ao_info_t info ={
    "Streaming Audio Manager audio output",
    "sam",
    "Michelle Daniels <michelledaniels@ucsd.edu> and Luc Renambot <renambot@gmail.com>",
    "based on ao_jack.c"
};

extern "C"
{
extern LIBAO_EXTERN(sam)
}

//! maximum number of channels supported, avoids lots of mallocs
#define MAX_CHANS 8
static float jack_latency;
static int estimate;
static volatile int paused = 0; ///< set if paused
static volatile int underrun = 0; ///< signals if an underrun occured

static volatile float callback_interval = 0;
static volatile float callback_time = 0;

//! size of one chunk, if this is too small MPlayer will start to "stutter"
//! after a short time of playback
#define CHUNK_SIZE (16 * 1024)
//! number of "virtual" chunks the buffer consists of
#define NUM_CHUNKS 8
#define BUFFSIZE (NUM_CHUNKS * CHUNK_SIZE)

//! buffer for audio data
static AVFifoBuffer *buffer;

/**
 * \brief insert len bytes into buffer
 * \param data data to insert
 * \param len length of data
 * \return number of bytes inserted into buffer
 *
 * If there is not enough room, the buffer is filled up
 */
static int write_buffer(unsigned char* data, int len)
{
    int free = av_fifo_space(buffer);
    if (len > free) len = free;
    return av_fifo_generic_write(buffer, data, len, NULL);
}

static void silence(float **bufs, int cnt, int num_bufs);

struct deinterleave
{
    float **bufs;
    int num_bufs;
    int cur_buf;
    int pos;
};

static void deinterleave(void *info, void *src, int len)
{
    struct deinterleave *di = (struct deinterleave*) info;
    float *s = (float*) src;
    int i;
    len /= sizeof (float);
    for (i = 0; i < len; i++)
    {
        di->bufs[di->cur_buf++][di->pos] = s[i];
        if (di->cur_buf >= di->num_bufs)
        {
            di->cur_buf = 0;
            di->pos++;
        }
    }
}

/**
 * \brief read data from buffer and splitting it into channels
 * \param bufs num_bufs float buffers, each will contain the data of one channel
 * \param cnt number of samples to read per channel
 * \param num_bufs number of channels to split the data into
 * \return number of samples read per channel, equals cnt unless there was too
 *         little data in the buffer
 *
 * Assumes the data in the buffer is of type float, the number of bytes
 * read is res * num_bufs * sizeof(float), where res is the return value.
 * If there is not enough data in the buffer remaining parts will be filled
 * with silence.
 */
static int read_buffer(float **bufs, int cnt, int num_bufs)
{
    struct deinterleave di = {bufs, num_bufs, 0, 0};
    int buffered = av_fifo_size(buffer);
    if (cnt * sizeof (float) * num_bufs > buffered)
    {
        silence(bufs, cnt, num_bufs);
        cnt = buffered / sizeof (float) / num_bufs;
    }
    av_fifo_generic_read(buffer, &di, cnt * num_bufs * sizeof (float), deinterleave);
    return cnt;
}

// end ring buffer stuff

static int control(int cmd, void *arg)
{
    return CONTROL_UNKNOWN;
}

/**
 * \brief fill the buffers with silence
 * \param bufs num_bufs float buffers, each will contain the data of one channel
 * \param cnt number of samples in each buffer
 * \param num_bufs number of buffers
 */
static void silence(float **bufs, int cnt, int num_bufs)
{
    int i;
    for (i = 0; i < num_bufs; i++)
        memset(bufs[i], 0, cnt * sizeof (float));
}

/**
 * \brief print suboption usage help
 */
static void print_help(void)
{
    mp_msg(MSGT_AO, MSGL_FATAL,
           "\n-ao sam commandline help:\n"
           "Example: mplayer -ao sam:ip=xxx.xxx.xxx.xxx:port=7770\n"
           "  connects MPlayer to the SAM at the given IP address and port\n"
           "\nOptions:\n"
           "  name=<client name>\n"
           "    Client name to pass to JACK\n"
           "  estimate\n"
           "    Estimates the amount of data in buffers (experimental)\n"
           "  ip\n"
           "    IP address of the Streaming Audio Manager\n"
           "  port\n"
           "    Port for contacting the Streaming Audio Manager\n"
           "  type\n"
           "    stream type (0=basic,1=tracked,2=array)\n"
           "  timeout\n"
           "    Timeout in milliseconds for contacting SAM\n"
           );
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// ----- SAC CALLBACKS -----

/**
 * \brief SAC audio Callback function
 * \param numChannels number of audio channels
 * \param nframes number of frames to fill into buffers
 * \param out pointer to output buffers: out[channel][frame]
 * \param arg unused
 * \return currently always 0
 *
 * Write silence into buffers if paused or an underrun occured
 */
bool sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void* arg)
{
    if (paused || underrun)
    {
        silence(out, nframes, numChannels);
    }
    else if (read_buffer(out, nframes, numChannels) < (int)nframes)
    {
        underrun = 1;
    }
    if (estimate)
    {
        float now = (float) GetTimer() / 1000000.0;
        float diff = callback_time + callback_interval - now;
        if ((diff > -0.002) && (diff < 0.002))
            callback_time += callback_interval;
        else
            callback_time = now;
        callback_interval = (float) nframes / (float) ao_data.samplerate;
    }
    return true;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

// Use to set the client name from a video module
extern char* vo_wintitle;


static int init(int rate, int channels, int format, int flags)
{
    char* client_name = NULL;
    char* sam_ip = NULL;
    int sam_port = SAM_OSC_PORT;
    int sam_type = SAM_DEFAULT_TYPE;
    int sam_timeout = SAM_REGISTER_TIMEOUT;
    int x = 0;
    int y = 0;
    int width = 100;
    int height = 100;
    int depth = 0;
    int returnCode = 0;
    const opt_t subopts[] = {
        {"name", OPT_ARG_MSTRZ, &client_name, NULL},
        {"estimate", OPT_ARG_BOOL, &estimate, NULL},
        {"ip", OPT_ARG_MSTRZ, &sam_ip, NULL},
        {"port", OPT_ARG_INT, &sam_port, NULL},
        {"type", OPT_ARG_INT, &sam_type, NULL},
        {"timeout", OPT_ARG_INT, &sam_timeout, NULL},
        {NULL}
    };
    estimate = 1;
    if (subopt_parse(ao_subdevice, subopts) != 0)
    {
        print_help();
        return 0;
    }
    if (channels > MAX_CHANS)
    {
        mp_msg(MSGT_AO, MSGL_FATAL, "[SAM] Invalid number of channels: %i\n", channels);
        goto err_out;
    }
    if (sam_timeout < 0)
    {
        mp_msg(MSGT_AO, MSGL_FATAL, "[SAM] Timeout cannot be negative\n");
        goto err_out;
    }
    if (!client_name)
    {
        client_name = (char*) malloc(64);
	memset(client_name, 0, 64);
	if (vo_wintitle)
		snprintf(client_name, 63, "MPlayer [%s]", vo_wintitle);
	else
		snprintf(client_name, 63, "MPlayer [%d]", getpid());
    }
    if (!sam_ip)
    {
        mp_msg(MSGT_AO, MSGL_FATAL, "[SAM] IP address of StreamingAudioManager not specified: using default 127.0.0.1\n");
	sam_ip = strdup("127.0.0.1");
        //goto err_out;
    }

    buffer = av_fifo_alloc(BUFFSIZE);
   
    //////////////////////////////////////////////////
    //// SAM Addon
    //////////////////////////////////////////////////
    if (!sac.isRunning())
	{
		sac.init(channels, 
				 (StreamingAudioType)sam_type,
				 client_name, 
				 sam_ip, 
				 sam_port);
    
		// register SAC callbacks
		sac.setAudioCallback(sac_audio_callback, NULL);
    
		returnCode = sac.start(x, y, width, height, depth, (unsigned int)sam_timeout);
		if (returnCode != SAC_SUCCESS)
		{
			mp_msg(MSGT_AO, MSGL_FATAL, "[SAM] Registration failed\n");
			goto err_out;
		}
	}
    
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////

    rate = sac.getSampleRate();
    jack_latency = sac.getLatency();
    /*jack_latency = (float) (jack_port_get_total_latency(client, ports[0]) +
            jack_get_buffer_size(client)) / (float) rate;*/
    callback_interval = 0;
    
    ao_data.channels = channels;
    ao_data.samplerate = rate;
    ao_data.format = AF_FORMAT_FLOAT_NE;
    ao_data.bps = channels * rate * sizeof (float);
    ao_data.buffersize = CHUNK_SIZE * NUM_CHUNKS;
    ao_data.outburst = CHUNK_SIZE;
    if (client_name) free(client_name);
    if (sam_ip) free(sam_ip);
    return 1;

err_out:
    if (client_name) free(client_name);
    if (sam_ip) free(sam_ip);
    av_fifo_free(buffer);
    buffer = NULL;
    return 0;
}

// close audio device

static void uninit(int immed)
{
    if (!immed)
        usec_sleep(get_delay() * 1000 * 1000);

    // HACK, make sure jack doesn't loop-output dirty buffers
    reset();
    usec_sleep(100 * 1000);
    av_fifo_free(buffer);
    buffer = NULL;
}

/**
 * \brief stop playing and empty buffers (for seeking/pause)
 */
static void reset(void)
{
    paused = 1;
    av_fifo_reset(buffer);
    paused = 0;
}

/**
 * \brief stop playing, keep buffers (for pause)
 */
static void audio_pause(void)
{
    paused = 1;
}

/**
 * \brief resume playing, after audio_pause()
 */
static void audio_resume(void)
{
    paused = 0;
}

static int get_space(void)
{
    return av_fifo_space(buffer);
}

/**
 * \brief write data into buffer and reset underrun flag
 */
static int play(void *data, int len, int flags)
{
    if (!(flags & AOPLAY_FINAL_CHUNK))
        len -= len % ao_data.outburst;
    underrun = 0;
    return write_buffer((unsigned char*) data, len);
}

static float get_delay(void)
{
    int buffered = av_fifo_size(buffer); // could be less
    float in_jack = jack_latency;
    if (estimate && callback_interval > 0)
    {
        float elapsed = (float) GetTimer() / 1000000.0 - callback_time;
        in_jack += callback_interval - elapsed;
        if (in_jack < 0) in_jack = 0;
    }
    return (float) buffered / (float) ao_data.bps + in_jack;
}

