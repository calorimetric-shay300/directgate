#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>

#include "xstd.h"
#include "crypt/aes.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "crypto_siv_openssl_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

#define SIV_TAG_SIZE 16

typedef struct siv_case_ {
    const char *pCipherName;
    size_t nHalfKeyBits;
} siv_case_t;

static void fill_range(uint8_t *pData, size_t nLen, uint8_t nStart)
{
    for (size_t i = 0; i < nLen; i++)
        pData[i] = (uint8_t)(nStart + i);
}

static uint8_t* openssl_siv_encrypt(const char *pCipherName,
                                    const uint8_t *pKey,
                                    const uint8_t *pPlain,
                                    size_t nPlainLen,
                                    size_t *pOutLen)
{
    if (pCipherName == NULL || pKey == NULL || pPlain == NULL ||
        pOutLen == NULL || nPlainLen > INT_MAX)
        return NULL;

    EVP_CIPHER *pCipher = EVP_CIPHER_fetch(NULL, pCipherName, NULL);
    EVP_CIPHER_CTX *pCtx = EVP_CIPHER_CTX_new();
    uint8_t *pOutput = malloc(SIV_TAG_SIZE + nPlainLen);
    int nWritten = 0;
    int nFinal = 0;

    if (pCipher == NULL || pCtx == NULL || pOutput == NULL ||
        EVP_EncryptInit_ex2(pCtx, pCipher, pKey, NULL, NULL) != 1 ||
        EVP_EncryptUpdate(pCtx, pOutput + SIV_TAG_SIZE, &nWritten,
            pPlain, (int)nPlainLen) != 1 ||
        EVP_EncryptFinal_ex(pCtx, pOutput + SIV_TAG_SIZE + nWritten, &nFinal) != 1 ||
        (size_t)(nWritten + nFinal) != nPlainLen ||
        EVP_CIPHER_CTX_ctrl(pCtx, EVP_CTRL_AEAD_GET_TAG, SIV_TAG_SIZE, pOutput) != 1)
    {
        free(pOutput);
        pOutput = NULL;
    }
    else
    {
        *pOutLen = SIV_TAG_SIZE + nPlainLen;
    }

    EVP_CIPHER_CTX_free(pCtx);
    EVP_CIPHER_free(pCipher);
    return pOutput;
}

static uint8_t* openssl_siv_decrypt(const char *pCipherName,
                                    const uint8_t *pKey,
                                    const uint8_t *pInput,
                                    size_t nInputLen,
                                    size_t *pOutLen)
{
    if (pCipherName == NULL || pKey == NULL || pInput == NULL ||
        pOutLen == NULL || nInputLen <= SIV_TAG_SIZE ||
        nInputLen - SIV_TAG_SIZE > INT_MAX)
        return NULL;

    size_t nCipherLen = nInputLen - SIV_TAG_SIZE;
    EVP_CIPHER *pCipher = EVP_CIPHER_fetch(NULL, pCipherName, NULL);
    EVP_CIPHER_CTX *pCtx = EVP_CIPHER_CTX_new();
    uint8_t *pOutput = malloc(nCipherLen);
    int nWritten = 0;
    int nFinal = 0;

    if (pCipher == NULL || pCtx == NULL || pOutput == NULL ||
        EVP_DecryptInit_ex2(pCtx, pCipher, pKey, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(pCtx, EVP_CTRL_AEAD_SET_TAG, SIV_TAG_SIZE, (void*)pInput) != 1 ||
        EVP_DecryptUpdate(pCtx, pOutput, &nWritten,
            pInput + SIV_TAG_SIZE, (int)nCipherLen) != 1 ||
        EVP_DecryptFinal_ex(pCtx, pOutput + nWritten, &nFinal) != 1 ||
        (size_t)(nWritten + nFinal) != nCipherLen)
    {
        free(pOutput);
        pOutput = NULL;
    }
    else
    {
        *pOutLen = nCipherLen;
    }

    EVP_CIPHER_CTX_free(pCtx);
    EVP_CIPHER_free(pCipher);
    return pOutput;
}

static int run_case(const siv_case_t *pCase, size_t nPlainLen)
{
    size_t nHalfKeyLen = pCase->nHalfKeyBits / 8;
    size_t nFullKeyLen = nHalfKeyLen * 2;
    uint8_t fullKey[64];
    uint8_t plaintext[65];

    fill_range(fullKey, nFullKeyLen, (uint8_t)(0x10 + nPlainLen));
    fill_range(plaintext, nPlainLen, (uint8_t)(0x80 + nHalfKeyLen));

    xaes_key_t key;
    xaes_t ctx;
    XAES_InitSIVKey(&key, fullKey, fullKey + nHalfKeyLen, pCase->nHalfKeyBits);
    CHECK(XAES_Init(&ctx, &key, XAES_MODE_SIV) == XSTDOK,
        "initialize bundled AES-SIV");

    size_t nBundledLen = nPlainLen;
    uint8_t *pBundled = XAES_Encrypt(&ctx, plaintext, &nBundledLen);
    CHECK(pBundled != NULL, "bundled AES-SIV encrypt");

    size_t nOpenSSLLen = 0;
    uint8_t *pOpenSSL = openssl_siv_encrypt(
        pCase->pCipherName, fullKey, plaintext, nPlainLen, &nOpenSSLLen);
    CHECK(pOpenSSL != NULL, "OpenSSL AES-SIV encrypt");
    CHECK(nBundledLen == nOpenSSLLen &&
          memcmp(pBundled, pOpenSSL, nBundledLen) == 0,
        "bundled AES-SIV differs from OpenSSL");

    size_t nBundledPlainLen = nOpenSSLLen;
    uint8_t *pBundledPlain = XAES_Decrypt(&ctx, pOpenSSL, &nBundledPlainLen);
    CHECK(pBundledPlain != NULL &&
          nBundledPlainLen == nPlainLen &&
          memcmp(pBundledPlain, plaintext, nPlainLen) == 0,
        "bundled AES-SIV cannot decrypt OpenSSL output");

    size_t nOpenSSLPlainLen = 0;
    uint8_t *pOpenSSLPlain = openssl_siv_decrypt(
        pCase->pCipherName, fullKey, pBundled, nBundledLen, &nOpenSSLPlainLen);
    CHECK(pOpenSSLPlain != NULL &&
          nOpenSSLPlainLen == nPlainLen &&
          memcmp(pOpenSSLPlain, plaintext, nPlainLen) == 0,
        "OpenSSL cannot decrypt bundled AES-SIV output");

    pBundled[0] ^= 0x01;
    size_t nTamperedLen = nBundledLen;
    CHECK(XAES_Decrypt(&ctx, pBundled, &nTamperedLen) == NULL,
        "bundled AES-SIV accepted a tampered tag");

    free(pOpenSSLPlain);
    free(pBundledPlain);
    free(pOpenSSL);
    free(pBundled);
    return 0;
}

int main(void)
{
    /* DirectGate uses RFC 5297 deterministic SIV without associated data.
       These lengths exercise both S2V final-block branches and CTR spanning. */
    const siv_case_t cases[] = {
        { "AES-128-SIV", 128 },
        { "AES-192-SIV", 192 },
        { "AES-256-SIV", 256 }
    };
    const size_t lengths[] = { 1, 15, 16, 17, 31, 32, 65 };

    EVP_CIPHER *pProbe = EVP_CIPHER_fetch(NULL, "AES-256-SIV", NULL);
    if (pProbe == NULL)
    {
        puts("crypto_siv_openssl_smoke: SKIP (AES-SIV provider unavailable)");
        return 0;
    }
    EVP_CIPHER_free(pProbe);

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        for (size_t j = 0; j < sizeof(lengths) / sizeof(lengths[0]); j++)
            CHECK(run_case(&cases[i], lengths[j]) == 0, "AES-SIV differential case");
    }

    puts("crypto_siv_openssl_smoke: OK");
    return 0;
}
