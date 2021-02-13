// socket.c
//
// Created on: Jan 27, 2019
//     Author: Jeff Manzione

#include "util/socket.h"

#include <stdlib.h>

#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/tuple/tuple.h"
#include "util/socket.h"
#include "vm/process/processes.h"

#define IS_TUPLE(entity)                                                       \
  ((NULL != entity) && (OBJECT == entity->type) &&                             \
   (Class_Tuple == entity->obj->_class))

#define BUFFER_SIZE 4096
#define SOCKET_ERROR (-1)

static Class *Class_SocketHandle;
static Class *Class_Socket;

void _Socket_init(Object *obj) {}
void _Socket_delete(Object *obj) {
  socket_delete((Socket *)obj->_internal_obj);
}
void _SocketHandle_init(Object *obj) {}
void _SocketHandle_delete(Object *obj) {
  sockethandle_delete((SocketHandle *)obj->_internal_obj);
}

Entity _SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                 Entity *args);

Entity _Socket_constructor(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  if (!IS_TUPLE(args)) {
    return raise_error(task, ctx, "Expected tuple input.");
  }
  Tuple *tuple = (Tuple *)args->obj->_internal_obj;

  //  DEBUGF("Input = (%I64d, %I64d, %I64d, %I64d %I64d)",
  //         tuple_get(tuple, 0).val.int_val, tuple_get(tuple, 1).val.int_val,
  //         tuple_get(tuple, 2).val.int_val, tuple_get(tuple, 3).val.int_val,
  //         tuple_get(tuple, 4).val.int_val);

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

Entity Socket_close(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)map_lookup(&data->state, strings_intern("socket"));
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  socket_close(socket);
  return NONE_ENTITY;
}

// To ease finding sockethandle class.
Entity Socket_accept(Task *task, Context *ctx, Object *obj, Entity *args) {
  Socket *socket = (Socket *)map_lookup(&data->state, strings_intern("socket"));
  if (NULL == socket) {
    return raise_error(task, ctx, "Weird Socket error.");
  }
  Entity socket_handle = create_external_obj(vm, sh_class);
  SocketHandle_constructor(vm, t, socket_handle.obj->external_data,
                           &data->object);

  return socket_handle;
}

Entity SocketHandle_constructor(Task *task, Context *ctx, Object *obj,
                                Entity *args) {
  Socket *socket = (Socket *)map_lookup(&arg->obj->external_data->state,
                                        strings_intern("socket"));
  if (NULL == socket) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  SocketHandle *sh = socket_accept(socket);
  map_insert(&data->state, strings_intern("handle"), sh);

  return data->object;
}

Entity SocketHandle_deconstructor(Task *task, Context *ctx, Object *obj,
                                  Entity *args) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  sockethandle_close(sh);
  sockethandle_delete(sh);
  return create_none();
}

Entity SocketHandle_close(Task *task, Context *ctx, Object *obj, Entity *args) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  sockethandle_close(sh);
  return create_none();
}

Entity SocketHandle_send(Task *task, Context *ctx, Object *obj, Entity *args) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }
  if (ISTYPE(*arg, class_string)) {
    String *msg = String_extract(*arg);
    sockethandle_send(sh, String_cstr(msg), String_size(msg));
    return create_none();
  } else if (ISTYPE(*arg, class_array)) {
    Array *arr = extract_array(*arg);
    int i, arr_len = Array_size(arr);
    for (i = 0; i < arr_len; ++i) {
      String *msg = String_extract(Array_get(arr, i));
      sockethandle_send(sh, String_cstr(msg), String_size(msg));
    }
    return create_none();
  } else {
    return throw_error(vm, t, "Cannot send non-string.");
  }
}

Entity SocketHandle_receive(Task *task, Context *ctx, Object *obj,
                            Entity *args) {
  SocketHandle *sh =
      (SocketHandle *)map_lookup(&data->state, strings_intern("handle"));
  if (NULL == sh) {
    return throw_error(vm, t, "Weird Socket error.");
  }

  char buf[BUFFER_SIZE];
  int chars_received = sockethandle_receive(sh, buf, BUFFER_SIZE);

  return string_create_len(vm, buf, chars_received);
}

void socket_add_native(Module *process) {
  Class_SocketHandle = native_class(process, strings_intern("SocketHandle"),
                                    _SocketHandle_init, _SocketHandle_delete);
  native_method(Class_SocketHandle, strings_intern("send"), SocketHandle_send);
  native_method(Class_SocketHandle, strings_intern("receive"),
                SocketHandle_receive);
  native_method(Class_SocketHandle, strings_intern("close"),
                SocketHandle_close);

  Class_Socket = create_external_class(process, strings_intern("Socket"),
                                       _Socket_init, _Socket_delete);
  add_external_method(Class_Socket, strings_intern("accept"), Socket_accept);
  add_external_method(Class_Socket, strings_intern("close"), Socket_close);
}