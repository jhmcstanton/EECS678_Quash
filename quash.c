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
static hashtable env_variables; 


/**************************************************************************
 * Private Functions 
 **************************************************************************/
static void maintenance(){
    /* This sets up the terminal prompt */
    char* cwd = malloc_command();
    char* hostname = malloc_command();
    int i, j = 0;
    // get and trim the current working directory
    getcwd(cwd, MAX_COMMAND_LENGTH);

    for(i = 0; i < MAX_COMMAND_LENGTH && cwd[i] != '\0'; i++){
	if(cwd[i] == '/'){
	    j = i + 1; 
	}
    }
    shift_str_left(j, cwd);
    
    gethostname(hostname, MAX_COMMAND_LENGTH);
    
    sprintf(terminal_prompt, "Quash![%s@%s %s]$ ", getlogin(), hostname, cwd);
    free(cwd);
    free(hostname);
    
}
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {
  running = true;
  // setup the system variables
  char* home_dir = malloc_command();
  get_home_dir(home_dir);
  env_variables = new_table();
  insert_key(PATH, DEFAULT_PATH, &env_variables);
  insert_key(HOME, home_dir, &env_variables);
  free(home_dir);
  
  // 
  maintenance();
}


/**************************************************************************
 * Public Functions 
 **************************************************************************/
bool is_running(){
  return running;
}

void terminate() {
    printf("Exiting Quash\n");
//    free_table(&env_variables);
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

    if(command_list.length >= 1){ // DEFINITELY not correct yet, just barebones
		cursor = command_list.char_arr[0];

	if(!strcmp(cursor, "exit") || !strcmp(cursor, "quit")){
	    free_str_arr(&command_list);
	    terminate();
	    return true; 
	} else if(!strcmp(cursor, "cd")){
	    cd(command_list);
	} else if(!strcmp(cursor, "pwd")){
	    pwd();
	} else if(!strcmp(cursor, "set")){
	    set(command_list);

	} else if(!strcmp(cursor, "jobs")){

	} else if(!strcmp(cursor, "echo")){
	    echo(command_list);
	    // ignores pipes and redirects right now		

	} else {
	    printf("Did not match any built in command\n");
	}
    }
    free_str_arr(&command_list);    
    return true;
	
}

void cd(str_arr command_list){
    char* path = malloc_command();
    char* cursor; 
    if(command_list.length == 1){ // no path specified, returning to home directory
	get_home_dir(path);
    } else if (root_is_home(command_list.char_arr[1])){ // path is specified and starts with ~
	cursor = command_list.char_arr[1];
	shift_str_left(1, cursor);
	char* helper_str = malloc_command();
	get_home_dir(helper_str);
	sprintf(path, "%s%s", helper_str, cursor);
	free(helper_str);
    } else { // path is specified and absolute
	strcpy(path, command_list.char_arr[1]);
    }
    if(chdir(path)){
	printf("Directory does not exist\n");
    }
    free(path);
}

void pwd(){
    char* temp_buffer = malloc_command();
    getcwd(temp_buffer, MAX_COMMAND_LENGTH);
    printf("%s\n", temp_buffer);
    free(temp_buffer);
}

void set(str_arr command_list){
    // set takes the form of set VARNAME=VALUE
    if(command_list.length < 2){
	printf("No value provided to set\n");
    } else {
	char* cursor = command_list.char_arr[1];
	char* variable_name = malloc_command();
	char* variable_val  = malloc_command();
	bool found_equals   = false;
	int i, j = 0;
	     
	
	for(i = 0; cursor[i] != '\0'; i++){
	    if(found_equals){ // working on VALUE
		if (cursor[i] == '$') {
		    char* env_var;
		    env_var = get_env_var(&i, cursor, &env_variables);
		    sprintf(variable_val, "%s%s", variable_val, env_var);
		    j = strlen(variable_val) - 1;
		    free(env_var);
		} else {
		    variable_val[j] = cursor[i];
		}
		j++;
	    } else if (cursor[i] == '='){ // betwen VARNAME and VALUE
		variable_name[i] = '\0';
		found_equals = true;
	    } else { // working on VALUE, may have to look up env values
		variable_name[i] = cursor[i];
	    }
	}
	if(found_equals){
	    variable_val[j] = '\0';
	    insert_key(variable_name, variable_val, &env_variables);
	}
	free(variable_name);
	free(variable_val );
    }
}

void execute(str_arr command_list){

}

void echo(str_arr command_list){
    int i;
    char* cursor;
    char* buffer = malloc_command();
    
    for(i = 1; i < command_list.length; i++){
	cursor = command_list.char_arr[i];
	expand_buff_with_vars(buffer, cursor, &env_variables);
	printf("%s ", buffer);
	
	/*
	if(i > 1){
	    printf(" "); // putting a space between each "command"
	}
	cursor = command_list.char_arr[i];
	for(j = 0; cursor[j] != '\0'; j++){ // this loop is required to find variables embedded in commands
	    if(cursor[j] == '$'){ // found a variable
		char* temp_buff = NULL;
		temp_buff = get_env_var(&j, cursor, &env_variables);			
		printf("%s", temp_buff);
		free(temp_buff);
	    } else {
		printf("%c", cursor[j]);
	    }
	}
	*/
    }
    printf("\n");
    free(buffer);
};

void shift_str_left(int shamt, char* str){
    int i;
    for(i = 0; str[i + shamt - 1] != '\0'; i++){
	str[i] = str[i + shamt];
    }
}

void expand_buff_with_vars(char* buffer, char* command, hashtable *table){
    int i;
    for(i = 0; command[i] != '\0'; i++){ // this loop is required to find variables embedded in commands
	if(command[i] == '$'){ // found a variable
	    char* temp_buff;
	    temp_buff = get_env_var(&i, command, &env_variables);			
	    sprintf(buffer, "%s%s", buffer, temp_buff);
	    free(temp_buff);
	} else {
	    buffer[i] = command[i];
	}
    }    
}

char* get_env_var(int *start_index, char* buffer_with_var, hashtable *table){
    char* name_to_lookup = malloc_command();
    char* var_buffer;
    int i, j;
    for(i = *start_index + 1, j = 0; isalpha(buffer_with_var[i]) || isdigit(buffer_with_var[i]); i++, j++){
        name_to_lookup[j] = buffer_with_var[i];
    }
    name_to_lookup[j] = '\0';
    var_buffer = lookup_key(name_to_lookup, table);
    if(var_buffer == NULL){
	var_buffer = (char *) malloc(sizeof(char));
	*var_buffer  = '\0';
    }

    *start_index = i - 1;     

    free(name_to_lookup);
    return var_buffer;
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

/************************************************
 PRETTY SURE THE MEM LEAK IS FROM THE PASSWD STRUCT HERE
********************************************************************/
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

path mk_path(const char* path_var){
    int i, j, k;
    str_arr new_path;
    // not a great algorithm, but should be fine
    // get the number of path bases based on the separator count (':')
    for(i = 0, j = 0; path_var[i] != '\0'; i++){
	if(path_var[i] == ':'){
	    j++;
	}
    }
    new_path.char_arr = (char**) malloc(j * sizeof(char*));
    new_path.length = j;

    // now parse the string for the various paths
    new_path.char_arr[0] = malloc_command();
    for(i = 0, j = 0, k = 0; path_var[i] != '\0'; i++, k++){
	if(path_var[i] == ':'){
	    k = 0;
	    j++;
	    new_path.char_arr[j] = malloc_command();
	} else {
	    new_path.char_arr[j][k] = path_var[i];
	}
    }
    return new_path;
}

void free_path(path *path){
    free_str_arr(path);
}

char* malloc_command(){
    char* new_buffer = (char*) malloc(MAX_COMMAND_LENGTH * sizeof(char));
    // these null characters helps to avoid weird behavior with printf
    // and string manipulatio    
    memset(new_buffer, 0, MAX_COMMAND_LENGTH);
    return new_buffer;
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
      if(is_running()){
	  printf("%s", terminal_prompt);
      }
      maintenance();
  }

  return EXIT_SUCCESS;
}
