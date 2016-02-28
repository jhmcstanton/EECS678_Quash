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
static int redirect_ct = 5;
static char *redirects[5] = { "|", ">>", "<<", ">", "<"}; 


/**************************************************************************
 * Private Functions 
 **************************************************************************/
static void maintenance(){
    /* This sets up the terminal prompt */    
    char* cwd      = malloc_command();
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
    

    // getlogin() causes a "reachable" memory leak according to
    // valgrind, which is apparently ok: http://valgrind.org/docs/manual/faq.html#faq.deflost
    // memory lost - 1,718 Bytes in 5 blocks
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
    int c_index, r_index, next_r_index, status;
    int **pipe_fds = (int**) malloc(command_list.r_length * sizeof(int*));
    int i;
    for(i = 0; i < command_list.r_length; i++){
	pipe_fds[i] = (int*) malloc(2 * sizeof(int));
    }
    pid_t pid;
    redirect redirect = 0;    
    char* cursor;

    if(command_list.length >= 1 && strlen(command_list.char_arr[0])){
	// check for exit or quit
	cursor = command_list.char_arr[0];
	if(!strcmp(cursor, "exit") || !strcmp(cursor, "quit")){
	    free_str_arr(&command_list);
	    terminate();
	    return true; 
	}  else if(!strcmp(cursor, "set")){ // set is also slightly special
	    set(command_list);
	} else if(!strcmp(cursor, "cd")){ // I gues this is too..
	    cd(command_list);
	} else if(validate(&command_list)){
	    // all other commands can be handled the same way (basically)
	    for(c_index = 0, r_index = 0; c_index < command_list.length; c_index++){
		if(command_list.r_length > 0 && command_list.redirects[r_index].r_index == c_index){
		    r_index++;
		}
	    }
	    int loop_delete_this = 0;

	    for(c_index = 0, r_index = 0; c_index < command_list.length; c_index++, r_index++){
		cursor = command_list.char_arr[c_index];
		printf("loop: %d, cursor: %s\n", loop_delete_this++, cursor);

		// make sure any required pipe opens correctly
		if(command_list.r_length > 0 && r_index < command_list.r_length && pipe(pipe_fds[r_index]) == -1){
		    fprintf(stderr, "Pipe creation error\n");
		    printf("r_i: %d, r_l: %d\n", r_index, command_list.r_length);
		    terminate();
		    free(pipe_fds);
		    free_str_arr(&command_list);
		    exit(EXIT_FAILURE);
		}
		// remove this
		if(r_index < command_list.r_length){
		    redirect = command_list.redirects[r_index].redirect;
		    printf("found redirect in redirects: %d\n", redirect);
		}
		pid = fork();
		
		if(pid == 0){ // child process
		    next_r_index = r_index < command_list.r_length ? command_list.redirects[r_index].r_index : command_list.length;
		    printf("start_i : %d, stop_i: %d, loop: %d\n", c_index, next_r_index, loop_delete_this);
		    // this isn't really doing anything yet
		    // delete this
		    if(r_index < command_list.r_length  && redirect == PIPE){
			printf("Found pipe redirect\n");
		    } else {
			printf("No pipe found\n");
		    }
//		    printf("n_r_i: %d\n", next_r_index);
/*		    printf("r_i: %d, r_length: %d\n", r_index, command_list.r_length);
		    printf("c_i: %d, c_length: %d\n", c_index, command_list.length);*/
		    if(r_index > 0 && command_list.r_length > 0){
			dup2(pipe_fds[r_index - 1][0], STDIN_FILENO); // commands after first read from previous output pipe
			fprintf(stderr, "reading from pipe %d instead of stdin\n", r_index - 1);
		    }
		    if(command_list.r_length > 0){
			close(pipe_fds[r_index][0]); // this process will be reading from the last pipe
		    }
		    if(next_r_index < command_list.length){
			fprintf(stderr, "STDOUT dup'd to write end of pipe: %d\n", r_index);
			dup2(pipe_fds[r_index][1], STDOUT_FILENO); // upcoming redirect requires pipe
		    } 

		    if(!strcmp(cursor, "pwd")){
			pwd();
		    } else if(!strcmp(cursor, "jobs")){
			
		    } else if(!strcmp(cursor, "echo")){
			echo(command_list, &c_index, next_r_index);
			// ignores pipes and redirects right now 
		    } else {
			fprintf(stderr, "in execute: bin: %s\n", command_list.char_arr[c_index]);
			fprintf(stderr, "c_i: %d, n_r_i: %d\n", c_index, next_r_index);
			if(status = execute(command_list, &c_index, next_r_index)){
//			printf("Exited with error: %d\n", status);
			}
		    }
		    
		    if(command_list.r_length > 0){
			close(pipe_fds[r_index][1]); // done writing to pipe, closing it up
			if(next_r_index > 0){
			    close(pipe_fds[r_index - 1][0]); // done reading
			}
		    }
		    // child process is done!
		    free_str_arr(&command_list);
		    free(pipe_fds);
		    exit(EXIT_SUCCESS);
		} else if(waitpid(pid, &status, 0) == -1){
		    fprintf(stderr, "Process encountered an error. ERROR%d", errno);
		    terminate();
		    for(i = 0; i < command_list.r_length; i++){
			free(pipe_fds[i]);
		    }
		    free(pipe_fds);
		    free_str_arr(&command_list);

		    close(pipe_fds[r_index][1]);		    
		    exit(EXIT_FAILURE);
		} else { // successful
  		    printf("fork finished successfully\n");
		    // not sure whethere the ...].rindex case should have -1 or - 0
		    c_index = r_index < command_list.r_length ? command_list.redirects[r_index].r_index - 1 : command_list.length;


		    // general redirect case 
		    if(command_list.r_length > 0){
			// done writing to this pipe
			if(r_index > 0){
			    // done reading from previous pipe
//			    close(pipe_fds[r_index - 1][1]);
			    close(pipe_fds[r_index - 1][0]);
			}
			if(r_index < command_list.r_length){
			    printf("closing write end of pipe %d\n", r_index);
			    close(pipe_fds[r_index][1]); // this is causing a segfault for somereason
			}
		    }
		    // handle > and >> cases
		    if((redirect == OWRITE_R || redirect == AWRITE_R) && command_list.r_length > 0){
			c_index++;
			
			size_t buf_size;
			char buf[MAX_COMMAND_LENGTH];
			int o_file = open(command_list.char_arr[command_list.redirects[r_index].r_index],
					  redirect == OWRITE_R ? O_CREAT | O_WRONLY | O_TRUNC :
					              O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
			while((buf_size = read(pipe_fds[r_index][0], buf, MAX_COMMAND_LENGTH)) > 0){
			    write(o_file, buf, buf_size);
			}
			close(o_file);
		    }
/*		    printf("bottom of loop\n");
		    printf("c_i :%d, r_i: %d\n", c_index, r_index);
		    printf("c_l: %d, r_l: %d\n", command_list.length, command_list.r_length);*/
		}
	    }
	} else {
	    printf("Parse error with redirects \n");
	}
    }
    for(i = 0; i < command_list.r_length; i++){
	free(pipe_fds[i]);
    }
    free(pipe_fds);
    free_str_arr(&command_list);    
    return true;
	
}

bool validate(str_arr *command_list){
    int i;
    for(i = 0; i < command_list->r_length; i++){
	if(command_list->redirects[i].r_index > command_list->length){
	    return false;
	}
    }
    return true;
}

void cd(str_arr command_list){
    char* path = malloc_command();
    char* cursor; 
    if(command_list.length == 1){ // no path specified, returning to home directory
	get_home_dir(path);
    } else if (command_list.char_arr[1][0] == '~'){ // path is specified and starts with ~
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
		    env_var = get_env_var(&i, cursor);
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
	    setenv(variable_name, variable_val, 1);
	} else {
	    unsetenv(variable_name);  
	}
	free(variable_name);
	free(variable_val );
    }
}

redirect which_redirect(char* str){
    int i;
    // ! This is assuming that redirects is in the same order as the enum
    // redirects is defined!
    for(i = 0; i < redirect_ct; i++){
	if(!strncmp(str, redirects[i], strlen(redirects[i]))){
	    return i;
	}
    }
    return -1;
}

int execute(str_arr command_list, int *start_index, int stop_index){
    char* full_exec_path = malloc_command();
    pid_t exec_proc; 

    char** args;
    int i, j, k, status;
    bool run_in_bg = false;
    
    for(i = *start_index; i < stop_index; i++){
	if(!strcmp(command_list.char_arr[i], "&")){ 
	    run_in_bg = true;
	    break;
	}	    
    }
    args = (char**) malloc(i * sizeof(char*));
    for(j = *start_index, k = 0; j < i; j++, k++){
	args[k]  = malloc_command();
	expand_buff_with_vars(args[k], command_list.char_arr[j]);
    }

    exec_proc = fork();
    
    if(exec_proc == 0){
	args[j] = NULL;

	if(run_in_bg){
	    // when running things in the background we can just eat the output
	    int hole = open("/dev/null", O_WRONLY);
	    dup2(hole, STDOUT_FILENO);
	    close(hole);
	}
	status = execvp(args[0], args);
	free_str_arr(&command_list);
	terminate();
	// free all args
	for(j = 0; j < k; j++){
	    free(args[j]);
	}
	free(args);
	exit(status);
    } else if(!run_in_bg){
	if(waitpid(exec_proc, &status, 0) == -1){
	    printf("Error in process: %d\n", exec_proc);
	}	    
    }
    // free all args
    fprintf(stderr, "args used: ");
    for(j = 0; j < k; j++){
	fprintf(stderr, j > 0 ? ", %s": "%s", args[j]);
	free(args[j]);
    }
    fprintf(stderr, ".\nnew start_index: %d\n", *start_index);
    free(args);
    
    free(full_exec_path);
    *start_index = j;
    return status;
}

void echo(str_arr command_list, int *start_index, int end_index){
    int i;
    char* cursor;
    char* buffer = malloc_command();
    
    for(i = *start_index + 1; i < end_index; i++){
	cursor = command_list.char_arr[i];
	expand_buff_with_vars(buffer, cursor);	
	printf("%s ", buffer);	
    }
    printf("\n");
    *start_index = i;
    free(buffer);
};

void shift_str_left(int shamt, char* str){
    int i;
    for(i = 0; str[i + shamt - 1] != '\0'; i++){
	str[i] = str[i + shamt];
    }
}

void expand_buff_with_vars(char* buffer, char* command){
    int i;
    for(i = 0; command[i] != '\0'; i++){ // this loop is required to find variables embedded in commands
	if(command[i] == '$'){ // found a variable
	    char* temp_buff;
	    temp_buff = get_env_var(&i, command);			
	    sprintf(buffer, "%s%s", buffer, temp_buff);
	    free(temp_buff);
	} else {
	    buffer[i] = command[i];
	}
    };
}

char* get_env_var(int *start_index, char* buffer_with_var){
    char* name_to_lookup = malloc_command();
    char* var_buffer, *found_val;
    int i, j;
    for(i = *start_index + 1, j = 0; isalpha(buffer_with_var[i]) || isdigit(buffer_with_var[i]); i++, j++){
        name_to_lookup[j] = buffer_with_var[i];
    }
    name_to_lookup[j] = '\0';
    found_val = getenv(name_to_lookup); 
    if(found_val == NULL){
	var_buffer = (char *) malloc(sizeof(char));
	*var_buffer  = '\0';
    } else {
	var_buffer = malloc_command();
	strcpy(var_buffer, found_val);
    }

    *start_index = i - 1;     

    free(name_to_lookup);
    return var_buffer;
}

/************************************************
 PRETTY SURE THE MEM LEAK IS FROM THE PASSWD STRUCT HERE
********************************************************************/
void get_home_dir(char* buffer){    
    buffer = strcpy(buffer, getenv("HOME"));
}

str_arr mk_str_arr(command_t* cmd){
    str_arr commands;
    bool last_space_was_delim = false, found_quote = false; // so commands can be more than one ' ' apart or surrounded by "
    int i, whitespace_count = 0, command_len = 0, redirect_index = 0;
    redirect found_redirect;

    // allocate number of strings (only allows up to *NUM_COMMANDS* fields right now!)
    commands.char_arr  = (char**) malloc(NUM_COMMANDS * sizeof(char*));
    commands.redirects = (ri_pair*) malloc(NUM_COMMANDS * sizeof(ri_pair));    
    // allocate some space for the strings (fairly arbitrary)
    for(i = 0; i < NUM_COMMANDS; i++){
	commands.char_arr[i] = (char*) malloc(MAX_ARR_STR_LENGTH * sizeof(char));	
    }
    // count the number of white spaces to get the word count
    // this will segfault if whitespace_count exceeds 20 or command_len exceeds MAX_COMMAND_LENGTH / 10
    for(i = 0; i < cmd->cmdlen + 1; i++){
	if((cmd->cmdstr[i] == ' ' && !found_quote) || cmd->cmdstr[i] == '\0'){ // found component of command or end
	    if(!last_space_was_delim){
		commands.char_arr[whitespace_count][command_len] = '\0';
		command_len = 0;
		whitespace_count++;
	    }
	    last_space_was_delim = true;
	} else if(cmd->cmdstr[i] == '"'){
	    last_space_was_delim = false;
	    found_quote = !found_quote;
	} else if( (found_redirect = which_redirect(&cmd->cmdstr[i])) + 1){
	    commands.redirects[redirect_index].redirect = found_redirect;
	    commands.redirects[redirect_index++].r_index = whitespace_count;
	    if(!last_space_was_delim){ // form is foo bar< ..
		// wrap up last command
		commands.char_arr[whitespace_count++][command_len++] = '\0';		
	    } 
	    if(found_redirect == AWRITE_R || found_redirect == AWRITE_L){
		i++; // increment past the second > or <
	    }
	    last_space_was_delim = true;
	} else { // building component of command
	    last_space_was_delim = false;
	    commands.char_arr[whitespace_count][command_len] = cmd->cmdstr[i];
	    command_len++;
	}
    }
    
    commands.length = whitespace_count;
    commands.r_length = redirect_index;

    return commands;
}

void free_str_arr(str_arr *str_arr){
    int i;
    for(i = 0; i < NUM_COMMANDS; i++){
	free(str_arr->char_arr[i]);
    }
    free(str_arr->redirects);
    free(str_arr->char_arr);
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
      handle_command(&cmd);
      if(is_running()){
	  printf("%s", terminal_prompt);
      }
      maintenance();
  }

  printf("Exiting Quash\n");

  return EXIT_SUCCESS;
}
