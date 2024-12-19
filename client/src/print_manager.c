
#ifdef TEST
#include "priv_print_manager.h"
#else
#include "print_manager.h"
#endif

#include "scrollback_window.h"
#include "../../libs/src/common.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

STATIC void move_cursor(WINDOW *window, int y, int x);
STATIC int concat_msg_tokens(cchar_t *buffer, int size, MessageTokens *messageTokens);

MessageTokens * create_message_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format) {

    MessageTokens *messageTokens = (MessageTokens *) malloc(sizeof(MessageTokens));

    messageTokens->useTimestamp = useTimestamp;
    messageTokens->separator = separator;
    messageTokens->origin = origin;
    messageTokens->content = content;
    messageTokens->format = format;

    return messageTokens;
}

void delete_message_tokens(MessageTokens *messageTokens) {

    free(messageTokens);
}

void print_tokens(BaseWindow *baseWindow, MessageTokens *messageTokens) {

    if (baseWindow == NULL || messageTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_window_type(baseWindow) != SCROLLBACK_WINDOW) {
        LOG(NO_ERRCODE, "Invalid window type");
        return;
    }

    WINDOW *window = get_window(baseWindow);
    Scrollback *scrollback = get_scrollback((ScrollbackWindow*)baseWindow);

    cchar_t buffer[MAX_CHARS + 1] = {0};

    concat_msg_tokens(buffer, ARRAY_SIZE(buffer), messageTokens);

    if (!is_sb_scrolled_up(scrollback)) {

        int y, x;
        (void) y;
        
        save_cursor(window, y, x);

        if (x != 0) {
            add_newline(window);
        }
        print_cx_string(window, buffer, -1, -1);
    }
    add_to_scrollback(scrollback, buffer);
}

void print_tokens_xy(WINDOW *window, MessageTokens *messageTokens, int y, int x) {

    if (window == NULL || messageTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int currentY, currentX;
 
    save_cursor(window, currentY, currentX);

    cchar_t buffer[MAX_CHARS + 1] = {0};

    concat_msg_tokens(buffer, ARRAY_SIZE(buffer), messageTokens);
    print_cx_string(window, buffer,  y, x);

    restore_cursor(window, currentY, currentX);
}

void print_cx_string(WINDOW *window, cchar_t *string, int y, int x) {

    if (window == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    move_cursor(window, y, x);

    int count = count_complex_chars(string);

    for (int i = 0; i < count; i++) {
        wadd_wch(window, &string[i]);
    }
}

void print_string(WINDOW *window, const char *string, int y, int x) {

    if (window == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    move_cursor(window, y, x);
    mvwaddstr(window, y, x, string);
}

STATIC void move_cursor(WINDOW *window, int y, int x) {

    if (window == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (y != -1 || x != -1) {

        int currentY, currentX;

        save_cursor(window, currentY, currentX);

        if (y == -1) {
            y = currentY;
        }

        if (x == -1) {
            x = currentX;
        }
        wmove(window, y, x);
    }
}

STATIC int concat_msg_tokens(cchar_t *buffer, int size, MessageTokens *messageTokens) {
    
    if (buffer == NULL || messageTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    cchar_t *bufferPtr = buffer;

    if (messageTokens->useTimestamp) {

        char timestamp[HM_TIME_LENGTH] = {'\0'};
        get_datetime(get_format_function(HM_TIME), timestamp, HM_TIME_LENGTH);
        
        bufferPtr += str_to_complex_str(bufferPtr, size - count_complex_chars(buffer), timestamp, 0);
    }
    if (messageTokens->separator != NULL) {

        bufferPtr += str_to_complex_str(bufferPtr, size - count_complex_chars(buffer), messageTokens->separator, encode_text_format(0, 3, (messageTokens->format & FORMAT_MASK_SEP)));
    }
    if (messageTokens->origin != NULL) {

        bufferPtr += str_to_complex_str(bufferPtr, size - count_complex_chars(buffer), messageTokens->origin, encode_text_format(1, 4, (messageTokens->format & FORMAT_MASK_ORG)));
    }
    if (messageTokens->content != NULL) {

        bufferPtr += str_to_complex_str(bufferPtr, size - count_complex_chars(buffer), messageTokens->content, encode_text_format(2, 5, (messageTokens->format & FORMAT_MASK_CNT)));
    }

    return bufferPtr - buffer;
}
