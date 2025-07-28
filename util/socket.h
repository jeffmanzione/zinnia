// socket.h
//
// Created on: Jan 27, 2019
//     Author: Jeff

#ifndef UTIL_SOCKET_H_
#define UTIL_SOCKET_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "util/platform.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN (15 + 1)
#endif

#ifndef OS_WINDOWS
typedef uint32_t SOCKET;
#endif

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

void sockethandle_close(SocketHandle *sh);

void sockethandle_delete(SocketHandle *sh);

unsigned long socket_inet_address(const char *host, size_t host_len);

typedef enum {
  ADDRESS_LOOKUP_STATUS_SUCCESS = 0,
  ADDRESS_LOOKUP_STATUS_FAILED_SOCKET = 1,
  ADDRESS_LOOKUP_STATUS_FAILED_CONNECT = 2,
  ADDRESS_LOOKUP_STATUS_FAILED_GET_SOCKET_NAME = 3,
  ADDRESS_LOOKUP_STATUS_FAILED_INET_NTOP = 4
} AddressLookupStatus;

AddressLookupStatus local_ip_address(char *buf);

#endif /* UTIL_SOCKET_H_ */