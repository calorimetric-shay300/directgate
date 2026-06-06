/*!
 * @file directgate-agent/src/common/version.c
 * @brief DirectGate version helpers.
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

#include "includes.h"
#include "version.h"

const char* DirectGate_GetVersionShort(void)
{
    return XSTRFY(DIRECTGATE_VERSION_MAJOR) "."
           XSTRFY(DIRECTGATE_VERSION_MINOR) "."
           XSTRFY(DIRECTGATE_VERSION_BUILD) "-"
           XSTRFY(DIRECTGATE_VERSION_PKG);
}

const char* DirectGate_GetVersionLong(void)
{
    return XSTRFY(DIRECTGATE_VERSION_MAJOR) "."
           XSTRFY(DIRECTGATE_VERSION_MINOR) " build "
           XSTRFY(DIRECTGATE_VERSION_BUILD) "-"
           XSTRFY(DIRECTGATE_VERSION_PKG) " (" __DATE__ ")";
}

