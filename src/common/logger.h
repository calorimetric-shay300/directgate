/*!
 * @file directgate-agent/src/common/logger.h
 * @brief DirectGate log config parsing and apply helpers.
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

#ifndef __DIRECTGATE_LOGGER_H__
#define __DIRECTGATE_LOGGER_H__

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIRECTGATE_LOG_DIR_MODE 0755

typedef struct directgate_log_ {
    char sIdent[XLOG_NAME_MAX];
    char sPath[XPATH_MAX];
    uint8_t nRTCLevel;
    xbool_t bLogRTC;
    xbool_t bToScreen;
    xbool_t bToFile;
    xbool_t bFlush;
    uint16_t nFlags;
} directgate_log_t;

void DirectGate_LogInit(directgate_log_t *pLog, const char *pDefaultIdent, uint16_t nDefaultFlags);
XSTATUS DirectGate_LogApply(const directgate_log_t *pLog);

xbool_t DirectGate_LogLoad(directgate_log_t *pLog, xjson_obj_t *pRoot);
xbool_t DirectGate_LogSave(const directgate_log_t *pLog, xjson_obj_t *pRoot);

int DirectGate_LogGetRTCLevel(const directgate_log_t *pLog);

#ifdef __cplusplus
}
#endif

#endif

