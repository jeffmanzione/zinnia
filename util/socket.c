// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff

#include "socket.h"

#include <errno.h>
#include <stdio.h>

#ifdef OS_LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "alloc/alloc.h"
#include "debug/debug.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET 0
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

struct __Socket {
  struct sockaddr_in in;
  SOCKET sock;
  bool is_closed;
};

struct __SocketHandle {
  struct sockaddr_in client;
  SOCKET client_sock;
  bool is_closed;
};

void sockets_init() {
#ifdef OS_WINDOWS
  WSADATA wsa_data = {0};
  WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

void sockets_cleanup() {
#ifdef OS_WINDOWS
  WSACleanup();
#endif
}

void _close_socket(int sock) {
#ifdef OS_WINDOWS
  closesocket(sock);
#else
  close(sock);
#endif
}

Socket *socket_create(int domain, int type, int protocol, unsigned long host,
                      uint16_t port) {
  Socket *sock = MNEW(Socket);
  sock->sock = socket(domain, type, protocol);
  sock->in.sin_family = domain;
  sock->in.sin_addr.s_addr = host;
  sock->in.sin_port = port == 0 ? 0 : htons(port);
  sock->is_closed = false;
  return sock;
}

bool socket_is_valid(const Socket *socket) {
#ifdef OS_WINDOWS
  return socket->sock != INVALID_SOCKET;
#else
  return socket->sock >= 0;
#endif
}

SocketStatus socket_bind(Socket *socket) {
  return bind(socket->sock, (struct sockaddr *)&socket->in, sizeof(socket->in));
}

SocketStatus socket_listen(Socket *socket, int num_connections) {
  return listen(socket->sock, num_connections);
}

SocketHandle *socket_accept(Socket *socket) {
  SocketHandle *sh = MNEW(SocketHandle);
  sh->is_closed = false;
  int addr_len = sizeof(sh->client);
  sh->client_sock = accept(socket->sock, (struct sockaddr *)&sh->client,
#ifdef OS_LINUX
                           (socklen_t *)
#endif
                           &addr_len);

  if (!sockethandle_is_valid(sh)) {
#ifdef OS_WINDOWS
    const int error_code = WSAGetLastError();
    printf("[accept] error_code=%d\n", error_code);
#else
    printf("[accept] errno=%d\n", errno);
#endif
    fflush(stdout);
  }
  return sh;
}

SocketHandle *socket_connect(Socket *socket) {
  SocketHandle *sh = MNEW(SocketHandle);
  sh->is_closed = false;

  const int result =
      connect(socket->sock, (struct sockaddr *)&socket->in, sizeof(socket->in));

  if (result == SOCKET_ERROR) {
#ifdef OS_WINDOWS
    const int error_code = WSAGetLastError();
    printf("[connect] error_code=%d\n", error_code);
#else
    printf("[connect] errno=%d\n", errno);
#endif
    fflush(stdout);
  }

  sh->client_sock = socket->sock;
  return sh;
}

AddressLookupStatus socket_address(Socket *socket, char *host_buf) {
  struct sockaddr_in sin;
  int len = sizeof(sin);

  if (getsockname(socket->sock, (struct sockaddr *)&sin, &len) == -1) {
    return ADDRESS_LOOKUP_STATUS_FAILED_GET_SOCKET_NAME;
  }
  if (inet_ntop(AF_INET, &sin.sin_addr, host_buf, INET_ADDRSTRLEN) == 0x0) {
    return ADDRESS_LOOKUP_STATUS_FAILED_INET_NTOP;
  }
  return ADDRESS_LOOKUP_STATUS_SUCCESS;
}

AddressLookupStatus socket_port(Socket *socket, int *port) {
  struct sockaddr_in sin;
  int len = sizeof(sin);

  if (getsockname(socket->sock, (struct sockaddr *)&sin, &len) == -1) {
    *port = -1;
    return ADDRESS_LOOKUP_STATUS_FAILED_GET_SOCKET_NAME;
  }

  *port = ntohs(sin.sin_port);
  return ADDRESS_LOOKUP_STATUS_SUCCESS;
}

void socket_close(Socket *socket) {
  socket->is_closed = true;
  _close_socket(socket->sock);
}

void socket_delete(Socket *socket) {
  if (!socket->is_closed) {
    socket_close(socket);
  }
  RELEASE(socket);
}

bool sockethandle_is_valid(const SocketHandle *sh) {
  return sh->client_sock != INVALID_SOCKET && sh->client_sock != -1;
}

int32_t sockethandle_send(SocketHandle *sh, const char *const msg,
                          int msg_len) {
  const int32_t result = send(sh->client_sock, msg, msg_len, 0);

  if (result == SOCKET_ERROR) {
#ifdef OS_WINDOWS
    const int error_code = WSAGetLastError();
    printf("[send] error_code=%d\n", error_code);
#else
    printf("[send] errno=%d\n", errno);
#endif
    fflush(stdout);
  }
  return result;
}

int32_t sockethandle_receive(SocketHandle *sh, char *buf, int buf_len) {
  const int32_t result = recv(sh->client_sock, buf, buf_len, 0);

  if (result == SOCKET_ERROR) {
#ifdef OS_WINDOWS
    const int error_code = WSAGetLastError();
    printf("[recv] error_code=%d\n", error_code);
#else
    printf("[recv] errno=%d\n", errno);
#endif
    fflush(stdout);
  }
  return result;
}

void sockethandle_close(SocketHandle *sh) {
  sh->is_closed = true;
  _close_socket(sh->client_sock);
}

void sockethandle_delete(SocketHandle *sh) {
  if (!sh->is_closed) {
    sockethandle_close(sh);
  }
  RELEASE(sh);
}

unsigned long socket_inet_address(const char *host, size_t host_len) {
  char *host_str = ALLOC_STRNDUP(host, host_len);
  unsigned long addr = inet_addr(host_str);
  RELEASE(host_str);
  return addr;
}

AddressLookupStatus local_ip_address(char *buf) {
  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in loopback;

  if (sock == -1) {
    return ADDRESS_LOOKUP_STATUS_FAILED_SOCKET;
  }

  memset(&loopback, 0, sizeof(loopback));
  loopback.sin_family = AF_INET;
  loopback.sin_addr.s_addr = 1337;  // can be any IP address
  loopback.sin_port = htons(9);     // using debug port

  if (connect(sock, (void *)(&loopback), sizeof(loopback)) == -1) {
    _close_socket(sock);
    return ADDRESS_LOOKUP_STATUS_FAILED_CONNECT;
  }

  int addrlen = sizeof(loopback);
  if (getsockname(sock, (void *)(&loopback), &addrlen) == -1) {
    _close_socket(sock);
    return ADDRESS_LOOKUP_STATUS_FAILED_GET_SOCKET_NAME;
  }

  _close_socket(sock);
  if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) {
    return ADDRESS_LOOKUP_STATUS_FAILED_INET_NTOP;
  }

  return ADDRESS_LOOKUP_STATUS_SUCCESS;
}