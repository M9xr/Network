/* sock_init */

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#endif
#include <winsock2.h>
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

/* In UNIX, a socket descriptor is represented by a standard
 * file descriptor. This means you can use any UNIX file I/O
 * functions on sockets. It is not true for Windows. 
 * On Unix, socket() returns and int, whereas
 * Windows returns "SOCKET", which is an unsigned int. To make the program more portable,
 * below is made a macro substitution that defines "SOCKET" int for Unix */
#if !defined(_WIN32)
#define SOCKET int	
#endif


/* Windows again shows its additional chromosome.
 * On Unix, socket() returns a negative number on failure.
 * Windows SOCKET type is unsigned... so we need to do sth about it */
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#endif


/* All sockets on Unix systems are also standard file descriptors,
 * so sockets can be closed by standard close() function. 
 * On system beloved by retards, a special close function is
 * used instead - closesocket(). A macro below, to abstract this out */
#if defined(_WIN32)
#define CLOSESOCKET(s) closesocket(s)
#else
#define CLOSESOCKET(s) close(s)
#endif 


/* When a socket function, such as socket(), binf(), accept(), and so on,
 * has an error on Unix platform, the error number gets stored in the thread-gloabl
 * "errno" variable. On Windows, the error number can be retrieved by calling "WSAGetLastError()" instead.
 * Macro to get rid of it */
#if defined(_WIN32)
#define GETSOCKETERRNO() (WSAGETLASTERROR())
#else
#define GETSOCKETERRNO() (errno)
#endif

#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

const char *get_error_text(void);

int main(void) {

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Failed to initialize.\n");
		return 1;
	}
#endif
	
	char *ptr_error; // pointer to error returned by standard function strerror() 

	printf("Configuring local address...\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *bind_address;
	getaddrinfo(0, "8069", &hints, &bind_address);

	printf("Creating socket...\n");
	SOCKET socket_listen;
	socket_listen = socket(bind_address->ai_family,
			bind_address->ai_socktype, bind_address->ai_protocol);

	if (!ISVALIDSOCKET(socket_listen)) {
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
		ptr_error = get_error_text();
		fprintf(stderr, "%s\n", ptr_error);
		return 1;
	}

	int option = 0;
	if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&option, sizeof(option))) {
		fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		ptr_error = get_error_text();
		fprintf(stderr, "%s\n", ptr_error);
		return 1;
	}

	printf("Binding socket to local adress...\n");
	if (bind(socket_listen,
			bind_address->ai_addr, bind_address->ai_addrlen)) {
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		ptr_error = get_error_text();
		fprintf(stderr, "%s\n", ptr_error);
		return 1;
	}

	/* release the address memory */
	freeaddrinfo(bind_address);
		
	printf("Listening...\n");
	if (listen(socket_listen, 10) < 0) {
		fprintf(stderr, "listen() failed, (%d)\n", GETSOCKETERRNO());
		ptr_error = get_error_text();
		fprintf(stderr, "%s\n", ptr_error);
		return 1;
	}

	printf("Waiting for connection...\n");
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	SOCKET socket_client = accept(socket_listen,
			(struct sockaddr*) &client_address, &client_len);
	if (!ISVALIDSOCKET(socket_client)) {
		fprintf(stderr, "accept() failed, (%d)\n", GETSOCKETERRNO());
		ptr_error = get_error_text();
		fprintf(stderr, "%s\n", ptr_error);
		return 1;
	}
	
	printf("Client is connected... ");
	char address_buffer[100];
	getnameinfo((struct sockaddr*) & client_address,
			client_len, address_buffer, sizeof(address_buffer), 0, 0,
			NI_NUMERICHOST);
	printf("%s\n", address_buffer);

	printf("Reading request...\n");
	char request[1024];
	int bytes_received = recv(socket_client, request, 1024, 0);
	printf("Received %d bytes\n", bytes_received);

	/* It is a common mistake to try printing data that's received
	 * from recv() directly as a C string. There is no guarantee that
	 * the data received from recv() is null terminated!
	 * If you try to print it with printf("%s", request), you will likely
	 * receive a segmentation fault error (or at best it will print some garbage) */
	printf("%.*s", bytes_received, request);


	/* Send response back */
	printf("Sending response...\n");
	const char *response =
		"HTTP/1.1 200 OK\r\n"
		"Conncetion: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";
	int bytes_sent = send(socket_client, response, strlen(response), 0);
	printf("Sent %d of %d bytes.\n", bytes_sent, (int) strlen(response));

	/* get the local time and send to the client */

	time_t timer;
	time(&timer);
	char *time_msg = ctime(&timer);
	bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
	printf("Sent %d of %d bytes.\n", bytes_sent, (int) strlen(time_msg));

	/* close connection to indicate to the browser that we've sent all of out data */
	printf("Closing connection...\n");
	CLOSESOCKET(socket_client);

	/* One is enough. Close the listening socket */
	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);


#if defined(_WIN32)
	WSACleanup();
#endif
	
	printf("Finished.\n");
	return 0;
}

/* get_error_text: retrive a text description of the error condition
 * return a pointer to an error message string */
const char *get_error_text(void) {

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
