#include "i18n.h"
static const char *EN[T_COUNT]={"Welcome to BEST","Choose a route","Start","Destination","Continue","Pay & confirm","Print ticket","Help","Back","Cash","Card","UPI","Printer needs paper","Ticket printed","Tap a coloured route","Tap start, then destination","Cancel","New ticket","Helpline: 1800-22-7550","Route","Fare","Click to select"};
static const char *MR[T_COUNT]={"बेस्टमध्ये आपले स्वागत आहे","मार्ग निवडा","सुरुवात","गंतव्य","पुढे","पैसे द्या व खात्री करा","तिकीट छापा","मदत","मागे","रोख","कार्ड","यूपीआय","प्रिंटरमध्ये कागद नाही","तिकीट छापले","रंगीत मार्ग निवडा","सुरुवात, नंतर गंतव्य निवडा","रद्द करा","नवीन तिकीट","मदत क्रमांक: १८००-२२-७५५०","मार्ग","भाडे","निवडण्यासाठी क्लिक करा"};
static const char *HI[T_COUNT]={"बेस्ट में आपका स्वागत है","मार्ग चुनें","प्रारंभ","गंतव्य","आगे बढ़ें","भुगतान करें और पुष्टि करें","टिकट प्रिंट करें","सहायता","वापस","नकद","कार्ड","यूपीआई","प्रिंटर में कागज़ नहीं है","टिकट प्रिंट हो गया","रंगीन मार्ग चुनें","पहले प्रारंभ, फिर गंतव्य चुनें","रद्द करें","नया टिकट","हेल्पलाइन: १८००-२२-७५५०","मार्ग","किराया","चुनने के लिए क्लिक करें"};
/* Noto Sans Gujarati's bundled subset has almost no ASCII punctuation
 * (confirmed via fc-query: just U+0020 space outside its own script block),
 * so these strings deliberately avoid plain ",", ":" and "-" in favour of
 * the Gujarati-safe danda (।, U+0964) and Unicode hyphen (‐, U+2010). */
static const char *GU[T_COUNT]={"બેસ્ટમાં આપનું સ્વાગત છે","માર્ગ પસંદ કરો","શરૂઆત","ગંતવ્ય","આગળ વધો","ચુકવણી કરો અને પુષ્ટિ કરો","ટિકિટ છાપો","મદદ","પાછા","રોકડ","કાર્ડ","યુપીઆઈ","પ્રિન્ટરમાં કાગળ નથી","ટિકિટ છપાઈ ગઈ","રંગીન માર્ગ પસંદ કરો","પહેલા શરૂઆત। પછી ગંતવ્ય પસંદ કરો","રદ કરો","નવી ટિકિટ","હેલ્પલાઇન ૧૮૦૦‐૨૨‐૭૫૫૦","માર્ગ","ભાડું","પસંદ કરવા માટે ક્લિક કરો"};
static const char **const TABLES[LANG_COUNT] = {EN, MR, HI, GU};
static const char *const NAMES[LANG_COUNT] = {"English", "मराठी", "हिन्दी", "ગુજરાતી"};
const char *tr(Language l, TextKey k) { return TABLES[l][k]; }
const char *language_name(Language l) { return NAMES[l]; }
