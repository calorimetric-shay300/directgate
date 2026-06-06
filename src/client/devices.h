/*!
 * @file directgate-agent/src/client/devices.h
 * @brief Client device management
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

#ifndef __DIRECTGATE_CLIENT_DEVICES_H__
#define __DIRECTGATE_CLIENT_DEVICES_H__

xbool_t DirectGate_Devices_Load(xmap_t *pMap, const char *pPath);
xbool_t DirectGate_Devices_Write(xmap_t *pMap, const char *pPath);
xbool_t DirectGate_Devices_Add(xmap_t *pMap, const char *pDeviceName, const char *pDeviceId, xbool_t bForce);
xbool_t DirectGate_Devices_Search(xmap_t *pMap, const char *pDeviceName, char *pDeviceId, size_t nIdSize);

#ifdef __cplusplus
}
#endif

#endif
