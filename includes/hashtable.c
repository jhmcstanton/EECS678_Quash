/**
 * @file hashtable.h
 * 
 * Definitions of hashtable struct and related functions.
 * The keys to this hashtable are strings, and values are strings.
 * Doesn't do anything clever for collisions!
 */

#include "hashtable.h"

/*
  The hashing function used for indexing
 */
static size_t hash(char* key){
    size_t hashed_index = 0;
    int i;
    
    for(i = 0; key[i] != '\0'; i++){
	hashed_index += key[i];
    }    
    // this hash will not be correct if hashtable becomes dynamically sized
    // this would potentially miss many indexes
    return (hashed_index * 449) % HASHTABLE_START_SIZE;
}

void insert_key(char* key, char* val, hashtable *table){
    size_t index = hash(key);
    char *table_value = (char *) malloc(COPY_SIZE * sizeof(char));
    strcpy(table_value, val);
    table->values[index] = table_value;
}

char* lookup_key(char* key, hashtable *table){
    size_t index = hash(key);
    char* table_val = table->values[index];
    if(table_val != NULL){
	char* copied_val = (char *) malloc(COPY_SIZE * sizeof(char));	
	strcpy(copied_val, table_val);
	return copied_val;    
    } else {
	return NULL;
    }
}

hashtable new_table(){
    hashtable table;
    table.size = HASHTABLE_START_SIZE;
    table.used = 0;
    int i;
    for(i = 0; i < table.used; i++){
	table.values[i] = NULL;
    }
    return table;
}

void free_table(hashtable *table){
    int i;
    for(i = 0; i < table->size; i++){
	if(table->values[i] != NULL){
	    free(table->values[i]);
	}
    }
}
