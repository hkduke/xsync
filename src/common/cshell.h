/**
 * cshell.h
 *   c interactive shell
 *
 * author: master@pepstack.com
 *
 * create: 2018-02-08
 * update: 2018-02-08
 */

#ifndef CSHELL_H_INCLUDED
#define CSHELL_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdio.h>      /* printf, scanf, puts */
#include <stdlib.h>     /* realloc, free, exit, NULL */

/**
 * ------ color output ------
 * 30: black
 * 31: red
 * 32: green
 * 33: yellow
 * 34: blue
 * 35: purple
 * 36: cyan
 * 37: white
 * --------------------------
 * "\033[31m RED   \033[0m"
 * "\033[32m GREEN \033[0m"
 * "\033[33m YELLOW \033[0m"
 */


#define csh_red_msg(message)     "\033[31m"message"\033[0m"
#define csh_green_msg(message)   "\033[32m"message"\033[0m"
#define csh_yellow_msg(message)  "\033[33m"message"\033[0m"
#define csh_blue_msg(message)    "\033[34m"message"\033[0m"
#define csh_purple_msg(message)  "\033[35m"message"\033[0m"
#define csh_cyan_msg(message)    "\033[36m"message"\033[0m"

#define CSH_RED_MSG(message)     "\033[1;31m"message"\033[0m"
#define CSH_GREEN_MSG(message)   "\033[1;32m"message"\033[0m"
#define CSH_YELLOW_MSG(message)  "\033[1;33m"message"\033[0m"
#define CSH_BLUE_MSG(message)    "\033[1;34m"message"\033[0m"
#define CSH_PURPLE_MSG(message)  "\033[1;35m"message"\033[0m"
#define CSH_CYAN_MSG(message)    "\033[1;36m"message"\033[0m"


__no_warning_unused(static)
const char * getinputline (const char *message, char *line, int maxsize)
{
    int i, c;
    char *pl = line;

    if (message) {
        printf("%s", message);
    }

    if (! line) {
        return NULL;
    }

    i = 0;

    while (i < maxsize) {
        c = fgetc(stdin);

        if (c == EOF) {
            break;
        }

        if ((*pl++ = c) == '\n') {
            break;
        }

        i++;
    }

    if (i > 0) {
        *--pl = '\0';
    } else {
        *line = '\0';
    }

    return line;
}


#if defined(__cplusplus)
}
#endif

#endif /* CSHELL_H_INCLUDED */
