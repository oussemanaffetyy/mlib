/*
 * M*LIB - VARIANT module
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
#ifndef MSTARLIB_VARIANT_H
#define MSTARLIB_VARIANT_H

#include "m-core.h"


/********************** External interface *************************/

/* Define the variant type and functions.
   USAGE:
     VARIANT_DEF2(name, [(field1, type1, oplist1), (field2, type2, oplist2), ...] ) */
#define VARIANT_DEF2(name, ...)                                               \
  VARIANT_DEF2_AS(name, M_C(name,_t), __VA_ARGS__)


/* Define the variant type and functions
  as the given name_t
   USAGE:
     VARIANT_DEF2_AS(name, name_t, [(field1, type1, oplist1), (field2, type2, oplist2), ...] ) */
#define VARIANT_DEF2_AS(name, name_t, ...)                                    \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_VAR1ANT_DEF2_P1( (name, name_t M_VAR1ANT_INJECT_GLOBAL(__VA_ARGS__)) )    \
  M_END_PROTECTED_CODE


/* Define the oplist of a variant.
   USAGE: VARIANT_OPLIST(name[, oplist of the first type, ...]) */
#define VARIANT_OPLIST(...)                                                   \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_VAR1ANT_OPLIST_P1((__VA_ARGS__, M_DEFAULT_OPLIST)),                      \
   M_VAR1ANT_OPLIST_P1((__VA_ARGS__ )))


/********************************** INTERNAL ************************************/

/* Contract of a variant. */
#define M_VAR1ANT_CONTRACT(name, my) do {                                     \
  M_ASSERT(my != NULL);                                                       \
  M_ASSERT(my->type >= M_C(name, _EMPTY));                                    \
  M_ASSERT(my->type <= (enum M_C(name, _enum)) M_C(name, _MAX_TYPE));         \
} while (0)

/* Inject the oplist within the list of arguments */
#define M_VAR1ANT_INJECT_GLOBAL(...)                                          \
  M_MAP(M_VAR1ANT_INJECT_OPLIST_A, __VA_ARGS__)

/* Transform (x, type) into (x, type, oplist) if there is global registered oplist 
   or (x, type, M_DEFAULT_OPLIST) if there is no global one,
   or keep (x, type, oplist) if oplist was already present */
#define M_VAR1ANT_INJECT_OPLIST_A( duo_or_trio )                              \
  M_VAR1ANT_INJECT_OPLIST_B duo_or_trio

#define M_VAR1ANT_INJECT_OPLIST_B( f, ... )                                   \
  M_DEFERRED_COMMA                                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)( (f, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)()), (f, __VA_ARGS__) )

// Deferred evaluation
#define M_VAR1ANT_DEF2_P1(...)                  M_VAR1ANT_DEF2_P2 __VA_ARGS__

// Test if all third argument of all arguments is an oplist
#define M_VAR1ANT_IF_ALL_OPLIST(...)                                          \
  M_IF(M_REDUCE(M_VAR1ANT_IS_OPLIST_P, M_AND, __VA_ARGS__))

// Test if the third argument is an oplist
#define M_VAR1ANT_IS_OPLIST_P(a)                                              \
  M_OPLIST_P(M_RET_ARG3 a)

/* Validate the oplist before going further */
#define M_VAR1ANT_DEF2_P2(name, name_t, ...)                                  \
  M_VAR1ANT_IF_ALL_OPLIST(__VA_ARGS__)(M_VAR1ANT_DEF2_P3, M_VAR1ANT_DEF2_FAILURE)(name, name_t, __VA_ARGS__)

/* Stop processing with a compilation failure */
#define M_VAR1ANT_DEF2_FAILURE(name, name_t, ...)                             \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(VARIANT_DEF2): at least one of the given argument is not a valid oplist: " #__VA_ARGS__)

/* Define the variant */
#define M_VAR1ANT_DEF2_P3(name, name_t, ...)                                  \
  M_VAR1ANT_DEFINE_TYPE(name, name_t, __VA_ARGS__)                            \
  M_VAR1ANT_CONTROL_ALL_OPLIST(name, __VA_ARGS__)                             \
  M_VAR1ANT_DEFINE_INIT(name, __VA_ARGS__)                                    \
  M_VAR1ANT_DEFINE_CLEAR(name, __VA_ARGS__)                                   \
  M_VAR1ANT_DEFINE_INIT_SET(name, __VA_ARGS__)                                \
  M_VAR1ANT_DEFINE_SET(name, __VA_ARGS__)                                     \
  M_VAR1ANT_DEFINE_TEST_P(name, __VA_ARGS__)                                  \
  M_VAR1ANT_IF_ALL(INIT, __VA_ARGS__)                                         \
  (M_VAR1ANT_DEFINE_INIT_FIELD(name, __VA_ARGS__),)                           \
  M_VAR1ANT_DEFINE_INIT_SETTER_FIELD(name, __VA_ARGS__)                       \
  M_VAR1ANT_DEFINE_SETTER_FIELD(name, __VA_ARGS__)                            \
  M_VAR1ANT_DEFINE_GETTER_FIELD(name, __VA_ARGS__)                            \
  M_VAR1ANT_DEFINE_RESET_FUNC(name, __VA_ARGS__)                              \
  M_VAR1ANT_IF_ALL(HASH, __VA_ARGS__)                                         \
  (M_VAR1ANT_DEFINE_HASH(name, __VA_ARGS__),)                                 \
  M_VAR1ANT_IF_ALL(EQUAL, __VA_ARGS__)                                        \
  (M_VAR1ANT_DEFINE_EQUAL(name, __VA_ARGS__),)                                \
  M_VAR1ANT_IF_ALL(GET_STR, __VA_ARGS__)                                      \
  (M_VAR1ANT_DEFINE_GET_STR(name, __VA_ARGS__),)                              \
  M_VAR1ANT_IF_ALL2(PARSE_STR, INIT, __VA_ARGS__)                             \
  (M_VAR1ANT_DEFINE_PARSE_STR(name, __VA_ARGS__),)                            \
  M_VAR1ANT_IF_ALL(OUT_STR, __VA_ARGS__)                                      \
  (M_VAR1ANT_DEFINE_OUT_STR(name, __VA_ARGS__),)                              \
  M_VAR1ANT_IF_ALL2(IN_STR, INIT, __VA_ARGS__)                                \
  (M_VAR1ANT_DEFINE_IN_STR(name, __VA_ARGS__),)                               \
  M_VAR1ANT_IF_ALL(OUT_SERIAL, __VA_ARGS__)                                   \
  (M_VAR1ANT_DEFINE_OUT_SERIAL(name, __VA_ARGS__),)                           \
  M_VAR1ANT_IF_ALL2(IN_SERIAL, INIT, __VA_ARGS__)                             \
  (M_VAR1ANT_DEFINE_IN_SERIAL(name, __VA_ARGS__),)                            \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)                                    \
  (M_VAR1ANT_DEFINE_INIT_MOVE(name, __VA_ARGS__),)                            \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)                                    \
  (M_VAR1ANT_DEFINE_MOVE(name, __VA_ARGS__),)                                 \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)                                    \
  (M_VAR1ANT_DEFINE_MOVER(name, __VA_ARGS__),)                                \
  M_VAR1ANT_IF_ALL(SWAP, __VA_ARGS__)                                         \
  (M_VAR1ANT_DEFINE_SWAP(name, __VA_ARGS__),)


/* Get the field name, the type, the oplist or the methods
   based on the variant (field, type, oplist) */
#define M_VAR1ANT_GET_FIELD(f,t,o)    f
#define M_VAR1ANT_GET_TYPE(f,t,o)     t
#define M_VAR1ANT_GET_OPLIST(f,t,o)   o
#define M_VAR1ANT_GET_INIT(f,t,o)     M_GET_INIT o
#define M_VAR1ANT_GET_INIT_SET(f,t,o) M_GET_INIT_SET o
#define M_VAR1ANT_GET_INIT_MOVE(f,t,o) M_GET_INIT_MOVE o
#define M_VAR1ANT_GET_MOVE(f,t,o)     M_GET_MOVE o
#define M_VAR1ANT_GET_SET(f,t,o)      M_GET_SET o
#define M_VAR1ANT_GET_CLEAR(f,t,o)    M_GET_CLEAR o
#define M_VAR1ANT_GET_CMP(f,t,o)      M_GET_CMP o
#define M_VAR1ANT_GET_HASH(f,t,o)     M_GET_HASH o
#define M_VAR1ANT_GET_EQUAL(f,t,o)    M_GET_EQUAL o
#define M_VAR1ANT_GET_STR(f,t,o)      M_GET_GET_STR o
#define M_VAR1ANT_GET_PARSE_STR(f,t,o) M_GET_PARSE_STR o
#define M_VAR1ANT_GET_OUT_STR(f,t,o)  M_GET_OUT_STR o
#define M_VAR1ANT_GET_IN_STR(f,t,o)   M_GET_IN_STR o
#define M_VAR1ANT_GET_OUT_SERIAL(f,t,o) M_GET_OUT_SERIAL o
#define M_VAR1ANT_GET_IN_SERIAL(f,t,o)  M_GET_IN_SERIAL o
#define M_VAR1ANT_GET_SWAP(f,t,o)     M_GET_SWAP o

/* Call the methods through API */
#define M_VAR1ANT_CALL_INIT(t, ...)       M_APPLY_API(M_VAR1ANT_GET_INIT t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_INIT_SET(t, ...)   M_APPLY_API(M_VAR1ANT_GET_INIT_SET t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_INIT_MOVE(t, ...)  M_APPLY_API(M_VAR1ANT_GET_INIT_MOVE t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_MOVE(t, ...)       M_APPLY_API(M_VAR1ANT_GET_MOVE t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_SET(t, ...)        M_APPLY_API(M_VAR1ANT_GET_SET t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_CLEAR(t, ...)      M_APPLY_API(M_VAR1ANT_GET_CLEAR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_CMP(t, ...)        M_APPLY_API(M_VAR1ANT_GET_CMP t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_HASH(t, ...)       M_APPLY_API(M_VAR1ANT_GET_HASH t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_EQUAL(t, ...)      M_APPLY_API(M_VAR1ANT_GET_EQUAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_GET_STR(t, ...)    M_APPLY_API(M_VAR1ANT_GET_STR t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_PARSE_STR(t, ...)  M_APPLY_API(M_VAR1ANT_GET_PARSE_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_OUT_STR(t, ...)    M_APPLY_API(M_VAR1ANT_GET_OUT_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_IN_STR(t, ...)     M_APPLY_API(M_VAR1ANT_GET_IN_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_OUT_SERIAL(t, ...) M_APPLY_API(M_VAR1ANT_GET_OUT_SERIAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_IN_SERIAL(t, ...)  M_APPLY_API(M_VAR1ANT_GET_IN_SERIAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_SWAP(t, ...)       M_APPLY_API(M_VAR1ANT_GET_SWAP t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)

/* Define the type */
#define M_VAR1ANT_DEFINE_TYPE(name, name_t, ...)                              \
  /* Define enum of all types of the variant */                               \
  enum M_C(name, _enum) { M_C(name, _EMPTY)                                   \
      M_MAP2(M_VAR1ANT_DEFINE_UNION_ELE, name, __VA_ARGS__)                   \
  };                                                                          \
  /* Define enum equal to the number of types of the variant */               \
  enum M_C(name, _enum_max) {                                                 \
    M_C(name, _MAX_TYPE) = M_NARGS(__VA_ARGS__)                               \
  };                                                                          \
  /* Define the variant */                                                    \
  typedef struct M_C(name, _s) {                                              \
    enum M_C(name, _enum) type;                                               \
    union {                                                                   \
      M_MAP(M_VAR1ANT_DEFINE_TYPE_ELE , __VA_ARGS__)                          \
    } value;                                                                  \
  } name_t[1];                                                                \
                                                                              \
  typedef struct M_C(name, _s) *M_C(name, _ptr);                              \
  typedef const struct M_C(name, _s) *M_C(name, _srcptr);                     \
  /* Define internal type for oplist */                                       \
  typedef name_t M_C(name, _ct);

#define M_VAR1ANT_DEFINE_UNION_ELE(name, a)                                   \
  , M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value)

#define M_VAR1ANT_DEFINE_TYPE_ELE(a)                                          \
  M_VAR1ANT_GET_TYPE a M_VAR1ANT_GET_FIELD a ;


/* Control that all given oplists of all parameters are really oplists */
#define M_VAR1ANT_CONTROL_ALL_OPLIST(name, ...)                               \
  M_MAP2(M_VAR1ANT_CONTROL_OPLIST, name, __VA_ARGS__)

#define M_VAR1ANT_CONTROL_OPLIST(name, a)                                     \
  M_CHECK_COMPATIBLE_OPLIST(name, M_VAR1ANT_GET_FIELD a,                      \
                            M_VAR1ANT_GET_TYPE a, M_VAR1ANT_GET_OPLIST a)


/* Define the INIT function. Init the variant to empty */
#define M_VAR1ANT_DEFINE_INIT(name, ...)                                      \
  static inline void M_C(name, _init)(M_C(name,_ct) my) {                     \
    my->type = M_C(name, _EMPTY);                                             \
  }


/* Define the INIT_SET function. */
#define M_VAR1ANT_DEFINE_INIT_SET(name, ...)                                  \
  static inline void M_C(name, _init_set)(M_C(name,_ct) my ,                  \
                                          M_C(name,_ct) const org) {          \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    my->type = org->type;                                                     \
    switch (org->type) {                                                      \
    case M_C(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_INIT_SET_FUNC, name, __VA_ARGS__)               \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
  }

#define M_VAR1ANT_DEFINE_INIT_SET_FUNC(name, a)                               \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a ,             \
                         org -> value.M_VAR1ANT_GET_FIELD a );                \
  break;


/* Define the SET function. */
#define M_VAR1ANT_DEFINE_SET(name, ...)                                       \
  static inline void M_C(name, _set)(M_C(name,_ct) my ,                       \
                                     M_C(name,_ct) const org) {               \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    if (my->type != org->type) {                                              \
      /* Different types: clear previous one and create new */                \
      M_C(name, _clear)(my);                                                  \
      M_C(name, _init_set)(my, org);                                          \
    } else {                                                                  \
      /* Same type: optimize the set */                                       \
      switch (org->type) {                                                    \
      case M_C(name, _EMPTY): break;                                          \
        M_MAP2(M_VAR1ANT_DEFINE_SET_FUNC, name, __VA_ARGS__)                  \
      default: M_ASSERT(false); break;                                        \
      }                                                                       \
    }                                                                         \
  }

#define M_VAR1ANT_DEFINE_SET_FUNC(name, a)                                    \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_SET(a, my -> value. M_VAR1ANT_GET_FIELD a ,                  \
                    org -> value.M_VAR1ANT_GET_FIELD a );                     \
  break;


/* Define the CLEAR function. */
#define M_VAR1ANT_DEFINE_CLEAR(name, ...)                                     \
  static inline void M_C(name, _clear)(M_C(name,_ct) my) {                    \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    switch (my->type) {                                                       \
    case M_C(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_CLEAR_FUNC, name,  __VA_ARGS__)                 \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    my->type = M_C(name, _EMPTY);                                             \
  }

#define M_VAR1ANT_DEFINE_CLEAR_FUNC(name, a)                                  \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_CLEAR(a, my -> value. M_VAR1ANT_GET_FIELD a);                \
  break;


/* Define the TEST_P function. */
#define M_VAR1ANT_DEFINE_TEST_P(name, ...)                                    \
  static inline bool M_C(name, _empty_p)(M_C(name,_ct) const my) {            \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type == M_C(name, _EMPTY);                                     \
  }                                                                           \
  static inline enum M_C(name, _enum)                                         \
  M_C(name, _type)(M_C(name,_ct) my) {                                        \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type;                                                          \
  }                                                                           \
  M_MAP2(M_VAR1ANT_DEFINE_TEST_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_TEST_FUNC(name, a)                                   \
  static inline bool                                                          \
  M_C4(name, _, M_VAR1ANT_GET_FIELD a, _p)(M_C(name,_ct) const my) {          \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type == M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);          \
  }


/* Define the INIT function. */
#define M_VAR1ANT_DEFINE_INIT_FIELD(name, ...)                                \
  M_MAP2(M_VAR1ANT_DEFINE_INIT_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_INIT_FIELD_FUNC(name, a)                             \
  static inline void                                                          \
  M_C3(name, _init_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) my) {               \
    /* Reinit variable with the given value */                                \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, my -> value. M_VAR1ANT_GET_FIELD a);               \
  }


/* Define the INIT_SET of a given type function. */
#define M_VAR1ANT_DEFINE_INIT_SETTER_FIELD(name, ...)                         \
  M_MAP2(M_VAR1ANT_DEFINE_INIT_SETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_INIT_SETTER_FIELD_FUNC(name, a)                      \
  static inline void                                                          \
  M_C3(name, _init_set_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) my,             \
                                               M_VAR1ANT_GET_TYPE a const M_VAR1ANT_GET_FIELD a  ) { \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,            \
                           M_VAR1ANT_GET_FIELD a);                            \
  }


/* Define the SET of a given type function. */
#define M_VAR1ANT_DEFINE_SETTER_FIELD(name, ...)                              \
  M_MAP2(M_VAR1ANT_DEFINE_SETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_SETTER_FIELD_FUNC(name, a)                           \
  static inline void                                                          \
  M_C3(name, _set_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) my,                  \
                                          M_VAR1ANT_GET_TYPE a const M_VAR1ANT_GET_FIELD a  ) { \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type == M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      M_VAR1ANT_CALL_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,               \
                        M_VAR1ANT_GET_FIELD a);                               \
    } else {                                                                  \
      M_C(name, _clear)(my);                                                  \
      /* Reinit variable with the given value */                              \
      my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                \
      M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,          \
                             M_VAR1ANT_GET_FIELD a);                          \
    }                                                                         \
  }


/* Define the GET_field of a given type function. */
#define M_VAR1ANT_DEFINE_GETTER_FIELD(name, ...)                              \
  M_MAP2(M_VAR1ANT_DEFINE_GETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_GETTER_FIELD_FUNC(name, a)                           \
  static inline M_VAR1ANT_GET_TYPE a *                                        \
  M_C3(name, _get_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) my) {                \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type != M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      return NULL;                                                            \
    }                                                                         \
    return &my -> value . M_VAR1ANT_GET_FIELD a;                              \
  }                                                                           \
                                                                              \
  static inline M_VAR1ANT_GET_TYPE a const *                                  \
  M_C3(name, _cget_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) const my) {         \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type != M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      return NULL;                                                            \
    }                                                                         \
    return &my -> value . M_VAR1ANT_GET_FIELD a;                              \
  }


/* Define the EQUAL_P function. */
#define M_VAR1ANT_DEFINE_EQUAL(name, ...)                                     \
  static inline bool M_C(name, _equal_p)(M_C(name,_ct) const e1 ,             \
                                         M_C(name,_ct) const e2) {            \
    bool b;                                                                   \
    M_VAR1ANT_CONTRACT(name, e1);                                             \
    M_VAR1ANT_CONTRACT(name, e2);                                             \
    if (e1->type != e2->type) return false;                                   \
    switch (e1->type) {                                                       \
    case M_C(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_EQUAL_FUNC , name, __VA_ARGS__)                 \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    return true;                                                              \
  }

#define M_VAR1ANT_DEFINE_EQUAL_FUNC(name, a)                                  \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  b = M_VAR1ANT_CALL_EQUAL(a, e1 -> value . M_VAR1ANT_GET_FIELD a ,           \
                          e2 -> value . M_VAR1ANT_GET_FIELD a );              \
  return b;                                                                   \
  break;


/* Define the HASH function. */
#define M_VAR1ANT_DEFINE_HASH(name, ...)                                      \
  static inline size_t M_C(name, _hash)(M_C(name,_ct) const e1) {             \
    M_VAR1ANT_CONTRACT(name, e1);                                             \
    M_HASH_DECL(hash);                                                        \
    M_HASH_UP (hash, (unsigned int) (e1 -> type));                            \
    switch (e1->type) {                                                       \
    case M_C(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_HASH_FUNC , name, __VA_ARGS__)                  \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    return M_HASH_FINAL (hash);                                               \
  }

#define M_VAR1ANT_DEFINE_HASH_FUNC(name, a)                                   \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_HASH_UP(hash, M_VAR1ANT_CALL_HASH(a, e1 -> value . M_VAR1ANT_GET_FIELD a) ); \
  break;


/* Define the INIT_MOVE function. */
#define M_VAR1ANT_DEFINE_INIT_MOVE(name, ...)                                 \
  static inline void                                                          \
  M_C(name, _init_move)(M_C(name,_ct) el, M_C(name,_ct) org) {                \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    el -> type = org -> type;                                                 \
    switch (el->type) {                                                       \
    case M_C(name, _EMPTY): break;                                            \
    M_MAP2(M_VAR1ANT_DEFINE_INIT_MOVE_FUNC , name, __VA_ARGS__)               \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    org -> type = M_C(name, _EMPTY);                                          \
  }

#define M_VAR1ANT_DEFINE_INIT_MOVE_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_INIT_MOVE(a, el -> value . M_VAR1ANT_GET_FIELD a,            \
                          org -> value . M_VAR1ANT_GET_FIELD a);              \
  break;


/* Define the MOVE function.
   This is not optimized version.
   It can be optimized if both types are the same.
*/
#define M_VAR1ANT_DEFINE_MOVE(name, ...)                                      \
  static inline void                                                          \
  M_C(name, _move)(M_C(name,_ct) el, M_C(name,_ct) org) {                     \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    M_C(name, _clear)(el);                                                    \
    M_C(name, _init_move)(el , org);                                          \
  }


/* Define the MOVE function of a given type */
#define M_VAR1ANT_DEFINE_MOVER(name, ...)                                     \
  M_MAP2(M_VAR1ANT_DEFINE_MOVER_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_MOVER_FUNC(name, a)                                  \
  static inline void                                                          \
  M_C3(name, _move_, M_VAR1ANT_GET_FIELD a)(M_C(name,_ct) my,                 \
                                           M_VAR1ANT_GET_TYPE a  M_VAR1ANT_GET_FIELD a  ) { \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_C(name, _clear)(my);                                                    \
    /* Reinit variable with the given value */                                \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT_MOVE(a, my -> value. M_VAR1ANT_GET_FIELD a,           \
                            M_VAR1ANT_GET_FIELD a);                           \
  }


/* Define the SWAP function  */
#define M_VAR1ANT_DEFINE_SWAP(name, ...)                                      \
  static inline void                                                          \
  M_C(name, _swap)(M_C(name,_ct) el1, M_C(name,_ct) el2) {                    \
    M_VAR1ANT_CONTRACT(name, el1);                                            \
    M_VAR1ANT_CONTRACT(name, el2);                                            \
    if (el1->type == el2->type) {                                             \
      switch (el1->type) {                                                    \
      case M_C(name, _EMPTY): break;                                          \
        M_MAP2(M_VAR1ANT_DEFINE_INIT_SWAP_FUNC , name, __VA_ARGS__)           \
      default: M_ASSERT(false); break;                                        \
      }                                                                       \
    } else {                                                                  \
      M_C(name,_ct) tmp;                                                      \
      M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)                                \
        (      /* NOTE: Slow implementation */                                \
         M_C(name, _init_move)(tmp, el1);                                     \
         M_C(name, _init_move)(el1, el2);                                     \
         M_C(name, _init_move)(el2, tmp);                                     \
         ,                                                                    \
         /* NOTE: Very slow implementation */                                 \
         M_C(name, _init_set)(tmp, el1);                                      \
         M_C(name, _set)(el1, el2);                                           \
         M_C(name, _set)(el2, tmp);                                           \
         M_C(name, _clear)(tmp);                                              \
               )                                                              \
    }                                                                         \
  }

#define M_VAR1ANT_DEFINE_INIT_SWAP_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_SWAP(a, el1 -> value . M_VAR1ANT_GET_FIELD a,                \
                     el2 -> value . M_VAR1ANT_GET_FIELD a);                   \
  break;


/* Define the GET_STR function */
#define M_VAR1ANT_DEFINE_GET_STR(name, ...)                                   \
  static inline void M_C(name, _get_str)(string_t str,                        \
                                         M_C(name,_ct) const el,              \
                                         bool append) {                       \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (str != NULL);                                                   \
    void (*func)(string_t, const char *);                                     \
    func = append ? string_cat_str : string_set_str;                          \
    switch (el->type) {                                                       \
    case M_C(name, _EMPTY): func(str, "@EMPTY@"); break;                      \
      M_MAP2(M_VAR1ANT_DEFINE_GET_STR_FUNC , name, __VA_ARGS__)               \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    string_push_back (str, '@');                                              \
  }

#define M_VAR1ANT_DEFINE_GET_STR_FUNC(name, a)                                \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  func(str, "@" M_AS_STR(M_VAR1ANT_GET_FIELD a) "@");                         \
  M_VAR1ANT_CALL_GET_STR(a, str, el -> value . M_VAR1ANT_GET_FIELD a, true);  \
  break;


/* Define the PARSE_STR function */
#define M_VAR1ANT_DEFINE_PARSE_STR(name, ...)                                 \
  static inline bool M_C(name, _parse_str)(M_C(name,_ct) el,                  \
                                           const char str[],                  \
                                           const char **endp) {               \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (str != NULL);                                                   \
    bool success = false;                                                     \
    char variantTypeBuf[M_USE_IDENTIFIER_ALLOC+1];                            \
    int  c = *str++;                                                          \
    unsigned int i = 0;                                                       \
    M_C(name, _reset)(el);                                                    \
    if (c != '@') goto exit;                                                  \
    /* First read the name of the type */                                     \
    c = *str++;                                                               \
    while (c != '@' && c != 0 && i < sizeof(variantTypeBuf) - 1) {            \
      variantTypeBuf[i++] = (char) c;                                         \
      c = *str++;                                                             \
    }                                                                         \
    if (c != '@') goto exit;                                                  \
    variantTypeBuf[i++] = 0;                                                  \
    M_ASSERT(i < sizeof(variantTypeBuf));                                     \
    /* In function of the type */                                             \
    if (strcmp(variantTypeBuf, "EMPTY") == 0) {                               \
      el->type = M_C(name, _EMPTY);                                           \
    }                                                                         \
    M_MAP2(M_VAR1ANT_DEFINE_PARSE_STR_FUNC , name, __VA_ARGS__)               \
    else goto exit;                                                           \
    success = (*str++ == '@');                                                \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    return success;                                                           \
  }

#define M_VAR1ANT_DEFINE_PARSE_STR_FUNC(name, a)                              \
  else if (strcmp (variantTypeBuf, M_AS_STR(M_VAR1ANT_GET_FIELD a)) == 0) {   \
    el->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    bool b = M_VAR1ANT_CALL_PARSE_STR(a, el -> value . M_VAR1ANT_GET_FIELD a, str, &str); \
    if (!b) goto exit;                                                        \
  }


/* Define the OUT_STR function */
#define M_VAR1ANT_DEFINE_OUT_STR(name, ...)                                   \
  static inline void M_C(name, _out_str)(FILE *f,                             \
                                         M_C(name,_ct) const el) {            \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (f != NULL);                                                     \
    switch (el->type) {                                                       \
    case M_C(name, _EMPTY): fprintf(f, "@EMPTY@"); break;                     \
      M_MAP2(M_VAR1ANT_DEFINE_OUT_STR_FUNC , name, __VA_ARGS__)               \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    fputc ('@', f);                                                           \
  }

#define M_VAR1ANT_DEFINE_OUT_STR_FUNC(name, a)                                \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  fprintf(f, "@" M_AS_STR(M_VAR1ANT_GET_FIELD a) "@");                        \
  M_VAR1ANT_CALL_OUT_STR(a, f, el -> value . M_VAR1ANT_GET_FIELD a);          \
  break;


/* Define the IN_STR function */
#define M_VAR1ANT_DEFINE_IN_STR(name, ...)                                    \
  static inline bool M_C(name, _in_str)(M_C(name,_ct) el,                     \
                                        FILE *f) {                            \
    M_VAR1ANT_CONTRACT(name, el);                                             \
     M_ASSERT (f != NULL);                                                    \
    char variantTypeBuf[M_USE_IDENTIFIER_ALLOC+1];                            \
    M_C(name, _reset)(el);                                                    \
    if (fgetc(f) != '@') return false;                                        \
    /* First read the name of the type */                                     \
    bool b = true;                                                            \
    int c = fgetc(f);                                                         \
    unsigned int i = 0;                                                       \
    while (c != '@' && c != EOF && i < sizeof(variantTypeBuf) - 1) {          \
      variantTypeBuf[i++] = (char) c;                                         \
      c = fgetc(f);                                                           \
    }                                                                         \
    if (c != '@') return false;                                               \
    variantTypeBuf[i++] = 0;                                                  \
    M_ASSERT(i < sizeof(variantTypeBuf));                                     \
    /* In function of the type */                                             \
    if (strcmp(variantTypeBuf, "EMPTY") == 0) {                               \
      el->type = M_C(name, _EMPTY);                                           \
    }                                                                         \
    M_MAP2(M_VAR1ANT_DEFINE_IN_STR_FUNC , name, __VA_ARGS__)                  \
    else { b = false; }                                                       \
    return b && (fgetc(f) == '@');                                            \
  }

#define M_VAR1ANT_DEFINE_IN_STR_FUNC(name, a)                                 \
  else if (strcmp (variantTypeBuf, M_AS_STR(M_VAR1ANT_GET_FIELD a)) == 0) {   \
    el->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    b = M_VAR1ANT_CALL_IN_STR(a, el -> value . M_VAR1ANT_GET_FIELD a, f);     \
  }


/* Return the STRING version of a parameter name */
#define M_VAR1ANT_STRINGIFY_NAME(a)                                           \
  M_AS_STR(M_VAR1ANT_GET_FIELD a)


/* Define the OUT_SERIAL function */
#define M_VAR1ANT_DEFINE_OUT_SERIAL(name, ...)                                \
  static inline m_serial_return_code_t                                        \
  M_C(name, _out_serial)(m_serial_write_t f,                                  \
                         M_C(name,_ct) const el) {                            \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_VAR1ANT_STRINGIFY_NAME, M_ID, __VA_ARGS__) };              \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    switch (el->type) {                                                       \
    case M_C(name, _EMPTY):                                                   \
      return f->m_interface->write_variant_start(local, f, field_name, field_max, -1); \
      break;                                                                  \
    M_MAP2(M_VAR1ANT_DEFINE_OUT_SERIAL_FUNC , name, __VA_ARGS__)              \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    ret |= f->m_interface->write_variant_end(local, f);                       \
    return ret & M_SERIAL_FAIL;                                               \
  }

#define M_VAR1ANT_DEFINE_OUT_SERIAL_FUNC(name, a)                             \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  ret = f->m_interface->write_variant_start(local, f, field_name, field_max,  \
                       M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) -1);      \
  M_VAR1ANT_CALL_OUT_SERIAL(a, f, el -> value . M_VAR1ANT_GET_FIELD a);       \
  break;


/* Define the IN_SERIAL function */
#define M_VAR1ANT_DEFINE_IN_SERIAL(name, ...)                                 \
  static inline m_serial_return_code_t                                        \
  M_C(name, _in_serial)(M_C(name,_ct) el,                                     \
                        m_serial_read_t f) {                                  \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_VAR1ANT_STRINGIFY_NAME, M_ID, __VA_ARGS__) };              \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    int id = -1;                                                              \
    M_C(name, _reset)(el);                                                    \
    ret = f->m_interface->read_variant_start(local, f, field_name, field_max, &id); \
    if (ret != M_SERIAL_OK_CONTINUE) return ret;                              \
    M_ASSERT (id >= 0 && id < field_max);                                     \
    el->type = (enum M_C(name, _enum))(id+1);                                 \
    switch (id+1) {                                                           \
      M_MAP2(M_VAR1ANT_DEFINE_IN_SERIAL_FUNC , name, __VA_ARGS__)             \
    default: M_ASSERT(false); break;                                          \
    }                                                                         \
    if (ret == M_SERIAL_OK_DONE)                                              \
      ret = f->m_interface->read_variant_end(local, f);                       \
    return ret;                                                               \
  }

#define M_VAR1ANT_DEFINE_IN_SERIAL_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    ret = M_VAR1ANT_CALL_IN_SERIAL(a, el -> value . M_VAR1ANT_GET_FIELD a, f); \
    break;                                                                    \


/* Define the RESET function */
#define M_VAR1ANT_DEFINE_RESET_FUNC(name, ...)                                \
  static inline void M_C(name, _reset)(M_C(name,_ct) my)                      \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_C(name, _clear)(my);                                                    \
    M_C(name, _init)(my);                                                     \
  }                                                                           \

/* deferred evaluation of the oplist */
#define M_VAR1ANT_OPLIST_P1(arg) M_VAR1ANT_OPLIST_P2 arg

/* Validate the oplist before going further */
#define M_VAR1ANT_OPLIST_P2(name, ...)                                        \
  M_IF(M_REDUCE(M_OPLIST_P, M_AND, __VA_ARGS__))(M_VAR1ANT_OPLIST_P3, M_VAR1ANT_OPLIST_FAILURE)(name, __VA_ARGS__)

/* Prepare a clean compilation failure */
#define M_VAR1ANT_OPLIST_FAILURE(name, ...)                                   \
  ((M_LIB_ERROR(ONE_ARGUMENT_OF_VARIANT_OPLIST_IS_NOT_AN_OPLIST, name, __VA_ARGS__)))

/* Define the oplist */
#define M_VAR1ANT_OPLIST_P3(name, ...)                                        \
  (INIT(M_C(name,_init)),                                                     \
   INIT_SET(M_C(name, _init_set)),                                            \
   SET(M_C(name,_set)),                                                       \
   CLEAR(M_C(name, _clear)),                                                  \
   RESET(M_C(name, _reset)),                                                  \
   NAME(name),                                                                \
   TYPE(M_C(name,_ct)),                                                       \
   EMPTY_P(M_C(name,_empty_p)),                                               \
   M_IF_METHOD_ALL(HASH, __VA_ARGS__)(HASH(M_C(name, _hash)),),               \
   M_IF_METHOD_ALL(EQUAL, __VA_ARGS__)(EQUAL(M_C(name, _equal_p)),),          \
   M_IF_METHOD_ALL(GET_STR, __VA_ARGS__)(GET_STR(M_C(name, _get_str)),),      \
   M_IF_METHOD2_ALL(PARSE_STR, INIT, __VA_ARGS__)(PARSE_STR(M_C(name, _parse_str)),), \
   M_IF_METHOD2_ALL(IN_STR, INIT, __VA_ARGS__)(IN_STR(M_C(name, _in_str)),),  \
   M_IF_METHOD_ALL(OUT_STR, __VA_ARGS__)(OUT_STR(M_C(name, _out_str)),),      \
   M_IF_METHOD2_ALL(IN_SERIAL, INIT, __VA_ARGS__)(IN_SERIAL(M_C(name, _in_serial)),), \
   M_IF_METHOD_ALL(OUT_SERIAL, __VA_ARGS__)(OUT_SERIAL(M_C(name, _out_serial)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(INIT_MOVE(M_C(name, _init_move)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(MOVE(M_C(name, _move)),),          \
   M_IF_METHOD_ALL(SWAP, __VA_ARGS__)(SWAP(M_C(name, _swap)),),               \
   M_IF_METHOD(NEW, M_RET_ARG1(__VA_ARGS__,))(NEW(M_DELAY2(M_GET_NEW) M_RET_ARG1(__VA_ARGS__,)),), \
   M_IF_METHOD(REALLOC, M_RET_ARG1(__VA_ARGS__,))(REALLOC(M_DELAY2(M_GET_REALLOC) M_RET_ARG1(__VA_ARGS__,)),), \
   M_IF_METHOD(DEL, M_RET_ARG1(__VA_ARGS__,))(DEL(M_DELAY2(M_GET_DEL) M_RET_ARG1(__VA_ARGS__,)),), \
   )

/* Macros for testing for method presence */
#define M_VAR1ANT_TEST_METHOD_P2(method, f, t, op)                            \
  M_TEST_METHOD_P(method, op)
#define M_VAR1ANT_TEST_METHOD_P(method, trio)                                 \
  M_APPLY(M_VAR1ANT_TEST_METHOD_P2, method, M_OPFLAT trio)
#define M_VAR1ANT_IF_ALL(method, ...)                                         \
  M_IF(M_REDUCE2(M_VAR1ANT_TEST_METHOD_P, M_AND, method, __VA_ARGS__))

#define M_VAR1ANT_TEST_METHOD2_P2(method1, method2, f, t, op)                 \
  M_AND(M_TEST_METHOD_P(method1, op), M_TEST_METHOD_P(method2, op))
#define M_VAR1ANT_TEST_METHOD2_P(method, trio)                                \
  M_APPLY(M_VAR1ANT_TEST_METHOD2_P2, M_PAIR_1 method, M_PAIR_2 method, M_OPFLAT trio)
#define M_VAR1ANT_IF_ALL2(method1, method2,  ...)                             \
  M_IF(M_REDUCE2(M_VAR1ANT_TEST_METHOD2_P, M_AND, (method1, method2), __VA_ARGS__))

#endif
