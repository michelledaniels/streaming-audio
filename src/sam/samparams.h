/**
 * @file samparams.cpp
 * Interface for parsing SAM parameters
 * @author Michelle Daniels
 * @date September 2013
 * @copyright UCSD 2013
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
