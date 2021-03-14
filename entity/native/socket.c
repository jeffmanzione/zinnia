// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#include "util/socket.h"

#include "alloc/arena/intern.h"
#include "entity/array/array.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
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

Entity _SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                 Entity *args);

void _Socket_init(Object *obj) {}
void _Socket_delete(Object *obj) {
  socket_delete((Socket *)obj->_internal_obj);
}
void _SocketHandle_init(Object *obj) {}
void _SocketHandle_delete(Object *obj) {
  sockethandle_delete((SocketHandle *)obj->_internal_obj);
}

Entity _Socket_constructor(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected tuple input.");
  }
  Tuple *tuple = (Tuple *)args->obj->_internal_obj;

  if (tuple_size(tuple) != 5) {
    return raise_error(task, ctx, "Expected tuple to have exactly 5 args.");
  }
  Socket *socket = socket_create(
      pint(&tuple_get(tuple, 0)->pri), pint(&tuple_get(tuple, 1)->pri),
      pint(&tuple_get(tuple, 2)->pri), pint(&tuple_get(tuple, 3)->pri));

  obj->_internal_obj = socket;

  if (!socket_is_valid(socket)) {
    return raise_error(task, ctx, "Invalid socket.");
  }
  if (SOCKET_ERROR == socket_bind(socket)) {
    return raise_error(task, ctx, "Could not bind to socket.");
  }
  if (SOCKET_ERROR == socket_listen(socket, pint(&tuple_get(tuple, 4)->pri))) {
    return raise_error(task, ctx, "Could not listen to socket.");
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
    return raise_error(task, ctx, "Weird Socket error.");
  }
  Object *socket_handle =
      heap_new(task->parent_process->heap, Class_SocketHandle);
  Entity arg = entity_object(obj);
  _SocketHandle_constructor(task, ctx, socket_handle, &arg);
  return entity_object(socket_handle);
}

Entity _SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                 Entity *args) {
  Socket *socket = (Socket *)args->obj->_internal_obj;
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
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
    return raise_error(task, ctx, "Weird Socket error.");
  }
  if (IS_CLASS(args, Class_String)) {
    String *msg = args->obj->_internal_obj;
    sockethandle_send(sh, msg->table, String_size(msg));
    return NONE_ENTITY;
  } else if (IS_CLASS(args, Class_Array)) {
    Array *arr = args->obj->_internal_obj;
    int i, arr_len = Array_size(arr);
    for (i = 0; i < arr_len; ++i) {
      String *msg = Array_get_ref(arr, i)->obj->_internal_obj;
      sockethandle_send(sh, msg->table, String_size(msg));
    }
    return NONE_ENTITY;
  } else {
    return raise_error(task, ctx, "Cannot send non-string.");
  }
}

Entity _SocketHandle_receive(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  SocketHandle *sh = (SocketHandle *)obj->_internal_obj;
  if (NULL == sh) {
    return raise_error(task, ctx, "Weird Socket error.");
  }

  char buf[BUFFER_SIZE];
  int chars_received = sockethandle_receive(sh, buf, BUFFER_SIZE);

  return entity_object(
      string_new(task->parent_process->heap, buf, chars_received));
}

Entity _init_sockets(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_init();
  return NONE_ENTITY;
}

Entity _cleanup_sockets(Task *task, Context *ctx, Object *obj, Entity *args) {
  sockets_cleanup();
  return NONE_ENTITY;
}

void socket_add_native(ModuleManager *mm, Module *socket) {
  native_function(socket, intern("__init"), _init_sockets);
  native_function(socket, intern("__cleanup"), _cleanup_sockets);
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
  native_background_method(Class_Socket, intern("accept"), _Socket_accept);
  native_method(Class_Socket, intern("close"), _Socket_close);
}