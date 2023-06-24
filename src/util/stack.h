/**
 * Simple stack implementation in c
 * (C) Ray Clemens 2023
 *
 * Updates:
 * 2023-03-16: Initial creation
 * 2023-03-17: Complete initial code base
 * 2023-03-18: Change status return to use false
 *             for the "no error" success condition
 *             and true for "error"
 * 2023-03-25: Added #undef of datatype-specific
 *             macros to end of header.
 * 2023-04-01: Add init param to prevent automatic
 *             shrinking of the internal array
 * 
 * USAGE:
 * Define STACK_DATA_T as the data type to be stored in the stack structure.
 * Define STACK_DATA_NAME as the data name for the associated function calls.
 * **NOTE**: Do not enclose the above macros in parens! For example, if the 
 *           data type is char* , define the macros as char* , not (char*)
 */

#include <stddef.h> // size_t
#include <stdbool.h>
#include <string.h> // memcpy()
#include <stdlib.h>

#ifndef STACK_H
#define STACK_H

#define STACK_DEFAULT_MAX_POSITIVE_LOAD_FACTOR_VARIANCE 0.2
#define STACK_DEFAULT_MAX_NEGATIVE_LOAD_FACTOR_VARIANCE 0.6
#define STACK_DEFAULT_LOAD_FACTOR 1.0

#define _STACK_GLUE(x, y) x##y
#define STACK_GLUE(x, y) _STACK_GLUE(x, y)

#define STACK_ALLOW_SHRINK true
#define STACK_NO_SHRINK false

typedef struct _stack_t {
    float current_load_factor;             // Load factor
    size_t number_of_items_in_table;       // Number of actual items currently in the internal array
    size_t array_size;                     // Size of the internal array
    size_t min_array_size;
    bool allow_shrink;
    char *table;                           // Internal array
} _stack_t;

bool __stack_init(_stack_t **stack, size_t element_size, size_t initial_size, bool allow_shrink);
bool __stack_clear(_stack_t *stack, size_t element_size);
bool __stack_is_empty(_stack_t *stack);
void __stack_destroy(_stack_t **stack);
bool __stack_peek(_stack_t *stack, size_t element_size, void *element);
bool __stack_peeki(_stack_t *stack, size_t element_size, void *element, size_t index);
bool __stack_pop(_stack_t *stack, size_t element_size, void *element);
bool __stack_drop(_stack_t *stack, size_t element_size);
bool __stack_dup(_stack_t *stack, size_t element_size);
bool __stack_swap(_stack_t *stack, size_t element_size);
bool __stack_rot(_stack_t *stack, size_t element_size);
bool __stack_push(_stack_t *stack, size_t element_size, void *element);
size_t __stack_get_num_elements(_stack_t *stack);

#endif

// Now on to the "type generic weirdness"
#ifndef __STACK_STACK_C

#if !defined(STACK_DATA_NAME) || !defined(STACK_DATA_T)
# error "Must define both STACK_DATA_NAME and STACK_DATA_T before including stack.h"
#endif

#define __STACK_T STACK_GLUE(STACK_DATA_NAME, _stack_t)
// MAINTAINERS NOTE: Keep this the same as the _stack_t struct
typedef struct __STACK_T {
    float current_load_factor;             // Load factor
    size_t number_of_items_in_table; // Number of actual items currently in the internal array
    size_t array_size;               // Size of the internal array
    size_t min_array_size;
    bool allow_shrink;
    STACK_DATA_T *table;                   // Internal array
} __STACK_T;

// Function prototypes
static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_init)(__STACK_T **stack, size_t initial_size, bool allow_shrink)
{
    return __stack_init((_stack_t**)stack, sizeof(STACK_DATA_T), initial_size, allow_shrink);
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_clear)(__STACK_T *stack)
{
    return __stack_clear((_stack_t*)stack, sizeof(STACK_DATA_T));
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_is_empty)(__STACK_T *stack)
{
    return __stack_is_empty((_stack_t*)stack);
}

static inline void STACK_GLUE(STACK_DATA_NAME, _stack_destroy)(__STACK_T **stack)
{
    __stack_destroy((_stack_t**)stack);
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_peek)(__STACK_T *stack, STACK_DATA_T *element)
{
    return __stack_peek((_stack_t*)stack, sizeof(STACK_DATA_T), (void *)element);
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_peeki)(__STACK_T *stack, STACK_DATA_T *element, size_t index)
{
    return __stack_peeki((_stack_t*)stack, sizeof(STACK_DATA_T), (void *)element, index);
}

// Alias for peek
static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_top)(__STACK_T *stack, STACK_DATA_T *element)
{
    return __stack_peek((_stack_t*)stack, sizeof(STACK_DATA_T), (void *)element);
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_pop)(__STACK_T *stack, STACK_DATA_T *element)
{
    return __stack_pop((_stack_t*)stack, sizeof(STACK_DATA_T), (void *)element);
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_drop)(__STACK_T *stack)
{
    return __stack_drop((_stack_t*)stack, sizeof(STACK_DATA_T));
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_dup)(__STACK_T *stack)
{
    return __stack_dup((_stack_t*)stack, sizeof(STACK_DATA_T));
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_swap)(__STACK_T *stack)
{
    return __stack_swap((_stack_t*)stack, sizeof(STACK_DATA_T));
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_rot)(__STACK_T *stack)
{
    return __stack_rot((_stack_t*)stack, sizeof(STACK_DATA_T));
}

static inline bool STACK_GLUE(STACK_DATA_NAME, _stack_push)(__STACK_T *stack, STACK_DATA_T element)
{
    return __stack_push((_stack_t*)stack, sizeof(STACK_DATA_T), (void*)&element);
}

static inline size_t STACK_GLUE(STACK_DATA_NAME, _stack_get_num_elements)(__STACK_T *stack)
{
    return __stack_get_num_elements((_stack_t*)stack);
}

#undef STACK_DATA_T
#undef STACK_DATA_NAME
#undef __STACK_T

#endif

