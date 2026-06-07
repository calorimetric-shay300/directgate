/*!
 * @file directgate-agent/src/common/siv.h
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

#ifndef __DIRECTGATE_SIV_H__
#define __DIRECTGATE_SIV_H__

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * AES-SIV (RFC 5297) authenticated encryption behind a single backend-agnostic
 * interface.
 *
 * A fresh nonce from the system CSPRNG is generated for every encryption and
 * incorporated into the S2V calculation as associated data. This provides
 * non-deterministic ciphertexts during normal operation and reduces
 * ciphertext repetition across otherwise identical messages.
 *
 * The design does not rely exclusively on nonce uniqueness. Protocol-specific
 * authenticated metadata, including a strictly monotonically increasing packet
 * counter, contributes to the synthetic IV derivation and therefore helps
 * avoid ciphertext repetition.
 *
 * AES-SIV further provides nonce-misuse resistance: accidental nonce reuse
 * does not result in the catastrophic confidentiality failures associated
 * with nonce-based AEAD modes such as AES-GCM. The random nonce is therefore
 * an additional defense-in-depth mechanism rather than a single point of
 * security.
 *
 * Two interchangeable implementations sit underneath and, for a given
 * (key, nonce, plaintext), emit byte-identical synthetic IV and ciphertext, so
 * a message encrypted by one backend decrypts under the other. Cross-backend
 * compatibility and output equivalence are verified by tests/crypto_siv_openssl_smoke.c.
 *
 *   - OpenSSL EVP AES-SIV  - preferred, used when the build links OpenSSL >= 3.0
 *                            and that build actually exposes the SIV cipher.
 *   - libxutils XAES       - portable fallback (XAES_MODE_SIV_NONCE), always compiled
 *                            in, used when OpenSSL is older or its SIV provider
 *                            is missing.
 *
 * The backend is probed once and cached. Output layout is
 * `nonce(16) || SIV_tag(16) || ciphertext`, i.e. 32 bytes longer than the
 * plaintext. The nonce travels with the ciphertext; the receiver needs no
 * out-of-band state.
 *
 * @param pCmacKey  S2V (MAC) key, nKeyBits/8 bytes.
 * @param pCtrKey   CTR key, nKeyBits/8 bytes.
 * @param nKeyBits  AES key size in bits: 128, 192 or 256.
 * @param pData     Input buffer (plaintext for encrypt; nonce||tag||ct for decrypt).
 * @param nLength   Input length in bytes (> 0 for encrypt; > 32 for decrypt).
 * @param pOutLen   Receives the output length.
 *
 * @return malloc'd output buffer the caller must free(), or NULL on failure
 *         (including a tag mismatch on decrypt).
 */
uint8_t* DirectGate_SIV_Encrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                const uint8_t *pData, size_t nLength, size_t *pOutLen);

uint8_t* DirectGate_SIV_Decrypt(const uint8_t *pCmacKey, const uint8_t *pCtrKey, size_t nKeyBits,
                                const uint8_t *pData, size_t nLength, size_t *pOutLen);

#ifdef __cplusplus
}
#endif

#endif /* __DIRECTGATE_SIV_H__ */
