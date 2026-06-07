/*!
 * @file directgate-agent/src/common/siv.c
 * @brief AES-SIV backend abstraction (OpenSSL when available, libxutils otherwise).
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

#include "siv.h"
#include <limits.h>

/* SIV synthetic tag is one AES block, prepended to the ciphertext. */
#define XSIV_TAG_SIZE   XAES_BLOCK_SIZE
/* Largest per-half key we accept (AES-256 -> 32-byte MAC + 32-byte CTR). */
#define XSIV_MAX_HALF   XAES_KEY_LENGTH

/* The EVP AES-SIV cipher, EVP_*Init_ex2 and EVP_CIPHER_fetch all arrived in
   OpenSSL 3.0. LibreSSL may advertise a 3.x version number without shipping
   any of them, so exclude it explicitly; the libxutils backend covers it.

   On modern platforms, DirectGate uses OpenSSL's native AES-SIV implementation
   by default. The libxutils AES-SIV backend exists primarily as a compatibility
   fallback for legacy systems where the required OpenSSL functionality is not
   available.

   To reduce the risk of implementation drift, the libxutils AES-SIV code is
   continuously validated in CI against OpenSSL test vectors and behavior.
   Nevertheless, preference is always given to OpenSSL's widely deployed,
   extensively reviewed, and independently audited implementation whenever it
   is available on the target system.
 */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
#define XSIV_HAVE_OPENSSL 1
#endif

/* ------------------------------------------------------------------------- */
/* libxutils (portable) backend                                              */
/* ------------------------------------------------------------------------- */

static uint8_t* DirectGate_SIV_XUtilsEncrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                             const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    xaes_key_t key;
    XAES_InitSIVKey(&key, pCmacKey, pCtrKey, nKeyBits);

    xaes_t ctx;
    XCHECK((XAES_Init(&ctx, &key, XAES_MODE_SIV) >= 0), NULL);

    size_t nEncLen = nLength;
    uint8_t *pEncrypted = XAES_Encrypt(&ctx, pData, &nEncLen);
    XCHECK_NL((pEncrypted != NULL), NULL);

    *pOutLen = nEncLen;
    return pEncrypted;
}

static uint8_t* DirectGate_SIV_XUtilsDecrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                             const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    xaes_key_t key;
    XAES_InitSIVKey(&key, pCmacKey, pCtrKey, nKeyBits);

    xaes_t ctx;
    XCHECK((XAES_Init(&ctx, &key, XAES_MODE_SIV) >= 0), NULL);

    size_t nDecLen = nLength;
    uint8_t *pDecrypted = XAES_Decrypt(&ctx, pData, &nDecLen);
    XCHECK_NL((pDecrypted != NULL), NULL);

    *pOutLen = nDecLen;
    return pDecrypted;
}

/* ------------------------------------------------------------------------- */
/* OpenSSL EVP backend                                                       */
/* ------------------------------------------------------------------------- */

#ifdef XSIV_HAVE_OPENSSL
static const char* DirectGate_SIV_CipherName(size_t nKeyBits)
{
    switch (nKeyBits)
    {
        case 128: return "AES-128-SIV";
        case 192: return "AES-192-SIV";
        case 256: return "AES-256-SIV";
        default:  return NULL;
    }
}

/* OpenSSL takes the SIV key as a single MAC||CTR buffer. */
static xbool_t DirectGate_SIV_JoinKey(uint8_t *pFullKey, size_t nFullSize, const uint8_t *pCmacKey,
                                      const uint8_t *pCtrKey, size_t nKeyBits, size_t *pHalfLen)
{
    size_t nHalf = nKeyBits / 8;
    XCHECK((nHalf > 0 && nHalf <= XSIV_MAX_HALF && nHalf * 2 <= nFullSize), XFALSE);
    memcpy(pFullKey, pCmacKey, nHalf);
    memcpy(pFullKey + nHalf, pCtrKey, nHalf);
    *pHalfLen = nHalf;
    return XTRUE;
}

/* One-time cached probe. OpenSSL >= 3.0 normally ships AES-SIV in the default
   provider, but a stripped provider configuration can omit it; fall back to
   libxutils in that case instead of failing. The benign race on first call
   only re-runs an idempotent, thread-safe fetch. */
static xbool_t DirectGate_SIV_OpenSSLReady(void)
{
    static int nReady = -1;
    if (nReady < 0)
    {
        EVP_CIPHER *pProbe = EVP_CIPHER_fetch(NULL, "AES-256-SIV", NULL);
        nReady = (pProbe != NULL);
        EVP_CIPHER_free(pProbe);
    }
    return nReady ? XTRUE : XFALSE;
}

static uint8_t* DirectGate_SIV_OpenSSLEncrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                              const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    const char *pName = DirectGate_SIV_CipherName(nKeyBits);
    XCHECK((pName != NULL && nLength <= INT_MAX), NULL);

    uint8_t fullKey[XSIV_MAX_HALF * 2];
    size_t nHalf = 0;
    XCHECK(DirectGate_SIV_JoinKey(fullKey, sizeof(fullKey), pCmacKey, pCtrKey, nKeyBits, &nHalf), NULL);

    EVP_CIPHER *pCipher = EVP_CIPHER_fetch(NULL, pName, NULL);
    EVP_CIPHER_CTX *pCtx = EVP_CIPHER_CTX_new();
    uint8_t *pOut = malloc(XSIV_TAG_SIZE + nLength);
    int nWritten = 0, nFinal = 0;

    if (pCipher == NULL || pCtx == NULL || pOut == NULL ||
        EVP_EncryptInit_ex2(pCtx, pCipher, fullKey, NULL, NULL) != 1 ||
        EVP_EncryptUpdate(pCtx, pOut + XSIV_TAG_SIZE, &nWritten, pData, (int)nLength) != 1 ||
        EVP_EncryptFinal_ex(pCtx, pOut + XSIV_TAG_SIZE + nWritten, &nFinal) != 1 ||
        (size_t)(nWritten + nFinal) != nLength ||
        EVP_CIPHER_CTX_ctrl(pCtx, EVP_CTRL_AEAD_GET_TAG, XSIV_TAG_SIZE, pOut) != 1)
    {
        free(pOut);
        pOut = NULL;
    }
    else *pOutLen = XSIV_TAG_SIZE + nLength;

    EVP_CIPHER_CTX_free(pCtx);
    EVP_CIPHER_free(pCipher);
    OPENSSL_cleanse(fullKey, sizeof(fullKey));
    return pOut;
}

static uint8_t* DirectGate_SIV_OpenSSLDecrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                              const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    const char *pName = DirectGate_SIV_CipherName(nKeyBits);
    XCHECK((pName != NULL && nLength > XSIV_TAG_SIZE && nLength - XSIV_TAG_SIZE <= INT_MAX), NULL);

    uint8_t fullKey[XSIV_MAX_HALF * 2];
    size_t nHalf = 0;
    XCHECK(DirectGate_SIV_JoinKey(fullKey, sizeof(fullKey), pCmacKey, pCtrKey, nKeyBits, &nHalf), NULL);

    size_t nCipherLen = nLength - XSIV_TAG_SIZE;
    EVP_CIPHER *pCipher = EVP_CIPHER_fetch(NULL, pName, NULL);
    EVP_CIPHER_CTX *pCtx = EVP_CIPHER_CTX_new();
    uint8_t *pOut = malloc(nCipherLen);
    int nWritten = 0, nFinal = 0;

    if (pCipher == NULL || pCtx == NULL || pOut == NULL ||
        EVP_DecryptInit_ex2(pCtx, pCipher, fullKey, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(pCtx, EVP_CTRL_AEAD_SET_TAG, XSIV_TAG_SIZE, (void*)pData) != 1 ||
        EVP_DecryptUpdate(pCtx, pOut, &nWritten, pData + XSIV_TAG_SIZE, (int)nCipherLen) != 1 ||
        EVP_DecryptFinal_ex(pCtx, pOut + nWritten, &nFinal) != 1 ||
        (size_t)(nWritten + nFinal) != nCipherLen)
    {
        free(pOut);
        pOut = NULL;
    }
    else *pOutLen = nCipherLen;

    EVP_CIPHER_CTX_free(pCtx);
    EVP_CIPHER_free(pCipher);
    OPENSSL_cleanse(fullKey, sizeof(fullKey));
    return pOut;
}
#endif /* XSIV_HAVE_OPENSSL */

/* ------------------------------------------------------------------------- */
/* Public dispatch                                                           */
/* ------------------------------------------------------------------------- */

uint8_t* DirectGate_SIV_Encrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    XCHECK((pCmacKey != NULL && pCtrKey != NULL), NULL);
    XCHECK((pData != NULL && nLength > 0), NULL);
    XCHECK((pOutLen != NULL), NULL);

#ifdef XSIV_HAVE_OPENSSL
    if (DirectGate_SIV_OpenSSLReady())
        return DirectGate_SIV_OpenSSLEncrypt(pCmacKey, pCtrKey, nKeyBits, pData, nLength, pOutLen);
#endif
    return DirectGate_SIV_XUtilsEncrypt(pCmacKey, pCtrKey, nKeyBits, pData, nLength, pOutLen);
}

uint8_t* DirectGate_SIV_Decrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                const uint8_t *pData, size_t nLength, size_t *pOutLen)
{
    XCHECK((pCmacKey != NULL && pCtrKey != NULL), NULL);
    XCHECK((pData != NULL && nLength > XSIV_TAG_SIZE), NULL);
    XCHECK((pOutLen != NULL), NULL);

#ifdef XSIV_HAVE_OPENSSL
    if (DirectGate_SIV_OpenSSLReady())
        return DirectGate_SIV_OpenSSLDecrypt(pCmacKey, pCtrKey, nKeyBits, pData, nLength, pOutLen);
#endif
    return DirectGate_SIV_XUtilsDecrypt(pCmacKey, pCtrKey, nKeyBits, pData, nLength, pOutLen);
}
