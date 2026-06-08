#include <stdio.h>
#include <string.h>

#include "src/agent/config.h"
#include "src/agent/enroll.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "enroll_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

int main(void)
{
    directgate_cfg_t cfg;
    DirectGate_InitConfig(&cfg);

    xstrncpy(cfg.sDeviceId, sizeof(cfg.sDeviceId), "device-1");
    xstrncpy(cfg.enroll.sApiUrl, sizeof(cfg.enroll.sApiUrl), "https://api.example.test");

    const char *pPairJson =
        "{"
            "\"deviceId\":\"device-1\","
            "\"accessToken\":\"access-1\","
            "\"accessTokenExpiresAt\":\"2026-04-01T12:00:00.000Z\","
            "\"refreshToken\":\"refresh-1\","
            "\"refreshTokenExpiresAt\":\"2026-05-01T12:00:00.000Z\","
            "\"enrollmentExpiresAt\":\"2026-05-01T12:00:00.000Z\","
            "\"relayUrl\":\"wss://relay.example.test/websock\","
            "\"routingKey\":\"rk-1\""
        "}";

    CHECK(DirectGate_Enroll_ApplyPairResponse(&cfg, (const uint8_t*)pPairJson, strlen(pPairJson)),
        "pair response should parse");
    CHECK(cfg.enroll.bEnrolled, "pair response should mark agent enrolled");
    CHECK(strcmp(cfg.enroll.sAccessToken, "access-1") == 0, "pair response should store access token");
    CHECK(strcmp(cfg.enroll.sRefreshToken, "refresh-1") == 0, "pair response should store refresh token");
    CHECK(strcmp(cfg.sRelayUrl, "wss://relay.example.test/websock") == 0, "pair response should store relay url");
    CHECK(strcmp(cfg.sRoutingKey, "rk-1") == 0, "pair response should store routing key");
    CHECK(strcmp(cfg.enroll.sEnrollExpiresAt, "2026-05-01T12:00:00.000Z") == 0,
        "pair response should store enrollment expiry");
    CHECK(cfg.enroll.nRefreshTokenExp > 0, "pair response should parse refresh expiry");

    uint64_t nInitialRefreshExp = cfg.enroll.nRefreshTokenExp;
    char sInitialRefreshToken[XSTR_MIN];
    xstrncpy(sInitialRefreshToken, sizeof(sInitialRefreshToken), cfg.enroll.sRefreshToken);

    const char *pRefreshStableJson =
        "{"
            "\"deviceId\":\"device-1\","
            "\"accessToken\":\"access-2\","
            "\"accessTokenExpiresAt\":\"2026-04-01T12:10:00.000Z\","
            "\"refreshTokenRotated\":false,"
            "\"enrollmentExpiresAt\":\"2026-05-01T12:00:00.000Z\","
            "\"relayUrl\":\"wss://relay.example.test/websock\","
            "\"routingKey\":\"rk-1\""
        "}";

    CHECK(DirectGate_Enroll_ApplyRefreshResponse(&cfg,
        (const uint8_t*)pRefreshStableJson, strlen(pRefreshStableJson)),
        "stable refresh response should parse");
    CHECK(strcmp(cfg.enroll.sAccessToken, "access-2") == 0,
        "stable refresh should replace access token");
    CHECK(strcmp(cfg.enroll.sRefreshToken, sInitialRefreshToken) == 0,
        "stable refresh should keep refresh token");
    CHECK(cfg.enroll.nRefreshTokenExp == nInitialRefreshExp,
        "stable refresh should keep refresh expiry");

    const char *pRefreshRotateJson =
        "{"
            "\"deviceId\":\"device-1\","
            "\"accessToken\":\"access-3\","
            "\"accessTokenExpiresAt\":\"2026-04-01T12:20:00.000Z\","
            "\"refreshTokenRotated\":true,"
            "\"refreshToken\":\"refresh-2\","
            "\"refreshTokenExpiresAt\":\"2026-06-01T12:00:00.000Z\","
            "\"enrollmentExpiresAt\":\"2026-06-01T12:00:00.000Z\","
            "\"relayUrl\":\"wss://relay.example.test/websock\","
            "\"routingKey\":\"rk-1\""
        "}";

    CHECK(DirectGate_Enroll_ApplyRefreshResponse(&cfg,
        (const uint8_t*)pRefreshRotateJson, strlen(pRefreshRotateJson)),
        "rotated refresh response should parse");
    CHECK(strcmp(cfg.enroll.sRefreshToken, "refresh-2") == 0,
        "rotated refresh should replace refresh token");
    CHECK(strcmp(cfg.enroll.sEnrollExpiresAt, "2026-06-01T12:00:00.000Z") == 0,
        "rotated refresh should update enrollment expiry");
    CHECK(cfg.enroll.nRefreshTokenExp > nInitialRefreshExp,
        "rotated refresh should update refresh expiry");

    char sReason[XSTR_TINY];
    directgate_enroll_status_t eStatus = DirectGate_Enroll_ClassifyRefreshFailure(
        &cfg,
        401,
        (const uint8_t*)"{\"code\":\"DEVICE_ENROLLMENT_EXPIRED\",\"message\":\"expired\"}",
        strlen("{\"code\":\"DEVICE_ENROLLMENT_EXPIRED\",\"message\":\"expired\"}"),
        sReason,
        sizeof(sReason));

    CHECK(eStatus == DIRECTGATE_ENROLL_REFRESH_TERMINAL,
        "expired enrollment should be terminal");
    CHECK(strcmp(sReason, "device-enrollment-expired") == 0,
        "expired enrollment should map to device-enrollment-expired");

    eStatus = DirectGate_Enroll_ClassifyRefreshFailure(
        &cfg,
        401,
        (const uint8_t*)"{\"code\":\"INVALID_REFRESH_TOKEN\",\"message\":\"invalid\"}",
        strlen("{\"code\":\"INVALID_REFRESH_TOKEN\",\"message\":\"invalid\"}"),
        sReason,
        sizeof(sReason));

    CHECK(eStatus == DIRECTGATE_ENROLL_REFRESH_TERMINAL,
        "invalid refresh token should be terminal");
    CHECK(strcmp(sReason, "invalid-refresh-token") == 0,
        "invalid refresh token should map to invalid-refresh-token");

#ifndef DIRECTGATE_DEBUG
    directgate_cfg_t insecureCfg;
    DirectGate_InitConfig(&insecureCfg);
    xstrncpy(insecureCfg.sDeviceId, sizeof(insecureCfg.sDeviceId), "device-insecure");
    xstrncpy(insecureCfg.enroll.sApiUrl, sizeof(insecureCfg.enroll.sApiUrl),
        "http://api.example.test");

    CHECK(!DirectGate_Enroll_Pair(&insecureCfg, "pairing-token"),
        "pairing must reject an unencrypted API endpoint");

    insecureCfg.enroll.bEnrolled = XTRUE;
    xstrncpy(insecureCfg.enroll.sRefreshToken, sizeof(insecureCfg.enroll.sRefreshToken),
        "refresh-token");
    CHECK(DirectGate_Enroll_Refresh(&insecureCfg, sReason, sizeof(sReason)) ==
          DIRECTGATE_ENROLL_REFRESH_TRANSIENT,
        "refresh must reject an unencrypted API endpoint");

    xstrncpy(insecureCfg.keyauth.sIdentityPubB64,
        sizeof(insecureCfg.keyauth.sIdentityPubB64), "agent-public-key");
    CHECK(!DirectGate_Enroll_RotateAgentKey(&insecureCfg),
        "agent key rotation must reject an unencrypted API endpoint");
#endif

    puts("enroll_smoke: OK");
    return 0;
}
