#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "src/common/common.h"

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "common_utils_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

int main(void)
{
    char sValue[32];
    CHECK(DirectGate_GetQueryValue("/websock?rk=abc&token=xyz", "rk",
        sValue, sizeof(sValue)) == 3, "query value length");
    CHECK(strcmp(sValue, "abc") == 0, "query value content");
    CHECK(DirectGate_GetQueryValue("/websock?token2=no&token=yes", "token",
        sValue, sizeof(sValue)) == 3, "query key must not match prefixes");
    CHECK(strcmp(sValue, "yes") == 0, "query prefix-safe content");
    CHECK(DirectGate_GetQueryValue("/websock", "rk", sValue, sizeof(sValue)) == 0,
        "missing query must return zero");
    CHECK(sValue[0] == '\0', "missing query must clear output");

    char sLine1[] = "hello\r\n";
    size_t nLine1 = strlen(sLine1);
    CHECK(DirectGate_RemoveNewLine(sLine1, &nLine1) == 5, "CRLF length");
    CHECK(nLine1 == 5 && strcmp(sLine1, "hello") == 0, "CRLF removal");

    char sLine2[] = "hello\n";
    size_t nLine2 = strlen(sLine2);
    CHECK(DirectGate_RemoveNewLine(sLine2, &nLine2) == 5, "LF length");
    CHECK(nLine2 == 5 && strcmp(sLine2, "hello") == 0, "LF removal");

    const uint8_t sBytes[] = { 'a', '\r', '\n', 'b' };
    size_t nOffset = 99;
    CHECK(DirectGate_FindCRLF(sBytes, sizeof(sBytes), &nOffset), "CRLF find");
    CHECK(nOffset == 1, "CRLF offset");
    CHECK(!DirectGate_FindCRLF((const uint8_t*)"abc\r", 4, NULL),
        "partial CRLF must not match");

    int64_t nValue = 0;
    CHECK(DirectGate_ParseI64((const uint8_t*)"-42", 3, &nValue),
        "parse negative int64");
    CHECK(nValue == -42, "negative int64 value");
    CHECK(!DirectGate_ParseI64((const uint8_t*)"42x", 3, &nValue),
        "invalid int64 must fail");

    char sTrim[] = "abc \t\r\n";
    DirectGate_TrimStringRight(sTrim);
    CHECK(strcmp(sTrim, "abc") == 0, "right trim");
    CHECK(*DirectGate_JumpWiteSpace(" \tvalue") == 'v', "jump whitespace");
    CHECK(*DirectGate_SkipToken("value rest") == ' ', "skip token");

    char sRoot[] = "/tmp/directgate_common_private.XXXXXX";
    CHECK(mkdtemp(sRoot) != NULL, "mkdtemp private root");

    char sPrivateDir[XPATH_MAX];
    char sPrivateFile[XPATH_MAX];
    snprintf(sPrivateDir, sizeof(sPrivateDir), "%s/private", sRoot);
    snprintf(sPrivateFile, sizeof(sPrivateFile), "%s/private/config.json", sRoot);

    const uint8_t sSecret[] = "secret";
    CHECK(DirectGate_WritePrivateFile(sPrivateFile, sSecret, sizeof(sSecret) - 1),
        "write private file");

    struct stat st;
    CHECK(stat(sPrivateDir, &st) == 0, "stat private dir");
    CHECK((st.st_mode & 0777) == 0700, "private dir mode");
    CHECK(stat(sPrivateFile, &st) == 0, "stat private file");
    CHECK((st.st_mode & 0777) == 0600, "private file mode");

    CHECK(unlink(sPrivateFile) == 0, "cleanup private file");
    CHECK(rmdir(sPrivateDir) == 0, "cleanup private dir");

    char sCwd[XPATH_MAX];
    CHECK(getcwd(sCwd, sizeof(sCwd)) != NULL, "getcwd before relative private file");
    CHECK(chdir(sRoot) == 0, "chdir relative private file root");
    CHECK(DirectGate_WritePrivateFile("relative-private.json", sSecret, sizeof(sSecret) - 1),
        "write private file without parent directory");
    CHECK(stat("relative-private.json", &st) == 0, "stat relative private file");
    CHECK((st.st_mode & 0777) == 0600, "relative private file mode");
    CHECK(unlink("relative-private.json") == 0, "cleanup relative private file");
    CHECK(DirectGate_WritePrivateFile("./relative-dot-private.json", sSecret, sizeof(sSecret) - 1),
        "write private file in current directory");
    CHECK(stat("./relative-dot-private.json", &st) == 0, "stat current directory private file");
    CHECK((st.st_mode & 0777) == 0600, "current directory private file mode");
    CHECK(unlink("./relative-dot-private.json") == 0, "cleanup current directory private file");
    CHECK(chdir(sCwd) == 0, "restore cwd after relative private file");

    char sExistingDir[XPATH_MAX];
    char sExistingFile[XPATH_MAX];
    snprintf(sExistingDir, sizeof(sExistingDir), "%s/existing", sRoot);
    snprintf(sExistingFile, sizeof(sExistingFile), "%s/existing/config.json", sRoot);
    CHECK(mkdir(sExistingDir, 0755) == 0, "mkdir existing shared dir");
    CHECK(chmod(sExistingDir, 0755) == 0, "chmod existing shared dir");
    CHECK(DirectGate_WritePrivateFile(sExistingFile, sSecret, sizeof(sSecret) - 1),
        "write private file in existing directory");
    CHECK(stat(sExistingDir, &st) == 0, "stat existing shared dir");
    CHECK((st.st_mode & 0777) == 0755,
        "private file writer must not chmod existing directories");
    CHECK(stat(sExistingFile, &st) == 0, "stat private file in existing directory");
    CHECK((st.st_mode & 0777) == 0600,
        "private file in existing directory mode");
    CHECK(unlink(sExistingFile) == 0, "cleanup private file in existing directory");
    CHECK(rmdir(sExistingDir) == 0, "cleanup existing shared dir");
    CHECK(rmdir(sRoot) == 0, "cleanup private root");

    xtime_t parsedTime;
    CHECK(XTime_FromISO(&parsedTime, "2024-02-29T12:34:56Z") == 6,
        "valid ISO leap-day timestamp should parse");
    CHECK(parsedTime.nYear == 2024 && parsedTime.nMonth == 2 && parsedTime.nDay == 29,
        "parsed ISO timestamp fields");
    CHECK(XTime_FromISO(&parsedTime, "2023-02-29T12:34:56Z") == 0,
        "invalid ISO leap-day timestamp should fail");
    CHECK(XTime_GetMonthDays(2024, 2) == 29 && XTime_GetMonthDays(2023, 2) == 28,
        "February day count");

    puts("common_utils_smoke: OK");
    return 0;
}
