// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#include "zinnia/util/socket.h"

#include "language-tools/intern.h"
#include "zinnia/entity/array/array.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/util/socket.h"
#include "zinnia/vm/process/processes.h"


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

Entity SocketHandle_constructor_(Task *task, Context *ctx, Object *obj,
                                 Entity *args);

void Socket_init_(Object *obj) {}
void Socket_delete_(Object *obj) {
  socket_delete((Socket *)obj->_internal_obj);
}
void SocketHandle_init_(Object *obj) {}
void SocketHandle_delete_(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  sockethandle_delete((SocketHandle *)obj->_internal_obj);
}

Entity Socket_constructor_(Task *task, Context *ctx, Object *obj,
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

Entity Socket_close_(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  socket_close(socket);
  return NONE_ENTITY;
}

// To ease finding sockethandle class.
Entity Socket_accept_(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  Object *socket_handle =
      native_background_new(task->parent_process, Class_SocketHandle);
  Entity arg = entity_object(obj);
  SocketHandle_constructor_(task, ctx, socket_handle, &arg);
  return entity_object(socket_handle);
}

Entity Socket_host_(Task *task, Context *ctx, Object *obj, Entity *args) {
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

Entity Socket_port_(Task *task, Context *ctx, Object *obj, Entity *args) {
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

Entity SocketHandle_connect_constructor_(Task *task, Context *ctx, Object *obj,
                                         Entity *args) {
  Socket *socket = (Socket *)args->obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  SocketHandle *sh = socket_connect(socket);
  obj->_internal_obj = sh;
  return entity_object(obj);
}

Entity Socket_connect_(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  Object *socket_handle =
      native_background_new(task->parent_process, Class_SocketHandle);
  Entity arg = entity_object(obj);
  SocketHandle_connect_constructor_(task, ctx, socket_handle, &arg);
  return entity_object(socket_handle);
}

Entity SocketHandle_constructor_(Task *task, Context *ctx, Object *obj,
                                 Entity *args) {
  Socket *socket = (Socket *)args->obj->_internal_obj;
  if (NULL == socket) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }
  SocketHandle *sh = socket_accept(socket);
  obj->_internal_obj = sh;
  return entity_object(obj);
}

Entity SocketHandle_close_(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  sockethandle_close(sh);
  return NONE_ENTITY;
}

Entity SocketHandle_send_(Task *task, Context *ctx, Object *obj, Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return native_background_raise_error(task, ctx, "Weird Socket error.");
  }

  char *msg;
  int msg_len;
  const bool is_string = extract_string(args, &msg, &msg_len);

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
          extract_string(Array_get_ref_unchecked(arr, i), &msg, &msg_len);
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

Entity SocketHandle_receive_(Task *task, Context *ctx, Object *obj,
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

Entity init_sockets_(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_init();
  return NONE_ENTITY;
}

Entity cleanup_sockets_(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_cleanup();
  return NONE_ENTITY;
}

Entity lookup_local_address_(Task *task, Context *ctx, Object *obj,
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
  native_function(socket, global_intern("__init"), init_sockets_);
  native_function(socket, global_intern("__cleanup"), cleanup_sockets_);
  native_function(socket, global_intern("__lookup_local_address"),
                  lookup_local_address_);
  Class_SocketHandle = native_class(socket, global_intern("SocketHandle"),
                                    SocketHandle_init_, SocketHandle_delete_);
  native_method(Class_SocketHandle, global_intern("new"),
                SocketHandle_constructor_);
  native_background_method(Class_SocketHandle, global_intern("send"),
                           SocketHandle_send_);
  native_background_method(Class_SocketHandle, global_intern("receive"),
                           SocketHandle_receive_);
  native_method(Class_SocketHandle, global_intern("close"),
                SocketHandle_close_);

  Class_Socket = native_class(socket, global_intern("Socket"), Socket_init_,
                              Socket_delete_);
  native_method(Class_Socket, global_intern("new"), Socket_constructor_);
  native_method(Class_Socket, global_intern("host"), Socket_host_);
  native_method(Class_Socket, global_intern("port"), Socket_port_);
  native_background_method(Class_Socket, global_intern("accept"),
                           Socket_accept_);
  native_background_method(Class_Socket, global_intern("connect"),
                           Socket_connect_);
  native_method(Class_Socket, global_intern("close"), Socket_close_);
}