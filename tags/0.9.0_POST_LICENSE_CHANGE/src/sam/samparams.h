/**
 * @file samparams.cpp
 * Interface for parsing SAM parameters
 * @author Michelle Daniels
 * @date September 2013
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

#ifndef SAMPARAMS_H
#define SAMPARAMS_H

#include <QList>
#include <QString>

namespace sam
{

class SamParams
{
public:
    SamParams();

    bool parseConfig(const char* configFile, int argc, char* argv[]);

    static bool parseChannels(QList<unsigned int>& channels, QString& channelString, unsigned int channelMax);

    int sampleRate;                       ///< The sampling rate for JACK
    int bufferSize;                       ///< The buffer size for JACK
    unsigned int numBasicChannels;        ///< The number of basic (non-spatialized) channels
    QString jackDriver;                   ///< The driver for JACK to use
    quint16 oscPort;                      ///< OSC server port
    quint16 rtpPort;                      ///< Base JackTrip port
    unsigned int maxOutputChannels;       ///< the maximum number of output channels to use
    float volume;                         ///< initial global volume
    float delayMillis;                    ///< initial global delay in milliseconds
    float maxDelayMillis;                 ///< maximum global delay in milliseconds
    float maxClientDelayMillis;           ///< maximum per-client delay in milliseconds
    QString renderHost;                   ///< host for the renderer
    quint16 renderPort;                   ///< port for the renderer
    quint32 packetQueueSize; 		      ///< default client packet queue size
    qint32 clockSkewThreshold;            ///< number of samples of clock skew that must be measured before compensating
    QString outJackClientNameBasic; 	  ///< jack client name to which SAM will connect outputs
    QString outJackPortBaseBasic;   	  ///< base jack port name to which SAM will connect outputs
    QString outJackClientNameDiscrete; 	  ///< jack client name to which SAM will connect outputs
    QString outJackPortBaseDiscrete;   	  ///< base jack port name to which SAM will connect outputs
    QList<unsigned int> basicChannels; 	  ///< list of basic channels to use
    QList<unsigned int> discreteChannels; ///< list of discrete channels to use
    int maxClients;                       ///< maximum number of clients that can be connected simultaneously
    float meterIntervalMillis;            ///< milliseconds between meter broadcasts to subscribers
    bool verifyPatchVersion;              ///< whether or not the patch versions have to match during version check
    QString hostAddress;                  ///< local host address to bind to (UDP)/listen on (TCP)
    bool useGui;                          ///< whether to run in GUI mode or not
    bool printHelp;                       ///< whether or not to print help to console
};

} // end namespace sam

#endif // SAMPARAMS_H
