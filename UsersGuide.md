# User's Guide #


## Getting Started ##
Download and unzip the latest release of the code from the [Downloads page](http://code.google.com/p/streaming-audio/downloads/list).

Alternatively, check out the bleeding-edge version of the code from the [Source page](http://code.google.com/p/streaming-audio/source).

To build the code, you will need the following third-party libraries:
  * [JACK Audio Connection Kit](http://jackaudio.org) (if running on a Mac, refer to [Jack OS X](http://www.jackosx.com))
  * [Qt 4](http://qt-project.org) (minimum version 4.7) (Qt 5 should also work)
Next, build and install using the following commands:

```

./configure
make
make install
```

This will install the `sam`, `saminput`, and `samugen` console applications in `/usr/local/bin`.

NOTE: On some Linux distributions, `qmake` for Qt 4 is called `qmake-qt4` and `qmake` refers to Qt 3.  The configure script attempts to use the correct version of `qmake` for your system, but if you encounter strange `qmake` errors, you might want to double-check this.

## Using SAM ##
SAM is a highly-configurable console application.  Parameters can be specified in a file called sam.conf, which should be located in whatever directory SAM is run from.  Some parameters can be overridden on the command line, but the config file will provide more flexibility.

### SAM and JACK ###
In order to run SAM, you must also run JACK to give SAM access to your audio hardware.  If JACK is not already running when SAM starts, SAM will attempt to start JACK using the buffer size, sample rate, and driver specified in sam.conf or on the command line.  However, this approach does not allow advanced configuration of JACK.  To specify more JACK parameters, such as a specific audio device for JACK to use, it is best to start JACK yourself and then start SAM.  If SAM detects that JACK is already running, it will use that instance, assuming that the sample rate and buffer size are what SAM expects.  When SAM starts JACK itself, it will also kill JACK when it quits, assuming that SAM is shutdown cleanly.  If SAM did not start JACK, then it will not attempt to stop JACK when exiting.

Note: On most Linux systems some configuration is necessary to allow JACK to run with realtime priority etc.  The JACK website gives guidelines for this configuration.  If you are new to JACK on Linux, the [JACK FAQ page](http://jackaudio.org/faq) is a good resource.


### Config File Parameters ###
SAM can be customized using the following configuration file parameters in sam.conf.  SAM currently only searches for sam.conf _in the directory from which SAM is run_, so if it looks like your configuration is being recognized, double-check the location of sam.conf.

| Parameter | Description | Possible Values | Default | Comments |
|:----------|:------------|:----------------|:--------|:---------|
| `BasicChannels` | string specifying which channels to use for basic output | format: "1-2,4,6-8,10-16" etc. | "" |  |
| `BufferSize` | audio buffer size | 16, 32, 64, 128, 256, 512, 1024, other sizes supported by your sound card | 256 |  |
| `DelayMillis` | initial global delay in milliseconds | >= 0 | 0 |  |
| `DiscreteChannels` | string specifying which channels to use for discrete output | format: "1-2,4,6-8,10-16" etc. | "" |  |
| `HostAddress` | IP address on which to listen for incoming connections  | Any  valid IPv4 address for the SAM machine | "" | This parameter is primarily useful for machines with multiple network interfaces. |
| `JackDriver` | JACK driver | `"alsa"`, `"coreaudio"`, etc. (see JACK documentation) | `"alsa"` on linux, `"coreaudio"` on OS X |This parameter is only used if SAM needs to start JACK itself (JACK is not running before SAM is started).|
| `MaxClients` | maximum possible concurrent client connections that SAM will accept | > 0 | 100 |  |
| `MaxClientDelayMillis` | maximum possible client delay in milliseconds | >= 0 | 1000 |  |
| `MaxDelayMillis` | maximum possible global delay in milliseconds | >= 0 | 1000 |  |
| `MaxOutputChannels` | maximum number of output channels to use, including basic channels | >= 1 |  128 |  |
| `NumBasicChannels` | number of basic output channels | >= 0 | 0 | **deprecated: use `BasicChannels` instead**|
| `OscPort` | port where SAM will listen for OSC messages |  1024 to 49151 | 7770 |  |
| `OutputJackClientNameBasic` | name of the JACK client to which SAM will send basic output audio channels | ex: `"system"` | `"system"` | this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.) |
| `OutputJackPortBaseBasic` | base name of JACK client ports to which SAM will connect for basic audio output channels | ex: `"playback_"` | `"playback_"` |  this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.)  |
| `OutputJackClientNameDiscrete` | name of the JACK client to which SAM will send discrete output audio channels | ex: `"system"` | `"system"`| this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.) |
| `OutputJackPortBaseDiscrete` | base name of JACK client ports to which SAM will connect for discrete output audio channels| ex: `"playback_"` | `"playback_"` | this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.)  |
| `PacketQueueSize` | length (in number of packets) of receiving packet queues in SAM | >= 1 | 4 | Increase this number to help mitigate jitter, but remember that latency will also increase |
| `RenderHost` | host name/IP address of rendering machine for automatic connection on startup | "" or "host"| "" |  |
| `RenderPort` | port where renderer will listen to OSC messages | 1024 to 49151 | 0 |  |
| `RtpPort` | base port for RTP streaming | 1024 to 49151 (max depends on how many clients you wish to stream simultaneously) | 4464 |  |
| `SampleRate` | audio sample rate | 44100, 48000, other values supported by your sound card | 48000 |  |
| `UseGui` | whether or not to use the experimental GUI mode | 0 (run in console mode) or 1 (run in GUI mode) | 0 |  |
| `VerifyPatchVersion` | whether or not SAM will verify that clients, renderers, and UIs share the same patch version as SAM (and if not, refuse registration) | 0 (don't verify) or 1 (verify) | 0 | In release versions sharing the same minor version number all patches should be interoperable, so this parameter is mostly for use during development when different patch versions might be incompatible. |
| `Volume` | initial global volume level | 0.0 to 1.0 | 1.0 |  |

Example sam.conf contents:
```
[General]
BasicChannels="1-2"
BufferSize=256
DelayMillis=0
DiscreteChannels="3-8"
HostAddress=""
JackDriver="coreaudio"
MaxClients=100
MaxClientDelayMillis=1000
MaxDelayMillis=1000
OscPort=7770
OutputJackClientNameBasic="system"
OutputJackPortBaseBasic="playback_"
OutputJackClientNameDiscrete="system"
OutputJackPortBaseDiscrete="playback_"
PacketQueueSize=4
RenderHost=127.0.0.1
RenderPort=7778
RtpPort=4464
SampleRate=48000
UseGui=0
VerifyPatchVersion=0
Volume=1.0
```

### Command Line Parameters ###
Command-line parameters to SAM can be used to override some config file parameters, but specifying parameters via the config file is preferred.

| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--numchannels` | number of basic output channels |
| `-r` | `--samplerate` | sample rate |
| `-p` | `--period` | audio period/buffer size |
| `-d` | `--driver` | driver to use for JACK (coreaudio, alsa, etc.) |
| `-o` | `--oscport`  | port where SAM listens for OSC messages |
| `-j` | `--jtport` | base RTP port |
| `-m` | `--maxout` | maximum number of output channels to use, including basic channels |
| `-g` | `--gui` | run in GUI mode |
| `-h` | `--help` | print help info and exit |

Linux example usage:
```
sam -n 2 -r 48000 -p 256 -d alsa -o 7770 -j 4464 -m 2```

OS X example usage:
```
sam -n 2 -r 48000 -p 256 -d coreaudio -o 7770 -j 4464 -m 2```

## Using Clients ##
### saminput ###
The `saminput` client captures audio from the specified input channels of your sound card and streams it to SAM.

Required parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--name` | client name |
| `-i` | `--ip` | SAM ip address |
| `-p` | `--port` | SAM OSC port |
| `-c` | `--channels` | string containing list of input channels to use, in the form "1-2,4,7-10" |
| `-t` | `--type` | type of SAM stream to use (basic = 0) |

Optional parameters:
| Short | Long | Description | Default |
|:------|:-----|:------------|:--------|
| `-x` | `--x` | initial x position | 0 |
| `-y` | `--y` | initial y position | 0 |
| `-w` | `--width` | initial width | 0 |
| `-h` | `--height` | initial height | 0 |
| `-d` | `--depth` | initial depth | 0 |
| `-q` | `--queue` | desired receiver packet queue length | -1 (causes SAM to use its default queue length) |
| `-p`  | `--preset` | rendering type preset | 0 |

Example saminput usage:
```
saminput -n "Example Client" -i "127.0.0.1" -p 7770 -c "1-2" -t 0```

### samugen ###
The `samugen` client streams the specified number of channels of white noise to SAM.  It is primarily useful for testing purposes.

Required parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--name` | client name |
| `-i` | `--ip` | SAM ip address |
| `-p` | `--port` | SAM OSC port |
| `-c` | `--channels` | number of channels to stream |
| `-t` | `--type` | type of SAM stream to use (basic = 0) |

Optional parameters:
| Short | Long | Description | Default |
|:------|:-----|:------------|:--------|
| `-x` | `--x` | initial x position | 0 |
| `-y` | `--y` | initial y position | 0 |
| `-w` | `--width` | initial width | 0 |
| `-h` | `--height` | initial height | 0 |
| `-d` | `--depth` | initial depth | 0 |
| `-p`  | `--preset` | rendering type preset | 0 |


Example samugen usage:
```
samugen -n "Example Client" -i "127.0.0.1" -p 7770 -c 2 -t 0```