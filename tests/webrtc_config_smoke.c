#include <stdio.h>
#include <string.h>

#include "src/common/webrtc.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "webrtc_config_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

int main(void)
{
    const char *pCustomJson =
        "{"
            "\"iceServers\":["
                "\"turn:one.example.test\","
                "\"stun:two.example.test\","
                "\"turn:three.example.test\","
                "\"stun:four.example.test\","
                "\"turn:five.example.test\","
                "\"stun:six.example.test\","
                "\"turn:seven.example.test\","
                "\"stun:eight.example.test\","
                "\"turn:nine.example.test\""
            "]"
        "}";

    xjson_t json;
    CHECK(XJSON_Parse(&json, NULL, pCustomJson, strlen(pCustomJson)),
        "parse custom ICE JSON");

    directgate_ice_server_t servers[DIRECTGATE_MAX_ICE_SERVERS];
    uint8_t nCount = 0;
    CHECK(DirectGate_WebRTC_LoadIceServers(servers, &nCount, json.pRootObj),
        "load custom ICE servers");
    CHECK(nCount == DIRECTGATE_MAX_ICE_SERVERS, "custom ICE server cap");
    CHECK(strcmp(servers[0], "turn:one.example.test") == 0,
        "custom ICE first entry");
    CHECK(strcmp(servers[DIRECTGATE_MAX_ICE_SERVERS - 1], "stun:eight.example.test") == 0,
        "custom ICE eighth entry");
    XJSON_Destroy(&json);

    const char *pDefaultJson = "{}";
    CHECK(XJSON_Parse(&json, NULL, pDefaultJson, strlen(pDefaultJson)),
        "parse default ICE JSON");
    memset(servers, 0, sizeof(servers));
    nCount = 0;
    CHECK(DirectGate_WebRTC_LoadIceServers(servers, &nCount, json.pRootObj),
        "load default ICE servers");
    CHECK(nCount == 2, "default ICE server count");
    CHECK(strcmp(servers[0], "stun:stun.cloudflare.com:3478") == 0,
        "default ICE first entry");
    CHECK(strcmp(servers[1], "stun:stun.l.google.com:19302") == 0,
        "default ICE second entry");
    XJSON_Destroy(&json);

    directgate_webrtc_t rtc;
    DirectGate_WebRTC_Init(&rtc);

    directgate_ice_server_t configured[3];
    memset(configured, 0, sizeof(configured));
    xstrncpy(configured[0], sizeof(configured[0]), "turn:configured.example.test");
    configured[1][0] = '\0';
    xstrncpy(configured[2], sizeof(configured[2]), "stun:configured.example.test");
    DirectGate_WebRTC_SetIceServers(&rtc, configured, 3);

    CHECK(rtc.nIceSrvCount == 2, "set ICE skips empty entries");
    CHECK(strcmp(rtc.sIceServers[0], "turn:configured.example.test") == 0,
        "set ICE first entry");
    CHECK(strcmp(rtc.sIceServers[1], "stun:configured.example.test") == 0,
        "set ICE second entry");
    DirectGate_WebRTC_Clear(&rtc);

    // BYO-shape URLs: longer creds, turns:, ?transport=tcp, plus stun colocation.
    const char *pByoJson =
        "{"
            "\"iceServers\":["
                "\"stun:stun.cloudflare.com:3478\","
                "\"stun:stun.l.google.com:19302\","
                "\"turn:alice:supers3cret-credential-that-is-fairly-long@relay.example.com:3478?transport=udp\","
                "\"turns:alice:supers3cret-credential-that-is-fairly-long@relay.example.com:5349?transport=tcp\""
            "]"
        "}";
    CHECK(XJSON_Parse(&json, NULL, pByoJson, strlen(pByoJson)),
        "parse BYO ICE JSON");
    memset(servers, 0, sizeof(servers));
    nCount = 0;
    CHECK(DirectGate_WebRTC_LoadIceServers(servers, &nCount, json.pRootObj),
        "load BYO ICE servers");
    CHECK(nCount == 4, "BYO ICE server count");
    CHECK(strcmp(servers[0], "stun:stun.cloudflare.com:3478") == 0,
        "BYO first stun");
    CHECK(strcmp(servers[2], "turn:alice:supers3cret-credential-that-is-fairly-long@relay.example.com:3478?transport=udp") == 0,
        "BYO turn url");
    CHECK(strcmp(servers[3], "turns:alice:supers3cret-credential-that-is-fairly-long@relay.example.com:5349?transport=tcp") == 0,
        "BYO turns url");
    XJSON_Destroy(&json);

    puts("webrtc_config_smoke: OK");
    return 0;
}
