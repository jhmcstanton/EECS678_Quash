/**
 * @file hashtable.h
 * 
 * Definitions of hashtable struct and related functions.
 * The keys to this hashtable are strings, and values are strings.
 * Doesn't do anything clever for collisions!
 */

/**
 * Specifies the minimum size of the hashtable array
 * useful for dyanmic allocations
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdlib.h>
#include <string.h>

#define HASHTABLE_START_SIZE 1024
#define COPY_SIZE            1024

/**
 * Hashtable for storing environmental variables
 */
typedef struct hashtable{
    char* values[HASHTABLE_START_SIZE]; // this could be made dynamic
    size_t size; 
    size_t used;
} hashtable;

/**
 * Using a naive hashing algorithm to get a position for @param val based
 * on @param key. Performs a deep copy of @param val. 
 * @param key - the string to hash into an index
 * @param val - the value to store at the hashed index
 * @param table - the table to insert into
 */
void insert_key(char* key, char* val, hashtable *table);

/**
 * Lookups a value in the table based on the hash made with @param val
 * @param key - the string to hash into an index
 * @param table - the table to perform the lookup in
 * @returns a deep copy of the value stored at the index found using the key
 */
char* lookup_key(char* val, hashtable *table);

/**
 * Makes a new, empty table with all fields initialized.
 * @returns the newly created table
 */
hashtable new_table();

#endif
