/*!
 * @file directgate-agent/src/client/relay.h
 * @brief Relay connection envelope fetch via API.
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

#ifndef __DIRECTGATE_CLIENT_RELAY_H__
#define __DIRECTGATE_CLIENT_RELAY_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

xbool_t DirectGate_Relay_FetchEnvelope(directgate_cfg_t *pCfg);

#ifdef __cplusplus
}
#endif

#endif
