#include <stdio.h>
#include <string.h>

#include "src/agent/directgate.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "connection_lifecycle_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

int DirectGate_ServiceCallback(xapi_ctx_t *pCtx, xapi_session_t *pApiSession);

static int dispatch(directgate_conn_t *pConn, xapi_session_t *pSession, xapi_cb_type_t eType)
{
    xapi_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.eCbType = eType;

    memset(pSession, 0, sizeof(*pSession));
    pSession->pSessionData = pConn;
    pSession->sock.nFD = XSOCK_INVALID;

    return DirectGate_ServiceCallback(&ctx, pSession);
}

int main(void)
{
    directgate_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    xstrncpy(cfg.auth.sSaltHex, sizeof(cfg.auth.sSaltHex),
        "00000000000000000000000000000000"
        "00000000000000000000000000000000");
    xstrncpy(cfg.auth.sVerifierHex, sizeof(cfg.auth.sVerifierHex), "configured");

    directgate_conn_t conn;
    memset(&conn, 0, sizeof(conn));
    conn.pCfg = &cfg;
    DirectGate_SessionMgr_Init(&conn.mgr, &cfg);

    xapi_session_t firstRelay;
    CHECK(dispatch(&conn, &firstRelay, XAPI_CB_CONNECTED) == XAPI_CONTINUE,
        "initial relay connection should be accepted");
    CHECK(conn.pWsSession == &firstRelay,
        "initial relay connection should be attached");
    CHECK(DirectGate_SessionMgr_IsEmpty(&conn.mgr),
        "new connection manager should be empty");

    directgate_session_t *pSession = DirectGate_SessionMgr_Create(&conn.mgr, 1);
    CHECK(pSession != NULL, "logical session should be created");
    pSession->pWsSession = &firstRelay;
    conn.mgr.nAuthWindowStartMs = XTime_GetMs();
    conn.mgr.nAuthAttempts = 7;

    CHECK(dispatch(&conn, &firstRelay, XAPI_CB_CLOSED) == XAPI_CONTINUE,
        "relay disconnect should be handled");
    CHECK(conn.pWsSession == NULL,
        "relay disconnect should detach the websocket");
    CHECK(DirectGate_SessionMgr_IsEmpty(&conn.mgr),
        "relay disconnect should remove every logical session");
    CHECK(conn.mgr.nAuthAttempts == 7,
        "relay disconnect should preserve auth-rate state");

    xapi_session_t secondRelay;
    CHECK(dispatch(&conn, &secondRelay, XAPI_CB_CONNECTED) == XAPI_CONTINUE,
        "clean relay reconnect should be accepted");
    CHECK(conn.pWsSession == &secondRelay,
        "clean relay reconnect should attach the new websocket");
    CHECK(conn.mgr.nAuthAttempts == 7,
        "relay reconnect should preserve auth-rate state");

    CHECK(dispatch(&conn, &secondRelay, XAPI_CB_CLOSED) == XAPI_CONTINUE,
        "second relay disconnect should be handled");
    CHECK(DirectGate_SessionMgr_Create(&conn.mgr, 2) != NULL,
        "test should create a stale logical session");

    xapi_session_t staleReconnect;
    CHECK(dispatch(&conn, &staleReconnect, XAPI_CB_CONNECTED) == XAPI_CONTINUE,
        "relay reconnect with stale logical sessions should recover");
    CHECK(conn.pWsSession == &staleReconnect,
        "recovered relay reconnect should attach its websocket");
    CHECK(DirectGate_SessionMgr_IsEmpty(&conn.mgr),
        "relay reconnect recovery should remove stale sessions");
    CHECK(conn.mgr.nAuthAttempts == 7,
        "stale reconnect recovery should preserve auth-rate state");

    CHECK(dispatch(&conn, &staleReconnect, XAPI_CB_CLOSED) == XAPI_CONTINUE,
        "recovered relay disconnect should be handled");
    DirectGate_SessionMgr_Destroy(&conn.mgr);
    puts("connection_lifecycle_smoke: OK");
    return 0;
}
