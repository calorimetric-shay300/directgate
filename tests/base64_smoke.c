#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libxutils/src/crypt/base64.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "base64_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

static int check_roundtrip(const uint8_t *pData, size_t nDataLen, const char *pLabel)
{
    size_t nEncLen = nDataLen;
    char *pEncoded = XBase64_Encrypt(pData, &nEncLen);
    CHECK(pEncoded != NULL, "base64 encode failed");

    size_t nDecLen = nEncLen;
    char *pDecoded = XBase64_Decrypt((const uint8_t*)pEncoded, &nDecLen);
    CHECK(pDecoded != NULL, "base64 decode failed");
    if (nDecLen != nDataLen) {
        fprintf(stderr, "base64_smoke: %s length mismatch: got(%zu), want(%zu), enc(%s)\n",
            pLabel, nDecLen, nDataLen, pEncoded);
        return 1;
    }
    if (memcmp(pDecoded, pData, nDataLen) != 0) {
        fprintf(stderr, "base64_smoke: %s bytes mismatch, enc(%s)\n", pLabel, pEncoded);
        return 1;
    }

    free(pDecoded);
    free(pEncoded);
    return 0;
}

static int check_url_roundtrip(const uint8_t *pData, size_t nDataLen, const char *pLabel)
{
    size_t nEncLen = nDataLen;
    char *pEncoded = XBase64_UrlEncrypt(pData, &nEncLen);
    CHECK(pEncoded != NULL, "base64url encode failed");

    size_t nDecLen = nEncLen;
    char *pDecoded = XBase64_UrlDecrypt((const uint8_t*)pEncoded, &nDecLen);
    CHECK(pDecoded != NULL, "base64url decode failed");
    if (nDecLen != nDataLen) {
        fprintf(stderr, "base64_smoke: %s length mismatch: got(%zu), want(%zu), enc(%s)\n",
            pLabel, nDecLen, nDataLen, pEncoded);
        return 1;
    }
    if (memcmp(pDecoded, pData, nDataLen) != 0) {
        fprintf(stderr, "base64_smoke: %s bytes mismatch, enc(%s)\n", pLabel, pEncoded);
        return 1;
    }

    free(pDecoded);
    free(pEncoded);
    return 0;
}

int main(void)
{
    uint8_t key[32];
    uint8_t sig[64];

    for (size_t i = 0; i < sizeof(key); i++) key[i] = (uint8_t)(i + 1);
    key[sizeof(key) - 1] = 0;

    for (size_t i = 0; i < sizeof(sig); i++) sig[i] = (uint8_t)(0xa0 + i);
    sig[sizeof(sig) - 1] = 0;

    CHECK(check_roundtrip(key, sizeof(key), "32-byte trailing-zero roundtrip") == 0,
        "32-byte trailing-zero roundtrip");
    CHECK(check_roundtrip(sig, sizeof(sig), "64-byte trailing-zero roundtrip") == 0,
        "64-byte trailing-zero roundtrip");
    CHECK(check_url_roundtrip(sig, sizeof(sig), "url trailing-zero roundtrip") == 0,
        "url trailing-zero roundtrip");

    printf("base64_smoke: OK\n");
    return 0;
}
