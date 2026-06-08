/*!
 * @file directgate-agent/src/client/relay.c
 * @brief Relay connection envelope fetch via API.
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

#include "includes.h"
#include "common.h"
#include "relay.h"

/*
 * One-shot relay envelope fetch via the merged session-connect endpoint.
 *
 * Replaces the legacy two-step (POST /sessions then POST /relay/connect)
 * dance with a single POST /api/v1/sessions/connect call. The API returns
 * the session id, session/device envelope, and the relay connection
 * envelope (relayUrl, browserJwt, routingKey, iceServers, exp) atomically.
 */
xbool_t DirectGate_Relay_FetchEnvelope(directgate_cfg_t *pCfg)
{
    XCHECK((pCfg != NULL), XFALSE);
    XCHECK((xstrused(pCfg->sApiUrl)), XFALSE);
    XCHECK((xstrused(pCfg->sApiToken)), XFALSE);
    XCHECK((xstrused(pCfg->sDeviceId)), XFALSE);

    if (!DirectGate_IsAPIEndpointAllowed(pCfg->sApiUrl))
    {
        xloge("Invalid or unencrypted API endpoint not allowed: apiUrl(%s)", pCfg->sApiUrl);
        return XFALSE;
    }

    char sUrl[XPATH_MAX + 64];
    xstrncpyf(sUrl, sizeof(sUrl), "%s/api/v1/sessions/connect", pCfg->sApiUrl);

    char sBody[XSTR_MID];
    xstrncpyf(sBody, sizeof(sBody), "{\"deviceId\": \"%s\"}", pCfg->sDeviceId);

    char sAuth[XSTR_MID];
    xstrncpyf(sAuth, sizeof(sAuth), "Bearer %s", pCfg->sApiToken);

    xhttp_t handle;
    XHTTP_InitRequest(&handle, XHTTP_POST, "/api/v1/sessions/connect", NULL);
    XHTTP_AddHeader(&handle, "Content-Type", "application/json");
    XHTTP_AddHeader(&handle, "Accept", "application/json");
    XHTTP_AddHeader(&handle, "Authorization", sAuth);

    xhttp_status_t status = XHTTP_EasyPerform(&handle,
        sUrl, (const uint8_t*)sBody, strlen(sBody));

    if (status != XHTTP_COMPLETE)
    {
        xloge("Session connect request failed: url(%s), status(%s)",
            sUrl, XHTTP_GetStatusStr(status));

        XHTTP_Clear(&handle);
        return XFALSE;
    }

    if (!XHTTP_IsSuccessCode(&handle))
    {
        const uint8_t *pErrBody = XHTTP_GetBody(&handle);
        size_t nErrBodySize = XHTTP_GetBodySize(&handle);

        xloge("Session connect API returned HTTP error: url(%s), code(%u)",
            sUrl, handle.nStatusCode);

        if (pErrBody && nErrBodySize > 0)
            xloge("Session connect response body: %.*s",
                (int)nErrBodySize, (const char*)pErrBody);

        XHTTP_Clear(&handle);
        return XFALSE;
    }

    const uint8_t *pRespBody = XHTTP_GetBody(&handle);
    size_t nRespSize = XHTTP_GetBodySize(&handle);

    if (pRespBody == NULL || nRespSize == 0)
    {
        xloge("Session connect response is empty");
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    xjson_t json;
    if (!XJSON_Parse(&json, NULL, (const char*)pRespBody, nRespSize))
    {
        xloge("Failed to parse session connect response JSON");
        XJSON_Destroy(&json);
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    xjson_obj_t *pRoot = json.pRootObj;
    const char *pSessionId = XJSON_GetString(XJSON_GetObject(pRoot, "sessionId"));

    if (!xstrused(pSessionId))
    {
        xloge("Session connect response is missing sessionId");
        XJSON_Destroy(&json);
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    xjson_obj_t *pRelay = XJSON_GetObject(pRoot, "relay");
    if (pRelay == NULL)
    {
        xloge("Session connect response is missing relay envelope");
        XJSON_Destroy(&json);
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    const char *pRelayUrl = XJSON_GetString(XJSON_GetObject(pRelay, "relayUrl"));
    const char *pBrowserJwt = XJSON_GetString(XJSON_GetObject(pRelay, "browserJwt"));
    const char *pRoutingKey = XJSON_GetString(XJSON_GetObject(pRelay, "routingKey"));

    if (!xstrused(pBrowserJwt))
    {
        xloge("Session connect response is missing relay.browserJwt");
        XJSON_Destroy(&json);
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    if (!xstrused(pRoutingKey))
    {
        xloge("Session connect response is missing relay.routingKey");
        XJSON_Destroy(&json);
        XHTTP_Clear(&handle);
        return XFALSE;
    }

    xstrncpy(pCfg->sAccessToken, sizeof(pCfg->sAccessToken), pBrowserJwt);
    xstrncpy(pCfg->sRoutingKey, sizeof(pCfg->sRoutingKey), pRoutingKey);

    if (xstrused(pRelayUrl))
        xstrncpy(pCfg->sSignalingUrl, sizeof(pCfg->sSignalingUrl), pRelayUrl);

    /* Load ICE servers from relay envelope if present */
    xjson_obj_t *pIce = XJSON_GetObject(pRelay, "iceServers");
    if (pIce != NULL && pIce->nType == XJSON_TYPE_ARRAY)
    {
        size_t nItems = XJSON_GetArrayLength(pIce);
        uint8_t nCount = 0;

        if (nItems > DIRECTGATE_MAX_ICE_SERVERS)
            nItems = DIRECTGATE_MAX_ICE_SERVERS;

        for (size_t i = 0; i < nItems; i++)
        {
            xjson_obj_t *pItem = XJSON_GetArrayItem(pIce, i);
            const char *pIceServer = XJSON_GetString(pItem);
            if (!xstrused(pIceServer)) continue;

            xstrncpy(pCfg->sIceServers[nCount++], DIRECTGATE_ICE_URL_SIZE, pIceServer);
        }

        if (nCount > 0) pCfg->nIceSrvCount = nCount;
    }

    xlogi("Session connect: sessionId(%s), relayUrl(%s), routingKey(%s)",
        pSessionId, pCfg->sSignalingUrl, pCfg->sRoutingKey);

    XJSON_Destroy(&json);
    XHTTP_Clear(&handle);
    return XTRUE;
}
