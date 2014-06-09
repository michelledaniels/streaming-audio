/**
 * @file testrenderer.h
 * SAM test renderer class
 * @author Michelle Daniels
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TESTRENDERER_H
#define TESTRENDERER_H

#include "jack/jack.h"

#include "samrenderer.h"

namespace sam
{

class TestRenderer
{
public:
    TestRenderer(SamRenderParams& params, const char* jackClientName, int numChannels);
    ~TestRenderer();
    bool init();
    void addStream(SamRenderStream& stream);
    void removeStream(int id);
    void changePosition(int id, int x, int y, int width, int height, int depth);
    void changeType(int id, int type, int preset);
    int processAudio(jack_nframes_t nframes);

private:

    bool init_input_ports();
    bool init_output_ports();

    static void stream_added_callback(SamRenderStream& stream, void* renderer);
    static void stream_removed_callback(int id, void* renderer);
    static void position_changed_callback(int id, int x, int y, int width, int height, int depth, void* renderer);
    static void type_changed_callback(int id, int type, int preset, void* renderer);
    static void render_disconnect_callback(void* renderer);
    static int jack_process(jack_nframes_t nframes, void* renderer);

    SamRenderer m_renderer;
    SamRenderParams m_params;

    // for JACK
    char* m_jackClientName;      ///< JACK client name
    int m_numInputChannels;      ///< number of input channels
    int m_numOutputChannels;     ///< number of output channels
    jack_client_t* m_client;     ///< local JACK client
    jack_port_t** m_inputPorts;  ///< array of JACK input ports
    jack_port_t** m_outputPorts; ///< array of JACK output ports
};

} // namespace sam

#endif // TESTRENDERER_H
