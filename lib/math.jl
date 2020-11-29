module math

self.EPSILON = 0.000001
self.PI = 3.141592653589793
self.E = 2.718281828459045

def min(args) {
	m = args[0]
  for i in args {
    if (i < m) {
      m = i
    }
  }
  m
}

def max(args) {
	m = args[0]
  for i in args {
    if (i > m) {
      m = i
    }
  }
  m
}

def pow(num, power) {
  __pow(num, power)
}

; if args.len < 2, defaults to natural log.
def log(args) {
  __log(args)
}

def ln(x) {
  __log(x)
}

def log10(x) {
  log(10, x)
}

def sqrt(x) pow(x, 0.5)