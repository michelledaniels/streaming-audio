## Overview ##
This streaming audio project consists of a single "Streaming Audio Manager" (SAM) which receives uncompressed audio via RTP streams from an arbitrary number of clients over high-performance wide-area networks.  Control data is exchanged via SAM and clients using Open Sound Control (OSC).  OSC is also used to communicate with third-party user interfaces.  Advanced audio rendering modules (spatialization, etc.) can be added on to SAM using a rendering interface.

On this site, you will find the source code for both SAM and a SAM client library, which can be used to develop new SAM clients.  Some example SAM client applications are provided.  SAM currently depends on Qt 4 and the JACK Audio Connection Kit audio API, and is only being tested for Linux and Mac OS X at this time.  Clients can run on Linux, Mac, or Windows systems, even on machines with no sound card.

[User's Guide](UsersGuide.md)

[Developer's Guide](DevelopersGuide.md)

## Announcements ##
**June 9, 2014**: To help support continuing work on this project, the license for SAM, the client library, the rendering library, and the soon-to-be-released UI library has been changed from the New BSD License to a University of California license.  This change applies to versions 0.9.0 and above.  The new license is available [here](http://streaming-audio.googlecode.com/svn/trunk/license.txt).  Unless you are using this software for commercial purposes, this change will not impact you.

**December 11, 2013**: A Google Group has been created for users of this streaming audio system: [Streaming Audio Google Group](https://groups.google.com/forum/#!forum/streamingaudio)

**October 20, 2013**: Michelle Daniels, creator and primary developer of this system, won a gold award in the Audio Engineering Society's Student Design Competition for her work on this project!

**September 18, 2013**: Version 0.6.0 has been released.  See [the revision log](RevisionLog.md) for details.  Updates to documentation are in progress and will follow soon.