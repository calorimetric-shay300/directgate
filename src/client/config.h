/*!
 * @file directgate-agent/src/client/config.h
 * @brief Client config and CLI parsing.
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

#ifndef __DIRECTGATE_CLIENT_CONFIG_H__
#define __DIRECTGATE_CLIENT_CONFIG_H__

#include "includes.h"
#include "auth.h"
#include "logger.h"
#include "webrtc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct directgate_cfg_ {
    directgate_ice_server_t sIceServers[DIRECTGATE_MAX_ICE_SERVERS];
    char sSignalingUrl[XPATH_MAX];
    char sDeviceList[XPATH_MAX];
    char sAccessToken[XSTR_MID];
    char sRoutingKey[XSTR_MID];
    char sApiUrl[XPATH_MAX];
    char sApiToken[XSTR_MID];
    char sDeviceName[XSTR_MID];
    char sDeviceId[XSTR_MID];
    char sCfgPath[XPATH_MAX];
    char sSecret[XSTR_MID];
    directgate_auth_t auth;
    directgate_log_t log;
    uint16_t nVerbose;
    uint8_t nIceSrvCount;
    xbool_t bSaveDevice;
    xbool_t bForce;
    xbool_t bInit;
} directgate_cfg_t;

void DirectGate_DisplayUsage(const char *pName);
void DirectGate_InitConfig(directgate_cfg_t *pCfg);

xbool_t DirectGate_LoadConfig(directgate_cfg_t *pCfg, const char *pPath);
XSTATUS DirectGate_ParseArgs(directgate_cfg_t *pCfg, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
