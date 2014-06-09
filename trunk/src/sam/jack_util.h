/**
 * @file jack_util.h
 * Shared JACK-related functionality
 * @author Michelle Daniels
 * @date November 2011
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

#ifndef JACK_UTIL_H
#define	JACK_UTIL_H

#include "jack/jack.h"

namespace sam
{

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

} // end of namespace SAM

#endif // JACK_UTIL_H

