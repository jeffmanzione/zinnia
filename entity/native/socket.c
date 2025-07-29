// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#include "util/socket.h"

#include "alloc/arena/intern.h"
#include "entity/array/array.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/native/native_helpers.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "util/socket.h"
#include "vm/process/processes.h"

#define BUFFER_SIZE 4096
#define SOCKET_ERROR (-1)

static Class *Class_SocketHandle;
static Class *Class_Socket;

static char *LOCAL_ADDRESS_ERROR_MESSAGES[] = {
    "OK",
    "socket() call failed",
    "connect() call failed",
    "getsocketname() call failed",
    "inet_ntop() call failed",
};

Entity _SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                 Entity *args);

void _Socket_init(Object *obj) {}
void _Socket_delete(Object *obj) {
  socket_delete((Socket *)obj->_internal_obj);
}
void _SocketHandle_init(Object *obj) {}
void _SocketHandle_delete(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  sockethandle_delete((SocketHandle *)obj->_internal_obj);
}

Entity _Socket_constructor(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  EXTRACT_TUPLE_ARGS(tuple, args, 6, task, ctx);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t domain, tuple, 0);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t type, tuple, 1);
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t protocol, tuple, 2);
  EXCTRACT_STRING_OR_THROW(host, host_len, tuple_get(tuple, 3));
  EXTRACT_INT_AT_INDEX_OR_THROW(const int64_t port, tuple, 4);
  EXTRACT_BOOL_AT_INDEX_OR_THROW(const bool is_server, tuple, 5);

  const uint64_t address = socket_inet_address(host, host_len);

  Socket *socket = socket_create(domain, type, protocol, address, port);

  obj->_internal_obj = socket;
  if (is_server) {
    if (!socket_is_valid(socket)) {
      return raise_error(task, ctx, "Invalid socket.");
    }
    if (SOCKET_ERROR == socket_bind(socket)) {
      return raise_error(task, ctx, "Could not bind to socket.");
    }
    if (SOCKET_ERROR == socket_listen(socket, port)) {
      return raise_error(task, ctx, "Could not listen to socket.");
    }
  }
  return entity_object(obj);
}

Entity _Socket_close(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  socket_close(socket);
  return NONE_ENTITY;
}

// To ease finding sockethandle class.
Entity _Socket_accept(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  Object *socket_handle =
      native_background_new(task->parent_process, Class_SocketHandle);
  Entity arg = entity_object(obj);
  _SocketHandle_constructor(task, ctx, socket_handle, &arg);
  return entity_object(socket_handle);
}

Entity _Socket_host(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }

  char host_buf[INET_ADDRSTRLEN];
  const AddressLookupStatus status = socket_address(socket, host_buf);

  if (status != ADDRESS_LOOKUP_STATUS_SUCCESS) {
    return raise_error(task, ctx, "Failed to look up local address. Error: %s",
                       LOCAL_ADDRESS_ERROR_MESSAGES[status]);
  }

  return entity_object(string_new(task->parent_process->heap, host_buf,
                                  strnlen(host_buf, INET_ADDRSTRLEN)));
}

Entity _Socket_port(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }

  int port;
  const AddressLookupStatus status = socket_port(socket, &port);

  if (status != ADDRESS_LOOKUP_STATUS_SUCCESS) {
    return raise_error(task, ctx, "Failed to look up local address. Error: %s",
                       LOCAL_ADDRESS_ERROR_MESSAGES[status]);
  }

  return entity_int(port);
}

Entity _SocketHandle_connect_constructor(Task *task, Context *ctx, Object *obj,
                                         Entity *args) {
  Socket *socket = (Socket *)args->obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  SocketHandle *sh = socket_connect(socket);
  obj->_internal_obj = sh;
  return entity_object(obj);
}

Entity _Socket_connect(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  Object *socket_handle =
      native_background_new(task->parent_process, Class_SocketHandle);
  Entity arg = entity_object(obj);
  _SocketHandle_connect_constructor(task, ctx, socket_handle, &arg);
  return entity_object(socket_handle);
}

Entity _SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                 Entity *args) {
  Socket *socket = (Socket *)args->obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  SocketHandle *sh = socket_accept(socket);
  obj->_internal_obj = sh;
  return entity_object(obj);
}

Entity _SocketHandle_close(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  sockethandle_close(sh);
  return NONE_ENTITY;
}

Entity _SocketHandle_send(Task *task, Context *ctx, Object *obj, Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }

  char *msg;
  int msg_len;
  const is_string = extract_string(args, &msg, &msg_len);

  if (is_string) {
    sockethandle_send(sh, msg, msg_len);
    return NONE_ENTITY;
  } else if (IS_CLASS(args, Class_Array)) {
    Array *arr = args->obj->_internal_obj;
    int i, arr_len = Array_size(arr);
    for (i = 0; i < arr_len; ++i) {
      char *msg;
      int msg_len;
      const bool is_string =
          extract_string(Array_get_ref(arr, i), &msg, &msg_len);
      if (!is_string) {
        return native_background_raise_error(
            task, ctx, "Cannot send non-string at index %d.", i);
      }
      sockethandle_send(sh, msg, msg_len);
    }
    return NONE_ENTITY;
  } else {
    return native_background_raise_error(task, ctx, "Cannot send non-string.");
  }
}

Entity _SocketHandle_receive(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }

  char buf[BUFFER_SIZE];
  const int32_t chars_received = sockethandle_receive(sh, buf, BUFFER_SIZE);
  if (chars_received == 0) {
    return NONE_ENTITY;
  }
  if (chars_received < 0) {
    return native_background_raise_error(
        task, ctx, "Invalid connection. recv() returned %d", chars_received);
  }
  return entity_object(
      native_background_string_new(task->parent_process, buf, chars_received));
}

Entity _init_sockets(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_init();
  return NONE_ENTITY;
}

Entity _cleanup_sockets(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_cleanup();
  return NONE_ENTITY;
}

Entity _lookup_local_address(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  char buf[INET_ADDRSTRLEN];
  const AddressLookupStatus status = local_ip_address(buf);

  if (status != ADDRESS_LOOKUP_STATUS_SUCCESS) {
    return raise_error(task, ctx, "Failed to look up local address. Error: %s",
                       LOCAL_ADDRESS_ERROR_MESSAGES[status]);
  }

  return entity_object(string_new(task->parent_process->heap, buf,
                                  strnlen(buf, INET_ADDRSTRLEN)));
}

void socket_add_native(ModuleManager *mm, Module *socket) {
  native_function(socket, intern("__init"), _init_sockets);
  native_function(socket, intern("__cleanup"), _cleanup_sockets);
  native_function(socket, intern("__lookup_local_address"),
                  _lookup_local_address);
  Class_SocketHandle = native_class(socket, intern("SocketHandle"),
                                    _SocketHandle_init, _SocketHandle_delete);
  native_method(Class_SocketHandle, intern("new"), _SocketHandle_constructor);
  native_background_method(Class_SocketHandle, intern("send"),
                           _SocketHandle_send);
  native_background_method(Class_SocketHandle, intern("receive"),
                           _SocketHandle_receive);
  native_method(Class_SocketHandle, intern("close"), _SocketHandle_close);

  Class_Socket =
      native_class(socket, intern("Socket"), _Socket_init, _Socket_delete);
  native_method(Class_Socket, intern("new"), _Socket_constructor);
  native_method(Class_Socket, intern("host"), _Socket_host);
  native_method(Class_Socket, intern("port"), _Socket_port);
  native_background_method(Class_Socket, intern("accept"), _Socket_accept);
  native_background_method(Class_Socket, intern("connect"), _Socket_connect);
  native_method(Class_Socket, intern("close"), _Socket_close);
}