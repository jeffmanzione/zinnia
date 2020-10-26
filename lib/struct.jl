module struct

import builtin

class Map {
  field table, keys
  new(field sz=31) {
    table = []
    table[sz] = None
    keys = []
  }
  method hash__(k) {
    hval = builtin.hash(k)
    pos = hval % sz
    if pos < 0 pos = -pos
    return pos
  }
  method []=(k, v) {
    pos = hash__(k)
    entries = table[pos]
    if ~entries {
      table[pos] = [(k,v)]
      keys.append(k)
      return None
    }
    for i=0, i<entries.len(), i=i+1 {
      if k == entries[i][0] {
        old_v = entries[i][1]
        entries[i] = (k,v)
        return old_v
      }
    }
    entries.append((k,v))
    keys.append(k)
    return None
  }
  method [](k) {
    pos = hash__(k)
    entries = table[pos]
    if ~entries {
      return None
    }
    for i=0, i<entries.len(), i=i+1 {
      if k == entries[i][0] {
        return entries[i][1]
      }
    }
    return None
  }
  method iter() {
    return KVIterator(keys.iter(), self)
  }
  method each(f) {
    for (k, v) in self {
      f(k, v)
    }
  }
  method to_s() {
    kvs = []
    each((k, v) -> kvs.append(cat(k, ': ', v)))
    return cat('{', ', '.join(kvs), '}')
  }
}