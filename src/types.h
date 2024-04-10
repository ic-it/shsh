#pragma once

#define m_print_slice(prfx, s)                                                 \
  printf("%s: '%.*s'\n", prfx, s.len, s.data + s.pos)
#define m_print_slicev(prfx, vec)                                              \
  for (int i = 0; i < vec.len; i++) {                                          \
    m_print_slice(prfx, vec.data[i]);                                          \
  }

/// @brief Slice of a string
/// @details A slice is a pointer to a string with a length and a position
typedef struct {
  char *data;
  int pos;
  int len;
} Slice;

/// @brief Slice assignment
void slice_assign(Slice *to, Slice from);
/// @brief Compare two slices
int slice_cmp(Slice a, Slice b);
/// @brief Convert a slice to a string
/// @details This function allocates memory for the string
char *slice_to_str(Slice s);
/// @brief Create a slice from a string
Slice slice_from_str(char *s);
/// @brief Create a slice from a substring
Slice slice_substr(Slice s, int start, int end);

/// @brief Slice Vector
/// @details A vector of slices
typedef struct {
  Slice *data;
  int len;
  int cap;
} SliceVec;

/// @brief Create a new slice vector
SliceVec slice_vec_new(void);
/// @brief Append a slice to a slice vector
void slice_vec_push(SliceVec *vec, Slice s);
/// @brief Free a slice vector
void slice_vec_free(SliceVec *vec);
