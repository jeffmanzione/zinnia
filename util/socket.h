// socket.h
//
// Created on: Jan 27, 2019
//     Author: Jeff

#ifndef UTIL_SOCKET_H_
#define UTIL_SOCKET_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(OS_WINDOWS)
typedef uint32_t SOCKET;
#endif

#include "util/platform.h"

typedef unsigned long ulong;

typedef struct __Socket Socket;
typedef struct __SocketAddress SocketAddress;

typedef int SocketStatus;

typedef struct __SocketHandle SocketHandle;

void sockets_init();

void sockets_cleanup();

Socket *socket_create(int domain, int type, int protocol, unsigned long host,
                      uint16_t port);

bool socket_is_valid(const Socket *socket);

SocketStatus socket_bind(Socket *socket);

SocketStatus socket_listen(Socket *socket, int num_connections);

SocketHandle *socket_accept(Socket *socket);
SocketHandle *socket_connect(Socket *socket);

void socket_close(Socket *socket);

void socket_delete(Socket *socket);

bool sockethandle_is_valid(const SocketHandle *sh);

SocketStatus sockethandle_send(SocketHandle *sh, const char *const msg,
                               int msg_len);
int32_t sockethandle_receive(SocketHandle *sh, char *buf, int buf_len);

SOCKET sockethandle_get_socket(SocketHandle *sh);

void sockethandle_close(SocketHandle *sh);

void sockethandle_delete(SocketHandle *sh);

unsigned long socket_inet_address(const char *host, size_t host_len);

#endif /* UTIL_SOCKET_H_ */