#define DirectGate_Enroll_IsEnrolled Test_DirectGate_Enroll_IsEnrolled
#define DirectGate_Enroll_Pair Test_DirectGate_Enroll_Pair
#define DirectGate_Enroll_RotateAgentKey Test_DirectGate_Enroll_RotateAgentKey
#include "src/agent/config.c"
#undef DirectGate_Enroll_IsEnrolled
#undef DirectGate_Enroll_Pair
#undef DirectGate_Enroll_RotateAgentKey

#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "enroll_prompt_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

xbool_t Test_DirectGate_Enroll_IsEnrolled(const directgate_cfg_t *pCfg)
{
    return pCfg != NULL && pCfg->enroll.bEnrolled;
}

xbool_t Test_DirectGate_Enroll_RotateAgentKey(directgate_cfg_t *pCfg)
{
    (void)pCfg;
    return XFALSE;
}

xbool_t Test_DirectGate_Enroll_Pair(directgate_cfg_t *pCfg, const char *pPairingToken)
{
    XCHECK((pCfg != NULL && xstrused(pPairingToken)), XFALSE);

    xstrncpy(pCfg->sRelayUrl, sizeof(pCfg->sRelayUrl),
        "wss://relay.prompt.example.test/websock");
    xstrncpy(pCfg->sRoutingKey, sizeof(pCfg->sRoutingKey), "rk-prompt");
    xstrncpy(pCfg->enroll.sAccessToken, sizeof(pCfg->enroll.sAccessToken),
        "access-prompt");
    xstrncpy(pCfg->enroll.sRefreshToken, sizeof(pCfg->enroll.sRefreshToken),
        "refresh-prompt");
    xstrncpy(pCfg->enroll.sEnrollExpiresAt, sizeof(pCfg->enroll.sEnrollExpiresAt),
        "2099-01-01T00:00:00.000Z");
    pCfg->enroll.nAccessTokenExp = 4070908800ULL;
    pCfg->enroll.nRefreshTokenExp = 4070908800ULL;
    pCfg->enroll.bEnrolled = XTRUE;

    return DirectGate_SaveConfig(pCfg);
}

static int run_enroll_child(const char *pCfgPath)
{
    directgate_cfg_t cfg;
    DirectGate_InitConfig(&cfg);
    xstrncpy(cfg.sCfgPath, sizeof(cfg.sCfgPath), pCfgPath);
    xstrncpy(cfg.sDeviceId, sizeof(cfg.sDeviceId), "prompt-device");
    xstrncpy(cfg.enroll.sApiUrl, sizeof(cfg.enroll.sApiUrl),
        "https://api.prompt.example.test");
    xstrncpy(cfg.sPairingToken, sizeof(cfg.sPairingToken), "pair-token");
    cfg.bEnroll = XTRUE;
    cfg.bSetSRP = XTRUE;

    return DirectGate_EnrollFromPrompt(&cfg) == XSTDOK ? 0 : 1;
}

int main(void)
{
    char sRoot[] = "/tmp/directgate_enroll_prompt.XXXXXX";
    CHECK(mkdtemp(sRoot) != NULL, "mkdtemp root");

    char sCfgPath[512];
    snprintf(sCfgPath, sizeof(sCfgPath), "%s/test.conf", sRoot);

    int nMaster = -1;
    int nSlave = -1;
    CHECK(openpty(&nMaster, &nSlave, NULL, NULL, NULL) == 0, "openpty");

    pid_t nPid = fork();
    CHECK(nPid >= 0, "fork");

    if (nPid == 0)
    {
        close(nMaster);
        setsid();
        (void)ioctl(nSlave, TIOCSCTTY, 0);
        dup2(nSlave, STDIN_FILENO);
        dup2(nSlave, STDOUT_FILENO);
        dup2(nSlave, STDERR_FILENO);
        if (nSlave > STDERR_FILENO) close(nSlave);

        _exit(run_enroll_child(sCfgPath));
    }

    close(nSlave);

    const char *pInput = "test-password\n"
                         "test-password\n";
    CHECK(write(nMaster, pInput, strlen(pInput)) == (ssize_t)strlen(pInput),
        "write password input");

    int nStatus = 0;
    CHECK(waitpid(nPid, &nStatus, 0) == nPid, "wait child");
    close(nMaster);

    CHECK(WIFEXITED(nStatus) && WEXITSTATUS(nStatus) == 0,
        "enrollment prompt child should succeed");

    directgate_cfg_t loaded;
    DirectGate_InitConfig(&loaded);
    xstrncpy(loaded.sCfgPath, sizeof(loaded.sCfgPath), sCfgPath);
    CHECK(DirectGate_LoadConfig(&loaded, sCfgPath), "load enrolled config");
    CHECK(DirectGate_AuthIsConfigured(&loaded.auth),
        "enrollment should persist SRP verifier");
    CHECK(xstrused(loaded.keyauth.sIdentitySeedB64),
        "enrollment should persist agent identity seed");
    CHECK(loaded.enroll.bEnrolled == XTRUE,
        "enrollment should persist enrolled flag");
    CHECK(strcmp(loaded.sRoutingKey, "rk-prompt") == 0,
        "enrollment should persist routing key");

    unlink(sCfgPath);
    rmdir(sRoot);

    puts("enroll_prompt_smoke: OK");
    return 0;
}
