/**
 * Simple hash table implementation in c
 * (C) Ray Clemens 2020 - 2023
 *
 * Updates:
 * 2020-10-xx: Created HT for GSG V0
 * 2023-02-08: Added function "generic" templating magic
 * 2023-02-09: Changed to macro cast wrappers
 * 2023-02-10: Fixed "definitely" and "indirectly" lost
 *             memory on table resize (valgrind)
 * 2023-02-17: Added __HT_HT_C #undef capability to allow
 *             for multiple hashtable generic types in
 *             the same compilation unit.
 * 2023-03-14: Reduced HT_INITIAL_SIZE from 16 to 8
 * 2023-03-18: Refactor to add type checking for all 
 *             functions.
 *             Add return status value to put() and sput()
 * 
 * USAGE:
 * Define HT_DATA_T as the data type to be stored in the hashtable structure.
 * Define HT_DATA_NAME as the data name for the associated function calls.
 * **NOTE**: Do not enclose the above macros in parens! For example, if the 
 *           data type is char* , define the macros as char* , not (char*)
 * **NOTE**: The hashtable internally stores pointers. For example, if HT_DATA_T
 *           is defined as char, internally, the hash table would be storing
 *           char*. This is different behavior than the Stack data structure.
 */

#ifndef HT_H
#define HT_H

#include <string.h> // memcpy()
#include <stdlib.h>
#include <stddef.h> // NULL
#include <stdbool.h>

#define HT_DEFAULT_MAX_POSITIVE_LOAD_FACTOR_VARIANCE 0.2
#define HT_DEFAULT_MAX_NGATIVE_LOAD_FACTOR_VARIANCE 0.5
#define HT_DEFAULT_LOAD_FACTOR 1.0
#define HT_INITIAL_SIZE 8

typedef unsigned long ht_key_t;
typedef size_t ht_index_t;

#define _HT_GLUE(x, y) x##y
#define HT_GLUE(x, y) _HT_GLUE(x, y)

typedef struct ht_t
{
    float currentLoadFactor;            // Load factor
    ht_index_t numberOfItemsInTable;    // Number of items in the table
    ht_index_t numberOfSlotsUsed;       // Number of table slots that have had data in them ("dirty slots")
    ht_index_t arraySize;               // Current size of array to store elements
    struct ht_entry_t **table;          // The table in which to store the elements
} ht_t;

typedef struct ht_entry_t
{
    struct ht_entry_t *next;     // Using collision lists, this points to the next node in the list
    ht_key_t key;
    void *value;
} ht_entry_t;

typedef struct ht_itr_t
{
    int currentTableIndex;
    int foundElements;
    int totalElements;
    int tableSize;
    struct ht_entry_t *currentNode;
    struct ht_entry_t **iteratorTable;
} ht_itr_t;


// Function Prototypes
bool __ht_init(ht_t **table);
void __ht_clear(ht_t *table);
bool __ht_put_nia(ht_t *table, ht_key_t key, void *value, ht_entry_t *item); // Assumes item has already been allocated
bool __ht_put(ht_t *table, ht_key_t key, void *value);
bool __ht_sput(ht_t *table, char *key, void *value);
void *__ht_get(ht_t *table, ht_key_t key);
void *__ht_sget(ht_t *table, char *key);
void *__ht_remove(ht_t *table, ht_key_t key);
void *__ht_sremove(ht_t *table, char *key);
bool __ht_contains_key(ht_t *table, ht_key_t key);
bool __ht_contains_skey(ht_t *table, char *key);
void __ht_destroy(ht_t **table);
bool __ht_is_empty(ht_t *table);
ht_index_t __ht_get_num_elements(ht_t *table);
ht_key_t __ht_hash_string(const char *string);

ht_itr_t *__ht_create_iterator(ht_t *table);
bool __ht_iterator_has_next(ht_itr_t *itr);
ht_entry_t *__ht_iterator_next(ht_itr_t *itr);
void __ht_iterator_free(ht_itr_t **itr);

#endif

#ifndef __HT_HT_C

#if !defined(HT_DATA_T) || !defined(HT_DATA_NAME)
# error "Must define HT_DATA_T and HT_DATA_NAME before including hashtable.h"
#endif

#define HT_T HT_GLUE(HT_DATA_NAME, _ht_t)
#define HT_ENTRY_T HT_GLUE(HT_DATA_NAME, _ht_entry_t)
#define HT_ITR_T HT_GLUE(HT_DATA_NAME, _ht_itr_t)

// MAINTAINERS NOTE: KEEP THESE STRUCTS THE SAME AS THE ABOVE
// NON-CAPS NAMED STRUCTS! This will ensure that casting works
// as expected!
typedef struct HT_T
{
    float currentLoadFactor;            // Load factor
    ht_index_t numberOfItemsInTable;    // Number of items in the table
    ht_index_t numberOfSlotsUsed;       // Number of table slots that have had data in them ("dirty slots")
    ht_index_t arraySize;               // Current size of array to store elements
    struct HT_ENTRY_T **table;          // The table in which to store the elements
} HT_T;

typedef struct HT_ENTRY_T
{
    struct ht_entry_t *next;     // Using collision lists, this points to the next node in the list
    ht_key_t key;
    HT_DATA_T *value;
} HT_ENTRY_T;

typedef struct HT_ITR_T
{
    int currentTableIndex;
    int foundElements;
    int totalElements;
    int tableSize;
    struct HT_ENTRY_T *currentNode;
    struct HT_ENTRY_T **iteratorTable;
} HT_ITR_T;



// "Macro Generic" templating wrappers
static inline bool HT_GLUE(HT_DATA_NAME, _ht_init)(HT_T **t)
{
    return __ht_init((ht_t**)t);
}

static inline void HT_GLUE(HT_DATA_NAME, _ht_clear)(HT_T *t)
{
    __ht_clear((ht_t*)t);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_put)(HT_T *t, ht_key_t k, HT_DATA_T *v)
{
    return __ht_put((ht_t*)t, k, (void*)v);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_sput)(HT_T *t, char *k, HT_DATA_T *v)
{
    return __ht_sput((ht_t*)t, k, (void*)v);
}

static inline HT_DATA_T *HT_GLUE(HT_DATA_NAME, _ht_get)(HT_T *t, ht_key_t k)
{
    return (HT_DATA_T*)__ht_get((ht_t*)t, k);
}

static inline HT_DATA_T *HT_GLUE(HT_DATA_NAME, _ht_sget)(HT_T *t, char *k)
{
    return (HT_DATA_T*)__ht_sget((ht_t*)t, k);
}

static inline HT_DATA_T *HT_GLUE(HT_DATA_NAME, _ht_remove)(HT_T *t, ht_key_t k)
{
    return (HT_DATA_T*)__ht_remove((ht_t*)t, k);
}

static inline HT_DATA_T *HT_GLUE(HT_DATA_NAME, _ht_sremove)(HT_T *t, char *k)
{
    return (HT_DATA_T*)__ht_sremove((ht_t*)t, k);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_contains_key)(HT_T *t, ht_key_t k)
{
    return __ht_contains_key((ht_t*)t, k);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_contains_skey)(HT_T *t, char *k)
{
    return __ht_contains_skey((ht_t*)t, k);
}

static inline void HT_GLUE(HT_DATA_NAME, _ht_destroy)(HT_T **t)
{
    __ht_destroy((ht_t**)t);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_is_empty)(HT_T *t)
{
    return __ht_is_empty((ht_t*)t);
}

static inline ht_index_t HT_GLUE(HT_DATA_NAME, _ht_get_num_elements)(HT_T *t)
{
    return __ht_get_num_elements((ht_t*)t);
}

static inline ht_key_t  HT_GLUE(HT_DATA_NAME, _ht_hash_string)(const char *string)
{
    return __ht_hash_string(string);
}

static inline HT_ITR_T *HT_GLUE(HT_DATA_NAME, _ht_create_iterator)(HT_T *t)
{
    return (HT_ITR_T*)__ht_create_iterator((ht_t*)t);
}

static inline bool HT_GLUE(HT_DATA_NAME, _ht_iterator_has_next)(HT_ITR_T *itr)
{
    return __ht_iterator_has_next((ht_itr_t*)itr);
}

static inline HT_ENTRY_T *HT_GLUE(HT_DATA_NAME, _ht_iterator_next)(HT_ITR_T *itr)
{
    return (HT_ENTRY_T*)__ht_iterator_next((ht_itr_t*)itr);
}

static inline void HT_GLUE(HT_DATA_NAME, _ht_iterator_free)(HT_ITR_T **itr)
{
    __ht_iterator_free((ht_itr_t**)itr);
}


#undef HT_DATA_T
#undef HT_DATA_NAME
#undef HT_ENTRY_T
#undef HT_T
#undef HT_ITR_T

// The programmer must undef this if multiple
// hashtable types are to be defined within
// the same compilation unit. This prevents
// multiple definition warnings if multiple
// header files include the hashtable within
// the same unit with the same generic type.
#define __HT_HT_C

#endif


