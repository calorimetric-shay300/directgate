/*!
 * @file directgate-agent/src/common/common.c
 * @brief Common utility functions.
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
#include "common.h"

size_t DirectGate_GetQueryValue(const char *pUri, const char *pKey, char *pBuffer, size_t nSize)
{
    XCHECK((pBuffer != NULL), XSTDNON);
    pBuffer[0] = XSTR_NUL;

    XCHECK_NL((xstrused(pUri) && xstrused(pKey) && nSize > 0), XSTDNON);
    int nQueryPos = xstrsrc(pUri, "?");
    XCHECK_NL((nQueryPos >= 0), XSTDNON);

    const char *pQuery = &pUri[nQueryPos + 1];
    size_t nKeyLen = strlen(pKey);

    while (*pQuery != XSTR_NUL)
    {
        while (*pQuery == '&') pQuery++;
        if (*pQuery == XSTR_NUL) break;

        const char *pItem = pQuery;
        while (*pQuery != XSTR_NUL && *pQuery != '&') pQuery++;

        size_t nItemLen = (size_t)(pQuery - pItem);
        if (nItemLen > nKeyLen + 1 &&
            xstrncmp(pItem, pKey, nKeyLen) &&
            pItem[nKeyLen] == '=')
        {
            const char *pValue = &pItem[nKeyLen + 1];
            size_t nValueLen = nItemLen - nKeyLen - 1;
            return xstrncpys(pBuffer, nSize, pValue, nValueLen);
        }
    }

    return XSTDNON;
}

int DirectGate_RemoveNewLine(char *pStr, size_t *pLen)
{
    if (pStr == NULL || pLen == NULL || *pLen == 0) return -1;
    size_t nLen = *pLen;

    if (pStr[nLen - 1] == '\n')
    {
        pStr[nLen - 1] = '\0';
        --nLen;
    }

    if (nLen > 0 && pStr[nLen - 1] == '\r')
    {
        pStr[nLen - 1] = '\0';
        --nLen;
    }

    *pLen = nLen;
    return nLen;
}

const char* DirectGate_JumpWiteSpace(const char *pStr)
{
    XCHECK((pStr != NULL), NULL);
    while (*pStr != '\0' && isspace((unsigned char)*pStr)) pStr++;
    return pStr;
}

const char* DirectGate_SkipToken(const char *pStr)
{
    XCHECK((pStr != NULL), NULL);
    while (*pStr != '\0' && !isspace((unsigned char)*pStr)) pStr++;
    return pStr;
}

void DirectGate_TrimStringRight(char *pStr)
{
    XCHECK_VOID((pStr != NULL));
    size_t nLen = strlen(pStr);
    while (nLen > 0)
    {
        unsigned char c = (unsigned char)pStr[nLen - 1];
        if (!isspace(c)) break;
        pStr[nLen - 1] = '\0';
        nLen--;
    }
}

xbool_t DirectGate_FindCRLF(const uint8_t *pData, size_t nSize, size_t *pOffset)
{
    XCHECK_NL((pData != NULL), XFALSE);

    for (size_t i = 0; i + 1 < nSize; i++)
    {
        if (pData[i] == '\r' && pData[i + 1] == '\n')
        {
            if (pOffset != NULL) *pOffset = i;
            return XTRUE;
        }
    }

    return XFALSE;
}

xbool_t DirectGate_ParseI64(const uint8_t *pData, size_t nLength, int64_t *pValue)
{
    XCHECK_NL((pData != NULL), XFALSE);
    XCHECK_NL((pValue != NULL), XFALSE);
    XCHECK_NL((nLength > 0 && nLength < 64), XFALSE);

    char sNumber[64];
    xstrncpys(sNumber, sizeof(sNumber), (const char*)pData, nLength);

    char *pEnd = NULL;
    long long nParsed = strtoll(sNumber, &pEnd, 10);
    XCHECK_NL((pEnd != NULL && *pEnd == XSTR_NUL), XFALSE);

    *pValue = (int64_t)nParsed;
    return XTRUE;
}

xbool_t DirectGate_EnsurePrivateFileParent(const char *pPath)
{
    XCHECK((xstrused(pPath)), XFALSE);

    char sDir[XPATH_MAX];
    xstrncpy(sDir, sizeof(sDir), pPath);

    char *pSlash = strrchr(sDir, '/');
    XCHECK_NL((pSlash != NULL), XTRUE);

    if (pSlash == sDir)
        return XTRUE;

    *pSlash = XSTR_NUL;
    if (!xstrused(sDir) || xstrcmp(sDir, "."))
        return XTRUE;

    xstat_t before;
    xbool_t bExisted = (xstat(sDir, &before) == XSTDOK);
    if (bExisted && !S_ISDIR(before.st_mode))
        return XFALSE;

    if (!XDir_Create(sDir, 0700))
        return XFALSE;

    xstat_t after;
    if (xstat(sDir, &after) != XSTDOK || !S_ISDIR(after.st_mode))
        return XFALSE;

#ifdef S_ISVTX
    if (after.st_mode & S_ISVTX)
        return XTRUE;
#endif

    if (!bExisted && xchmod(sDir, 0700) < 0)
        return XFALSE;

    return XTRUE;
}

xbool_t DirectGate_WritePrivateFile(const char *pPath, const uint8_t *pData, size_t nSize)
{
    XCHECK((xstrused(pPath)), XFALSE);
    XCHECK((pData != NULL), XFALSE);
    XCHECK((nSize > 0), XFALSE);

    struct stat target;
    if (xstat(pPath, &target) == XSTDOK)
    {
        if (S_ISLNK(target.st_mode))
            return XFALSE;
    }
    else if (errno != ENOENT)
    {
        xloge("Failed to stat file: path(%s), errno(%d)", pPath, errno);
        return XFALSE;
    }

    if (!DirectGate_EnsurePrivateFileParent(pPath))
        return XFALSE;

    char sTempPath[XPATH_MAX];
    int nTempLen = snprintf(sTempPath, sizeof(sTempPath), "%s.tmp.XXXXXX", pPath);
    if (nTempLen < 0 || (size_t)nTempLen >= sizeof(sTempPath)) return XFALSE;

    /* mkstemp automatically creates file with 0600 permissions.
       Then we anyway set it to 0600 for defense in depth. */
    int nFd = mkstemp(sTempPath);
    if (nFd < 0) return XFALSE;

    int nFdFlags = fcntl(nFd, F_GETFD);
    if (nFdFlags >= 0) (void)fcntl(nFd, F_SETFD, nFdFlags | FD_CLOEXEC);

    if (fchmod(nFd, 0600) != 0)
    {
        close(nFd);
        unlink(sTempPath);
        return XFALSE;
    }

    size_t nWritten = 0;
    while (nWritten < nSize)
    {
        ssize_t nRet = write(nFd, pData + nWritten, nSize - nWritten);
        if (nRet < 0 && errno == EINTR) continue;

        if (nRet <= 0)
        {
            close(nFd);
            unlink(sTempPath);
            return XFALSE;
        }

        nWritten += (size_t)nRet;
    }

    while (fsync(nFd) != 0)
    {
        if (errno == EINTR) continue;

        close(nFd);
        unlink(sTempPath);
        return XFALSE;
    }

    if (close(nFd) != 0)
    {
        unlink(sTempPath);
        return XFALSE;
    }

    if (rename(sTempPath, pPath) != 0)
    {
        unlink(sTempPath);
        return XFALSE;
    }

    return XTRUE;
}


xbool_t DirectGate_PromptString(const char *pLabel, char *pOut, size_t nSize,
                                const char *pDefault, xbool_t bRequired)
{
    XCHECK((pLabel != NULL), XFALSE);
    XCHECK((pOut != NULL), XFALSE);

    for (;;)
    {
        char sDefault[XPATH_MAX] = {0};
        char sInput[XPATH_MAX] = {0};
        char sPrompt[256] = {0};

        const char *pShow = xstrused(pOut) ? pOut : pDefault;
        if (xstrused(pShow)) xstrncpy(sDefault, sizeof(sDefault), pShow);

        if (!xstrused(sDefault)) xstrncpyf(sPrompt, sizeof(sPrompt), "%s: ", pLabel);
        else xstrncpyf(sPrompt, sizeof(sPrompt), "%s [%s]: ", pLabel, sDefault);

        XSTATUS nStatus = XCLI_GetInput(sPrompt, sInput, sizeof(sInput), XTRUE);
        if (nStatus < 0) return XFALSE;

        if (!xstrused(sInput))
        {
            if (xstrused(sDefault))
            {
                xstrncpy(pOut, nSize, sDefault);
                return bRequired ? xstrused(pOut) : XTRUE;
            }

            if (!bRequired) return XTRUE;
            continue;
        }

        xstrncpy(pOut, nSize, sInput);
        return XTRUE;
    }
}

xbool_t DirectGate_PromptBool(const char *pLabel, xbool_t *pValue)
{
    XCHECK((pLabel != NULL), XFALSE);
    XCHECK((pValue != NULL), XFALSE);

    char sInput[16];
    char sPrompt[128];
    const char *pDefault = (*pValue) ? "y" : "n";

    xstrncpyf(sPrompt, sizeof(sPrompt), "%s [%s]: ", pLabel, pDefault);
    XSTATUS nStatus = XCLI_GetInput(sPrompt, sInput, sizeof(sInput), XTRUE);
    if (nStatus < 0) return XFALSE;
    if (!xstrused(sInput)) return XTRUE;

    if (sInput[0] == 'y' || sInput[0] == 'Y' || sInput[0] == '1') *pValue = XTRUE;
    else if (sInput[0] == 'n' || sInput[0] == 'N' || sInput[0] == '0') *pValue = XFALSE;
    else return XFALSE;

    return XTRUE;
}

xbool_t DirectGate_PromptU16(const char *pLabel, uint16_t *pValue)
{
    XCHECK((pLabel != NULL), XFALSE);
    XCHECK((pValue != NULL), XFALSE);

    char sInput[32];
    char sPrompt[128];

    uint16_t nDefault = *pValue;
    if (nDefault) xstrncpyf(sPrompt, sizeof(sPrompt), "%s [%u]: ", pLabel, nDefault);
    else xstrncpyf(sPrompt, sizeof(sPrompt), "%s: ", pLabel);

    XSTATUS nStatus = XCLI_GetInput(sPrompt, sInput, sizeof(sInput), XTRUE);
    if (nStatus < 0) return XFALSE;
    if (!xstrused(sInput)) return XTRUE;

    char *pEnd = NULL;
    uint16_t nVal = (uint16_t)strtoul(sInput, &pEnd, 10);
    if (pEnd == sInput) return XFALSE;

    *pValue = nVal;
    return XTRUE;
}

xbool_t DirectGate_PromptU32(const char *pLabel, uint32_t *pValue)
{
    XCHECK((pLabel != NULL), XFALSE);
    XCHECK((pValue != NULL), XFALSE);

    char sInput[32];
    char sPrompt[128];

    uint32_t nDefault = *pValue;
    if (nDefault) xstrncpyf(sPrompt, sizeof(sPrompt), "%s [%u]: ", pLabel, nDefault);
    else xstrncpyf(sPrompt, sizeof(sPrompt), "%s: ", pLabel);

    XSTATUS nStatus = XCLI_GetInput(sPrompt, sInput, sizeof(sInput), XTRUE);
    if (nStatus < 0) return XFALSE;
    if (!xstrused(sInput)) return XTRUE;

    char *pEnd = NULL;
    uint32_t nVal = (uint32_t)strtoul(sInput, &pEnd, 10);
    if (pEnd == sInput) return XFALSE;

    *pValue = nVal;
    return XTRUE;
}
