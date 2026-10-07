#ifndef I18N_H
#define I18N_H
typedef enum { LANG_EN, LANG_MR, LANG_HI, LANG_GU, LANG_COUNT } Language;
typedef enum { T_WELCOME, T_SELECT_ROUTE, T_START, T_DESTINATION, T_CONTINUE, T_PAY, T_PRINT, T_HELP, T_BACK, T_CASH, T_CARD, T_UPI, T_PRINTER_EMPTY, T_TICKET_READY, T_TAP_ROUTE, T_TAP_STOPS, T_CANCEL, T_NEW_TICKET, T_HELPLINE, T_ROUTE, T_FARE, T_TOUCH_HINT, T_COUNT } TextKey;
const char *tr(Language language, TextKey key);
/* Self-name of a language (e.g. "English", "मराठी"), used for the language
 * switch button label -- shows the name of the language it switches to. */
const char *language_name(Language language);
#endif
