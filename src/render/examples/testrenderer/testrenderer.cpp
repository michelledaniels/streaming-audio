/**
 * @file testrenderer.h
 * SAM test renderer class
 * @author Michelle Daniels
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <string.h>
#include <errno.h> // for EEXIST

#include <QCoreApplication> // for QCoreApplication::quit() on disconnect

#include "testrenderer.h"

namespace sam
{

static const int MAX_PORT_NAME = 64;

TestRenderer::TestRenderer(SamRenderParams& params, const char* jackClientName, int numChannels) :
    m_jackClientName(NULL),
    m_numInputChannels(numChannels),
    m_numOutputChannels(2),
    m_client(NULL),
    m_inputPorts(NULL),
    m_outputPorts(NULL)
{
    m_params.samPort = params.samPort;
    m_params.replyPort = params.replyPort;

    int len = strlen(params.samIP);
    m_params.samIP = new char[len + 1];
    strncpy(m_params.samIP, params.samIP, len + 1);

    if (params.replyIP)
    {
        len = strlen(params.replyIP);
        m_params.replyIP = new char[len + 1];
        strncpy(m_params.replyIP, params.replyIP, len + 1);
    }
    else
    {
        m_params.replyIP = NULL;
    }

    len = strlen(jackClientName);
    m_jackClientName = new char[len + 1];
    strncpy(m_jackClientName, jackClientName, len + 1);
}

TestRenderer::~TestRenderer()
{
    if (m_client)
    {
        int result = jack_deactivate(m_client);
        if (result != 0)
        {
            qDebug("TestRenderer::~TestRenderer() couldn't deactivate jack client");
        }
    }

    // free input ports
    if (m_inputPorts)
    {
        for (int ch = 0; ch < m_numInputChannels; ch++)
        {
            if (m_inputPorts[ch])
            {
                jack_port_unregister(m_client, m_inputPorts[ch]);
                m_inputPorts[ch] = NULL;
            }
        }
        delete[] m_inputPorts;
        m_inputPorts = NULL;
    }

    // free output ports
    if (m_outputPorts)
    {
        for (int ch = 0; ch < m_numOutputChannels; ch++)
        {
            if (m_outputPorts[ch])
            {
                jack_port_unregister(m_client, m_outputPorts[ch]);
                m_outputPorts[ch] = NULL;
            }
        }
        delete[] m_outputPorts;
        m_outputPorts = NULL;
    }

    if (m_client != NULL)
    {
        // close this client
        int result = jack_client_close(m_client);
        if (result == 0)
        {
            m_client = NULL;
        }
        else
        {
            qWarning("TestRenderer::~TestRenderer() couldn't close JACK client");
        }
    }

    if (m_params.samIP)
    {
        delete[] m_params.samIP;
        m_params.samIP = NULL;
    }

    if (m_params.replyIP)
    {
        delete[] m_params.replyIP;
        m_params.replyIP = NULL;
    }

    if (m_jackClientName)
    {
        delete[] m_jackClientName;
        m_jackClientName = NULL;
    }
}

bool TestRenderer::init()
{
    if (m_renderer.init(m_params) != SAMRENDER_SUCCESS)
    {
        qWarning("Couldn't initialize SamRenderer");
        return false;
    }

    if (m_renderer.start() != SAMRENDER_SUCCESS)
    {
        qWarning("Couldn't start SamRenderer");
        return false;
    }

    // add a rendering type with two presets
    int presetIds[2] = {0, 1};
    const char* presetNames[2] = {"Mono", "Stereo"};
    m_renderer.addType(1, "Simple Mix-Down", 2, presetIds, presetNames);

    // register renderer callbacks
    m_renderer.setStreamAddedCallback(stream_added_callback, this);
    m_renderer.setStreamRemovedCallback(stream_removed_callback, this);
    m_renderer.setPositionCallback(position_changed_callback, this);
    m_renderer.setTypeCallback(type_changed_callback, this);
    m_renderer.setDisconnectCallback(render_disconnect_callback, this);

    // open a client connection to the JACK server
    jack_status_t status;
    m_client = jack_client_open(m_jackClientName, JackUseExactName, &status);
    if (m_client == NULL)
    {
        qWarning("TestRenderer::init jack_client_open() failed, status = 0x%2.0x", status);
        if (status & JackServerFailed)
        {
            qWarning("TestRenderer::init unable to connect to JACK server");
        }
        return false;
    }
    if (status & JackServerStarted)
    {
        qDebug("TestRenderer::init started new JACK server");
    }
    if (status & JackNameNotUnique)
    {
        qWarning("TestRenderer::init unique client name `%s' assigned", jack_get_client_name(m_client));
    }

    // register jack callbacks
    jack_set_process_callback(m_client, jack_process, this);

    // activate client (starts processing)
    if (jack_activate(m_client) != 0)
    {
        qWarning("TestRenderer::init couldn't activate JACK client");
        return false;
    }

    if (!init_input_ports())
    {
        qWarning("TestRenderer::init couldn't initialize input JACK ports");
        return false;
    }

    if (!init_output_ports())
    {
        qWarning("TestRenderer::init couldn't initialize output JACK ports");
        return false;
    }

    return true;
}

void TestRenderer::addStream(SamRenderStream& stream)
{
    qWarning("TestRenderer::addStream stream added with ID = %d, %d channels", stream.id, stream.numChannels);
    m_renderer.subscribeToPosition(stream.id);
}

void TestRenderer::removeStream(int id)
{
    qWarning("TestRenderer::removeStream stream with ID = %d removed", id);
}

void TestRenderer::changePosition(int id, int x, int y, int width, int height, int depth)
{
    qWarning("TestRenderer::changePosition stream with ID = %d position changed to x = %d, y = %d, width = %d, height = %d, depth = %d", id, x, y, width, height, depth);
}

void TestRenderer::changeType(int id, int type, int preset)
{
    qWarning("TestRenderer::changeType stream with ID = %d type changed to %d, preset = %d", id, type, preset);
}

int TestRenderer::processAudio(jack_nframes_t nframes)
{
    // TODO: process audio here

    return 0;
}

bool TestRenderer::init_input_ports()
{
    // get all jack ports that correspond to physical inputs
    if (!m_client) return false;
    const char** inputPorts = jack_get_ports(m_client, NULL, NULL, JackPortIsOutput | JackPortIsPhysical);

    // count the number of physical input ports
    const char** currentPort = inputPorts;
    int numPhysicalPortsIn = 0;
    while (*currentPort != NULL)
    {
        numPhysicalPortsIn++;
        qDebug("TestRenderer::init_input_ports() counted port %s", *currentPort);
        currentPort++;
    }
    qDebug("TestRenderer::init_input_ports() counted %d physical inputs", numPhysicalPortsIn);

    if (m_numInputChannels > numPhysicalPortsIn)
    {
        qWarning("Only %d physical input ports but %d were requested", numPhysicalPortsIn, m_numInputChannels);
        jack_free(inputPorts);
        return false; // requested more input ports than actually exist
    }

    // allocate array of input port pointers
    m_inputPorts = new jack_port_t*[m_numInputChannels];
    for (int ch = 0; ch < m_numInputChannels; ch++)
    {
        m_inputPorts[ch] = NULL;
    }

    // register JACK input ports and connect physical inputs to client inputs
    char portName[MAX_PORT_NAME];
    for (int i = 0; i < m_numInputChannels; i++)
    {
        // register input port
        snprintf(portName, MAX_PORT_NAME, "input_%d", i + 1);
        m_inputPorts[i] = jack_port_register(m_client, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        if (!m_inputPorts[i])
        {
            qWarning("TestRenderer::init_input_ports() ERROR: couldn't register input port for channel %d", i + 1);
            jack_free(inputPorts);
            return false;
        }

        // connect the specified physical input to the newly-created input port
        const char* inputPortName = jack_port_name(m_inputPorts[i]);
        int result = jack_connect(m_client, inputPorts[i], inputPortName);
        if (result == EEXIST)
        {
            qWarning("TestRenderer::init_input_ports() WARNING: %s and %s were already connected", inputPorts[i], inputPortName);
        }
        else if (result != 0)
        {
            qWarning("TestRenderer::init_input_ports() ERROR: couldn't connect %s to %s", inputPorts[i], inputPortName);
            jack_free(inputPorts);
            return false;
        }
    }

    jack_free(inputPorts);
    return true;
}

bool TestRenderer::init_output_ports()
{
    // get all jack ports that correspond to physical inputs
    if (!m_client) return false;
    const char** outputPorts = jack_get_ports(m_client, NULL, NULL, JackPortIsInput | JackPortIsPhysical);

    // count the number of physical output ports
    const char** currentPort = outputPorts;
    int numPhysicalPortsOut = 0;
    while (*currentPort != NULL)
    {
        numPhysicalPortsOut++;
        qDebug("TestRenderer::init_output_ports() counted port %s", *currentPort);
        currentPort++;
    }
    qDebug("TestRenderer::init_output_ports() counted %d physical outputs", numPhysicalPortsOut);

    if (m_numOutputChannels > numPhysicalPortsOut)
    {
        qWarning("Only %d physical output ports but %d were requested", numPhysicalPortsOut, m_numOutputChannels);
        jack_free(outputPorts);
        return false; // requested more output ports than actually exist
    }

    // allocate array of input port pointers
    m_outputPorts = new jack_port_t*[m_numOutputChannels];
    for (int ch = 0; ch < m_numOutputChannels; ch++)
    {
        m_outputPorts[ch] = NULL;
    }

    // register JACK input ports and connect physical inputs to client inputs
    char portName[MAX_PORT_NAME];
    for (int i = 0; i < m_numOutputChannels; i++)
    {
        // register output port
        snprintf(portName, MAX_PORT_NAME, "output_%d", i + 1);
        m_outputPorts[i] = jack_port_register(m_client, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!m_outputPorts[i])
        {
            qWarning("TestRenderer::init_output_ports() ERROR: couldn't register output port for channel %d", i + 1);
            jack_free(outputPorts);
            return false;
        }

        // connect the specified physical output to the newly-created output port
        const char* outputPortName = jack_port_name(m_outputPorts[i]);
        int result = jack_connect(m_client, outputPortName, outputPorts[i]);
        if (result == EEXIST)
        {
            qWarning("TestRenderer::init_output_ports() WARNING: %s and %s were already connected", outputPortName, outputPorts[i]);
        }
        else if (result != 0)
        {
            qWarning("TestRenderer::init_output_ports() ERROR: couldn't connect %s to %s", outputPortName, outputPorts[i]);
            jack_free(outputPorts);
            return false;
        }
    }

    jack_free(outputPorts);
    return true;
}

// ----- static callbacks -----
void TestRenderer::stream_added_callback(SamRenderStream& stream, void* renderer)
{
    ((TestRenderer*)renderer)->addStream(stream);
}

void TestRenderer::stream_removed_callback(int id, void* renderer)
{
    ((TestRenderer*)renderer)->removeStream(id);
}

void TestRenderer::position_changed_callback(int id, int x, int y, int width, int height, int depth, void* renderer)
{
    ((TestRenderer*)renderer)->changePosition(id, x, y, width, height, depth);
}

void TestRenderer::type_changed_callback(int id, int type, int preset, void* renderer)
{
    ((TestRenderer*)renderer)->changeType(id, type, preset);
}

void TestRenderer::render_disconnect_callback(void*)
{
    qWarning("TestRenderer::render_disconnect_callback lost connection with SAM, shutting down...");
    QCoreApplication::quit();
}

int TestRenderer::jack_process(jack_nframes_t nframes, void* renderer)
{
    ((TestRenderer*)renderer)->processAudio(nframes);
    return 0;
}

} // namespace sam
