/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h" // Putting this above the other includes allows us to ensure
                   // this file's headder's #include statements are self
                   // contained.

#include <string.h>

/**************************************************************************
 * Private Variables
 **************************************************************************/
/**
 * Keep track of whether Quash should request another command or not.
 */
// NOTE: "static" causes the "running" variable to only be declared in this
// compilation unit (this file and all files that include it). This is similar
// to private in other languages.
static bool running;


/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {
  running = true;
}

/**************************************************************************
 * Public Functions 
 **************************************************************************/
bool is_running() {
  return running;
}

void terminate() {
  running = false;
}

bool get_command(command_t* cmd, FILE* in) {
  if (fgets(cmd->cmdstr, MAX_COMMAND_LENGTH, in) != NULL) {
    size_t len = strlen(cmd->cmdstr);
    char last_char = cmd->cmdstr[len - 1];

    if (last_char == '\n' || last_char == '\r') {
      // Remove trailing new line character.
      cmd->cmdstr[len - 1] = '\0';
      cmd->cmdlen = len - 1;
    }
    else
      cmd->cmdlen = len;
    
    return true;
  }
  else
    return false;
}

/*
  Call internal builtins 
 */
bool handle_command(command_t* cmd){
    str_arr command_list = mk_str_arr(cmd);
    int i;
    char* cursor;
    // DEFINITELY not correct yet, just barebones
    for(i = 0; i < command_list.length; i++){
	cursor = command_list.char_arr[i];
	if(!strcmp(cursor, "cd")){

	} else if(!strcmp(cursor, "pwd")){
	    
	} else if(!strcmp(cursor, "echo")){

	} else if(!strcmp(cursor, "set=")){

	} else if(!strcmp(cursor, "exit") || !strcmp(cursor, "quit")){
	    printf("Exiting Quash\n");
	    exit(EXIT_SUCCESS);
	} else {
	    printf("Did not match any built in command\n");
	}
    }
    
    return true;
}

str_arr mk_str_arr(command_t* cmd){
    str_arr commands;
    int i, whitespace_count = 0, command_len = 0;

    // allocate number of strings (only allows up to *NUM_COMMANDS* fields right now!)
    commands.char_arr = (char**) malloc(NUM_COMMANDS * sizeof(char*));
    // allocate some space for the strings (fairly arbitrary)
    for(i = 0; i < NUM_COMMANDS; i++){
	commands.char_arr[i] = (char*) malloc((MAX_COMMAND_LENGTH / 10) * sizeof(char));	
    }
    // count the number of white spaces to get the word count
    // this will segfault if whitespace_count exceeds 20 or command_len exceeds MAX_COMMAND_LENGTH / 10
    for(i = 0; i < (cmd->cmdlen) + 1; i++){
	if(cmd->cmdstr[i] == ' ' || cmd->cmdstr[i] == '\0'){ // found component of command or end
	    commands.char_arr[whitespace_count][command_len] = '\0';
	    command_len = 0;
	    whitespace_count++;
	} else { // building component of command
	    commands.char_arr[whitespace_count][command_len] = cmd->cmdstr[i];
	    command_len++;
	}
    }
    
    commands.length = whitespace_count;
    return commands;
}

/**
 * Quash entry point
 *
 * @param argc argument count from the command line
 * @param argv argument vector from the command line
 * @return program exit status
 */
int main(int argc, char** argv) { 
  command_t cmd; //< Command holder argument
      
  start();
  
  puts("Welcome to Quash!");
  puts("Type \"exit\" to quit");

  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
    // NOTE: I would not recommend keeping anything inside the body of
    // this while loop. It is just an example.

    // The commands should be parsed, then executed.
      /*   if (!strcmp(cmd.cmdstr, "exit"))
      terminate(); // Exit Quash
    else 
      puts(cmd.cmdstr); // Echo the input string
      */
      handle_command(&cmd);
  }

  return EXIT_SUCCESS;
}
