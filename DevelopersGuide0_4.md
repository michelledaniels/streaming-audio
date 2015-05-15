# Developer's Guide for Version 0.4 #


[Comprehensive List of OSC Messages (version 0.4)](OSCMessages_0_4.md)

## Developing Clients ##
The streaming audio project operates under a client/server model.  The Streaming Audio Manager (SAM) is the server, and an arbitrary number of Streaming Audio Clients (SACs) can connect and send audio to a single SAM instance.  The Streaming Audio Client Library provides the functionality necessary for connecting a client and streaming audio to SAM.  The SAC library encapsulates OSC message sending and receiving between the client and SAM as well as RTP audio streaming from the client to SAM.  Multiple SACs can be instantiated in a single application to stream audio to multiple SAMs, provided that the multiple SAMs have been configured to use different ranges of RTP ports.

### Example Clients ###
Two example SAM clients are available in the src/client/examples directory.  The `samugen` client can be built without JACK if desired, and demonstrates how to send client-generated audio to SAM via an audio callback.  The `saminput` client requires JACK and pulls audio from the physical audio interface inputs rather than generating audio in the client itself using an audio callback.  Both clients are simple command-line applications that use Qt and generate makefiles using `qmake`.

### Dependencies ###
The SAC library uses Qt 4, version 4.7 or higher.  The default SAC build links dynamically against the Qt libraries, so client applications must also link against them.  If using qmake, the .pro files for the example clients described above are good references.

SAC also links dynamically against the JACK Audio Connection Kit, unless you define SAC\_NO\_JACK when running qmake on libsac.pro.

To account for these dependencies, if you are writing your own makefile, you will probably need to add a variation of the following for headers:
```
-I/usr/include/QtCore -I/usr/include/QtNetwork -I$(SAM_ROOT)/src -I$(SAM_ROOT)/src/client
```

and a variation of the following for the linker:
```
-L/usr/lib64/qt48 -lQtNetwork -lQtCore -L$(SAM_ROOT)/lib -lsac -ljack
```


### Initializing a Client ###
To initialize a client, you must provide the number of channels of audio you wish to send to SAM, the rendering type for this client, a user-readable name for the client (for use in UIs, for example), the IP address of the SAM instance this client will connect to, and the port on which the SAM instance is listening for OSC messages.
```
#include "sam_client.h"
using namespace sam;
StreamingAudioClient sac;
if (sac.init(numChannels, type, name, samIP, samPort) != SAC_SUCCESS)
{
    printf("Couldn't initialize client");
}

```

Note that `StreamingAudioClient` is in the `sam` namespace.

### The Streaming Audio Callback ###
The streaming audio callback is called each time the client is ready to send a buffer of samples to SAM.  Inside, you can generate your own audio, pull audio from physical inputs, copy audio from another sample buffer, etc.
```
bool sac_audio_callback(unsigned int nChannels, unsigned int nFrames, float** out, void* data)
{
    // play silence
    for (unsigned int ch = 0; ch < nChannels; ch++)
    {
        for (unsigned int n = 0; n < nFrames; n++)
        {
            out[ch][n] = 0.0f;
        }
    }
    return true;
}
```

Then register the audio callback with your `StreamingAudioClient`:
```
sac.setAudioCallback(sac_audio_callback, NULL);
```
As with other callbacks, the second parameter to `setAudioCallback` is a pointer to data that the client will pass you each time the callback is called.  If you have no need to maintain state between callbacks, pass `NULL` as the second parameter.

### Streaming From Physical Audio Inputs ###
To create a client that will stream audio directly from the physical audio inputs of the machine, you can write an audio callback which will pull audio from those inputs.  Alternatively, the streaming audio client library provides a built-in method for doing exactly that when using JACK.  To use it, do not register an audio callback. Instead, use the following:
```
sac.setPhysicalInputs(inputChannels);
```
Here, `inputChannels` is an array of `unsigned int`s indicating the input channels that will be used.

The call to `setPhysicalInputs` must be made after the client has already been initialized and started and will return an error otherwise.


### Driving Audio Externally ###
(Introduced in version 0.6)
In some cases, you may wish to drive the audio streaming with an external clock rather than with the internal JACK-based clock or virtual clock.  To enable this, initialize the SAC instance with the driveExternally flag set to true.  Do not call `setAudioCallback()` or `setPhysicalInputs()`.

```
if (sac.init(numChannels, type, name, samIP, samPort, 0, PAYLOAD_PCM_16, true) != SAC_SUCCESS)
{
    printf("Couldn't initialize client");
}

```

Then start the client as described above.  After starting the client, you must query the SAC instance for the buffer size and sample rate that SAM expects the client to use:

```
unsigned int bufferSize = sac.getBufferSize();
unsigned int sampleRate = sac.getSampleRate();

```

You are then responsible for calling `sendAudio` once every `bufferSize` samples, at the `sampleRate` returned, providing a buffer of samples to send to SAM in the form `in[channels][sample]`.

```
if (sendAudio(buffer) != SAC_SUCCESS)
{
    printf("Couldn't send audio to SAM");
}

```

### Starting a Client ###
After successfully initializing the client, the client must be started:
```
if (sac.start(x, y, width, height, depth, timeout) != SAC_SUCCESS)
{
    printf("Couldn't start client");
}

```

Starting the client initiates the process of registering with SAM using OSC over TCP.  The method then blocks waiting for a response from SAM.  You can specify a timeout (in milliseconds) for the blocking/waiting, after which point `start` will return an error code indicating that it has timed out and did not receive a response from SAM.  If starting was successful, a TCP connection between the client and SAM remains in place for the exchange of OSC messages until the client is stopped.

### Position Information ###

The position data passed to `start` is intended to provide the current position of a client's corresponding video for use in advanced rendering modes.  If your client has no corresponding video or image, or has no need of spatial rendering, those parameters will be ignored.  If you do wish to take advantage of spatial rendering modes that rely on position information, then whenever your client changes position you must update SAM:
```
sac.setPosition(x, y, width, height, depth);
```

### Stopping a Client ###
To stop a client, simply destroy the `StreamingAudioClient` object.  This will cause the client to unregister with SAM and clean itself up neatly.  If your application crashes and the `StreamingAudioClient` destructor is never called, your app may remain registered with SAM.  However, SAM should detect that its TCP connection with the client has been disconnected and correctly unregister the client.

Note that unregistering a client with SAM causes the client's unique ID to be released for reuse by another client and loses information about parameters set for that client in SAM (volume, mute, delay, etc.).  SAM currently has no concept of "pausing" a client, so if you don't wish to unregister your client but simply have no audio data to send, you should have your audio callback send silence to SAM until more audio data is available.

## Developing Renderers ##
Renderers provide advanced rendering modes to extend SAM functionality. For example, when using SAM to provide audio for large-scale tiled display environments such as SAGE, a renderer can be used to spatially align audio with a client's visual image.

Internally, SAM understands two types of output: basic and discrete.  Clients using basic output (always rendering type 0) are all mixed down to the basic output channels specified in the SAM config file (typically two channels).  Clients using discrete output (non-zero rendering type) are allocated output channels and sent discretely to the renderer with no mixing in SAM.
In this version of SAM (v0.4), only one renderer may be registered with SAM at any given time.  However, that renderer is allowed to support multiple rendering modes/types.

### Connecting SAM and a Renderer ###
The renderer may run on a separate machine from SAM, in which case the two machines must be connected by physical audio interfaces and cables, and the number of clients which can use discrete rendering is limited by the number of physical channels connecting the two machines.  Alternatively, the renderer may run on the same machine as SAM.  In this case, routing between SAM and the renderer is performed using JACK.  The renderer should be running before SAM in this case, and the SAM config file must specify the renderer's JACK client name and how it names its ports.

### Registering a Renderer ###
The renderer must register with SAM in order to receive notifications of clients added or removed.  The OSC messages used for SAM/renderer communication can be found [here](OSCMessages_0_4#SAM/Renderer_Messages.md).  SAM can auto-register an already-running renderer at startup time if the renderer's IP address and OSC listening port are specified in the SAM config file.  If a renderer receives a `/sam/render/regdeny` message when attempting to register, you should confirm that the SAM config file does not specify a renderer for auto-registration, since only one renderer can be registered at a time.

### Adding a Client ###
When a new client registers with SAM, SAM notifies the renderer and tells the renderer what type of rendering the client has requested and what output channels SAM has allocated for that client (note that the client's allocated channels can be non-consecutive).  The renderer must know how SAM's output channels are connected to its own input channels so it knows what input channels that client's audio will arrive on.  The renderer is then responsible for rendering the client audio in the requested manner.


## Developing User Interfaces ##
SAM UIs communicate with SAM using Open Sound Control (OSC) messages.  OSC messages can be sent over UDP or over TCP using SLIP encoding.

### Registering a UI with SAM ###
A UI must register with SAM in order to receive notifications of clients being added or removed as well as global parameter updates (volume changed etc.).  The format of the OSC messages for registering and unregistering a UI can be found [here](OSCMessages_0_4#SAM/UI_Messages.md).

### Subscribing to Parameter Changes ###
When a client is added, SAM notifies all registered UIs and provides a unique ID for the client. If a UI wishes to receive notifications when that client's parameters change, the UI must subscribe to the parameters of interest using the OSC messages listed [here](OSCMessages_0_4#Subscribing_to_Changes.md). All subscription messages require the unique ID for the client to which the UI wishes to subscribe.

Subscribers are notified when a parameter has changed using `/sam/val` messages described [here](OSCMessages_0_4#Getting_Parameters.md). To request the current value of a parameter at other times, the `/sam/get` messages listed [here](OSCMessages_0_4#Getting_Parameters.md) can also be used. Metering updates are broadcast once per second. UIs can unsubscribe from parameter notifications using the messages [here](OSCMessages_0_4#Unsubscribing_from_Changes.md).


### Setting Parameter Values ###
The `/sam/set` messages listed [here](OSCMessages_0_4#Setting_Parameters.md) can be used to set the value of a parameter. Global parameters are set by using an id of -1, while client parameters are set using the unique client ID provided to the UI when the client was added.