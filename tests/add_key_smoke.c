#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "src/common/keyauth.h"
#include "src/agent/config.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "add_key_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

static int build_test_pubkey_b64(char *pOut, size_t nOutSize, uint8_t nSeed)
{
    uint8_t raw[DIRECTGATE_KEYAUTH_ED25519_PUB_SIZE];
    for (size_t i = 0; i < sizeof(raw); i++) raw[i] = (uint8_t)(nSeed + i);
    return DirectGate_KeyAuth_Base64Encode(raw, sizeof(raw), pOut, nOutSize) ? 0 : 1;
}

int main(void)
{
    const char *srpSalt = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    const char *srpVerifier = "1234abcd";
    char sTmpPath[] = "/tmp/directgate_add_key_smoke.XXXXXX";
    int nFd = mkstemp(sTmpPath);
    CHECK(nFd >= 0, "mkstemp");
    close(nFd);
    unlink(sTmpPath);

    directgate_cfg_t cfg;
    DirectGate_InitConfig(&cfg);
    xstrncpy(cfg.sCfgPath, sizeof(cfg.sCfgPath), sTmpPath);
    xstrncpy(cfg.sDeviceId, sizeof(cfg.sDeviceId), "dev-smoke");

    /* --- Invalid input rejected --- */
    CHECK(DirectGate_AddAuthorizedKey(&cfg, NULL) == DIRECTGATE_ADD_KEY_INVALID,
        "NULL pubkey must be rejected");
    CHECK(DirectGate_AddAuthorizedKey(&cfg, "") == DIRECTGATE_ADD_KEY_INVALID,
        "empty pubkey must be rejected");
    CHECK(DirectGate_AddAuthorizedKey(&cfg, "not-a-real-key") == DIRECTGATE_ADD_KEY_INVALID,
        "non-base64 pubkey must be rejected");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == 0,
        "invalid keys must not be stored");

    /* --- Append first valid key, preserving the full 44-char base64 value --- */
    const char *key1 = "G8tzS4EVKlvTqIPkKxTV/jJMHCvQkQCnYJO1B4EFRzE=";
    CHECK(strlen(key1) == 44, "reported key fixture must be 44 chars");
    CHECK(DirectGate_AddAuthorizedKey(&cfg, key1) == DIRECTGATE_ADD_KEY_ADDED,
        "first key must be ADDED");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == 1,
        "count after first add");
    CHECK(xstrcmp(cfg.keyauth.sAuthorizedKeys[0], key1),
        "first key stored verbatim");

    /* --- Duplicate returns ALREADY without mutating state --- */
    CHECK(DirectGate_AddAuthorizedKey(&cfg, key1) == DIRECTGATE_ADD_KEY_ALREADY,
        "duplicate must return ALREADY");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == 1,
        "duplicate must not grow list");

    /* --- Distinct key appended --- */
    char key2[DIRECTGATE_KEYAUTH_PUB_B64_SIZE];
    CHECK(build_test_pubkey_b64(key2, sizeof(key2), 0x42) == 0, "build key2");
    CHECK(DirectGate_AddAuthorizedKey(&cfg, key2) == DIRECTGATE_ADD_KEY_ADDED,
        "second key must be ADDED");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == 2,
        "count after second add");

    /* --- Fill to capacity --- */
    for (uint8_t i = 2; i < DIRECTGATE_MAX_AUTHORIZED_KEYS; i++)
    {
        char sKey[DIRECTGATE_KEYAUTH_PUB_B64_SIZE];
        CHECK(build_test_pubkey_b64(sKey, sizeof(sKey), (uint8_t)(0x80 + i)) == 0,
            "build filler key");
        CHECK(DirectGate_AddAuthorizedKey(&cfg, sKey) == DIRECTGATE_ADD_KEY_ADDED,
            "filler key must be ADDED");
    }
    CHECK(cfg.keyauth.nAuthorizedKeyCount == DIRECTGATE_MAX_AUTHORIZED_KEYS,
        "filled to capacity");

    /* --- Capacity exhausted --- */
    char overflow[DIRECTGATE_KEYAUTH_PUB_B64_SIZE];
    CHECK(build_test_pubkey_b64(overflow, sizeof(overflow), 0xF0) == 0,
        "build overflow key");
    CHECK(DirectGate_AddAuthorizedKey(&cfg, overflow) == DIRECTGATE_ADD_KEY_FULL,
        "overflow must return FULL");
    CHECK(cfg.keyauth.nAuthorizedKeyCount == DIRECTGATE_MAX_AUTHORIZED_KEYS,
        "overflow must not extend list");

    char agentSeed[DIRECTGATE_KEYAUTH_PUB_B64_SIZE];
    char agentPub[DIRECTGATE_KEYAUTH_PUB_B64_SIZE];
    CHECK(build_test_pubkey_b64(agentSeed, sizeof(agentSeed), 0x21) == 0,
        "build agent identity seed fixture");
    CHECK(build_test_pubkey_b64(agentPub, sizeof(agentPub), 0x31) == 0,
        "build agent identity pub fixture");
    xstrncpy(cfg.auth.sSaltHex, sizeof(cfg.auth.sSaltHex), srpSalt);
    xstrncpy(cfg.auth.sVerifierHex, sizeof(cfg.auth.sVerifierHex), srpVerifier);
    xstrncpy(cfg.keyauth.sIdentitySeedB64, sizeof(cfg.keyauth.sIdentitySeedB64), agentSeed);
    xstrncpy(cfg.keyauth.sIdentityPubB64, sizeof(cfg.keyauth.sIdentityPubB64), agentPub);

    /* --- Persist + reload preserves authorizedKeys --- */
    CHECK(DirectGate_SaveConfig(&cfg), "SaveConfig must succeed");
    struct stat st;
    CHECK(stat(sTmpPath, &st) == 0, "saved config stat");
    CHECK((st.st_mode & 0777) == 0600,
        "saved config should be private");

    xbyte_buffer_t saved;
    CHECK(XPath_LoadBuffer(sTmpPath, &saved) > 0, "saved config must be readable");

    xjson_t json;
    CHECK(XJSON_Parse(&json, NULL, (const char*)saved.pData, saved.nUsed),
        "saved config must be valid JSON");
    xjson_obj_t *pRoot = json.pRootObj;
    CHECK(XJSON_GetObject(pRoot, "agentIdentity") == NULL,
        "saved config must not write legacy root agentIdentity");
    CHECK(XJSON_GetObject(pRoot, "authorizedKeys") == NULL,
        "saved config must not write legacy root authorizedKeys");

    xjson_obj_t *pAuth = XJSON_GetObject(pRoot, "auth");
    CHECK(pAuth != NULL && pAuth->nType == XJSON_TYPE_OBJECT,
        "saved config must contain auth object");
    CHECK(XJSON_GetObject(pAuth, "salt") == NULL,
        "saved config must not write legacy auth.salt");
    CHECK(XJSON_GetObject(pAuth, "verifier") == NULL,
        "saved config must not write legacy auth.verifier");

    xjson_obj_t *pSrp = XJSON_GetObject(pAuth, "srp");
    CHECK(pSrp != NULL && pSrp->nType == XJSON_TYPE_OBJECT,
        "saved config must contain auth.srp");
    CHECK(xstrcmp(XJSON_GetString(XJSON_GetObject(pSrp, "salt")), srpSalt),
        "saved config must write auth.srp.salt");
    CHECK(xstrcmp(XJSON_GetString(XJSON_GetObject(pSrp, "verifier")), srpVerifier),
        "saved config must write auth.srp.verifier");

    xjson_obj_t *pKey = XJSON_GetObject(pAuth, "key");
    CHECK(pKey != NULL && pKey->nType == XJSON_TYPE_OBJECT,
        "saved config must contain auth.key");
    xjson_obj_t *pIdentity = XJSON_GetObject(pKey, "agentIdentity");
    CHECK(pIdentity != NULL && pIdentity->nType == XJSON_TYPE_OBJECT,
        "saved config must contain auth.key.agentIdentity");
    CHECK(xstrcmp(XJSON_GetString(XJSON_GetObject(pIdentity, "seed")), agentSeed),
        "saved config must write auth.key.agentIdentity.seed");
    CHECK(xstrcmp(XJSON_GetString(XJSON_GetObject(pIdentity, "pub")), agentPub),
        "saved config must write auth.key.agentIdentity.pub");

    xjson_obj_t *pKeys = XJSON_GetObject(pKey, "authorizedKeys");
    CHECK(pKeys != NULL && pKeys->nType == XJSON_TYPE_ARRAY,
        "saved config must contain auth.key.authorizedKeys");
    CHECK(XJSON_GetArrayLength(pKeys) == DIRECTGATE_MAX_AUTHORIZED_KEYS,
        "saved config must preserve authorized key count");
    CHECK(xstrcmp(XJSON_GetString(XJSON_GetArrayItem(pKeys, 0)), key1),
        "saved config must preserve full first authorized key");
    XJSON_Destroy(&json);
    XByteBuffer_Clear(&saved);

    directgate_cfg_t reloaded;
    DirectGate_InitConfig(&reloaded);
    CHECK(DirectGate_LoadConfig(&reloaded, sTmpPath),
        "LoadConfig must succeed");
    CHECK(reloaded.keyauth.nAuthorizedKeyCount == DIRECTGATE_MAX_AUTHORIZED_KEYS,
        "reloaded authorized key count must match");
    CHECK(xstrcmp(reloaded.keyauth.sAuthorizedKeys[0], key1),
        "reloaded key 0 must match key1");
    CHECK(xstrcmp(reloaded.keyauth.sAuthorizedKeys[1], key2),
        "reloaded key 1 must match key2");
    CHECK(strcmp(reloaded.auth.sSaltHex, srpSalt) == 0,
        "reloaded SRP salt must match");
    CHECK(strcmp(reloaded.auth.sVerifierHex, srpVerifier) == 0,
        "reloaded SRP verifier must match");
    CHECK(strcmp(reloaded.keyauth.sIdentitySeedB64, agentSeed) == 0,
        "reloaded agent identity seed must match");
    CHECK(strcmp(reloaded.keyauth.sIdentityPubB64, agentPub) == 0,
        "reloaded agent identity pub must match");

    /* --- Duplicate detection survives reload --- */
    CHECK(DirectGate_AddAuthorizedKey(&reloaded, key1) == DIRECTGATE_ADD_KEY_ALREADY,
        "reloaded duplicate must return ALREADY");

    /* --- Legacy auth layout still loads for migration compatibility --- */
    char legacyJson[1024];
    int nLegacyLen = snprintf(legacyJson, sizeof(legacyJson),
        "{"
            "\"auth\":{\"salt\":\"%s\",\"verifier\":\"%s\"},"
            "\"agentIdentity\":{\"seed\":\"%s\",\"pub\":\"%s\"},"
            "\"authorizedKeys\":[\"%s\"]"
        "}",
        srpSalt, srpVerifier, agentSeed, agentPub, key1);
    CHECK(nLegacyLen > 0 && (size_t)nLegacyLen < sizeof(legacyJson),
        "legacy JSON fixture must fit");
    CHECK(XPath_Write(sTmpPath, (uint8_t*)legacyJson, (size_t)nLegacyLen, "cwt") > 0,
        "legacy config fixture must be writable");

    directgate_cfg_t legacy;
    DirectGate_InitConfig(&legacy);
    CHECK(DirectGate_LoadConfig(&legacy, sTmpPath),
        "legacy config must load");
    CHECK(strcmp(legacy.auth.sSaltHex, srpSalt) == 0,
        "legacy auth.salt must load into SRP auth");
    CHECK(strcmp(legacy.auth.sVerifierHex, srpVerifier) == 0,
        "legacy auth.verifier must load into SRP auth");
    CHECK(strcmp(legacy.keyauth.sIdentityPubB64, agentPub) == 0,
        "legacy agentIdentity must load into key auth");
    CHECK(legacy.keyauth.nAuthorizedKeyCount == 1 &&
        strcmp(legacy.keyauth.sAuthorizedKeys[0], key1) == 0,
        "legacy authorizedKeys must load into key auth");

    unlink(sTmpPath);
    printf("agent_add_key_smoke: OK\n");
    return 0;
}
