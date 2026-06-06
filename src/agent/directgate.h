/*!
 * @file directgate-agent/src/agent/directgate.h
 * @brief Agent-side WS client that exposes a PTY over the bridge server.
 *
 *  Copyright (c) 2025-2026 DirectGate. All rights reserved.
 *  Author: Sandro Kalatozishvili (sandro@directgate.io)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __DIRECTGATE_H__
#define __DIRECTGATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "includes.h"
#include "session.h"
#include "config.h"

typedef struct directgate_conn_ {
    xlink_t relayLink;
    directgate_cfg_t *pCfg;
    directgate_session_mgr_t mgr;
    xapi_session_t *pWsSession;
    uint32_t nReconnectAttempt;
    uint64_t nNextReconnectMs;
    uint64_t nLastRelayRecvMs;
    /* Force one API refresh before the first connect of this process so relay
     * assignment is re-evaluated on agent startup instead of blindly reusing
     * the relayUrl persisted in the config. */
    xbool_t bStartupRelayRefreshDone;
    /* Throttle window for re-resolving the relay URL via the API after
     * sustained connect failures. Set to "now + cooldown" right after a
     * probe so we don't hammer the API on every backoff tick. */
    uint64_t nNextRefreshProbeMs;
    xstr_tiny_t sDisconnectReason;
    xbool_t bReconnectSuppressed;
    xbool_t bRoleSent;
} directgate_conn_t;

#ifdef __cplusplus
}
#endif

#endif
