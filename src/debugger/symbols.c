/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 *
 * Debugger symbol support
 * 
 */

#include <stdio.h>
#include <ctype.h>

#include "symbols.h"


/**
 * Initialize a symbol table
 * 
 * @param **st The symbol table
 * @return True on failure, False on success
 */
bool st_init(symbol_table_t **st)
{
    if (!st) {
        return true;
    }
    if (*st) {
        free(*st);
    }

    *st = malloc(sizeof(**st));
    if (!*st) {
        return true;
    }

    if (sym_ht_init(&((*st)->by_ident))) {
        free(*st);
        return true;
    }
    if (sym_ht_init(&((*st)->by_addr))) {
        sym_ht_destroy(&((*st)->by_ident));
        free(*st);
        return true;
    }
    return false;
}


/**
 * Load in a symbol file and add its contents to the symbol table
 * 
 * @param *st Symbol Table
 * @param *filepath The file to open
 * @param *linenum The linenumber to be returned on error
 * @return execution status
 */
st_status_t st_load_file(symbol_table_t *st, char *filepath, int *linenum)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return ST_ERR_NO_FILE;
    }

    char ident[20];
    uint32_t addr;
    int state = 0;
    bool done = false;
    st_status_t err = ST_OK;
    int c;
    size_t index = 0;
    
    *linenum = 1;
    while (!done) {
        c = fgetc(fp);
        switch (state) {
        case 0:
            if (c == EOF) {
                done = true;
            }
            else if (isspace(c)) {
                state = 1;
                if (c == '\n') {
                    *linenum += 1;
                }
            }
            else if (c == ';') {
                state = 2;
            }
            else if (IS_VALID_IDENT(c) && index < 20) {
                state = 3;
                index = 0;
                ident[index++] = (char)c;
            }
            else {
                err = ST_ERR_UNEXPECTED_CHAR;
            }
            break;

        case 1: // Leading whitespace
            if (c == EOF) {
                done = true;
            }
            else if (!isspace(c)) {
                state = 3;
                index = 0;
                ident[index++] = (char)c;
            }
            break;

        case 2: // Comments
            if (c == EOF) {
                done = true;
            }
            else if (c == '\n') {
                index = 0;
                state = 0;
                *linenum += 1;
            }
            break;

        case 3: // Identifier
            if (IS_VALID_IDENT(c)) {
                ident[index++] = (char)c;
            }
            else if (isspace(c) && c != '\n') {
                state = 4;
            }
            else {
                done = true;
                err = ST_ERR_UNEXPECTED_CHAR;
            }
            break;

        case 4: // Trailing-identifier whitespace
            if (!isspace(c)) {
                if (c == '.') {
                    state = 5;
                }
                else {
                    state = 6;
                }
            }
            else {
                /* pass */
            }
            break;

        case 5:
            if (c == 'e') {
                state = 6;
            }
            else {
                done = true;
                err = ST_ERR_MISSING_DELIM;
            }
            break;

        case 6:
            if (c == 'q') {
                state = 7;
            }
            else {
                done = true;
                err = ST_ERR_MISSING_DELIM;
            }
            break;
            
        case 7:
            if (c == 'u') {
                state = 8;
            }
            else {
                done = true;
                err = ST_ERR_MISSING_DELIM;
            }
            break;

        case 8: // Trailing-identifier whitespace
            if (isspace(c)) {
                /* pass */
            }
            else {
                if (c == '$') {
                    state = 9;
                    addr = 0;
                }
                else if (isdigit(c)) {
                    state = 10;
                    addr = c & 0xf;
                }
                else {
                    done = true;
                    err = ST_ERR_MISSING_VALUE;
                }
            }
            break;

        case 9: // Parse hex char
            if (isxdigit(c)) {
                addr <<= 4;
                char t;
                t = c & 0xf;
                if (c <= '9') { addr += t; }
                else          { addr += t + 9; }
            }
            else if (c == '\n') {
                symbol_t *sym = malloc(sizeof(*sym));
                if (!sym) {
                    done = true;
                    err = ST_ERR_NO_MEM;
                }
                sym->ident = malloc(sizeof(*(sym->ident)) * (index + 1));
                if (!sym->ident) {
                    free(sym);
                    done = true;
                    err = ST_ERR_NO_MEM;
                }
                else {
                    memcpy(sym->ident, ident, index);
                    sym->ident[index] = '\0';
                    sym->addr = addr;
        
                    if (!sym_ht_contains_key(st->by_addr, addr)) {
                        sym_ht_put(st->by_addr, addr, sym);
                    }
                    if (!sym_ht_contains_skey(st->by_addr, ident)) {
                        sym_ht_sput(st->by_ident, ident, sym);
                    }
                    // TODO: Fix memory leak here if not entered into either table!

                    *linenum += 1;
                    index = 0;
                    state = 0;
                }
            }
            else if (isspace(c)) {
                /* pass */
            }
            else {
                done = true;
                err = ST_ERR_UNEXPECTED_CHAR;
            }
            break;

        case 10: // Parse dec char
            if (isdigit(c)) {
                addr <<= 4;
                addr += c & 0xf;
            }
            else if (c == '\n') {
                symbol_t *sym = malloc(sizeof(*sym));
                if (!sym) {
                    done = true;
                    err = ST_ERR_NO_MEM;
                }
                sym->ident = malloc(sizeof(*(sym->ident)) * (index + 1));
                if (!sym->ident) {
                    free(sym);
                    done = true;
                    err = ST_ERR_NO_MEM;
                }
                else {
                    memcpy(sym->ident, ident, index);
                    sym->ident[index] = '\0';
                    sym->addr = addr;

                    if (!sym_ht_contains_key(st->by_addr, addr)) {
                        sym_ht_put(st->by_addr, addr, sym);
                    }
                    if (!sym_ht_contains_skey(st->by_addr, ident)) {
                        sym_ht_sput(st->by_ident, ident, sym);
                    }
                    // TODO: Fix memory leak here if not entered into either table!

                    *linenum += 1;
                    index = 0;
                    state = 0;
                }
            }
            else if (isspace(c)) {
                /* pass */
            }
            else {
                done = true;
                err = ST_ERR_UNEXPECTED_CHAR;
            }
            break;
                        
        default:
            index = 0;
            state = 0;
        }

        if (c == EOF) {
            done = true;
        }
    }

    fclose(fp);

    return err;
}


/**
 * Free memory associated with a symbol table
 * 
 * @param **st 
 */
void st_destroy(symbol_table_t **st)
{
    if (!st || !*st) {
        return;
    }
    
    sym_ht_itr_t *sti = sym_ht_create_iterator((*st)->by_ident);
    symbol_t *entry;
    while (sym_ht_iterator_has_next(sti)) {
        entry = sym_ht_iterator_next(sti)->value;
        if (entry->ident) {
            free(entry->ident);
        }
        free(entry);
    }
    sym_ht_iterator_free(&sti);

    // Don't need to free the second table's entries since
    // they pointed to the ones that we just free'd in the
    // ident table.
    sym_ht_destroy(&(*st)->by_ident);
    sym_ht_destroy(&(*st)->by_addr);
    *st = NULL;
}


/**
 * Return a symbol entry from the symbol table
 * 
 * @param *st Symbol Table
 * @param *ident Identifier to look up
 * @return NULL on failure, otherwise a symbol_t struct
 */
symbol_t *st_resolve_by_ident(symbol_table_t *st, char *ident)
{
    return sym_ht_sget(st->by_ident, ident);
}


/**
 * Return a symbol entry from the symbol table
 * 
 * @param *st Symbol Table
 * @param *addr Address to look up
 * @return NULL on failure, otherwise a symbol_t struct
 */
symbol_t *st_resolve_by_addr(symbol_table_t *st, uint32_t addr)
{
    return sym_ht_get(st->by_addr, addr);
}

