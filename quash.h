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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h> 

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
 * Given a buffer that contains a variable at the provided index of the form $VARNAME this will
 * lookup the value of that variable in the provided hashtable. 
 * @param start_index - the index at which the variable name begins - updated in place
 * @param buffer_with_var - the buffer containing the variable
 * @returns the value that was found or "" - this will be malloced!
 */
char* get_env_var(int *start_index, char* buffer_with_var);


/**
 *  Makes a pointer with enough space for a command string
 * @returns a malloced command pointer
 */
char* malloc_command();

/**
 * Performs the builtin command "echo" - handles sys environment variables as well
 * @param command_list - the commands parsed by caller - the first field is expected to be
 * the echo command.
 */
void echo(str_arr command_list);

/**
 * Assigns a new environment variable. Takes the form of set VARNAME=VARVAL
 * @param command_list - the parsed commands where the first field is equal to "set"
 */
void set(str_arr command_list);

/**
 * Prints the current working directory.  
 */
void pwd();


/**
 * Changes to the desired directory. Can be in three forms:
 *
 * 1. cd  - changes to the root directory 
 * 2. cd ~/path/here - changes to a path relative to the home directory
 * 3. cd /absolute/path - changes to an absolute path
 *
 * @param command_list - a parsed list of commands that may have 1 or 2 commands, 
 * and the first command is always cd
 */
void cd(str_arr command_list);

/**
 * Executes a binary and provides forwards the provided arguments to
 * it. Searches the path for the executable.
 *
 * @param command_list - a parsed commandstr with the first element
 * the binary and any additional ones are passed as arguments
 * @returns the integer return status of the executable
 */
int execute(str_arr command_list);

/**
 * Fills a provided buffer with the fully expanded command provided after performing 
 * variable lookups.
 *
 * @param buffer - the buffer to fill - expected to be allocated already
 * @param command - the command to expand and perform lookups from
 */
void expand_buff_with_vars(char* buffer, char* command);

#endif // QUASH_H
