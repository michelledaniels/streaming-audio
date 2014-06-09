/**
 * @file sam_shared.h
 * Location for things shared between SAM, clients, and renderers
 * @author Michelle Daniels
 * @date 2014
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef SAM_SHARED_H
#define SAM_SHARED_H

namespace sam
{
static const int VERSION_MAJOR = 0;
static const int VERSION_MINOR = 9;
static const int VERSION_PATCH = 0;

/**
 * @enum SamErrorCode
 * Possible error codes returned by SAM in OSC messages to clients, renderers, and UIs.
 */
enum SamErrorCode
{
    SAM_ERR_DEFAULT = 0,        ///< non-specific error
    SAM_ERR_VERSION_MISMATCH,   ///< the client, renderer, or UI version didn't match SAM's version
    SAM_ERR_MAX_CLIENTS,        ///< the maximum number of clients has been reached
    SAM_ERR_NO_FREE_OUTPUT,     ///< there are no more output channels (JACK ports) available in SAM
    SAM_ERR_INVALID_ID,         ///< invalid client id
    SAM_ERR_INVALID_TYPE        ///< invalid rendering type
};

/**
 * @enum StreamingAudioType
 * The possible types of audio streams for an app.
 * @deprecated possible non-basic types are now defined dynamically by the renderer
 */
enum StreamingAudioType
{
    TYPE_BASIC = 0
};

}
#endif // SAM_SHARED_H
