; Basic math functions.
module math

; A very small number that is greater than 0.
;
; This is useful for identifying numbers that are very small but aren't 0.
; An example where this is useful:
;
; ```
; for i=1, i > math.EPSILON, i=i/10 {
;   io.println(i)
; }
; ```
;
self.EPSILON = 0.000001

; Pi approximated.
;
; The ratio of the circumference of the circle to its diameter.
; Note: This is only a constant approximation of pi at double precision.
;
self.PI = 3.141592653589793

; Euler's number approximated.
;
; It approximates the value of (1 + 1/n)*n as n approaches infinity. It is the
; base of natural logarithm.
;
self.E = exp(1)

; Returns the comparatively smalles number from [sequence].
def min(sequence) {
	m = sequence[0]
  for (_, i) in sequence {
    if (i < m) {
      m = i
    }
  }
  m
}

; Returns the comparatively largest number from [sequence].
def max(sequence) {
	m = sequence[0]
  for (_, i) in sequence {
    if (i > m) {
      m = i
    }
  }
  m
}

; Returns the value of [num] raised to [power], i.e., num^power.
; def pow(num, power)

; Returns the absolute value of [x].
; def abs(x)

; Returns the ceiling of [x].
; def ceil(x)

; Returns the floor of [x].
; def floor(x)

; Returns the logarithm of the specified value [x] with base [base].
def log(x, base=math.E) {
  __log(x, base)
}

; Returns the natural logarithm of the specified valued [x].
def ln(x) {
  __log(x)
}

; Returns the base-10 logarithm of the specified value [x].
def log10(x) {
  log(10, x)
}

; Returns the square root of [x].
def sqrt(x) pow(x, 0.5)

; Returns the remainder of [x] divided by [y].
; def mod(x, y)