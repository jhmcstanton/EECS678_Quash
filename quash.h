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

/**
 * Specify the maximum number of characters accepted by the command string
 */
#define MAX_COMMAND_LENGTH (1024)


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

#endif // QUASH_H
