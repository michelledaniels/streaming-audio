# Communicating with SAM (0.4 branch) via OSC #
SAM uses the [Open Sound Control](http://opensoundcontrol.org/) (OSC) protocol for communicating with clients, renderers, and UIs.



## SAM/Client Messages ##

In the following messages, the word `app` refers to a SAM client.  When a client registers with SAM, it is assigned a unique identifier (non-negative integer), called `id` in the remainder of this document.


| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/app/register` | `siiiiiiii` | `name`, `channels`, `x`, `y`, `width`, `height`, `depth`, `type`, `replyPort` | `/sam/app/regconfirm` or `/sam/app/regdeny` |  |
| `/sam/app/regconfirm` | `iii` | `id`, `sampleRate`, `bufferSize`, `rtpBasePort` | none |  |
| `/sam/app/regdeny` | `i`| `errorCode` | none | error codes TBD |
| `/sam/app/unregister` | `i` | `id` | none | sending an `id` of `-1` tells SAM to unregister all clients |


## SAM/Renderer Messages ##

| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
|`/sam/render/register` | `i` | `replyPort` | `/sam/render/regconfirm`, followed by `/sam/stream/add` for each app already registered with SAM, or `/sam/render/regdeny`|  |
| `/sam/render/regconfirm`|  |  |  |  |
| `/sam/render/regdeny` | `i` | `errorCode` | none | error codes TBD |
|`/sam/render/unregister` | none  | none | none |  |
| `/sam/stream/add` | `iii...` | `id`, `type`, `channels`, followed by a list of all channels (type `i`) | none | Sent by SAM to the renderer when a new stream is added (app registered).  Specifies the output channels that stream is using. This is a variable-length message depending on the number of channels. |
| `/sam/stream/remove` | `i` | `id` | none | Sent by SAM when a stream is removed (app unregistered). |

## SAM/UI Messages ##

| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/ui/register` | `i` | `replyPort` | `/sam/ui/regconfirm` followed by `/sam/app/registered` for each app already registered with SAM, or `/sam/ui/regdeny`|  |
| `/sam/ui/regconfirm` | `iif` | `currentNumApps`, `globalMute`, `globalVolume`| none |  |
|`/sam/ui/regdeny` | `i` | `errorCode` | none | error codes TBD |
|`/sam/ui/unregister` | `i` | `replyPort` | none |  |
|`/sam/app/registered` | `isiiiiiii` | `id`, `name`, `channels`, `x`, `y`, `width`, `height`, `depth`, `type` | none |  |
| `/sam/app/unregistered` | `i` | `id` | none |  |

## Control Messages ##
| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/quit` |  |  | none | tells SAM to quit |
| `/sam/err/idinvalid` | `i` | `id` | none |sent by SAM in response to a message with an unknown/invalid id |

### Setting Parameters ###
| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/set/volume` | `if` | `id`, `volume` | none | `volume` must be in the range `[0.0, 1.0]`, `id` of `-1` sets the global SAM volume |
| `/sam/set/mute` | `ii` | `id`, `mute` | none | `mute` should be `1` to mute, `0` to unmute, `id` of `-1` mutes or unmutes SAM globally |
| `/sam/set/solo` | `ii` | `id`, `solo` | none | `solo` should be `1` to solo a client, `0` to un-solo |
| `/sam/set/delay` | `if` | `id`, `delay` | none | `delay` is specified in milliseconds and should be greater than 0 and less than or equal to the maximum delay specified in the SAM config file, `id` of `-1` sets the global delay (which is added to the per-client delay) |
| `/sam/set/position` | `iiiiii` | `id`, `x`, `y`, `width`, `height`, `depth` | none |  |
| `/sam/set/type` |  `iii` | `id`, `type`, `replyPort` | `sam/type/confirm` if able to change type or `/sam/type/deny` if unable to change type |  |
| `/sam/type/confirm` | `i` | `type` | none |  |
| `/sam/type/deny` | `i` | `errorCode` | none | error codes TBD |

### Getting Parameters ###
| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/get/volume` | `ii` | `id`, `replyPort` | `/sam/val/volume` | `id` of `-1` gets the global SAM volume |
| `/sam/val/volume` | `if` | `id`, `volume` | none | `volume` will be in the range `[0.0, 1.0]` |
| `/sam/get/mute` | `ii` | `id`, `replyPort` | `/sam/val/mute` | `id` of `-1` gets the global SAM mute status |
| `/sam/val/mute` | `ii` | `id`, `mute` | none | `mute` will be `0` if muted, `1` if not muted |
| `/sam/get/solo` | `ii` | `id`, `replyPort` | `/sam/val/solo` |  |
| `/sam/val/solo` | `ii` | `id`, `solo` | none | `solo` will be `0` if soloed, `1` if not soloed |
| `/sam/get/delay` | `ii` | `id`, `replyPort` | `/sam/val/delay` |  |
| `/sam/val/delay` | `if` | `id`, `delay` | none | `delay` is specified in milliseconds |
| `/sam/get/position` | `ii` | `id`, `replyPort` | `/sam/val/position` |  |
| `/sam/val/position` | `iiiiii` | `id`, `x`, `y`, `width`, `height`, `depth` | none |  |
| `/sam/get/type` | `ii` | `id`, `replyPort` | `/sam/val/type` |  |
| `/sam/val/type` | `ii` | `id`, `type` | none |  |
| `/sam/get/meter` | `ii` | `id`, `replyPort` | `/sam/val/meter` |  |
| `/sam/val/meter` | `iiffff...` | `id`, number of channels to follow, input rms level for ch1, input peak for ch1,  output rms level for ch1, output peak for ch1, repeated for all channels... | none | This is a variable-length message depending on the number of channels. |

## Subscriptions ##
### Subscribing to Changes ###
| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/subscribe/volume` | `ii` | `id`, `replyPort` | none | `/sam/val/volume` messages will be sent when volume changes, `id` of `-1` subscribes to the global volume parameter |
| `/sam/subscribe/mute` | `ii` | `id`, `replyPort` | none | `/sam/val/mute` messages will be sent when mute status changes, `id` of `-1` subscribes to the global mute parameter |
| `/sam/subscribe/solo` | `ii` | `id`, `replyPort` | none | `/sam/val/solo` messages will be sent when solo status changes |
| `/sam/subscribe/delay` | `ii` | `id`, `replyPort` | none | `/sam/val/delay` messages will be sent when delay changes, `id` of `-1` subscribes to the global delay parameter |
| `/sam/subscribe/position` | `ii` | `id`, `replyPort` | none | `/sam/val/position` messages will be sent when position changes |
| `/sam/subscribe/type` | `ii` | `id`, `replyPort` | none | `/sam/val/type` messages will be sent when type changes |
| `/sam/subscribe/meter` | `ii` | `id`, `replyPort` | none | `/sam/val/meter` messages will be sent at regular intervals |
| `/sam/subscribe/all` | `ii` | `id`, `replyPort` | none | subscribes to all parameters (volume, mute, solo, delay, position, type, and meter |

### Unsubscribing from Changes ###

| **OSC Address Pattern** | **OSC Type Tag String** | **OSC Arguments** | **Response** | **Comments** |
|:------------------------|:------------------------|:------------------|:-------------|:-------------|
| `/sam/unsubscribe/volume` | `ii` | `id`, `replyPort` | none | `id` of `-1` unsubscribes from the global volume parameter |
| `/sam/unsubscribe/mute` | `ii` | `id`, `replyPort` | none | `id` of `-1` unsubscribes from the global mute parameter |
| `/sam/unsubscribe/solo` | `ii` | `id`, `replyPort` | none |  |
| `/sam/unsubscribe/delay` | `ii` | `id`, `replyPort` | none | `id` of `-1` unsubscribes from the global delay parameter |
| `/sam/unsubscribe/position` | `ii` | `id`, `replyPort` | none |  |
| `/sam/unsubscribe/type` | `ii` | `id`, `replyPort` | none |  |
| `/sam/unsubscribe/meter` | `ii` | `id`, `replyPort` | none |  |
| `/sam/unsubscribe/all` | `ii` | `id`, `replyPort` | none | unsubscribes from all parameters (volume, mute, solo, delay, position, type, and meter |