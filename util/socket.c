// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff

#include "socket.h"

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

Socket *socket_create(int domain, int type, int protocol, unsigned long host,
                      uint16_t port) {
  Socket *sock = ALLOC2(Socket);
  sock->sock = socket(domain, type, protocol);
  sock->in.sin_family = domain;
  sock->in.sin_addr.s_addr = host;
  sock->in.sin_port = htons(port);
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
  SocketHandle *sh = ALLOC2(SocketHandle);
  sh->is_closed = false;
  int addr_len = sizeof(sh->client);
  sh->client_sock = accept(socket->sock, (struct sockaddr *)&sh->client,
#ifdef OS_LINUX
                           (socklen_t *)
#endif
                           &addr_len);
  return sh;
}

SocketHandle *socket_connect(Socket *socket) {
  SocketHandle *sh = ALLOC2(SocketHandle);
  sh->is_closed = false;
  connect(socket->sock, (struct sockaddr *)&socket->in, sizeof(socket->in));
  sh->client_sock = socket->sock;
  return sh;
}

void socket_close(Socket *socket) {
  socket->is_closed = true;
#ifdef OS_WINDOWS
  closesocket(socket->sock);
#else
  close(socket->sock);
#endif
}

void socket_delete(Socket *socket) {
  if (!socket->is_closed) {
    socket_close(socket);
  }
  DEALLOC(socket);
}

bool sockethandle_is_valid(const SocketHandle *sh) {
  return sh->client_sock != INVALID_SOCKET && sh->client_sock != -1;
}

int32_t sockethandle_send(SocketHandle *sh, const char *const msg,
                          int msg_len) {
  return send(sh->client_sock, msg, msg_len, 0);
}

int32_t sockethandle_receive(SocketHandle *sh, char *buf, int buf_len) {
  return recv(sh->client_sock, buf, buf_len, 0);
}

// SOCKET sockethandle_get_socket(SocketHandle *sh) { return sh->client_sock; }

void sockethandle_close(SocketHandle *sh) {
  sh->is_closed = true;
#ifdef OS_WINDOWS
  closesocket(sh->client_sock);
#else
  close(sh->client_sock);
#endif
}

void sockethandle_delete(SocketHandle *sh) {
  if (!sh->is_closed) {
    sockethandle_close(sh);
  }
  DEALLOC(sh);
}

unsigned long socket_inet_address(const char *host, size_t host_len) {
  char *host_str = ALLOC_STRNDUP(host, host_len);
  unsigned long addr = inet_addr(host_str);
  DEALLOC(host_str);
  return addr;
}