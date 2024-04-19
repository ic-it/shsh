#include "types.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void slice_assign(Slice *to, Slice from) {
  to->data = from.data;
  to->len = from.len;
}

int slice_cmp(Slice a, Slice b) {
  if (a.len != b.len) {
    return a.len - b.len;
  }
  for (size_t i = 0; i < a.len; i++) {
    if (a.data[i] != b.data[i]) {
      return a.data[i] - b.data[i];
    }
  }
  return 0;
}

char *slice_to_str(Slice s) {
  char *str = malloc(s.len + 2); // +1 angel's share, +1 null terminator
  if (str == NULL) {
    return NULL;
  }
  memmove(str, s.data, s.len);
  str[s.len] = '\0';
  return str;
}

Slice slice_from_str(char *s) { return (Slice){.data = s, .len = strlen(s)}; }

Slice slice_substr(Slice s, size_t start, size_t end) {
  if (start < 0)
    start = 0;
  if (end > s.len)
    end = s.len;
  if (start >= end)
    return (Slice){.data = NULL, .len = 0};
  return (Slice){.data = s.data + start, .len = end - start};
}

SliceVec slice_vec_new(void) {
  return (SliceVec){.data = NULL, .len = 0, .cap = 0};
}

void slice_vec_push(SliceVec *vec, Slice s) {
  if (vec->len == vec->cap) {
    int new_cap = vec->cap == 0 ? 1 : vec->cap * 2;
    Slice *new_data = realloc(vec->data, new_cap * sizeof(Slice));
    if (new_data == NULL)
      return;
    vec->data = new_data;
    vec->cap = new_cap;
  }
  vec->data[vec->len++] = s;
}

void slice_vec_free(SliceVec *vec) {
  free(vec->data);
  vec->data = NULL;
  vec->len = 0;
  vec->cap = 0;
}
