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
#include <sys/stat.h>

/**
 * Specify the maximum number of characters accepted by the command string
 */
#define MAX_COMMAND_LENGTH (1024)
#define MAX_ARR_STR_LENGTH (1024 / 10)

/**
 * These are error codes that we will look out for
 */ 
#define E_BIN_MISSING 65280

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

/**
 * Enum to a handle redirect handling and inspection.
 */
typedef enum redirect{
    PIPE, // |
    AWRITE_R, // >>
    //    AWRIT_L, // <<
    OWRITE_R, // >
    READ_L, // <    
} redirect;

/**
 *  The number of redirects in the enum type, useful for looping over them
 */
#define redirect_ct (READ_L + 1)

/**
 * A structure to keep track of all redirects and their locations in the command
 */
typedef struct ri_pair{
    redirect redirect;
    size_t r_index;
} ri_pair;

/**
 * Holds parsed information from a command_t
 */
typedef struct str_arr {
    char** char_arr;
    ri_pair* redirects;
    size_t length;
    size_t r_length;
} str_arr;

/**
 * Structure holding all job info
 */ 
typedef struct job_t {
    pid_t pid; 
    //    int jid;
    char command[MAX_COMMAND_LENGTH];
} job_t;

/** 
 * Arbitrarily assigned max number of jobs
 */
#define MAX_NUM_JOBS 100

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
 * @param start_index - the index to start the echo command from, updates in place
 * @param end_index - the final index to echo
 */
void echo(str_arr command_list, int *start_index, int end_index);

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
 * Prints the current jobs in the global jobs list. These are printed in the form of
 * [job_id] process_id command_str
 */
void print_jobs();

/**
 * Logs a new background job to the global jobs list. If this job would exceen #MAX_NUM_JOBS
 * then nothing is logged.
 * @param proc_id - the process id from the fork call used to run the command 
 * @param command - the command, minus arguments, that is being run
 */
void log_job(pid_t proc_id, char* command);

/**
 * Checks all background jobs from the global jobs list. If anything has EXITed the waitpid
 * call *should* reap it, and every job with a higher job id (index) will be shifted left by 1
 */
void check_all_jobs();


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
 * @param start_index - the index where the binary execute begins (star_index is the binary)
 * @param stop_index  - the index of the final argument to the executable
 * @returns the integer return status of the executable
 */
int execute(str_arr command_list, int *start_index, int stop_index);

/**
 * Fills a provided buffer with the fully expanded command provided after performing 
 * variable lookups.
 *
 * @param buffer - the buffer to fill - expected to be allocated already
 * @param command - the command to expand and perform lookups from
 */
void expand_buff_with_vars(char* buffer, char* command);

/**
 * Checks if a string is an IO redirect string
 *
 * @param str - the string to compare
 * @returns 0 if the string is not a redirect, otherwise it returns the redirect enum
 */
redirect which_redirect(char* str);

/**
 * Validates that redirects all occur in the command such that each redirect has a command to its right.
 * Ex: *echo a b c d e >> test.txt* is valid
 * Ex: *echo a b c d e >>* is not valid
 * 
 * @param command_list - the list to parse
 * @returns true if matches the simple, expected form laid out above, false otherwise
 */
bool validate(str_arr *command_list);

#endif // QUASH_H
