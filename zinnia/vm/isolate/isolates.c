#include "zinnia/vm/isolate/isolates.h"

IMPL_ARRAYLIKE(EntityStack, Entity);
IMPL_SETLIKE(FrameSet, Frame *);
IMPL_ARRAYLIKE(FrameArray, Frame *);

uint32_t hash_frame(const Frame *tsk, uint32_t size) {
  return (uint32_t)(intptr_t)tsk;
}

int32_t compare_frames(const Frame *tsk1, uint32_t size1, const Frame *tsk2,
                       uint32_t size2) {
  return (intptr_t)tsk1 - (intptr_t)tsk2;
}