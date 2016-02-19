/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h" // Putting this above the other includes allows us to ensure
                   // this file's header's #include statements are self
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
static char terminal_prompt[MAX_COMMAND_LENGTH];


/**************************************************************************
 * Private Functions 
 **************************************************************************/
static void maintenance(){
    /* This sets up the terminal prompt */
    char* cwd = (char*) malloc(MAX_COMMAND_LENGTH * sizeof(char));
    char* hostname = (char*) malloc(MAX_COMMAND_LENGTH * sizeof(char));
    int i, j = 0;
    // get and trim the current working directory
    getcwd(cwd, MAX_COMMAND_LENGTH);
    for(i = 0; i < MAX_COMMAND_LENGTH || cwd[i] != '\0'; i++){
	if(cwd[i] == '/'){
	    j = i + 1; 
	}
    }
    shift_str_left(j, cwd);
    
    gethostname(hostname, MAX_COMMAND_LENGTH);
    
    sprintf(terminal_prompt, "[%s@%s %s]$ ", getlogin(), hostname, cwd);
    free(cwd);
    free(hostname);
    
}
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {
  running = true;
  maintenance();
}


/**************************************************************************
 * Public Functions 
 **************************************************************************/
bool is_running(){
  return running;
}

void terminate() {   
  running = false;
}

/**************************************************************************
 * Job Stuff
 **************************************************************************/
const int MAX_NUM_JOBS = 10; // arbitrary. my system's max is 32768...
int CUR_NUM_JOBS = 0; // to be incremented/decremented with job creation/deletion

// Job Structure
struct job_t {
    pid_t pid; 
    int jid;
    char command[MAX_COMMAND_LENGTH];
};
//struct job_t jobs[MAX_NUM_JOBS]; /* The job list */

void printJobs(struct job_t *jobs){
	int i;
	for(i = 0; i < MAX_NUM_JOBS; i++) {
		printf("[%d] %d %s", jobs->jid, jobs->pid, jobs->command);
	}
}
/* ********************************************************************** */


/**************************************************************************
 * Pipe Stuff
 **************************************************************************/
void createPipes(int numPipesNeeded){  // numPipesNeeded is parsed from command line input string
	int fds[numPipesNeeded][2];

	// Set up necessary pipes
	int j;
	for (j = 0; j < numPipesNeeded; j++){
		pipe(fds[j]);
	}
}
/* ********************************************************************** */


/**************************************************************************
 * I/O Redirection
 **************************************************************************/
// standard output to file, ls > a.txt (>> appends)

// standard input from file, ls < a.txt (<< appends)
/* ********************************************************************** */


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
    char* cursor;
    // DEFINITELY not correct yet, just barebones
    if(command_list.length >= 1){
		cursor = command_list.char_arr[0];

	if(!strcmp(cursor, "exit") || !strcmp(cursor, "quit")){
	    printf("Exiting Quash\n");
	    free_str_arr(&command_list);
	    exit(EXIT_SUCCESS);
	} else if(!strcmp(cursor, "cd")){
	    char* path = (char *) malloc(MAX_COMMAND_LENGTH * sizeof(char));
	    if(command_list.length == 1){ // no path specified, returning to home directory
			get_home_dir(path);
			chdir(path);
	    } else if (root_is_home(command_list.char_arr[1])){ // path is specified and starts with ~
				cursor = command_list.char_arr[1];
		shift_str_left(1, cursor);
		char* helper_str = (char*) malloc(MAX_COMMAND_LENGTH * sizeof(char));
		get_home_dir(helper_str);
		sprintf(path, "%s%s", helper_str, cursor);
		chdir(path);
		free(helper_str);
	    } else { // path is specified and absolute
		strcpy(path, command_list.char_arr[1]);
		chdir(path);
	    }
	    free(path);
	} else if(!strcmp(cursor, "pwd")){
	    char* temp_buffer = (char *) malloc(MAX_COMMAND_LENGTH * sizeof(char));
	    getcwd(temp_buffer, MAX_COMMAND_LENGTH);
	    printf("%s\n", temp_buffer);
	    free(temp_buffer);
	} else if(!strncmp(cursor, "set", 3)){
		
	} else if(!strcmp(cursor, "jobs")){

	} else if(!strcmp(cursor, "echo")){
		
	} else if(!strcmp(cursor, "set")){

	} else {
	    printf("Did not match any built in command\n");
	}
    }
    free_str_arr(&command_list);    
    return true;
	
}
void shift_str_left(int shamt, char* str){
    int i;
    for(i = 0; str[i + shamt - 1] != '\0'; i++){
	str[i] = str[i + shamt];
    }
}

bool starts_with(char c, char* str){
    if(str == NULL){
	return false;
    }
    return str[0] == c;
}

bool root_is_home(char* path){
    return starts_with('~', path);
}

bool is_env_var_req(char* maybe_var){
    return starts_with('$', maybe_var);
}

void get_home_dir(char* buffer){    
    struct passwd *pw = getpwuid(getuid());
    buffer = strcpy(buffer, pw->pw_dir);
//    free(pw);
}

str_arr mk_str_arr(command_t* cmd){
    str_arr commands;
    bool last_space_was_whitespace = false; // so commands can be more than one ' ' apart
    int i, whitespace_count = 0, command_len = 0;

    // allocate number of strings (only allows up to *NUM_COMMANDS* fields right now!)
    commands.char_arr = (char**) malloc(NUM_COMMANDS * sizeof(char*));
    // allocate some space for the strings (fairly arbitrary)
    for(i = 0; i < NUM_COMMANDS; i++){
	commands.char_arr[i] = (char*) malloc(MAX_ARR_STR_LENGTH * sizeof(char));	
    }
    // count the number of white spaces to get the word count
    // this will segfault if whitespace_count exceeds 20 or command_len exceeds MAX_COMMAND_LENGTH / 10
    for(i = 0; i < (cmd->cmdlen) + 1; i++){
	if(cmd->cmdstr[i] == ' ' || cmd->cmdstr[i] == '\0'){ // found component of command or end
	    if(!last_space_was_whitespace){
		commands.char_arr[whitespace_count][command_len] = '\0';
		command_len = 0;
		whitespace_count++;
	    }
	    last_space_was_whitespace = true;
	} else { // building component of command
	    last_space_was_whitespace = false;
	    commands.char_arr[whitespace_count][command_len] = cmd->cmdstr[i];
	    command_len++;
	}
    }
    
    commands.length = whitespace_count;
    return commands;
}

void free_str_arr(str_arr *str_arr){
    int i;
    for(i = 0; i < NUM_COMMANDS; i++){
	free(str_arr->char_arr[i]);
    }
    free(str_arr->char_arr);
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
  printf("Type \"exit\" to quit\n%s", terminal_prompt);

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
      maintenance();
      printf("%s", terminal_prompt);
  }

  return EXIT_SUCCESS;
}
