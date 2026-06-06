/*!
 * @file directgate-agent/src/common/hkdf.h
 * @brief HKDF-SHA256 key derivation (RFC 5869).
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

#ifndef __DIRECTGATE_HKDF_H__
#define __DIRECTGATE_HKDF_H__

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XHKDF_SHA256_LEN    32

/*!
 * @brief HKDF-Extract: PRK = HMAC-SHA256(salt, IKM)
 * @param pSalt Salt value (if NULL, uses zeros)
 * @param nSaltLen Length of salt
 * @param pIKM Input keying material
 * @param nIKMLen Length of IKM
 * @param pPRK Output pseudorandom key (must be 32 bytes)
 * @return XTRUE on success
 */
xbool_t DirectGate_HKDF_Extract(const uint8_t *pSalt, size_t nSaltLen,
                                const uint8_t *pIKM, size_t nIKMLen,
                                uint8_t *pPRK);

/*!
 * @brief HKDF-Expand: OKM = T(1) || T(2) || ...
 *        T(i) = HMAC-SHA256(PRK, T(i-1) || info || i)
 * @param pPRK Pseudorandom key from Extract (32 bytes)
 * @param nPRKLen Length of PRK
 * @param pInfo Context/application-specific info string
 * @param pOKM Output keying material
 * @param nOKMLen Desired output length (max 255 * 32 = 8160 bytes)
 * @return XTRUE on success
 */
xbool_t DirectGate_HKDF_Expand(const uint8_t *pPRK, size_t nPRKLen,
                                const char *pInfo, uint8_t *pOKM,
                                size_t nOKMLen);

/*!
 * @brief Convenience: HKDF-Extract + HKDF-Expand in one call
 */
xbool_t DirectGate_HKDF_SHA256(const uint8_t *pSalt, size_t nSaltLen,
                                const uint8_t *pIKM, size_t nIKMLen,
                                const char *pInfo, uint8_t *pOKM,
                                size_t nOKMLen);

#ifdef __cplusplus
}
#endif

#endif /* __DIRECTGATE_HKDF_H__ */
