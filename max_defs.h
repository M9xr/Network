/* chap03.h */

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0600
#endif
#include <windsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif


#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif 

#include <stdio.h>
#include <string.h>

/* get_error_text: retrive a text description of the error condition
 * return a pointer to an error message string */
char *get_error_text(void) {

#if defined(_WIN32)

        static char message[256] = {0};
        FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                0, WSAGetLastError(), 0, message, 256, 0);
        char *nl = strrchr(message, '\n');
        if (nl) *nl = 0;
        return message;

#else
        return strerror(errno);
#endif
}
