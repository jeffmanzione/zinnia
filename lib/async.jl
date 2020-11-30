module async

import io

; Allows for the manual completion of a future.
;
; Example:
; ```
; do_something = (c) async { c.complete('Hello') }
; completer = async.Completer()
; do_something(completer)
; io.println(await completer.as_future())
; ```
;
class Completer {
  field completed, _value, _future

  new() {
    completed = False
    _value = None
    _future = () async {
      while ~(() -> completed)() {}
      return _value
    } ()
  }

  ; Completes the future with the value [v].
  method complete(v) {
    _value = v
    completed = True
  }

  ; Returns a future to the completed value.
  method as_future() {
    return _future
  }
}