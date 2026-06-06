/*!
 * @file directgate-agent/src/common/srp.h
 * @brief SRP-6a server-side implementation helpers.
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

#ifndef __DIRECTGATE_SRP_H__
#define __DIRECTGATE_SRP_H__

#include "includes.h"
#include <openssl/bn.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIRECTGATE_SRP_ALGO            "srp6a"
#define DIRECTGATE_SRP_SALT_SIZE       32
#define DIRECTGATE_SRP_NONCE_SIZE      32
#define DIRECTGATE_SRP_KEY_SIZE        32
#define DIRECTGATE_SRP_USERNAME_SIZE   64

typedef enum {
    DIRECTGATE_SRP_STATE_IDLE = 0,
    DIRECTGATE_SRP_STATE_CHALLENGE_SENT,
    DIRECTGATE_SRP_STATE_AUTHENTICATED,
    DIRECTGATE_SRP_STATE_FAILED
} directgate_srp_state_t;

typedef struct directgate_srp_ {
    directgate_srp_state_t eState;

    BIGNUM *N;
    BIGNUM *g;
    BIGNUM *k;
    BIGNUM *v;
    BIGNUM *b;
    BIGNUM *B;
    BIGNUM *A;

    uint8_t salt[DIRECTGATE_SRP_SALT_SIZE];
    uint8_t nonce[DIRECTGATE_SRP_NONCE_SIZE];
    uint8_t clientNonce[DIRECTGATE_SRP_NONCE_SIZE];
    uint8_t K[DIRECTGATE_SRP_KEY_SIZE];

    char sDeviceId[DIRECTGATE_SRP_USERNAME_SIZE];
    xbool_t bAuthenticated;
    uint8_t nAttempts;
} directgate_srp_t;

xbool_t DirectGate_SRP_HexToBytes(const char *pHex, uint8_t *pOut,
                                  size_t nOutSize, size_t *pOutLen);

xbool_t DirectGate_SRP_Init(directgate_srp_t *pSRP);
void DirectGate_SRP_Destroy(directgate_srp_t *pSRP);

const char *DirectGate_SRP_StateName(directgate_srp_state_t eState);
xbool_t DirectGate_SRP_SetClientPublic(directgate_srp_t *pSRP, const char *pAHex);

xbool_t DirectGate_SRP_LoadVerifier(directgate_srp_t *pSRP,
                                    const uint8_t *pSalt,
                                    const size_t nSaltLen,
                                    const char *pVerifierHex);

xbool_t DirectGate_SRP_GenerateChallenge(directgate_srp_t *pSRP,
                                         char *pBHex, size_t nBSize,
                                         char *pNonceHex, size_t nNonceSize);

xbool_t DirectGate_SRP_VerifyClientProof(directgate_srp_t *pSRP,
                                         const char *pM1Hex,
                                         char *pM2Hex, size_t nM2Size);

xbool_t DirectGate_SRP_CreateVerifier(const char *pPassword,
                                      const uint8_t *pSalt, size_t nSaltLen,
                                      char *pVerifierHex, size_t nSize);

/* SHA256-based verifier for web client compatibility.
 * x = SHA256(salt || password) instead of scrypt. */
xbool_t DirectGate_SRP_CreateVerifierCompat(const char *pPassword,
                                            const uint8_t *pSalt, size_t nSaltLen,
                                            char *pVerifierHex, size_t nSize);

/* Client-side SRP-6a functions */
typedef struct directgate_srp_client_ {
    BIGNUM *N;
    BIGNUM *g;
    BIGNUM *k;
    BIGNUM *a;
    BIGNUM *A;
    uint8_t K[DIRECTGATE_SRP_KEY_SIZE];
    uint8_t M1[DIRECTGATE_SRP_KEY_SIZE];
    uint8_t nonce[DIRECTGATE_SRP_NONCE_SIZE];
    uint8_t agentNonce[DIRECTGATE_SRP_NONCE_SIZE];
    char sDeviceId[DIRECTGATE_SRP_USERNAME_SIZE];
} directgate_srp_client_t;

xbool_t DirectGate_SRP_ClientInit(directgate_srp_client_t *pClient);
void DirectGate_SRP_ClientCleanse(directgate_srp_client_t *pClient);

xbool_t DirectGate_SRP_ClientGenerateA(directgate_srp_client_t *pClient,
                                       char *pAHex, size_t nASize,
                                       char *pNonceHex, size_t nNonceSize);

xbool_t DirectGate_SRP_ClientVerifyM2(const directgate_srp_client_t *pClient,
                                      const char *pBHex,
                                      const char *pM2Hex);

xbool_t DirectGate_SRP_ClientComputeKey(directgate_srp_client_t *pClient,
                                        const char *pDeviceId,
                                        const char *pPassword,
                                        const char *pSaltHex,
                                        const char *pBHex,
                                        char *pM1Hex,
                                        size_t nM1Size);

#ifdef __cplusplus
}
#endif

#endif /* __DIRECTGATE_SRP_H__ */
