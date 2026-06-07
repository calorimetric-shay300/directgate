#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "src/common/auth.h"
#include "src/common/srp.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "srp_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

static void bytes_to_hex(const uint8_t *pData, size_t nLen,
                         char *pOut, size_t nOutSize)
{
    static const char *pHex = "0123456789abcdef";
    if (nOutSize < nLen * 2 + 1)
    {
        if (nOutSize) pOut[0] = '\0';
        return;
    }

    for (size_t i = 0; i < nLen; i++)
    {
        pOut[i * 2] = pHex[(pData[i] >> 4) & 0x0f];
        pOut[i * 2 + 1] = pHex[pData[i] & 0x0f];
    }

    pOut[nLen * 2] = '\0';
}

int main(void)
{
    uint8_t salt[DIRECTGATE_SRP_SALT_SIZE];
    for (size_t i = 0; i < sizeof(salt); i++) salt[i] = (uint8_t)i;

    char saltHex[DIRECTGATE_SRP_SALT_SIZE * 2 + 1];
    bytes_to_hex(salt, sizeof(salt), saltHex, sizeof(saltHex));

    uint8_t parsedSalt[DIRECTGATE_SRP_SALT_SIZE];
    CHECK(DirectGate_AuthSaltHexToBytes(saltHex, parsedSalt, sizeof(parsedSalt)),
        "auth salt hex parses");
    CHECK(memcmp(parsedSalt, salt, sizeof(salt)) == 0,
        "auth salt bytes match");
    CHECK(!DirectGate_AuthSaltHexToBytes("00", parsedSalt, sizeof(parsedSalt)),
        "short auth salt rejected");
    CHECK(!DirectGate_AuthSaltHexToBytes(
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1x",
        parsedSalt, sizeof(parsedSalt)), "invalid auth salt hex rejected");

    char verifierHex[1024];
    const char *pPassword = "correct horse battery staple";
    const char *pDeviceId = "dev-srp";
    CHECK(DirectGate_SRP_CreateVerifier(pPassword, salt, sizeof(salt),
        verifierHex, sizeof(verifierHex)), "create scrypt verifier");
    CHECK(strlen(verifierHex) > 0, "verifier is non-empty");

    directgate_srp_client_t client;
    directgate_srp_t server;
    CHECK(DirectGate_SRP_ClientInit(&client), "client init");
    CHECK(DirectGate_SRP_Init(&server), "server init");
    xstrncpy(server.sDeviceId, sizeof(server.sDeviceId), pDeviceId);

    CHECK(!DirectGate_SRP_LoadVerifier(&server, salt, sizeof(salt), "0"),
        "zero verifier rejected");
    CHECK(DirectGate_SRP_LoadVerifier(&server, salt, sizeof(salt), verifierHex),
        "server load verifier");
    CHECK(!DirectGate_SRP_SetClientPublic(&server, "0"),
        "zero client public rejected");

    char aHex[1024];
    char clientNonceHex[DIRECTGATE_SRP_NONCE_SIZE * 2 + 1];
    CHECK(DirectGate_SRP_ClientGenerateA(&client, aHex, sizeof(aHex),
        clientNonceHex, sizeof(clientNonceHex)), "client generate A");
    CHECK(strlen(aHex) > 0, "A is non-empty");
    CHECK(strlen(clientNonceHex) == DIRECTGATE_SRP_NONCE_SIZE * 2,
        "client nonce hex length");

    CHECK(DirectGate_SRP_SetClientPublic(&server, aHex),
        "server accepts client public");

    char bHex[1024];
    char agentNonceHex[DIRECTGATE_SRP_NONCE_SIZE * 2 + 1];
    CHECK(DirectGate_SRP_GenerateChallenge(&server, bHex, sizeof(bHex),
        agentNonceHex, sizeof(agentNonceHex)), "server challenge");
    CHECK(strlen(bHex) > 0, "B is non-empty");
    CHECK(strlen(agentNonceHex) == DIRECTGATE_SRP_NONCE_SIZE * 2,
        "agent nonce hex length");

    char m1Hex[DIRECTGATE_SRP_KEY_SIZE * 2 + 1];
    char m2Hex[DIRECTGATE_SRP_KEY_SIZE * 2 + 1];
    CHECK(DirectGate_SRP_ClientComputeKey(&client, pDeviceId, pPassword,
        saltHex, bHex, DIRECTGATE_SRP_SUITE, m1Hex, sizeof(m1Hex)),
        "client compute key");
    CHECK(strlen(m1Hex) == DIRECTGATE_SRP_KEY_SIZE * 2, "M1 hex length");
    CHECK(DirectGate_SRP_VerifyClientProof(&server, m1Hex, m2Hex, sizeof(m2Hex)),
        "server verifies M1");
    CHECK(server.bAuthenticated, "server authenticated flag");
    CHECK(memcmp(server.K, client.K, sizeof(server.K)) == 0,
        "server/client SRP session key match");
    CHECK(DirectGate_SRP_ClientVerifyM2(&client, bHex, m2Hex),
        "client verifies M2");

    directgate_srp_t tamperServer;
    CHECK(DirectGate_SRP_Init(&tamperServer), "tamper server init");
    xstrncpy(tamperServer.sDeviceId, sizeof(tamperServer.sDeviceId), pDeviceId);
    CHECK(DirectGate_SRP_LoadVerifier(&tamperServer, salt, sizeof(salt), verifierHex),
        "tamper server load verifier");
    CHECK(DirectGate_SRP_SetClientPublic(&tamperServer, aHex),
        "tamper server accepts A");

    char tamperBHex[1024];
    char tamperNonceHex[DIRECTGATE_SRP_NONCE_SIZE * 2 + 1];
    char tamperM1Hex[DIRECTGATE_SRP_KEY_SIZE * 2 + 1];
    char tamperM2Hex[DIRECTGATE_SRP_KEY_SIZE * 2 + 1];
    CHECK(DirectGate_SRP_GenerateChallenge(&tamperServer, tamperBHex,
        sizeof(tamperBHex), tamperNonceHex, sizeof(tamperNonceHex)),
        "tamper server challenge");
    CHECK(DirectGate_SRP_ClientComputeKey(&client, pDeviceId, pPassword,
        saltHex, tamperBHex, DIRECTGATE_SRP_SUITE, tamperM1Hex, sizeof(tamperM1Hex)),
        "tamper client compute key");
    tamperM1Hex[0] = tamperM1Hex[0] == '0' ? '1' : '0';
    CHECK(!DirectGate_SRP_VerifyClientProof(&tamperServer, tamperM1Hex,
        tamperM2Hex, sizeof(tamperM2Hex)), "tampered M1 rejected");
    CHECK(!tamperServer.bAuthenticated, "tamper server remains unauthenticated");

    size_t nOutLen = 0;
    CHECK(DirectGate_SRP_HexToBytes("0a0B", parsedSalt, sizeof(parsedSalt), &nOutLen),
        "generic hex parser accepts mixed case");
    CHECK(nOutLen == 2 && parsedSalt[0] == 0x0a && parsedSalt[1] == 0x0b,
        "generic hex parser bytes");
    CHECK(!DirectGate_SRP_HexToBytes("abc", parsedSalt, sizeof(parsedSalt), NULL),
        "generic hex parser rejects odd length");

    DirectGate_SRP_Destroy(&tamperServer);
    DirectGate_SRP_Destroy(&server);
    DirectGate_SRP_ClientCleanse(&client);

    puts("srp_smoke: OK");
    return 0;
}
