#include <stdio.h>
#include <unistd.h>

#include "libxutils/src/net/api.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "api_invalid_role_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

typedef struct {
    xapi_session_t *pSession;
    int nErrors;
    int nClosed;
    int nRegistered;
} test_ctx_t;

static int api_callback(xapi_ctx_t *pCtx, xapi_session_t *pSession)
{
    test_ctx_t *pTest = NULL;
    if (pCtx != NULL && pCtx->pApi != NULL)
        pTest = (test_ctx_t*)pCtx->pApi->pUserCtx;

    if (pTest == NULL || pCtx == NULL)
        return XAPI_CONTINUE;

    if (pCtx->eCbType == XAPI_CB_REGISTERED)
    {
        pTest->pSession = pSession;
        pTest->nRegistered++;
    }
    else if (pCtx->eCbType == XAPI_CB_ERROR)
    {
        pTest->nErrors++;
    }
    else if (pCtx->eCbType == XAPI_CB_CLOSED)
    {
        pTest->nClosed++;
    }

    return XAPI_CONTINUE;
}

int main(void)
{
    int fds[2] = { -1, -1 };
    CHECK(pipe(fds) == 0, "pipe failed");

    test_ctx_t test = { 0 };
    xapi_t api;
    CHECK(XAPI_Init(&api, api_callback, &test) == XSTDOK, "api init failed");

    xapi_endpoint_t endpt;
    XAPI_InitEndpoint(&endpt);
    endpt.eType = XAPI_WS;
    endpt.eRole = XAPI_CUSTOM;
    endpt.nFD = fds[0];
    endpt.nEvents = XPOLLIN;
    endpt.pSessionData = &test;

    CHECK(XAPI_AddEvent(&api, &endpt) == XSTDOK, "api add event failed");
    CHECK(test.nRegistered == 1, "registered callback missing");
    CHECK(test.pSession != NULL, "registered session missing");

    test.pSession->eRole = XAPI_INACTIVE;
    test.pSession->pSessionData = NULL;

    const char cByte = 'x';
    CHECK(write(fds[1], &cByte, sizeof(cByte)) == (ssize_t)sizeof(cByte), "pipe write failed");
    CHECK(XAPI_Service(&api, 100) == XEVENTS_SUCCESS, "api service failed");
    CHECK(test.nErrors == 0, "inactive role was reported as api error");
    CHECK(test.nClosed == 0, "inactive role dispatched closed callback");

    close(fds[1]);
    XAPI_Destroy(&api);

    printf("api_invalid_role_smoke: OK\n");
    return 0;
}
