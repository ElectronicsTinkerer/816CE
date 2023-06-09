/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 *
 * Debugger symbol support
 * 
 */

#ifndef _DEBUGSYMBOL_H
#define _DEBUGSYMBOL_H

#include <stdint.h>

typedef struct symbol_table_t {
    struct sym_ht_t *by_ident;
    struct sym_ht_t *by_addr;
} symbol_table_t;

typedef struct symbol_t {
    char *ident;
    uint32_t addr;
} symbol_t;

typedef enum st_status_t {
    ST_OK,
    ST_ERR_NO_MEM,
    ST_ERR_NO_FILE,
    ST_ERR_MISSING_IDENT,
    ST_ERR_MISSING_DELIM,
    ST_ERR_MISSING_VALUE,
    ST_ERR_UNEXPECTED_CHAR
} st_status_t;

#define HT_DATA_T symbol_t
#define HT_DATA_NAME sym
#include "hashtable.h"

#define IS_VALID_IDENT(c) (isalnum(c) || (c) == '_' || (c) == '.')

bool st_init(symbol_table_t **);
void st_destroy(symbol_table_t **);
st_status_t st_load_file(symbol_table_t *, char *, int *);
symbol_t *st_resolve_by_ident(symbol_table_t *, char *);
symbol_t *st_resolve_by_addr(symbol_table_t *, uint32_t);

#endif

