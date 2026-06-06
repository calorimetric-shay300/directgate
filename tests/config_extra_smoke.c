#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "src/agent/config.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "config_extra_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

static int write_file(const char *pPath, const char *pData)
{
    FILE *pFile = fopen(pPath, "wb");
    if (pFile == NULL) return 0;

    size_t nLen = strlen(pData);
    int nOk = fwrite(pData, 1, nLen, pFile) == nLen;
    fclose(pFile);
    return nOk;
}

int main(void)
{
    char sRoot[] = "/tmp/directgate_config_extra.XXXXXX";
    CHECK(mkdtemp(sRoot) != NULL, "mkdtemp root");

    char sPath[512];
    char sInvalidPath[512];
    char sSavedPath[512];
    snprintf(sPath, sizeof(sPath), "%s/agent-rich.json", sRoot);
    snprintf(sInvalidPath, sizeof(sInvalidPath), "%s/agent-invalid.json", sRoot);
    snprintf(sSavedPath, sizeof(sSavedPath), "%s/nested/saved-agent.json", sRoot);

    const char *pKey1 = "G8tzS4EVKlvTqIPkKxTV/jJMHCvQkQCnYJO1B4EFRzE=";
    const char *pKey2 = "QkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGE=";
    const char *pSeed = "ISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0A=";
    const char *pPub = "MTIzNDU2Nzg5Ojs8PT4/QEFCQ0RFRkdISUpLTE1OT1A=";
    const char *pSalt = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

    char sJson[4096];
    int nJson = snprintf(sJson, sizeof(sJson),
        "{"
            "\"signalingUrl\":\"wss://legacy.example.test/websock\","
            "\"routingKey\":\"rk-rich\","
            "\"deviceId\":\"dev-rich\","
            "\"kaInterval\":33,"
            "\"allowTCP\":true,"
            "\"iceServers\":["
                "\"turn:one.example.test\","
                "\"\","
                "\"turn:three.example.test\","
                "\"turn:four.example.test\","
                "\"turn:five.example.test\","
                "\"turn:six.example.test\","
                "\"turn:seven.example.test\","
                "\"turn:eight.example.test\","
                "\"turn:nine.example.test\""
            "],"
            "\"shell\":{\"user\":\"shell-user\",\"home\":\"/tmp\"},"
            "\"auth\":{"
                "\"srp\":{\"salt\":\"%s\",\"verifier\":\"verifier-rich\"},"
                "\"key\":{"
                    "\"agentIdentity\":{\"seed\":\"%s\",\"pub\":\"%s\"},"
                    "\"authorizedKeys\":[\"%s\",\"\",\"%s\"]"
                "}"
            "},"
            "\"enrollment\":{"
                "\"apiUrl\":\"https://api.rich.example.test\","
                "\"accessToken\":\"access-rich\","
                "\"refreshToken\":\"refresh-rich\","
                "\"accessTokenExp\":\"1234567890\","
                "\"refreshTokenExp\":\"2234567890\","
                "\"enrollmentExpiresAt\":\"2099-01-01T00:00:00.000Z\","
                "\"refreshSkewSec\":77,"
                "\"enrolled\":true"
            "}"
        "}",
        pSalt, pSeed, pPub, pKey1, pKey2);

    CHECK(nJson > 0 && (size_t)nJson < sizeof(sJson), "rich JSON fits");
    CHECK(write_file(sPath, sJson), "write rich config");
    CHECK(write_file(sInvalidPath, "{"), "write invalid config");

    directgate_cfg_t cfg;
    DirectGate_InitConfig(&cfg);
    xstrncpy(cfg.sCfgPath, sizeof(cfg.sCfgPath), sPath);
    CHECK(DirectGate_LoadConfig(&cfg, sPath), "load rich config");
    CHECK(strcmp(cfg.sRelayUrl, "wss://legacy.example.test/websock") == 0,
        "signalingUrl fallback should load as relayUrl");
    CHECK(strcmp(cfg.sRoutingKey, "rk-rich") == 0, "routing key should load");
    CHECK(strcmp(cfg.sDeviceId, "dev-rich") == 0, "device id should load");
    CHECK(cfg.nKAInterval == 33, "ka interval should load");
    CHECK(cfg.bAllowTCP == XTRUE, "allowTCP true should load");
    CHECK(cfg.nIceSrvCount == 7, "ICE loader should skip empty capped entries");
    CHECK(strcmp(cfg.sIceServers[1], "turn:three.example.test") == 0,
        "ICE loader should compact non-empty entries");
    CHECK(strcmp(cfg.sShellUser, "shell-user") == 0, "shell user should load");
    CHECK(strcmp(cfg.sShellHome, "/tmp") == 0, "shell home should load");
    CHECK(strcmp(cfg.auth.sSaltHex, pSalt) == 0, "SRP salt should load");
    CHECK(strcmp(cfg.auth.sVerifierHex, "verifier-rich") == 0,
        "SRP verifier should load");
    CHECK(strcmp(cfg.keyauth.sIdentitySeedB64, pSeed) == 0,
        "agent identity seed should load");
    CHECK(strcmp(cfg.keyauth.sIdentityPubB64, pPub) == 0,
        "agent identity pub should load");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == 2,
        "authorized key loader should skip empty strings");
    CHECK(strcmp(cfg.keyauth.sAuthorizedKeys[0], pKey1) == 0,
        "authorized key 1 should load");
    CHECK(strcmp(cfg.keyauth.sAuthorizedKeys[1], pKey2) == 0,
        "authorized key 2 should load");
    CHECK(strcmp(cfg.enroll.sApiUrl, "https://api.rich.example.test") == 0,
        "enrollment api should load");
    CHECK(strcmp(cfg.enroll.sAccessToken, "access-rich") == 0,
        "access token should load");
    CHECK(strcmp(cfg.enroll.sRefreshToken, "refresh-rich") == 0,
        "refresh token should load");
    CHECK(cfg.enroll.nAccessTokenExp == 1234567890ULL,
        "access exp should load");
    CHECK(cfg.enroll.nRefreshTokenExp == 2234567890ULL,
        "refresh exp should load");
    CHECK(cfg.enroll.nRefreshSkewSec == 77, "refresh skew should load");
    CHECK(cfg.enroll.bEnrolled == XTRUE, "enrolled flag should load");

    directgate_cfg_t invalid;
    DirectGate_InitConfig(&invalid);
    xstrncpy(invalid.sCfgPath, sizeof(invalid.sCfgPath), sInvalidPath);
    CHECK(!DirectGate_LoadConfig(&invalid, sInvalidPath),
        "invalid JSON config should fail");

    xstrncpy(cfg.sCfgPath, sizeof(cfg.sCfgPath), sSavedPath);
    xstrncpy(cfg.sRelayUrl, sizeof(cfg.sRelayUrl), "wss://saved.example.test/websock");
    CHECK(DirectGate_SaveConfig(&cfg), "save rich config");

    directgate_cfg_t reloaded;
    DirectGate_InitConfig(&reloaded);
    xstrncpy(reloaded.sCfgPath, sizeof(reloaded.sCfgPath), sSavedPath);
    CHECK(DirectGate_LoadConfig(&reloaded, sSavedPath), "reload saved config");
    CHECK(strcmp(reloaded.sRelayUrl, "wss://saved.example.test/websock") == 0,
        "saved relay should reload");
    CHECK(strcmp(reloaded.sRoutingKey, cfg.sRoutingKey) == 0,
        "saved routing key should reload");
    CHECK(reloaded.nIceSrvCount == cfg.nIceSrvCount,
        "saved ICE count should reload");
    CHECK(strcmp(reloaded.keyauth.sIdentityPubB64, pPub) == 0,
        "saved agent pub should reload");
    CHECK(reloaded.keyauth.nAuthorizedKeyCount == 2,
        "saved authorized key count should reload");
    CHECK(strcmp(reloaded.enroll.sRefreshToken, "refresh-rich") == 0,
        "saved refresh token should reload");

    unlink(sPath);
    unlink(sInvalidPath);
    unlink(sSavedPath);
    char sNested[512];
    snprintf(sNested, sizeof(sNested), "%s/nested", sRoot);
    rmdir(sNested);
    rmdir(sRoot);

    puts("config_extra_smoke: OK");
    return 0;
}
