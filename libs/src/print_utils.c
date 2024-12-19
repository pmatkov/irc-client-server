#include "print_utils.h"
#include "common.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

/* ncurses text styles */
static const unsigned ATTR_CONVERT[] = {
    A_NORMAL,
    A_BOLD,
    A_STANDOUT,
    A_DIM,
    A_ITALIC
};

int str_to_complex_str(cchar_t *buffer, int size, const char *string, uint32_t format) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (string == NULL || !strlen(string)) {
        return 0;
    }

    if (strlen(string) >= size - count_complex_chars(buffer)) {
        LOG(ERROR, "Not enough space in the buffer");
        return 0;
    }

    int color = format & 0xF;
    attr_t style = (format >> BITS_PER_HEX) & 0xF;

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};
    int wcharCount = 0;

    /* convert chars to wide chars (wchar_t)
        representation */
    if ((wcharCount = mbstowcs(wstring, string, MAX_CHARS)) == -1) {
        FAILED(NO_ERRCODE, "Error converting char to wchar");
    }

    wchar_t wch[2];
    int i = 0;

    /* convert wide chars to complex chars (cchar_t)
        representation */
    while (i < wcharCount && wstring[i] != L'\0') {

        wch[0] = wstring[i];
        wch[1] = L'\0';  

        if (setcchar(buffer++, wch, ATTR_CONVERT[(Attributes)style], color, NULL) != OK) {
            FAILED(NO_ERRCODE, "Error converting wchar to cchar");
        }
        i++;
    }
    return i;
}

int count_complex_chars(cchar_t *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int i = 0;

    while (string[i].chars[0] != L'\0') {

        i++;
    }
    return i;
}