#ifndef COMMON_H
#define COMMON_H

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define ASSERT_ARRAY_SIZE(array, count) static_assert(ARRAY_SIZE(array) == count, "Array size mismatch");

#define UNASSIGNED -1
#define MAX_CHARS 512
#define MAX_SEPARATOR_LEN 1
#define MAX_DIGITS 10
#define MAX_NICKNAME_LEN 9
#define MAX_CHANNEL_LEN 50
#define CRLF "\r\n"
#define CRLF_LEN 2

#endif