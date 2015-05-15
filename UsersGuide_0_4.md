# User's Guide for Version 0.4 #


## Getting Started ##
Download and unzip the latest release of the code from the [Downloads page](http://code.google.com/p/streaming-audio/downloads/list).

Alternatively, check out the bleeding-edge version of the code from the [Source page](http://code.google.com/p/streaming-audio/source).

To build the code, you will need the following third-party libraries:
  * [JACK Audio Connection Kit](http://jackaudio.org) (if running on a Mac, refer to [Jack OS X](http://www.jackosx.com))
  * [Qt 4](http://qt-project.org) (minimum version 4.7)
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

| Parameter | Description | Possible Values | Comments |
|:----------|:------------|:----------------|:---------|
| `NumBasicChannels` | number of basic output channels | >= 0 | deprecated: use `BasicChannels` instead |
| `SampleRate` | audio sample rate | 44100, 48000, other values supported by your sound card |  |
| `BufferSize` | audio buffer size | 16, 32, 64, 128, 256, 512, 1024, other sizes supported by your sound card |  |
| `JackDriver` | JACK driver | `alsa`, `coreaudio`, etc. (see JACK documentation) |  |
| `OscPort` | port where SAM will listen for OSC messages |  1024 to 49151 |  |
| `RtpPort` | base port for RTP streaming | 1024 to 49151 (max depends on how many clients you wish to stream simultaneously) |  |
| `OutputPortOffset` | output port offset | >= 0 | deprecated: use `BasicChannels` and `DiscreteChannels` instead |
| `MaxOutputChannels` | maximum number of output channels to use, including basic channels | >= 1 |  |
| `Volume` | initial global volume level | 0.0 to 1.0 |  |
| `DelayMillis` | initial global delay in milliseconds | >= 0 |  |
| `MaxDelayMillis` | maximum possible delay in milliseconds | >= 0 |  |
| `RenderHost` | host name/IP address of rendering machine for automatic connection on startup | "" or "host"|  |
| `RenderPort` | port where renderer will listen to OSC messages | 1024 to 49151 |  |
| `PacketQueueSize` | length (in number of packets) of receiving packet queues in SAM | >= 1 | Increase this number to help mitigate jitter, but remember that latency will also increase |
| `BasicChannels` | string specifying which channels to use for basic output | format: "1-2,4,6-8,10-16" etc. |  |
| `DiscreteChannels` | string specifying which channels to use for discrete output | format: "1-2,4,6-8,10-16" etc. |  |
| `OutputJackClientName` | name of the JACK client to which SAM will send output audio | ex: `system` | this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.) |
| `OutputJackPortBase` | base name of JACK client ports to which SAM will connect for output | ex: `playback_` |  this parameter should only be necessary if you want SAM to output to something other than system outputs (eg max, pd etc.)  |


Example sam.conf contents:
```

[General]
SampleRate=48000
BufferSize=256
JackDriver=coreaudio
OscPort=7770
RtpPort=4464
Volume=1.0
DelayMillis=0
MaxDelayMillis=2000
RenderHost=127.0.0.1
RenderPort=7778
PacketQueueSize=4
BasicChannels="1-2"
DiscreteChannels="3-8"
OutputJackClientName="system"
OutputJackPortBase="playback_"
```

### Command Line Parameters ###

| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--numchannels` | number of basic output channels |
| `-r` | `--samplerate` | sample rate |
| `-p` | `--period` | audio period/buffer size |
| `-d` | `--driver` | driver to use for JACK (coreaudio, alsa, etc.) |
| `-o` | `--oscport`  | port where SAM listen for OSC messages |
| `-j` | `--jtport` | base RTP port |
| `-f` | `--outoffset` | output port offset |
| `-m` | `--maxout` | maximum number of output channels to use, including basic channels |
| `-h` | `--help` | print help info and exit |

Linux example usage:
```
sam -n 2 -r 48000 -p 256 -d alsa -o 7770 -j 4464 -f 0 -m 2```

OS X example usage:
```
sam -n 2 -r 48000 -p 256 -d coreaudio -o 7770 -j 4464 -f 0 -m 2```

## Using Clients ##
### saminput ###
Required parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--name` | client name |
| `-i` | `--ip` | SAM ip address |
| `-p` | `--port` | SAM OSC port |
| `-c` | `--channels` | string containing list of input channels to use, in the form "1-2,4,7-10" |
| `-t` | `--type` | type of SAM stream to use (basic = 0) |

Optional parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-x` | `--x` | initial x position |
| `-y` | `--y` | initial y position |
| `-w` | `--width` | initial width |
| `-h` | `--height` | initial height |
| `-d` | `--depth` | initial depth |

Example saminput usage:
```
saminput -n "Example Client" -i "127.0.0.1" -p 7770 -c "1-2" -t 0```

### samugen ###
Required parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-n` | `--name` | client name |
| `-i` | `--ip` | SAM ip address |
| `-p` | `--port` | SAM OSC port |
| `-c` | `--channels` | number of channels to stream |
| `-t` | `--type` | type of SAM stream to use (basic = 0) |

Optional parameters:
| Short | Long | Description |
|:------|:-----|:------------|
| `-x` | `--x` | initial x position |
| `-y` | `--y` | initial y position |
| `-w` | `--width` | initial width |
| `-h` | `--height` | initial height |
| `-d` | `--depth` | initial depth |

Example samugen usage:
```
samugen -n "Example Client" -i "127.0.0.1" -p 7770 -c 2 -t 0```