[← Back to README](../README.md)

# WebRTC P2P & connectivity

## How it works

After the client successfully authenticates with the agent, it automatically initiates WebRTC peer-to-peer negotiation:

1. **Client creates a PeerConnection** and a data channel named `"directgate"`
2. libdatachannel **auto-generates an SDP offer** when the data channel is created
3. The offer is **relayed to the agent** (wrapped in a `webrtc/offer` message)
4. The agent **creates an answer** and sends it back via `webrtc/answer`
5. ICE candidates are exchanged via `webrtc/ice` messages through the relay
6. Once both sides complete ICE negotiation, the **data channel opens**
7. From this point on, all terminal I/O and file transfers flow **directly between peers**
8. ICE negotiation happens only after authentication, and all traffic remains end-to-end encrypted

## ICE/TURN server configuration

For NAT traversal behind symmetric NATs you typically need a TURN server. **By default, DirectGate's own managed TURN servers are used** - there is nothing to set up, and it works out of the box (subject to a fair-use quota on your account).

If you would rather **opt out of that quota** and use your own infrastructure, add your TURN servers in your account settings on [directgate.io](https://directgate.io) - you still do not edit anything on the agent. The API delivers your configured ICE servers to the agent inside the enrollment response, and refreshes them on every token refresh, so changes you make in the dashboard are picked up automatically the next time the agent connects.

The agent selects ICE servers in this order:

1. **ICE servers delivered by the API** - by default these are DirectGate's managed TURN servers; if you add your own in settings, those are sent instead.
2. **A local `iceServers` array** in the agent config - a manual override, useful for debugging:

   ```json
   {
     "iceServers": [
       "stun:stun.cloudflare.com:3478",
       "turn:username:password@turn.example.com:3478"
     ]
   }
   ```

3. **Built-in defaults** (`stun:stun.cloudflare.com:3478` and `stun:stun.l.google.com:19302`) when neither of the above is present.

TURN server URLs follow the format `turn:user:pass@agent:port`. Up to **8** ICE servers can be used.

## Fallback

The connection degrades gracefully through three tiers, and the client always shows which one is active:

1. **Direct P2P** - a direct peer-to-peer WebRTC data channel. The client shows the **P2P** icon.
2. **TURN** - if a direct path cannot be established (e.g. symmetric NAT), WebRTC relays through a TURN server. It is still an end-to-end-encrypted WebRTC data channel, just routed via TURN; the client replaces the P2P icon with a **TURN** icon.
3. **WebSocket relay** - if WebRTC cannot connect at all (even via TURN), the session transparently falls back to the WebSocket relay path through directgate.io, and the icon disappears. End-to-end encryption is preserved, and the relay still cannot decrypt.

The terminal session stays fully functional on every tier, and traffic remains end-to-end encrypted regardless of which one is in use - so you always know whether the connection is direct, TURN-relayed, or on the WebSocket relay.

## See also

- [Architecture](architecture.md) - how the agent, relay, and client fit together
- [Configuration](configuration.md) - the `iceServers` config field
