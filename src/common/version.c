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

/* Build channel tag baked in at configure time (CMake DIRECTGATE_BUILD_TAG,
 * e.g. amd64_deb, x86_64_rpm, brew_silicon). Appended as "-<tag>", producing
 * versions like 1.0.19-5-amd64_deb. Builds without the definition keep the
 * plain 1.0.19-5 format, which the API accepts as well. */
#ifdef DIRECTGATE_BUILD_TAG
#define DIRECTGATE_VERSION_TAG_SUFFIX "-" DIRECTGATE_BUILD_TAG
#else
#define DIRECTGATE_VERSION_TAG_SUFFIX ""
#endif

const char* DirectGate_GetVersionShort(void)
{
    return XSTRFY(DIRECTGATE_VERSION_MAJOR) "."
           XSTRFY(DIRECTGATE_VERSION_MINOR) "."
           XSTRFY(DIRECTGATE_VERSION_BUILD) "-"
           XSTRFY(DIRECTGATE_VERSION_PKG)
           DIRECTGATE_VERSION_TAG_SUFFIX;
}

const char* DirectGate_GetVersionLong(void)
{
    return XSTRFY(DIRECTGATE_VERSION_MAJOR) "."
           XSTRFY(DIRECTGATE_VERSION_MINOR) " build "
           XSTRFY(DIRECTGATE_VERSION_BUILD) "-"
           XSTRFY(DIRECTGATE_VERSION_PKG)
           DIRECTGATE_VERSION_TAG_SUFFIX " (" __DATE__ ")";
}

