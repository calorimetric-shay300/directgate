#include "src/agent/term.c"

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "term_priv_smoke: %s\n", msg); \
            return 1; \
        } \
    } while (0)

int main(void)
{
    directgate_term_t term;
    memset(&term, 0, sizeof(term));
    xstrncpy(term.sShellUser, sizeof(term.sShellUser),
        "directgate-no-such-user-privdrop");

    CHECK(DirectGate_Term_ApplyShell(&term) == XSTDERR,
        "missing configured shell user should fail closed");

    struct passwd *pSelf = getpwuid(getuid());
    if (pSelf != NULL && xstrused(pSelf->pw_name))
    {
        memset(&term, 0, sizeof(term));
        xstrncpy(term.sShellUser, sizeof(term.sShellUser), pSelf->pw_name);
        xstrncpy(term.sShellHome, sizeof(term.sShellHome), ".");
        CHECK(DirectGate_Term_ApplyShell(&term) == XSTDOK,
            "current shell user should not require a privilege drop");
    }

    puts("term_priv_smoke: OK");
    return 0;
}
