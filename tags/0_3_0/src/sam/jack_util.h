/**
 * @file jack_util.h
 * Shared JACK-related functionality
 * @author Michelle Daniels
 * @date November 2011
 * @copyright UCSD 2011
 */

#ifndef JACK_UTIL_H
#define	JACK_UTIL_H

#include "jack/jack.h"

/**
 * Check if a JACK server is already running.
 * @return true if JACK is running, false if not
 */
bool JackServerIsRunning();

/**
 * Start the JACK server.
 * @param sampleRate the audio sampling rate for JACK
 * @param bufferSize the audio buffer size for JACK
 * @param outChannels max number of output channels
 * @param driver driver to use for JACK (ie "coreaudio" or "alsa")
 * @return true on success, false on failure
 * @see StopJack
 */
pid_t StartJack(int sampleRate, int bufferSize, int outChannels, const char* driver);

/**
 * Stop the JACK server.
 * @return true on success, false on failure
 * @see StartJack
 */
bool StopJack(pid_t jackPID);

/**
 * Check if a port's JackPortIsInput flag is set.
 * @param port pointer to the port struct (assumes non-NULL)
 * @return true if port is an input port, false otherwise
 * @see PortIsOutput
 */
bool PortIsInput(const jack_port_t* port);

/**
 * Check if a port's JackPortIsOutput flag is set.
 * @param port pointer to the port struct (assumes non-NULL)
 * @return true if port is an output port, false otherwise
 * @see PortIsInput
 */
bool PortIsOutput(const jack_port_t* port);

#endif // JACK_UTIL_H

