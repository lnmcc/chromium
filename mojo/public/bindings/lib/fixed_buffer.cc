// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/bindings/lib/fixed_buffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "mojo/public/bindings/lib/bindings_serialization.h"

namespace mojo {
namespace internal {

FixedBuffer::FixedBuffer(size_t size)
    : ptr_(NULL),
      cursor_(0),
      size_(internal::Align(size)) {
  ptr_ = static_cast<char*>(calloc(size_, 1));
}

FixedBuffer::~FixedBuffer() {
  free(ptr_);
}

void* FixedBuffer::Allocate(size_t delta, Destructor dtor) {
  assert(!dtor);

  delta = internal::Align(delta);

  // TODO(darin): Using <assert.h> is probably not going to cut it.
  assert(delta > 0);
  assert(cursor_ + delta <= size_);
  if (cursor_ + delta > size_)
    return NULL;

  char* result = ptr_ + cursor_;
  cursor_ += delta;

  return result;
}

void* FixedBuffer::Leak() {
  char* ptr = ptr_;
  ptr_ = NULL;
  cursor_ = 0;
  size_ = 0;
  return ptr;
}

}  // namespace internal
}  // namespace mojo
