/*
 * M*LIB - Dynamic Size String Module
 *
 * Copyright (c) 2017-2021, Patrick Pelissier
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MSTARLIB_STRING_H
#define MSTARLIB_STRING_H

#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/********************************** INTERNAL ************************************/

// This macro defines the contract of a string.
// Note: A ==> B is represented as not(A) or B
// Note: use of strlen can slow down a lot the program in some cases.
#define M_STR1NG_CONTRACT(v) do {                                             \
    M_ASSERT (v != NULL);                                                     \
    M_ASSERT_SLOW (string_size(v) == strlen(string_get_cstr(v)));             \
    M_ASSERT (string_get_cstr(v)[string_size(v)] == 0);                       \
    M_ASSERT (string_size(v) < string_capacity(v));                           \
    M_ASSERT (string_capacity(v) < sizeof (string_heap_ct) || !m_str1ng_stack_p(v)); \
  } while(0)

// string if it is heap allocated
typedef struct string_heap_s {
  size_t size;
  size_t alloc;
} string_heap_ct;
// string if it is stack allocated
typedef struct string_stack_s {
  char buffer[sizeof (string_heap_ct)];
} string_stack_ct;
// both cases of string are possible
typedef union string_union_u {
  string_heap_ct heap;
  string_stack_ct stack;
} string_union_ct;

/****************************** EXTERNAL *******************************/

/***********************************************************************/
/*                                                                     */
/*                           DYNAMIC     STRING                        */
/*                                                                     */
/***********************************************************************/

/* Index returned in case of error instead of the position within the string */
#define STRING_FAILURE ((size_t)-1)

/* This is the main structure of this module. */

// Dynamic string
typedef struct string_s {
  string_union_ct u;
  char *ptr;
} string_t[1];

// Pointer to a Dynamic string
typedef struct string_s *string_ptr;

// Constant pointer to a Dynamic string
typedef const struct string_s *string_srcptr;

/* Input option for string_fgets 
   STRING_READ_LINE  (read line), 
   STRING_READ_PURE_LINE (read line and remove CR and LF)
   STRING_READ_FILE (read all file)
*/
typedef enum string_fgets_e {
  STRING_READ_LINE = 0, STRING_READ_PURE_LINE = 1, STRING_READ_FILE = 2
} string_fgets_t;

/* Internal method to test if the string is stack based or heap based
   We test if the ptr field points to the heap allocated buffer or not.
   This is no particularly efficient from a memory point of view
   (as we may reuse the pointer field to store some data)
   but it should be enough for most of "small" strings as most
   of the strings are less than 14 characters.
   Moreover it generates quite efficient code.
 */
static inline bool
m_str1ng_stack_p(const string_t s)
{
  // Function can be called when contract is not fullfilled
  return (s->ptr == NULL);
}

/* Internal method to set the size of the string */
static inline void
m_str1ng_set_size(string_t s, size_t size)
{
  // Function can be called when contract is not fullfilled
  if (m_str1ng_stack_p(s)) {
    M_ASSERT (size < sizeof (string_heap_ct) - 1);
    // The size of the string is stored as the last char of the buffer.
    s->u.stack.buffer[sizeof (string_heap_ct) - 1] = (char) size;
  } else
    s->u.heap.size = size;
}

/* Return the number of characters of the string */
static inline size_t
string_size(const string_t s)
{
  // Function can be called when contract is not fullfilled
  // Reading both values before calling the '?' operator allows compiler to generate branchless code
  const size_t s_stack = (size_t) s->u.stack.buffer[sizeof (string_heap_ct) - 1];
  const size_t s_heap  = s->u.heap.size;
  return m_str1ng_stack_p(s) ?  s_stack : s_heap;
}

/* Return the capacity of the string */
static inline size_t
string_capacity(const string_t s)
{
  // Function can be called when contract is not fullfilled
  // Reading both values before calling the '?' operator allows compiler to generate branchless code
  const size_t c_stack = sizeof (string_heap_ct) - 1;
  const size_t c_heap  = s->u.heap.alloc;
  return m_str1ng_stack_p(s) ?  c_stack : c_heap;
}

/* Return a writable pointer to the array of char of the string */
static inline char*
m_str1ng_get_cstr(string_t v)
{
  // Function can be called when contract is not fullfilled
  char *const ptr_stack = &v->u.stack.buffer[0];
  char *const ptr_heap  = v->ptr;
  return m_str1ng_stack_p(v) ?  ptr_stack : ptr_heap;
}

/* Return the string view a classic C string (const char *) */
static inline const char*
string_get_cstr(const string_t v)
{
  // Function cannot be called when contract is not fullfilled
  // but it is called by contract (so no contract check to avoid infinite recursion).
  const char *const ptr_stack = &v->u.stack.buffer[0];
  const char *const ptr_heap  = v->ptr;
  return m_str1ng_stack_p(v) ?  ptr_stack : ptr_heap;
}

/* Initialize the dynamic string (constructor) 
  and make it empty */
static inline void
string_init(string_t s)
{
  s->ptr = NULL;
  s->u.stack.buffer[0] = 0;
  m_str1ng_set_size(s, 0);
  M_STR1NG_CONTRACT(s);
}

/* Clear the Dynamic string (destructor) */
static inline void
string_clear(string_t v)
{
  M_STR1NG_CONTRACT(v);
  if (!m_str1ng_stack_p(v)) {    
    M_MEMORY_FREE(v->ptr);
    v->ptr   = NULL;
  }
  /* This is not needed but is safer to make
     the string invalid so that it can be detected. */
  v->u.stack.buffer[sizeof (string_heap_ct) - 1] = CHAR_MAX;
}

/* NOTE: Internaly used by STRING_DECL_INIT */
static inline void m_str1ng_clear2(string_t *v) { string_clear(*v); }
static inline string_ptr m_str1ng_init_ref(string_t v) { string_init(v); return v; }

/* Clear the Dynamic string (destructor)
  and return a heap pointer to the string.
  The ownership of the data is transfered back to the caller
  and the returned pointer has to be released by M_MEMORY_FREE. */
static inline char *
string_clear_get_str(string_t v)
{
  M_STR1NG_CONTRACT(v);
  char *p = v->ptr;
  if (m_str1ng_stack_p(v)) {
    // The string was stack allocated.
    p = v->u.stack.buffer;
    // Need to allocate a heap string to return the copy.
    size_t alloc = string_size(v)+1;
    char *ptr = M_MEMORY_REALLOC (char, NULL, alloc);
    if (M_UNLIKELY (ptr == NULL)) {
      M_MEMORY_FULL(sizeof (char) * alloc);
      return NULL;
    }
    M_ASSERT(ptr != NULL && p != NULL);
    memcpy(ptr, p, alloc);
    p = ptr;
  }
  v->ptr = NULL;
  v->u.stack.buffer[sizeof (string_heap_ct) - 1] = CHAR_MAX;
  return p;
}

/* Make the string empty */
static inline void
string_reset(string_t v)
{
  M_STR1NG_CONTRACT (v);
  m_str1ng_set_size(v, 0);
  m_str1ng_get_cstr(v)[0] = 0;
  M_STR1NG_CONTRACT (v);
}

/* Return the selected character of the string */
static inline char
string_get_char(const string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX(index, string_size(v));
  return string_get_cstr(v)[index];
}

/* Set the selected character of the string */
static inline void
string_set_char(string_t v, size_t index, const char c)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX(index, string_size(v));
  m_str1ng_get_cstr(v)[index] = c;
}

/* Test if the string is empty or not */
static inline bool
string_empty_p(const string_t v)
{
  M_STR1NG_CONTRACT (v);
  return string_size(v) == 0;
}

/* Internal method to fit the string to the given size
   Ensures that the string capacity is greater than size_alloc
   (size_alloc shall include the final null char).
   It may move the string from stack based to heap based.
   Return a pointer to the writable string.
*/
static inline char *
m_str1ng_fit2size (string_t v, size_t size_alloc)
{
  M_ASSERT_INDEX (0, size_alloc);
  // Note: this function may be called in context where the contract
  // is not fullfilled.
  const size_t old_alloc = string_capacity(v);
  // This line enables the compiler to completly remove this function
  // for very short constant strings.
  M_ASSUME(old_alloc >= sizeof (string_heap_ct) - 1);
  if (M_UNLIKELY (size_alloc > old_alloc)) {
    size_t alloc = size_alloc + size_alloc / 2;
    if (M_UNLIKELY (alloc <= old_alloc)) {
      /* Overflow in alloc computation */
      M_MEMORY_FULL(sizeof (char) * alloc);
      // NOTE: Return is broken...
      abort();
      return NULL;
    }
    char *ptr = M_MEMORY_REALLOC (char, v->ptr, alloc);
    if (M_UNLIKELY (ptr == NULL)) {
      M_MEMORY_FULL(sizeof (char) * alloc);
      // NOTE: Return is currently broken.
      abort();
      return NULL;
    }
    M_ASSERT(ptr != &v->u.stack.buffer[0]);
    if (m_str1ng_stack_p(v)) {
      /* Copy the stack allocation into the new heap allocation */
      const size_t size = (size_t) v->u.stack.buffer[sizeof (string_heap_ct) - 1] + 1U;
      M_ASSERT( size <= alloc);
      M_ASSERT( size <= sizeof (v->u.stack.buffer)-1);
      memcpy(ptr, &v->u.stack.buffer[0], size);
    }
    v->ptr = ptr;
    v->u.heap.alloc = alloc;
    return ptr;
  }
  return m_str1ng_get_cstr(v);
}

/* Modify the string capacity to be able to handle at least 'alloc'
   characters (including final nul char) */
static inline void
string_reserve(string_t v, size_t alloc)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = string_size(v);
  /* NOTE: Reserve below needed size, perform a shrink to fit */
  if (size + 1 > alloc) {
    alloc = size+1;
  }
  M_ASSERT (alloc > 0);
  if (alloc < sizeof (string_heap_ct)) {
    // Allocation can fit in the stack space
    if (!m_str1ng_stack_p(v)) {
      /* Transform Heap Allocate to Stack Allocate */
      char *ptr = &v->u.stack.buffer[0];
      memcpy(ptr, v->ptr, size+1);
      M_MEMORY_FREE(v->ptr);
      v->ptr = NULL;
      m_str1ng_set_size(v, size);
    } else {
      /* Already a stack based alloc: nothing to do */
    }
  } else {
    // Allocation cannot fit in the stack space
    // Need to allocate in heap space
    char *ptr = M_MEMORY_REALLOC (char, v->ptr, alloc);
    if (M_UNLIKELY (ptr == NULL) ) {
      M_MEMORY_FULL(sizeof (char) * alloc);
      return;
    }
    if (m_str1ng_stack_p(v)) {
      // Copy from stack space to heap space the string
      char *ptr_stack = &v->u.stack.buffer[0];
      memcpy(ptr, ptr_stack, size+1);
      v->u.heap.size = size;
    }
    v->ptr = ptr;
    v->u.heap.alloc = alloc;
  }
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the C string str */
static inline void
string_set_str(string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str != NULL);
  size_t size = strlen(str);
  char *ptr = m_str1ng_fit2size(v, size+1);
  memcpy(ptr, str, size+1);
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the n first characters of the C string str */
static inline void
string_set_strn(string_t v, const char str[], size_t n)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str != NULL);
  size_t len  = strlen(str);
  size_t size = M_MIN (len, n);
  char *ptr = m_str1ng_fit2size(v, size+1);
  memcpy(ptr, str, size);
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the other one */
static inline void
string_set (string_t v1, const string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  if (M_LIKELY (v1 != v2)) {
    const size_t size = string_size(v2);
    char *ptr = m_str1ng_fit2size(v1, size+1);
    memcpy(ptr, string_get_cstr(v2), size+1);
    m_str1ng_set_size(v1, size);
  }
  M_STR1NG_CONTRACT (v1);
}

/* Set the string to the n first characters of other one */
static inline void
string_set_n(string_t v, const string_t ref, size_t offset, size_t length)
{
  M_STR1NG_CONTRACT (v);
  M_STR1NG_CONTRACT (ref);
  M_ASSERT_INDEX (offset, string_size(ref) + 1);
  size_t size = M_MIN (string_size(ref) - offset, length);
  char *ptr = m_str1ng_fit2size(v, size+1);
  memmove(ptr, string_get_cstr(ref) + offset, size);
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Initialize the string and set it to the other one 
   (constructor) */
static inline void
string_init_set(string_t v1, const string_t v2)
{
  string_init(v1);
  string_set(v1,v2);
}

/* Initialize the string and set it to the C string
   (constructor) */
static inline void
string_init_set_str(string_t v1, const char str[])
{
  string_init(v1);
  string_set_str(v1, str);
}

/* Initialize the string, set it to the other one,
   and destroy the other one.
   (constructor & destructor) */
static inline void
string_init_move(string_t v1, string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  memcpy(v1, v2, sizeof (string_t));
  // Note: nullify v2 to be safer
  v2->ptr   = NULL;
  M_STR1NG_CONTRACT (v1);
}

/* Swap the two strings v1 and v2 */
static inline void
string_swap(string_t v1, string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  M_SWAP (size_t, v1->u.heap.size,  v2->u.heap.size);
  M_SWAP (size_t, v1->u.heap.alloc, v2->u.heap.alloc);
  M_SWAP (char *, v1->ptr,   v2->ptr);
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
}

/* Set the string to the other one,
   and destroy the other one.
   (destructor) */
static inline void
string_move(string_t v1, string_t v2)
{
  string_clear(v1);
  string_init_move(v1,v2);
}

/* Push the character 'c' in the string 'v' */
static inline void
string_push_back (string_t v, char c)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = string_size(v);
  char *ptr = m_str1ng_fit2size(v, size+2);
  ptr[size+0] = c;
  ptr[size+1] = 0;
  m_str1ng_set_size(v, size+1);
  M_STR1NG_CONTRACT (v);
}

/* Concatene the string with the C string */
static inline void
string_cat_str(string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const size_t old_size = string_size(v);
  const size_t size = strlen(str);
  char *ptr = m_str1ng_fit2size(v, old_size + size + 1);
  memcpy(&ptr[old_size], str, size + 1);
  m_str1ng_set_size(v, old_size + size);
  M_STR1NG_CONTRACT (v);
}

/* Concatene the string with the other string */
static inline void
string_cat(string_t v, const string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  M_STR1NG_CONTRACT (v);
  const size_t size = string_size(v2);
  if (M_LIKELY (size > 0)) {
    const size_t old_size = string_size(v);
    char *ptr = m_str1ng_fit2size(v, old_size + size + 1);
    memcpy(&ptr[old_size], string_get_cstr(v2), size);
    ptr[old_size + size] = 0;
    m_str1ng_set_size(v, old_size + size);
  }
  M_STR1NG_CONTRACT (v);
}

/* Compare the string to the C string and
  return the sort order (negative if less, 0 if equal, positive if greater) */
static inline int
string_cmp_str(const string_t v1, const char str[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (str != NULL);
  return strcmp(string_get_cstr(v1), str);
}

/* Compare the string to the other string and
  return the sort order (negative if less, 0 if equal, positive if greater) */
static inline int
string_cmp(const string_t v1, const string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  return strcmp(string_get_cstr(v1), string_get_cstr(v2));
}

/* Test if the string is equal to the given C string */
static inline bool
string_equal_str_p(const string_t v1, const char str[])
{
  M_STR1NG_CONTRACT(v1);
  M_ASSERT (str != NULL);
  return string_cmp_str(v1, str) == 0;
}

/* Test if the string is equal to the other string */
static inline bool
string_equal_p(const string_t v1, const string_t v2)
{
  /* string_equal_p can be called with (at most) one string which is an OOR value.
     In case of OOR value, .ptr is NULL and .alloc is the maximum or the maximum-1.
     As it will detect a stack based string, it will read the size from the alloc fied
     as 0xFF or 0xFE. In both cases, the size cannot be equal to a normal string
     so the test string_size(v1) == string_size(v2) is false in this case.
  */
  M_ASSERT(v1 != NULL);
  M_ASSERT(v2 != NULL);
  /* Optimization: both strings shall have at least the same size */
  return string_size(v1) == string_size(v2) && string_cmp(v1, v2) == 0;
}

/* Test if the string is equal to the C string 
   (case insentive according to the current locale)
   Note: doesn't work with UTF-8 strings.
*/
static inline int
string_cmpi_str(const string_t v1, const char p2[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (p2 != NULL);
  // strcasecmp is POSIX only
  const char *p1 = string_get_cstr(v1);
  int c1, c2;
  do {
    // To avoid locale without 1 per 1 mapping.
    c1 = toupper((unsigned char) *p1++);
    c2 = toupper((unsigned char) *p2++);
    c1 = tolower((unsigned char) c1);
    c2 = tolower((unsigned char) c2);
  } while (c1 == c2 && c1 != 0);
  return c1 - c2;
}

/* Test if the string is equal to the other string 
   (case insentive according to the current locale)
   Note: doesn't work with UTF-8 strings.
*/
static inline int
string_cmpi(const string_t v1, const string_t v2)
{
  return string_cmpi_str(v1, string_get_cstr(v2));
}

/* Search for the position of the character c
   from the position 'start' (include)  in the string 
   Return STRING_FAILURE if not found.
   By default, start is zero.
*/
static inline size_t
string_search_char (const string_t v, char c, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, string_size(v) + 1);
  const char *p = M_ASSIGN_CAST(const char*,
                                strchr(string_get_cstr(v)+start, c));
  return p == NULL ? STRING_FAILURE : (size_t) (p-string_get_cstr(v));
}

/* Reverse Search for the position of the character c
   from the position 'start' (include)  in the string 
   Return STRING_FAILURE if not found.
   By default, start is zero.
*/
static inline size_t
string_search_rchar (const string_t v, char c, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, string_size(v)+1);
  // NOTE: Can be implemented in a faster way than the libc function
  // by scanning backward from the bottom of the string (which is
  // possible since we know the size).
  // However, does it worth the effort?
  const char *p = M_ASSIGN_CAST(const char*,
                                strrchr(string_get_cstr(v)+start, c));
  return p == NULL ? STRING_FAILURE : (size_t) (p-string_get_cstr(v));
}

/* Search for the sub C string in the string from the position start
   Return STRING_FAILURE if not found.
   By default, start is zero. */
static inline size_t
string_search_str (const string_t v, const char str[], size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, string_size(v)+1);
  M_ASSERT (str != NULL);
  const char *p = M_ASSIGN_CAST(const char*,
                                strstr(string_get_cstr(v)+start, str));
  return p == NULL ? STRING_FAILURE : (size_t) (p-string_get_cstr(v));
}

/* Search for the sub other string v2 in the string v1 from the position start
   Return STRING_FAILURE if not found.
   By default, start is zero. */
static inline size_t
string_search (const string_t v1, const string_t v2, size_t start)
{
  M_STR1NG_CONTRACT (v2);
  M_ASSERT_INDEX (start, string_size(v1)+1);
  return string_search_str(v1, string_get_cstr(v2), start);
}

/* Search for the first matching character in the given C string 
   in the string v1 from the position start
   Return STRING_FAILURE if not found.
   By default, start is zero. */
static inline size_t
string_search_pbrk(const string_t v1, const char first_of[], size_t start)
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT_INDEX (start, string_size(v1)+1);
  M_ASSERT (first_of != NULL);
  const char *p = M_ASSIGN_CAST(const char*,
                                strpbrk(string_get_cstr(v1)+start, first_of));
  return p == NULL ? STRING_FAILURE : (size_t) (p-string_get_cstr(v1));
}

/* Compare the string to the C string using strcoll */
static inline int
string_strcoll_str(const string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  return strcoll(string_get_cstr(v), str);
}

/* Compare the string to the other string using strcoll */
static inline int
string_strcoll (const string_t v1, const string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return string_strcoll_str(v1, string_get_cstr(v2));
}

/* Return the number of bytes of the segment of s
   that consists entirely of bytes in accept */
static inline size_t
string_spn(const string_t v1, const char accept[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (accept != NULL);
  return strspn(string_get_cstr(v1), accept);
}

/* Return the number of bytes of the segment of s
   that consists entirely of bytes not in reject */
static inline size_t
string_cspn(const string_t v1, const char reject[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (reject != NULL);
  return strcspn(string_get_cstr(v1), reject);
}

/* Return the string left truncated to the first 'index' bytes */
static inline void
string_left(string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = string_size(v);
  if (index >= size)
    return;
  m_str1ng_get_cstr(v)[index] = 0;
  m_str1ng_set_size(v,index);
  M_STR1NG_CONTRACT (v);
}

/* Return the string right truncated from the 'index' position to the last position */
static inline void
string_right(string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  char *ptr = m_str1ng_get_cstr(v);
  const size_t size = string_size(v);
  if (index >= size) {
    ptr[0] = 0;
    m_str1ng_set_size(v, 0);
    M_STR1NG_CONTRACT (v);
    return;
  }
  size_t s2 = size - index;
  memmove (&ptr[0], &ptr[index], s2+1);
  m_str1ng_set_size(v, s2);
  M_STR1NG_CONTRACT (v);
}

/* Return the string from position index to size bytes.
   See also string_set_n
 */
static inline void
string_mid (string_t v, size_t index, size_t size)
{
  string_right(v, index);
  string_left(v, size);
}

/* Return in the string the C string str1 into the C string str2 from start
   By default, start is zero.
*/
static inline size_t
string_replace_str (string_t v, const char str1[], const char str2[], size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str1 != NULL && str2 != NULL);
  size_t i = string_search_str(v, str1, start);
  if (i != STRING_FAILURE) {
    const size_t str1_l = strlen(str1);
    const size_t str2_l = strlen(str2);
    const size_t size   = string_size(v);
    M_ASSERT(size + 1 + str2_l > str1_l);
    char *ptr = m_str1ng_fit2size (v, size + str2_l - str1_l + 1);
    if (str1_l != str2_l) {
      memmove(&ptr[i+str2_l], &ptr[i+str1_l], size - i - str1_l + 1);
      m_str1ng_set_size(v, size + str2_l - str1_l);
    }
    memcpy (&ptr[i], str2, str2_l);
    M_STR1NG_CONTRACT (v);
  }
  return i;
}

/* Return in the string the string str1 into the string str2 from start
   By default, start is zero.
*/
static inline size_t
string_replace (string_t v, const string_t v1, const string_t v2, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  return string_replace_str(v, string_get_cstr(v1), string_get_cstr(v2), start);
}

/* Replace in the string the sub-string at position 'pos' for 'len' bytes
   into the C string str2. */
static inline void
string_replace_at (string_t v, size_t pos, size_t len, const char str2[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str2 != NULL);
  const size_t str1_l = len;
  const size_t str2_l = strlen(str2);
  const size_t size   = string_size(v);
  char *ptr;
  if (str1_l != str2_l) {
    // Move bytes from the string 
    M_ASSERT_INDEX (str1_l, size + str2_l + 1);
    ptr = m_str1ng_fit2size (v, size + str2_l - str1_l + 1);
    M_ASSERT_INDEX (pos + str1_l, size + 1);
    M_ASSUME (pos + str1_l < size + 1);
    memmove(&ptr[pos+str2_l], &ptr[pos+str1_l], size - pos - str1_l + 1);
    m_str1ng_set_size(v, size + str2_l - str1_l);
  } else {
    ptr = m_str1ng_get_cstr(v);
  }
  memcpy (&ptr[pos], str2, str2_l);
  M_STR1NG_CONTRACT (v);
}

/* Replace all occurences of str1 into str2 when strlen(str1) >= strlen(str2) */
static inline void
m_str1ng_replace_all_str_1ge2 (string_t v, const char str1[], size_t str1len, const char str2[], size_t str2len)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str1len >= str2len);

  /* str1len >= str2len so the string doesn't need to be resized */
  size_t vlen = string_size(v);
  char *org = m_str1ng_get_cstr(v);
  char *src = org;
  char *dst = org;

  // Go through all the characters of the string
  while (*src != 0) {
    // Get a new occurence of str1 in the v string.
    char *occ = strstr(src, str1);
    if (occ == NULL) {
      // No new occurence
      break;
    }
    M_ASSERT(occ >= src);
    // Copy the data until the new occurence
    if (src != dst) {
      memmove(dst, src, (size_t) (occ - src));
    }
    dst += (occ - src);
    src  = occ;
    // Copy the replaced string
    memcpy(dst, str2, str2len);
    dst += str2len;
    // Advance src pointer
    src  = occ + str1len;
  }
  // Finish copying the string until the end
  M_ASSERT (src <= org + vlen );
  if (src != dst) {
    memmove(dst, src, (size_t) (org + vlen + 1 - src) );
  }
  // Update string size
  m_str1ng_set_size(v, (size_t) (dst + vlen - src) );
  M_STR1NG_CONTRACT (v);
}

/* Reverse strstr from the end of the string
  org is the start of the string
  src is the current character pointer (shall be initialized to the end of the string)
  pattern / pattern_size: the pattern to search.
  */
static inline char *
m_str1ng_strstr_r(char org[], char src[], const char pattern[], size_t pattern_size)
{
  M_ASSERT(pattern_size >= 1);
  src -= pattern_size - 1;
  while (org <= src) {
    if (src[0] == pattern[0]
      && src[pattern_size-1] == pattern[pattern_size-1]
      && memcmp(src, pattern, pattern_size) == 0) {
        return src;
    }
    src --;
  }
  return NULL;
}

/* Replace all occurences of str1 into str2 when strlen(str1) < strlen(str2) */
static inline void
m_str1ng_replace_all_str_1lo2 (string_t v, const char str1[], size_t str1len, const char str2[], size_t str2len)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str1len < str2len);

  /* str1len < str2len so the string may need to be resized
    Worst case if when v is composed fully of str1 substrings.
    Needed size : v.size / str1len * str2len
   */
  size_t vlen = string_size(v);
  size_t alloc = 1 + vlen / str1len * str2len;
  char *org = m_str1ng_fit2size(v, alloc);
  char *src = org + vlen - 1;
  char *end = org + string_capacity(v);
  char *dst = end;

  // Go through all the characters of the string in reverse !
  while (src >= org) {
    char *occ = m_str1ng_strstr_r(org, src, str1, str1len);
    if (occ == NULL) {
      break;
    }
    M_ASSERT(occ + str1len - 1 <= src);
    // Copy the data until the new occurence
    dst -= (src - (occ + str1len - 1));
    memmove(dst, occ+str1len, (size_t) (src - (occ + str1len - 1)));
    // Copy the replaced string
    dst -= str2len;
    memcpy(dst, str2, str2len);
    // Advance src pointer
    src  = occ - 1;
  }
  // Finish moving data back to their natural place
  memmove(src + 1, dst, (size_t) (end - dst) );
  // Update string size
  vlen = (size_t) (src - org + end - dst + 1);
  org[vlen] = 0;
  m_str1ng_set_size(v, vlen );
  M_STR1NG_CONTRACT (v);
}

static inline void
string_replace_all_str (string_t v, const char str1[], const char str2[])
{
  size_t str1_l = strlen(str1);
  size_t str2_l = strlen(str2);
  assert(str1_l > 0);
  if (str1_l >= str2_l) {
    m_str1ng_replace_all_str_1ge2(v, str1, str1_l, str2, str2_l );
  } else {
    m_str1ng_replace_all_str_1lo2(v, str1, str1_l, str2, str2_l );
  }
}

static inline void
string_replace_all (string_t v, const string_t str1, const string_t str2)
{
  size_t str1_l = string_size(str1);
  size_t str2_l = string_size(str2);
  assert(str1_l > 0);
  assert(str2_l > 0);
  if (str1_l >= str2_l) {
    m_str1ng_replace_all_str_1ge2(v, string_get_cstr(str1), str1_l, string_get_cstr(str2), str2_l );
  } else {
    m_str1ng_replace_all_str_1lo2(v, string_get_cstr(str1), str1_l, string_get_cstr(str2), str2_l );
  }
}

#if M_USE_STDARG

/* Format in the string the given printf format */
static inline int
string_vprintf (string_t v, const char format[], va_list args)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  int size;
  va_list args_org;
  va_copy(args_org, args);
  char *ptr = m_str1ng_get_cstr(v);
  size_t alloc = string_capacity(v);
  size = vsnprintf (ptr, alloc, format, args);
  if (size > 0 && ((size_t) size+1 >= alloc) ) {
    // We have to realloc our string to fit the needed size
    ptr = m_str1ng_fit2size (v, (size_t) size + 1);
    alloc = string_capacity(v);
    // and redo the parsing.
    va_copy(args, args_org);
    size = vsnprintf (ptr, alloc, format, args);
    M_ASSERT (size > 0 && (size_t)size < alloc);
  }
  if (M_LIKELY (size >= 0)) {
    m_str1ng_set_size(v, (size_t) size);
  } else {
    // An error occured during the conversion: Assign an empty string.
    m_str1ng_set_size(v, 0);
    ptr[0] = 0;
  }
  M_STR1NG_CONTRACT (v);
  return size;
}

/* Format in the string the given printf format */
static inline int
string_printf (string_t v, const char format[], ...)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  va_list args;
  va_start (args, format);
  int ret = string_vprintf(v, format, args);
  va_end (args);
  return ret;
}

/* Append to the string the formatted string of the given printf format */
static inline int
string_cat_printf (string_t v, const char format[], ...)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  va_list args;
  int size;
  size_t old_size = string_size(v);
  char  *ptr      = m_str1ng_get_cstr(v);
  size_t alloc    = string_capacity(v);
  va_start (args, format);
  size = vsnprintf (&ptr[old_size], alloc - old_size, format, args);
  if (size > 0 && (old_size+(size_t)size+1 >= alloc) ) {
    // We have to realloc our string to fit the needed size
    ptr = m_str1ng_fit2size (v, old_size + (size_t) size + 1);
    alloc = string_capacity(v);
    // and redo the parsing.
    va_end (args);
    va_start (args, format);
    size = vsnprintf (&ptr[old_size], alloc - old_size, format, args);
    M_ASSERT (size >= 0);
  }
  if (size >= 0) {
    m_str1ng_set_size(v, old_size + (size_t) size);
  } else {
    // vsnprintf may have output some characters before returning an error.
    // Undo this to have a clean state
    ptr[old_size] = 0;
  }
  va_end (args);
  M_STR1NG_CONTRACT (v);
  return size;
}

#endif // Have stdarg

#if M_USE_STDIO

/* Get a line/pureline/file from the FILE and store it in the string */
static inline bool
string_fgets(string_t v, FILE *f, string_fgets_t arg)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  char *ptr = m_str1ng_fit2size (v, 100);
  size_t size = 0;
  size_t alloc = string_capacity(v);
  ptr[0] = 0;
  bool retcode = false; /* Nothing has been read yet */
  /* alloc - size is very unlikely to be bigger than INT_MAX
    but fgets accepts an int as the size argument */
  while (fgets(&ptr[size], (int) M_MIN( (alloc - size), (size_t) INT_MAX ), f) != NULL) {
    retcode = true; /* Something has been read */
    // fgets doesn't return the number of characters read, so we need to count.
    size += strlen(&ptr[size]);
    assert(size >= 1);
    if (arg != STRING_READ_FILE && ptr[size-1] == '\n') {
      if (arg == STRING_READ_PURE_LINE) {
        size --;
        ptr[size] = 0;         /* Remove EOL */
      }
      m_str1ng_set_size(v, size);
      M_STR1NG_CONTRACT(v);
      return retcode; /* Normal terminaison */
    } else if (ptr[size-1] != '\n' && !feof(f)) {
      /* The string buffer is not big enough:
         increase it and continue reading */
      /* v cannot be stack alloc */
      ptr   = m_str1ng_fit2size (v, alloc + alloc/2);
      alloc = string_capacity(v);
    }
  }
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
  return retcode; /* Abnormal terminaison */
}

/* Get a word from the FILE and store it in the string.
   Words are supposed to be separated each other by the given list of separator
   separator shall be a CONSTANT C array.
 */
static inline bool
string_fget_word (string_t v, const char separator[], FILE *f)
{
  char buffer[128];
  char c = 0;
  int d;
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  M_ASSERT_INDEX (1+20+2+strlen(separator)+3, sizeof buffer);
  size_t size = 0;
  bool retcode = false;

  /* Skip separator first */
  do {
    d = fgetc(f);
    if (d == EOF) {
      return false;
    }
  } while (strchr(separator, d) != NULL);
  ungetc(d, f);

  size_t alloc = string_capacity(v);
  char *ptr    = m_str1ng_get_cstr(v);
  ptr[0] = 0;

  /* NOTE: We generate a buffer which we give to scanf to parse the string,
     that it is to say, we generate the format dynamically!
     The format is like " %49[^ \t.\n]%c"
     So in this case, we parse up to 49 chars, up to the separators char,
     and we read the next char. If the next char is a separator, we successful
     read a word, otherwise we have to continue parsing.
     The user shall give a constant string as the separator argument,
     as a control over this argument may give an attacker
     an opportunity for stack overflow */
  while (snprintf(buffer, sizeof buffer -1, " %%%zu[^%s]%%c", (size_t) alloc-1-size, separator) > 0
         /* We may read one or two argument(s) */
         && m_core_fscanf(f, buffer, m_core_arg_size(&ptr[size], alloc-size), &c) >= 1) {
    retcode = true;
    size += strlen(&ptr[size]);
    /* If we read only one argument 
       or if the final read character is a separator */
    if (c == 0 || strchr(separator, c) != NULL)
      break;
    /* Next char is not a separator: continue parsing */
    m_str1ng_set_size(v, size);
    ptr = m_str1ng_fit2size (v, alloc + alloc/2);
    alloc = string_capacity(v);
    M_ASSERT (alloc > size + 1);
    ptr[size++] = c;
    ptr[size] = 0;
    // Invalid c character for next iteration
    c= 0;
  }
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT(v);  
  return retcode;
}

/* Put the string in the given FILE without formatting */
static inline bool
string_fputs(FILE *f, const string_t v)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (f != NULL);
  return fputs(string_get_cstr(v), f) >= 0;
}

#endif // Have stdio

/* Test if the string starts with the given C string */
static inline bool
string_start_with_str_p(const string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const char *v_str = string_get_cstr(v);
  while (*str){
    if (*str != *v_str)
      return false;
    str++;
    v_str++;
  }
  return true;
}

/* Test if the string starts with the other string */
static inline bool
string_start_with_string_p(const string_t v, const string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return string_start_with_str_p (v, string_get_cstr(v2));
}

/* Test if the string ends with the C string */
static inline bool
string_end_with_str_p(const string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const char *v_str  = string_get_cstr(v);
  const size_t v_l   = string_size(v);
  const size_t str_l = strlen(str);
  if (v_l < str_l)
    return false;
  return (memcmp(str, &v_str[v_l - str_l], str_l) == 0);
}

/* Test if the string ends with the other string */
static inline bool
string_end_with_string_p(const string_t v, const string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return string_end_with_str_p (v, string_get_cstr(v2));
}

/* Compute a hash for the string */
static inline size_t
string_hash(const string_t v)
{
  M_STR1NG_CONTRACT (v);
  return m_core_hash(string_get_cstr(v), string_size(v));
}

// Return true if c is a character from charac
static bool
m_str1ng_strim_char(char c, const char charac[])
{
  for(const char *s = charac; *s; s++) {
    if (c == *s)
      return true;
  }
  return false;
}

/* Remove any characters from charac that are present 
   in the begining of the string and the end of the string. */
static inline void
string_strim(string_t v, const char charac[])
{
  M_STR1NG_CONTRACT (v);
  char *ptr = m_str1ng_get_cstr(v);
  char *b   = ptr;
  size_t size = string_size(v);
  while (size > 0 && m_str1ng_strim_char(b[size-1], charac))
    size --;
  if (size > 0) {
    while (m_str1ng_strim_char(*b, charac))
      b++;
    M_ASSERT (b >= ptr &&  size >= (size_t) (b - ptr) );
    size -= (size_t) (b - ptr);
    memmove (ptr, b, size);
  }
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Test if the string is equal to the OOR value */
static inline bool
string_oor_equal_p(const string_t s, unsigned char n)
{
  return (s->ptr == NULL) & (s->u.heap.alloc == ~(size_t)n);
}

/* Set the unitialized string to the OOR value */
static inline void
string_oor_set(string_t s, unsigned char n)
{
  s->ptr = NULL;
  s->u.heap.alloc = ~(size_t)n;
}

/* I/O */
/* Output: "string" with quote around
   Replace " by \" within the string (and \ to \\)
   \n, \t & \r by their standard representation
   and other not printable character with \0xx */

/* Transform the string 'v2' into a formatted string
   and set it to (or append in) the string 'v'. */
static inline void
string_get_str(string_t v, const string_t v2, bool append)
{
  M_STR1NG_CONTRACT(v2);
  M_STR1NG_CONTRACT(v);
  M_ASSERT (v != v2); // Limitation
  size_t size = append ? string_size(v) : 0;
  size_t v2_size = string_size(v2);
  size_t targetSize = size + v2_size + 3;
  char *ptr = m_str1ng_fit2size(v, targetSize);
  ptr[size ++] = '"';
  for(size_t i = 0 ; i < v2_size; i++) {
    const char c = string_get_char(v2,i);
    switch (c) {
    case '\\':
    case '"':
    case '\n':
    case '\t':
    case '\r':
      // Special characters which can be displayed in a short form.
      m_str1ng_set_size(v, size);
      ptr = m_str1ng_fit2size(v, ++targetSize);
      ptr[size ++] = '\\';
      // This string acts as a perfect hashmap which supposes an ASCII mapping
      // and (c^(c>>5)) is the hash function
      ptr[size ++] = " tn\" r\\"[(c ^ (c >>5)) & 0x07];
      break;
    default:
      if (M_UNLIKELY (!isprint(c))) {
        targetSize += 3;
        m_str1ng_set_size(v, size);
        ptr = m_str1ng_fit2size(v, targetSize);
        int d1 = c & 0x07, d2 = (c>>3) & 0x07, d3 = (c>>6) & 0x07;
        ptr[size ++] = '\\';
        ptr[size ++] = (char) ('0' + d3);
        ptr[size ++] = (char) ('0' + d2);
        ptr[size ++] = (char) ('0' + d1);
      } else {
        ptr[size ++] = c;
      }
      break;
    }
  }
  ptr[size ++] = '"';
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_ASSERT (size <= targetSize);
  M_STR1NG_CONTRACT (v);
}

#if M_USE_STDIO

/* Transform the string 'v2' into a formatted string
   and output it in the given FILE */
static inline void
string_out_str(FILE *f, const string_t v)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  fputc('"', f);
  size_t size = string_size(v);
  for(size_t i = 0 ; i < size; i++) {
    const char c = string_get_char(v, i);
    switch (c) {
    case '\\':
    case '"':
    case '\n':
    case '\t':
    case '\r':
      fputc('\\', f);
      fputc(" tn\" r\\"[(c ^ c >>5) & 0x07], f);
      break;
    default:
      if (M_UNLIKELY (!isprint(c))) {
        int d1 = c & 0x07, d2 = (c>>3) & 0x07, d3 = (c>>6) & 0x07;
        fputc('\\', f);
        fputc('0' + d3, f);
        fputc('0' + d2, f);
        fputc('0' + d1, f);
      } else {
        fputc(c, f);
      }
      break;
    }
  }
  fputc('"', f);
}

/* Read the formatted string from the FILE
   and set the converted value in the string 'v'.
   Return true in case of success */
static inline bool
string_in_str(string_t v, FILE *f)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  int c = fgetc(f);
  if (c != '"') return false;
  string_reset(v);
  c = fgetc(f);
  while (c != '"' && c != EOF) {
    if (M_UNLIKELY (c == '\\')) {
      c = fgetc(f);
      switch (c) {
      case 'n':
      case 't':
      case 'r':
      case '\\':
      case '\"':
        // This string acts as a perfect hashmap which supposes an ASCII mapping
        // and (c^(c>>5)) is the hash function
        c = " \r \" \n\\\t"[(c^(c>>5))& 0x07];
        break;
      default:
        if (!(c >= '0' && c <= '7'))
          return false;
        int d1 = c - '0';
        c = fgetc(f);
        if (!(c >= '0' && c <= '7'))
          return false;
        int d2 = c - '0';
        c = fgetc(f);
        if (!(c >= '0' && c <= '7'))
          return false;
        int d3 = c - '0';
        c = (d1 << 6) + (d2 << 3) + d3;
        break;
      }
    }
    string_push_back (v, (char) c);
    c = fgetc(f);
  }
  return c == '"';
}

#endif // Have stdio

/* Read the formatted string from the C string
   and set the converted value in the string 'v'.
   Return true in case of success
   If endptr is not null, update the position of the parsing.
*/
static inline bool
string_parse_str(string_t v, const char str[], const char **endptr)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (str != NULL);
  bool success = false;
  int c = *str++;
  if (c != '"') goto exit;
  string_reset(v);
  c = *str++;
  while (c != '"' && c != 0) {
    if (M_UNLIKELY (c == '\\')) {
      c = *str++;
      switch (c) {
      case 'n':
      case 't':
      case 'r':
      case '\\':
      case '\"':
        // This string acts as a perfect hashmap which supposes an ASCII mapping
        // and (c^(c>>5)) is the hash function
        c = " \r \" \n\\\t"[(c^(c>>5))& 0x07];
        break;
      default:
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d1 = c - '0';
        c = *str++;
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d2 = c - '0';
        c = *str++;
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d3 = c - '0';
        c = (d1 << 6) + (d2 << 3) + d3;
        break;
      }
    }
    string_push_back (v, (char) c);
    c = *str++;
  }
  success = (c == '"');
 exit:
  if (endptr != NULL) *endptr = str;
  return success;
}

/* Transform the string 'v2' into a formatted string
   and output it in the given serializer
   See serialization for return code.
*/
static inline m_serial_return_code_t
string_out_serial(m_serial_write_t serial, const string_t v)
{
  M_ASSERT (serial != NULL && serial->m_interface != NULL);
  return serial->m_interface->write_string(serial, string_get_cstr(v), string_size(v) );
}

/* Read the formatted string from the serializer
   and set the converted value in the string 'v'.
   See serialization for return code.
*/
static inline m_serial_return_code_t
string_in_serial(string_t v, m_serial_read_t serial)
{
  M_ASSERT (serial != NULL && serial->m_interface != NULL);
  return serial->m_interface->read_string(serial, v);
}

/* State of the UTF8 decoding machine state */
typedef enum {
  M_STRING_UTF8_STARTING = 0,
  M_STRING_UTF8_DECODING_1 = 8,
  M_STRING_UTF8_DECODING_2 = 16,
  M_STRING_UTF8_DOCODING_3 = 24,
  M_STRING_UTF8_ERROR = 32
} m_str1ng_utf8_state_e;

/* An unicode value */
typedef unsigned int string_unicode_t;

/* Error in case of decoding */
#define STRING_UNICODE_ERROR (UINT_MAX)

/* UTF8 character classification:
 * 
 * 0*       --> type 1 byte  A
 * 10*      --> chained byte B
 * 110*     --> type 2 byte  C
 * 1110*    --> type 3 byte  D
 * 11110*   --> type 4 byte  E
 * 111110*  --> invalid      I
 */
/* UTF8 State Transition table:
 *    ABCDEI
 *   +------
 *  S|SI123III
 *  1|ISIIIIII
 *  2|I1IIIIII
 *  3|I2IIIIII
 *  I|IIIIIIII
 */

/* The use of a string enables the compiler/linker to factorize it. */
#define M_STRING_UTF8_STATE_TAB                                               \
  "\000\040\010\020\030\040\040\040"                                          \
  "\040\000\040\040\040\040\040\040"                                          \
  "\040\010\040\040\040\040\040\040"                                          \
  "\040\020\040\040\040\040\040\040"                                          \
  "\040\040\040\040\040\040\040\040"

/* Main generic UTF8 decoder
   It shall be (nearly) branchless on any CPU.
   It takes a character, and the previous state and the previous value of the unicode value.
   It updates the state and the decoded unicode value.
   A decoded unicoded value is valid only when the state is STARTING.
 */
static inline void
m_str1ng_utf8_decode(char c, m_str1ng_utf8_state_e *state,
                    string_unicode_t *unicode)
{
  const unsigned int type = m_core_clz32((unsigned char)~c) - (unsigned) (sizeof(uint32_t) - 1) * CHAR_BIT;
  const string_unicode_t mask1 = (UINT_MAX - (string_unicode_t)(*state != M_STRING_UTF8_STARTING) + 1);
  const string_unicode_t mask2 = (0xFFU >> type);
  *unicode = ((*unicode << 6) & mask1) | ((unsigned int) c & mask2);
  *state = (m_str1ng_utf8_state_e) M_STRING_UTF8_STATE_TAB[(unsigned int) *state + type];
}

/* Check if the given array of characters is a valid UTF8 stream */
/* NOTE: Non-canonical representation are not rejected */
static inline bool
m_str1ng_utf8_valid_str_p(const char str[])
{
  m_str1ng_utf8_state_e s = M_STRING_UTF8_STARTING;
  string_unicode_t u = 0;
  while (*str) {
    m_str1ng_utf8_decode(*str, &s, &u);
    if ((s == M_STRING_UTF8_ERROR)
        ||(s == M_STRING_UTF8_STARTING
           &&(u > 0x10FFFF /* out of range */
              ||(u >= 0xD800 && u <= 0xDFFF) /* surrogate halves */)))
      return false;
    str++;
  }
  return true;
}

/* Computer the number of unicode characters are represented in the UTF8 stream
   Return SIZE_MAX (aka -1) in case of error
 */
static inline size_t
m_str1ng_utf8_length(const char str[])
{
  size_t size = 0;
  m_str1ng_utf8_state_e s = M_STRING_UTF8_STARTING;
  string_unicode_t u = 0;
  while (*str) {
    m_str1ng_utf8_decode(*str, &s, &u);
    if (M_UNLIKELY (s == M_STRING_UTF8_ERROR)) return SIZE_MAX;
    size += (s == M_STRING_UTF8_STARTING);
    str++;
  }
  return size;
}

/* Encode an unicode into an UTF8 stream of characters */
static inline int
m_str1ng_utf8_encode(char buffer[5], string_unicode_t u)
{
  if (M_LIKELY (u <= 0x7Fu)) {
    buffer[0] = (char) u;
    buffer[1] = 0;
    return 1;
  } else if (u <= 0x7FFu) {
    buffer[0] = (char) (0xC0u | (u >> 6));
    buffer[1] = (char) (0x80 | (u & 0x3Fu));
    buffer[2] = 0;
    return 2;
  } else if (u <= 0xFFFFu) {
    buffer[0] = (char) (0xE0u | (u >> 12));
    buffer[1] = (char) (0x80u | ((u >> 6) & 0x3Fu));
    buffer[2] = (char) (0x80u | (u & 0x3Fu));
    buffer[3] = 0;
    return 3;
  } else {
    buffer[0] = (char) (0xF0u | (u >> 18));
    buffer[1] = (char) (0x80u | ((u >> 12) & 0x3Fu));
    buffer[2] = (char) (0x80u | ((u >> 6) & 0x3Fu));
    buffer[3] = (char) (0x80u | (u & 0x3F));
    buffer[4] = 0;
    return 4;
  }
}

/* Iterator on a string over UTF8 encoded characters */
typedef struct string_it_s {
  string_unicode_t u;
  const char *ptr;
  const char *next_ptr;
} string_it_t[1];

/* Start iteration over the UTF8 encoded unicode value */
static inline void
string_it(string_it_t it, const string_t str)
{
  M_STR1NG_CONTRACT(str);
  M_ASSERT(it != NULL);
  it->ptr      = string_get_cstr(str);
  it->next_ptr = it->ptr;
  it->u        = 0;
}

/* Set the iterator to the end of string 
   The iterator references therefore nothing.
*/
static inline void
string_it_end(string_it_t it, const string_t str)
{
  M_STR1NG_CONTRACT(str);
  M_ASSERT(it != NULL);
  it->ptr      = &string_get_cstr(str)[string_size(str)];
  it->next_ptr = 0;
  it->u        = 0;
}

/* Set the iterator to the same position than the other one */
static inline void
string_it_set(string_it_t it, const string_it_t itsrc)
{
  M_ASSERT(it != NULL && itsrc != NULL);
  it->ptr      = itsrc->ptr;
  it->next_ptr = itsrc->next_ptr;
  it->u        = itsrc->u;
}

/* Test if the iterator has reached the end of the string */
static inline bool
string_end_p (string_it_t it)
{
  M_ASSERT (it != NULL);
  if (M_UNLIKELY (*it->ptr == 0))
    return true;
  m_str1ng_utf8_state_e state =  M_STRING_UTF8_STARTING;
  string_unicode_t u = 0;
  const char *str = it->ptr;
  do {
    m_str1ng_utf8_decode(*str, &state, &u);
    str++;
  } while (state != M_STRING_UTF8_STARTING && state != M_STRING_UTF8_ERROR && *str != 0);
  it->next_ptr = str;
  it->u = M_UNLIKELY (state == M_STRING_UTF8_ERROR) ? STRING_UNICODE_ERROR : u;
  return false;
}

/* Test if the iterator is equal to the other one */
static inline bool
string_it_equal_p(const string_it_t it1, const string_it_t it2)
{
  M_ASSERT(it1 != NULL && it2 != NULL);
  // IT1.ptr == IT2.ptr ==> IT1 == IT2 ==> All fields are equal
  M_ASSERT(it1->ptr != it2->ptr || (it1->next_ptr == it2->next_ptr && it1->u == it2->u));
  return it1->ptr == it2->ptr;
}

/* Advance the iterator to the next UTF8 unicode character */
static inline void
string_next (string_it_t it)
{
  M_ASSERT (it != NULL);
  it->ptr = it->next_ptr;
}

/* Return the unicode value associated to the iterator */
static inline string_unicode_t
string_get_cref (const string_it_t it)
{
  M_ASSERT (it != NULL);
  return it->u;
}

/* Return the unicode value associated to the iterator */
static inline const string_unicode_t *
string_cref (const string_it_t it)
{
  M_ASSERT (it != NULL);
  return &it->u;
}

/* Push unicode into string, encoding it in UTF8 */
static inline void
string_push_u (string_t str, string_unicode_t u)
{
  M_STR1NG_CONTRACT(str);
  char buffer[4+1];
  m_str1ng_utf8_encode(buffer, u);
  string_cat_str(str, buffer);
}

/* Compute the length in UTF8 characters in the string */
static inline size_t
string_length_u(string_t str)
{
  M_STR1NG_CONTRACT(str);
  return m_str1ng_utf8_length(string_get_cstr(str));
}

/* Check if a string is a valid UTF8 encoded stream */
static inline bool
string_utf8_p(string_t str)
{
  M_STR1NG_CONTRACT(str);
  return m_str1ng_utf8_valid_str_p(string_get_cstr(str));
}


/* Define the split & the join functions 
   in case of usage with the algorithm module */
#define STRING_SPLIT(name, oplist, type_oplist)                               \
  static inline void M_C(name, _split)(M_GET_TYPE oplist cont,                \
                                   const string_t str, const char sep)        \
  {                                                                           \
    size_t begin = 0;                                                         \
    string_t tmp;                                                             \
    size_t size = string_size(str);                                           \
    string_init(tmp);                                                         \
    M_CALL_RESET(oplist, cont);                                               \
    for(size_t i = 0 ; i < size; i++) {                                       \
      char c = string_get_char(str, i);                                       \
      if (c == sep) {                                                         \
        string_set_strn(tmp, &string_get_cstr(str)[begin], i - begin);        \
        /* If push move method is available, use it */                        \
        M_IF_METHOD(PUSH_MOVE,oplist)(                                        \
                                      M_CALL_PUSH_MOVE(oplist, cont, &tmp);   \
                                      string_init(tmp);                       \
                                      ,                                       \
                                      M_CALL_PUSH(oplist, cont, tmp);         \
                                                                        )     \
          begin = i + 1;                                                      \
      }                                                                       \
    }                                                                         \
    string_set_strn(tmp, &string_get_cstr(str)[begin], size - begin);         \
    M_CALL_PUSH(oplist, cont, tmp);                                           \
    /* HACK: if method reverse is defined, it is likely that we have */       \
    /* inserted the items in the wrong order (aka for a list) */              \
    M_IF_METHOD(REVERSE, oplist) (M_CALL_REVERSE(oplist, cont);, )            \
    string_clear(tmp);                                                        \
  }                                                                           \
                                                                              \
  static inline void M_C(name, _join)(string_t dst, M_GET_TYPE oplist cont,   \
                                      const string_t str)                     \
  {                                                                           \
    bool init_done = false;                                                   \
    string_reset (dst);                                                       \
    for M_EACH(item, cont, oplist) {                                          \
        if (init_done) {                                                      \
          string_cat(dst, str);                                               \
        }                                                                     \
        string_cat (dst, *item);                                              \
        init_done = true;                                                     \
    }                                                                         \
  }                                                                           \


/* Use of Compound Literals to init a constant string.
   NOTE: The use of the additional structure layer is to ensure
   that the pointer to char is properly aligned to an int (this
   is a needed asumption by string_hash).
   Otherwise it could have been :
   #define STRING_CTE(s)                                                      \
     ((const string_t){{.size = sizeof(s)-1, .alloc = sizeof(s),              \
     .ptr = s}})
   which produces faster code.
   Note: This code doesn't work with C++ (use of c99 feature
   of recursive struct definition and compound literral). 
   As such, there is a separate C++ definition.
*/
#ifndef __cplusplus
/* Initialize a constant string with the given C string */
# define STRING_CTE(s)                                                        \
  (string_srcptr)((const string_t){{.u.heap = { .size = sizeof(s)-1, .alloc = sizeof(s) } , \
        .ptr = ((struct { long long _n; char _d[sizeof (s)]; }){ 0, s })._d }})
#else
namespace m_lib {
  template <unsigned int N>
    struct m_aligned_string {
      string_t string;
      union  {
        char str[N];
        long long i;
      };
    inline m_aligned_string(const char lstr[])
      {
        this->string->u.heap.size = N -1;
        this->string->u.heap.alloc = N;
        memcpy (this->str, lstr, N);
        this->string->ptr = this->str;
      }
    };
}
/* Initialize a constant string with the given C string (C++ mode) */
#define STRING_CTE(s)                                                         \
  m_lib::m_aligned_string<sizeof (s)>(s).string
#endif

/* Initialize and set a string to the given formatted value. */
#define string_init_printf(v, ...)                                            \
  (string_printf ( m_str1ng_init_ref(v), __VA_ARGS__) )

/* Initialize and set a string to the given formatted value. */
#define string_init_vprintf(v, format, args)                                  \
  (string_vprintf ( m_str1ng_init_ref(v), format, args) )

/* Initialize a string with the given list of arguments.
   Check if it is a formatted input or not by counting the number of arguments.
   If there is only one argument, it can only be a set to C string.
   It is much faster in this case to call string_init_set_str.
   In C11 mode, it uses the fact that string_init_set is overloaded to handle both
   C string and string. */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define M_STRING_INIT_WITH(v, ...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(string_init_set, string_init_printf)(v, __VA_ARGS__)
#else
#define M_STRING_INIT_WITH(v, ...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(string_init_set_str, string_init_printf)(v, __VA_ARGS__)
#endif

/* NOTE: Use GCC extension (OBSOLETE) */
#define STRING_DECL_INIT(v)                                                   \
  string_t v __attribute__((cleanup(m_str1ng_clear2))) = {{ 0, 0, NULL}}

/* NOTE: Use GCC extension (OBSOLETE) */
#define STRING_DECL_INIT_PRINTF(v, format, ...)                               \
  STRING_DECL_INIT(v);                                                        \
  string_printf (v, format, __VA_ARGS__)


/* Define the OPLIST of a STRING */
#define STRING_OPLIST                                                         \
  (INIT(string_init),INIT_SET(string_init_set), SET(string_set),              \
   INIT_WITH(M_STRING_INIT_WITH),                                             \
   INIT_MOVE(string_init_move), MOVE(string_move),                            \
   SWAP(string_swap), RESET(string_reset),                                    \
   EMPTY_P(string_empty_p),                                                   \
   CLEAR(string_clear), HASH(string_hash), EQUAL(string_equal_p),             \
   CMP(string_cmp), TYPE(string_t),                                           \
   PARSE_STR(string_parse_str), GET_STR(string_get_str),                      \
   OUT_STR(string_out_str), IN_STR(string_in_str),                            \
   OUT_SERIAL(string_out_serial), IN_SERIAL(string_in_serial),                \
   EXT_ALGO(STRING_SPLIT),                                                    \
   OOR_EQUAL(string_oor_equal_p), OOR_SET(string_oor_set)                     \
   ,SUBTYPE(string_unicode_t)                                                 \
   ,IT_TYPE(string_it_t)                                                      \
   ,IT_FIRST(string_it)                                                       \
   ,IT_END(string_it_end)                                                     \
   ,IT_SET(string_it_set)                                                     \
   ,IT_END_P(string_end_p)                                                    \
   ,IT_EQUAL_P(string_it_equal_p)                                             \
   ,IT_NEXT(string_next)                                                      \
   ,IT_CREF(string_cref)                                                      \
   )

/* Register the OPLIST as a global one */
#define M_OPL_string_t() STRING_OPLIST


/* Macro encapsulation to give a default value of 0 for start offset */

/* Search for a character in a string (string, character[, start=0]) */
#define string_search_char(v, ...)                                            \
  M_APPLY(string_search_char, v, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Reverse Search for a character in a string (string, character[, start=0]) */
#define string_search_rchar(v, ...)                                           \
  M_APPLY(string_search_rchar, v, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Search for a C string in a string (string, c_string[, start=0]) */
#define string_search_str(v, ...)                                             \
  M_APPLY(string_search_str, v, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Search for a string in a string (string, string[, start=0]) */
#define string_search(v, ...)                                                 \
  M_APPLY(string_search, v, M_IF_DEFAULT1(0, __VA_ARGS__))

/* PBRK for a string in a string (string, string[, start=0]) */
#define string_search_pbrk(v, ...)                                            \
  M_APPLY(string_search_pbrk, v, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Replace a C string to another C string in a string (string, c_src_string, c_dst_string, [, start=0]) */
#define string_replace_str(v, s1, ...)                                        \
  M_APPLY(string_replace_str, v, s1, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Replace a string to another string in a string (string, src_string, dst_string, [, start=0]) */
#define string_replace(v, s1, ...)                                            \
  M_APPLY(string_replace, v, s1, M_IF_DEFAULT1(0, __VA_ARGS__))

/* Strim a string from the given set of characters (default is " \n\r\t") */
#define string_strim(...)                                                     \
  M_APPLY(string_strim, M_IF_DEFAULT1("  \n\r\t", __VA_ARGS__))

/* Concat a set strings (or const char * if C1)) into one string */
#define string_cats(a, ...)                                                   \
  M_MAP2_C(string_cat, a, __VA_ARGS__)

/* Set a string to a set strings (or const char * if C1)) */
#define string_sets(a, ...)                                                   \
  (string_reset(a), M_MAP2_C(string_cat, a, __VA_ARGS__) )

/* Macro encapsulation for C11 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

/* Select either the string function or the str function depending on
   the b operade to the function.
   func1 is the string function / func2 is the str function. */
# define M_STRING_SELECT2(func1,func2,a,b)                                    \
  _Generic((b)+0,                                                             \
           char*: func2,                                                      \
           const char *: func2,                                               \
           default : func1                                                    \
           )(a,b)
# define M_STRING_SELECT3(func1,func2,a,b,c)                                  \
  _Generic((b)+0,                                                             \
           char*: func2,                                                      \
           const char *: func2,                                               \
           default : func1                                                    \
           )(a,b,c)

/* Init & Set the string a to the string (or C string) b (constructor) */
#define string_init_set(a,b) M_STRING_SELECT2(string_init_set, string_init_set_str, a, b)

/* Set the string a to the string (or C string) b */
#define string_set(a,b) M_STRING_SELECT2(string_set, string_set_str, a, b)

/* Concatene the string (or C string) b to the string a */
#define string_cat(a,b) M_STRING_SELECT2(string_cat, string_cat_str, a, b)

/* Compare the string a to the string (or C string) b and return the sort order */
#define string_cmp(a,b) M_STRING_SELECT2(string_cmp, string_cmp_str, a, b)

/* Compare for equality the string a to the string (or C string) b */
#define string_equal_p(a,b) M_STRING_SELECT2(string_equal_p, string_equal_str_p, a, b)

/* strcoll the string a to the string (or C string) b */
#define string_strcoll(a,b) M_STRING_SELECT2(string_strcoll, string_strcoll_str, a, b)

#undef string_search
/* Search for a string in a string (or C string) (string, string[, start=0]) */
#define string_search(v, ...)                                                 \
  M_APPLY(M_STRING_SELECT3, string_search, string_search_str,                 \
          v, M_IF_DEFAULT1(0, __VA_ARGS__))

/**************** Override m-core default macros to add support of string_t ******/

/* Internal Macro: Provide GET_STR method to default type */
#undef M_GET_STR_METHOD_FOR_DEFAULT_TYPE
#define M_GET_STR_METHOD_FOR_DEFAULT_TYPE GET_STR(M_GET_STRING_ARG)

/* Internal Macro: Provide support of string_t to the macro M_PRINT in C11 */

// Extend the format specifier to support string_t
#undef M_PRINTF_FORMAT_EXTEND
#define M_PRINTF_FORMAT_EXTEND()                                              \
          , string_ptr: "%s"                                                  \
          , string_srcptr: "%s"

// Add type conversion for string_t (into a const char*)
#undef M_CORE_PRINTF_ARG
#define M_CORE_PRINTF_ARG(x) _Generic( ((void)0,(x))                          \
          , string_ptr: string_get_cstr( M_AS_TYPE(string_ptr, x))            \
          , string_srcptr: string_get_cstr(M_AS_TYPE(string_srcptr,x))        \
          , default: x )

#endif

/* Internal Macro: Provide GET_STR method to enum type */
#undef M_GET_STR_METHOD_FOR_ENUM_TYPE
#define M_GET_STR_METHOD_FOR_ENUM_TYPE GET_STR(M_ENUM_GET_STR)




/***********************************************************************/
/*                                                                     */
/*                BOUNDED STRING, aka char[N+1]                        */
/*                                                                     */
/***********************************************************************/

/* Define a bounded (fixed) string of exactly 'max_size' characters
 * (excluding the final nul char).
 */
#define BOUNDED_STRING_DEF(name, max_size)                                    \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BOUNDED_STR1NG_DEF_P2(name, max_size, M_C(name, _t) )                     \
  M_END_PROTECTED_CODE

/* Define the OPLIST of a BOUNDED_STRING */
#define BOUNDED_STRING_OPLIST(name)                                           \
  (INIT(M_C(name,_init)),                                                     \
   INIT_SET(M_C(name,_init_set)),                                             \
   SET(M_C(name,_set)),                                                       \
   CLEAR(M_C(name,_clear)),                                                   \
   NAME(name),                                                                \
   INIT_WITH( API_1(M_BOUNDED_STR1NG_INIT_WITH)),                             \
   HASH(M_C(name,_hash)),                                                     \
   EQUAL(M_C(name,_equal_p)),                                                 \
   CMP(M_C(name,_cmp)),                                                       \
   TYPE(M_C(name,_ct)),                                                       \
   OOR_EQUAL(M_C(name,_oor_equal_p)),                                         \
   OOR_SET(M_C(name, _oor_set)),                                              \
   PARSE_STR(M_C(name,_parse_str)),                                           \
   GET_STR(M_C(name,_get_str)),                                               \
   OUT_STR(M_C(name,_out_str)),                                               \
   IN_STR(M_C(name,_in_str)),                                                 \
   OUT_SERIAL(M_C(name,_out_serial)),                                         \
   IN_SERIAL(M_C(name,_in_serial)),                                           \
   )

/************************** INTERNAL ***********************************/

/* Contract of a bounded string.
 * A bounded string last characters is always zero. */
#define M_BOUNDED_STR1NG_CONTRACT(var, max_size) do {                         \
    M_ASSERT(var != NULL);                                                    \
    M_ASSERT(var->s[max_size] == 0);                                          \
  } while (0)

/* Expand the functions for a bounded string */
#define M_BOUNDED_STR1NG_DEF_P2(name, max_size, bounded_t)                    \
                                                                              \
  /* Define of an array with one more to store the final nul char */          \
  typedef char M_C(name, _array_t)[max_size+1];                               \
                                                                              \
  typedef struct M_C(name, _s) {                                              \
    char s[max_size+1];                                                       \
  } bounded_t[1];                                                             \
                                                                              \
  /* Internal types for oplist */                                             \
  typedef bounded_t M_C(name, _ct);                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _init)(bounded_t s)                                               \
  {                                                                           \
    M_ASSERT(s != NULL);                                                      \
    M_STATIC_ASSERT(max_size >= 1, M_LIB_INTERNAL, "M*LIB: max_size parameter shall be greater than 0."); \
    s->s[0] = 0;                                                              \
    s->s[max_size] = 0;                                                       \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _clear)(bounded_t s)                                              \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* nothing to do */                                                       \
    (void) s;                                                                 \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _reset)(bounded_t s)                                              \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    s->s[0] = 0;                                                              \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_C(name, _size)(const bounded_t s)                                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return strlen(s->s);                                                      \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_C(name, _capacity)(const bounded_t s)                                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    (void)s; /* unused */                                                     \
    return max_size+1;                                                        \
  }                                                                           \
                                                                              \
  static inline char                                                          \
  M_C(name, _get_char)(const bounded_t s, size_t index)                       \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT_INDEX(index, max_size);                                          \
    return s->s[index];                                                       \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _empty_p)(const bounded_t s)                                      \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return s->s[0] == 0;                                                      \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _set_str)(bounded_t s, const char str[])                          \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    m_core_strncpy(s->s, max_size+1, str, max_size);                          \
    s->s[max_size] = 0;                                                       \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _set_strn)(bounded_t s, const char str[], size_t n)               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    size_t len = M_MIN(max_size, n);                                          \
    m_core_strncpy(s->s, max_size+1, str, len);                               \
    s->s[len] = 0;                                                            \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  static inline const char *                                                  \
  M_C(name, _get_cstr)(const bounded_t s)                                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return s->s;                                                              \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _set)(bounded_t s, const bounded_t str)                           \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_C(name, _set_str)(s, str->s);                                           \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _set_n)(bounded_t s, const bounded_t str,                         \
                    size_t offset, size_t length)                             \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_ASSERT_INDEX (offset, max_size+1);                                      \
    M_C(name, _set_strn)(s, str->s+offset, length);                           \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _init_set)(bounded_t s, const bounded_t str)                      \
  {                                                                           \
    M_C(name,_init)(s);                                                       \
    M_C(name,_set)(s, str);                                                   \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _init_set_str)(bounded_t s, const char str[])                     \
  {                                                                           \
    M_C(name,_init)(s);                                                       \
    M_C(name,_set_str)(s, str);                                               \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _cat_str)(bounded_t s, const char str[])                          \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT (str != NULL);                                                   \
    M_ASSERT_INDEX (strlen(s->s), max_size+1);                                \
    m_core_strncat(s->s, max_size+1, str, max_size-strlen(s->s));             \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _cat)(bounded_t s, const bounded_t  str)                          \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_C(name, _cat_str)(s, str->s);                                           \
  }                                                                           \
                                                                              \
  static inline int                                                           \
  M_C(name, _cmp_str)(const bounded_t s, const char str[])                    \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    return strcmp(s->s, str);                                                 \
  }                                                                           \
                                                                              \
  static inline int                                                           \
  M_C(name, _cmp)(const bounded_t s, const bounded_t str)                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    return strcmp(s->s, str->s);                                              \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _equal_str_p)(const bounded_t s, const char str[])                \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    return strcmp(s->s, str) == 0;                                            \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _equal_p)(const bounded_t s, const bounded_t str)                 \
  {                                                                           \
    /* _equal_p may be called in context OOR. So contract cannot be verified */ \
    return (s->s[max_size] == str->s[max_size]) & (strcmp(s->s, str->s) == 0); \
  }                                                                           \
                                                                              \
  static inline int                                                           \
  M_C(name, _printf)(bounded_t s, const char format[], ...)                   \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(format != NULL);                                                 \
    va_list args;                                                             \
    int ret;                                                                  \
    va_start (args, format);                                                  \
    ret = vsnprintf (s->s, max_size+1, format, args);                         \
    va_end (args);                                                            \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline int                                                           \
  M_C(name, _cat_printf)(bounded_t s, const char format[], ...)               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(format != NULL);                                                 \
    va_list args;                                                             \
    int ret;                                                                  \
    va_start (args, format);                                                  \
    size_t length = strlen(s->s);                                             \
    M_ASSERT(length <= max_size);                                             \
    ret = vsnprintf (&s->s[length], max_size+1-length, format, args);         \
    va_end (args);                                                            \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _fgets)(bounded_t s, FILE *f, string_fgets_t arg)                 \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    M_ASSERT(arg != STRING_READ_FILE);                                        \
    char *ret = fgets(s->s, max_size+1, f);                                   \
    s->s[max_size] = 0;                                                       \
    if (ret != NULL && arg == STRING_READ_PURE_LINE) {                        \
      size_t length = strlen(s->s);                                           \
      if (length > 0 && s->s[length-1] == '\n')                               \
        s->s[length-1] = 0;                                                   \
    }                                                                         \
    return ret != NULL;                                                       \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _fputs)(FILE *f, const bounded_t s)                               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    return fputs(s->s, f) >= 0;                                               \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_C(name, _hash)(const bounded_t s)                                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* Cannot use m_core_hash: alignment not sufficent */                     \
    M_HASH_DECL(hash);                                                        \
    const char *str = s->s;                                                   \
    while (*str) {                                                            \
      size_t h = (size_t) *str++;                                             \
      M_HASH_UP(hash, h);                                                     \
    }                                                                         \
    return M_HASH_FINAL(hash);                                                \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _oor_equal_p)(const bounded_t s, unsigned char n)                 \
  {                                                                           \
    /* s may be invalid contract */                                           \
    M_ASSERT (s != NULL);                                                     \
    M_ASSERT ( (n == 0) || (n == 1));                                         \
    return s->s[max_size] == (char) (n+1);                                    \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _oor_set)(bounded_t s, unsigned char n)                           \
  {                                                                           \
    /* s may be invalid contract */                                           \
    M_ASSERT (s != NULL);                                                     \
    M_ASSERT ( (n == 0) || (n == 1));                                         \
    s->s[max_size] = (char) (n+1);                                            \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _get_str)(string_t v, const bounded_t s, bool append)             \
  {                                                                           \
    M_STR1NG_CONTRACT(v);                                                     \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* Build dummy string to reuse string_get_str */                          \
    uintptr_t ptr = (uintptr_t) &s->s[0];                                     \
    string_t v2;                                                              \
    v2->u.heap.size = strlen(s->s);                                           \
    v2->u.heap.alloc = v2->u.heap.size + 1;                                   \
    v2->ptr = (char*)ptr;                                                     \
    string_get_str(v, v2, append);                                            \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_C(name, _out_str)(FILE *f, const bounded_t s)                             \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    /* Build dummy string to reuse string_get_str */                          \
    uintptr_t ptr = (uintptr_t) &s->s[0];                                     \
    string_t v2;                                                              \
    v2->u.heap.size = strlen(s->s);                                           \
    v2->u.heap.alloc = v2->u.heap.size + 1;                                   \
    v2->ptr = (char*)ptr;                                                     \
    string_out_str(f, v2);                                                    \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _in_str)(bounded_t v, FILE *f)                                    \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    string_t v2;                                                              \
    string_init(v2);                                                          \
    bool ret = string_in_str(v2, f);                                          \
    m_core_strncpy(v->s, max_size+1, string_get_cstr(v2), max_size);          \
    string_clear(v2);                                                         \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_C(name, _parse_str)(bounded_t v, const char str[], const char **endptr)   \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    string_t v2;                                                              \
    string_init(v2);                                                          \
    bool ret = string_parse_str(v2, str, endptr);                             \
    m_core_strncpy(v->s, max_size+1, string_get_cstr(v2), max_size);          \
    string_clear(v2);                                                         \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline m_serial_return_code_t                                        \
  M_C(name, _out_serial)(m_serial_write_t serial, const bounded_t v)          \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT (serial != NULL && serial->m_interface != NULL);                 \
    return serial->m_interface->write_string(serial, v->s, strlen(v->s) );    \
  }                                                                           \
                                                                              \
  static inline m_serial_return_code_t                                        \
  M_C(name, _in_serial)(bounded_t v, m_serial_read_t serial)                  \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT (serial != NULL && serial->m_interface != NULL);                 \
    string_t tmp;                                                             \
    /* TODO: Not optimum */                                                   \
    string_init(tmp);                                                         \
    m_serial_return_code_t r = serial->m_interface->read_string(serial, tmp); \
    m_core_strncpy(v->s, max_size+1, string_get_cstr(tmp), max_size);         \
    string_clear(tmp);                                                        \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    return r;                                                                 \
  }


   
/* Init a constant bounded string.
   Try to do a clean cast */
/* Use of Compound Literals to init a constant string.
   See above */
#ifndef __cplusplus
#define BOUNDED_STRING_CTE(name, string)                                      \
  ((const struct M_C(name, _s) *)((M_C(name, _array_t)){string}))
#else
namespace m_lib {
  template <unsigned int N>
    struct m_bounded_string {
      char s[N];
      inline m_bounded_string(const char lstr[])
      {
        memset(this->s, 0, N);
        m_core_strncpy(this->s, N, lstr, N-1);
      }
    };
}
#define BOUNDED_STRING_CTE(name, string)                                      \
  ((const struct M_C(name, _s) *)(m_lib::m_bounded_string<sizeof (M_C(name, _t))>(string).s))
#endif


/* Initialize a bounded string with the given list of arguments.
   Check if it is a formatted input or not by counting the number of arguments.
   If there is only one argument, it can only be a set to C string.
   It is much faster in this case to call string_init_set_str.
*/
#define M_BOUNDED_STR1NG_INIT_WITH(oplist, v, ...)                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(M_C(M_GET_NAME oplist, _init_set_str)(v, __VA_ARGS__), M_BOUNDED_STR1NG_INIT_PRINTF(oplist, v, __VA_ARGS__))

#define M_BOUNDED_STR1NG_INIT_PRINTF(oplist, v, ...)                          \
  (M_GET_INIT oplist (v), M_C(M_GET_NAME oplist, _printf)(v, __VA_ARGS__))

M_END_PROTECTED_CODE

#endif
