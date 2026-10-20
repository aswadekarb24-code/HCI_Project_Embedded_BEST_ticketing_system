#include "i18n.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
/* Every TextKey must have a real translation in every language -- catches a
 * future TextKey or Language addition that forgets to extend a table. */
int main(void) {
    for (Language l = LANG_EN; l < LANG_COUNT; l++) {
        assert(language_name(l) && language_name(l)[0]);
        for (TextKey k = T_WELCOME; k < T_COUNT; k++) {
            const char *s = tr(l, k);
            assert(s && s[0]);
        }
    }
    /* English and Marathi strings must differ (sanity check that the table
     * lookup isn't accidentally aliased to the same array). */
    assert(strcmp(tr(LANG_EN, T_WELCOME), tr(LANG_MR, T_WELCOME)) != 0);
    assert(strcmp(tr(LANG_HI, T_WELCOME), tr(LANG_GU, T_WELCOME)) != 0);
    puts("i18n tests passed");
}
