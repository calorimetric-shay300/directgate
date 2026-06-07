/*!
 * @file directgate-agent/src/common/auth.h
 * @brief SRP authentication config helpers for directgate.
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

#ifndef __DIRECTGATE_AUTH_H__
#define __DIRECTGATE_AUTH_H__

#include "includes.h"
#include "srp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIRECTGATE_AUTH_SALT_HEX_SIZE        ((DIRECTGATE_SRP_SALT_SIZE * 2) + 1)
#define DIRECTGATE_AUTH_VERIFIER_HEX_SIZE    1024

typedef struct directgate_auth_ {
    char sSaltHex[DIRECTGATE_AUTH_SALT_HEX_SIZE];
    char sVerifierHex[DIRECTGATE_AUTH_VERIFIER_HEX_SIZE];
    uint32_t nSuite;
} directgate_auth_t;

void DirectGate_AuthInit(directgate_auth_t *pAuth);
xbool_t DirectGate_AuthLoad(directgate_auth_t *pAuth, xjson_obj_t *pRoot);
xbool_t DirectGate_AuthIsConfigured(const directgate_auth_t *pAuth);

xbool_t DirectGate_AuthSaltHexToBytes(const char *pSaltHex, uint8_t *pSalt, size_t nSaltSize);
xbool_t DirectGate_AuthGenerateRecord(directgate_auth_t *pAuth, const char *pPassword);

#ifdef __cplusplus
}
#endif

#endif
