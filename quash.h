/**
 * @file quash.h
 *
 * Quash essential functions and structures.
 */

#ifndef QUASH_H
#define QUASH_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pwd.h>

/**
 * Specify the maximum number of characters accepted by the command string
 */
#define MAX_COMMAND_LENGTH (1024)
#define MAX_ARR_STR_LENGTH (1024 / 10)

/**
 * Holds information about a command.
 */
typedef struct command_t {
  char cmdstr[MAX_COMMAND_LENGTH]; ///< character buffer to store the
                                   ///< command string. You may want
                                   ///< to modify this to accept
                                   ///< arbitrarily long strings for
                                   ///< robustness.
  size_t cmdlen;                   ///< length of the cmdstr character buffer

  // Extend with more fields if needed
} command_t;

#define NUM_COMMANDS 20
typedef struct str_arr {
    char** char_arr;
    size_t length;
} str_arr;

/**
 * Query if quash should accept more input or not.
 *
 * @return True if Quash should accept more input and false otherwise
 */
bool is_running();

/**
 * Causes the execution loop to end.
 */
void terminate();

/**
 *  Read in a command and setup the #command_t struct. Also perform some minor
 *  modifications to the string to remove trailing newline characters.
 *
 *  @param cmd - a command_t structure. The #command_t.cmdstr and
 *               #command_t.cmdlen fields will be modified
 *  @param in - an open file ready for reading
 *  @return True if able to fill #command_t.cmdstr and false otherwise
 */
bool get_command(command_t* cmd, FILE* in);

/**
 * Takes a #command_t struct and parses it into a number of null
 * terminated character arrays. 
 * 
 * @param cmd - a command_t structure. Nothing is modified.
 * @return a fully instantiated #str_arr
 */
str_arr mk_str_arr(command_t* cmd);

/**
 * Takes a #command_t struct and internally parses it using #mk_str_arr
 * then dispatches to either builtin commands or executables
 * @param cmd - a command_t structure. Nothing is modified.
 * @return True if successful, false otherwise
 */
bool handle_command(command_t* cmd);

/**
 * Shift a #char* to the left by @param shamt indexes. 
 * Only shifts until the null character. Does not clean up characters
 * after null character (some duplication will occur). 
 * @param shamt - the amount to shift the string by
 * @param str   - the string to shift
 */
void shift_str_left(int shamt, char* str);

/**
 * Frees the internal char*s inside a #str_arr.
 * @param str_arr - a #str_arr to free up. The struct itself is not freed.
 */
void free_str_arr(str_arr *str_arr);

/**
 * Fills a #char* buffer with the user's home directory. Assumes memory
 * is already allocated.
 * @param buffer - the buffer to fill.
 */
void get_home_dir(char* buffer);

/**
 * Helper function to check what a string's first character is, used by other functions.
 * @param c - the letter to be checked for at index 0 of str.
 * @param str - the string to check. May be NULL
 * @returns True if @param str's element 0 is c, otherwise return false.
 */
bool starts_with(char c, char* str);

/**
 * Helper function to check if the caller provided path starts with '~'
 * @param path - a file path that may or may not contain a '~' at index 0. May be NULL
 * @returns True if the path starts with '~', false otherwise.
 */
bool root_is_home(char* path);

/**
 * Helper function to check if a string is in the form of a system variable ($XXXXXXX).
 * Does NO lookup. 
 * @param maybe_var - the string to analyze. May be NULL.
 * @returns True if the first character of @param maybe_var is '$', false otherwise.
 */
bool is_env_var_req(char* maybe_var);


#endif // QUASH_H
