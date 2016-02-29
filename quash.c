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
static char *redirects[redirect_ct] = { "|", ">>", ">", "<"}; 
static job_t jobs[MAX_NUM_JOBS];
static size_t next_new_job = 0;


/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Performed on every iteration of quash.
 * Performs routine frees, updates, etc. All frequently required cleanup/ update
 * function should be in here. 
 */
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
    if(isatty(fileno(stdin))){
	sprintf(terminal_prompt, "Quash![%s@%s %s]$ ", getlogin(), hostname, cwd);
    }
    free(cwd);
    free(hostname);

    // Woo, all job stuff
    check_all_jobs();
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

void print_jobs(){
    int i;
    for(i = 0; i < next_new_job; i++) {
	printf("[%d] %d %s\n", i, jobs[i].pid, jobs[i].command);
    }
}

void log_job(pid_t proc_id, char* command){
    if(next_new_job < MAX_NUM_JOBS){
	jobs[next_new_job].pid = proc_id;
	strcpy(jobs[next_new_job++].command, command);
    } else {
	fprintf(stderr, "Exceeding max number of jobs (%d)!\n Not logging new job.\n", MAX_NUM_JOBS);
    }
}

void check_all_jobs(){
    int i, back_track, status;
    for(i = 0; i < next_new_job; i++){

	if(waitpid(jobs[i].pid, &status, WNOHANG) != 0){
	    // next process to look at will have the same index as this one had
	    printf("[%d] %d finished %s\n", i, jobs[i].pid, jobs[i].command);
	    back_track = i - 1;
	    for(; i < next_new_job - 1; i++){
		jobs[i] = jobs[i + 1];
	    }
	    next_new_job--;
	    i = back_track; 
	}
    }
}

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
  Call internal builtins and executables
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

	    for(c_index = 0, r_index = 0; c_index < command_list.length; c_index++, r_index++){
		cursor = command_list.char_arr[c_index];
//		printf("c_i: %d, r_i: %d, cursor: %s\n", c_index, r_index, cursor);

		// make sure any required pipe opens correctly
		if(command_list.r_length > 0 && r_index < command_list.r_length && pipe(pipe_fds[r_index]) == -1){
		    fprintf(stderr, "Pipe creation error\n");
		    terminate();
		    free(pipe_fds);
		    free_str_arr(&command_list);
		    exit(EXIT_FAILURE);
		}
		// remove this
		if(r_index < command_list.r_length){
		    redirect = command_list.redirects[r_index].redirect;
		}
		pid = fork();
		
		if(pid == 0){ // child process
	  	    status = EXIT_SUCCESS; // being optimistic
		    next_r_index = r_index < command_list.r_length ? command_list.redirects[r_index].r_index : command_list.length;
		   
		    int input_file = STDIN_FILENO; // placeholder value
		    if(command_list.r_length > 0){
			// found a < in the command, this is the priority stdin redirect
			if(redirect == READ_L && r_index < command_list.r_length && r_index + 1 < command_list.length){
			    input_file = open(command_list.char_arr[r_index + 1], O_RDONLY);
			    
			    if(input_file < 0){
				fprintf(stderr, "error opening file %s\n", command_list.char_arr[r_index + 1]);
				free(pipe_fds);
				free_str_arr(&command_list);
				exit(2); // file missing
			    }
			    dup2(input_file, STDIN_FILENO);
			} else if(r_index > 0) { // otherwise redirect to previous commands pipe if available 
			    dup2(pipe_fds[r_index - 1][0], STDIN_FILENO); // commands after first read from previous output pipe      
			}
		    }
		    if(command_list.r_length > 0 && r_index < command_list.r_length){
			close(pipe_fds[r_index][0]); // this process will be reading from the previous pipe
		    }

		    if((redirect != READ_L && next_r_index < command_list.length) || 
		       (redirect == READ_L && next_r_index < command_list.length - 1)){ // hopefully accounts for commands with a middle <
			dup2(pipe_fds[r_index][1], STDOUT_FILENO); // upcoming redirect requires pipe
		    } else if(command_list.run_in_bg){
			// hide this output if it is last AND it is to be run in the background
			int hole = open("/dev/null", O_WRONLY);
			dup2(hole, STDOUT_FILENO);
			close(hole);			
		    }

		    if(!strcmp(cursor, "pwd")){
			pwd();
		    } else if(!strcmp(cursor, "jobs")){
			print_jobs();			
		    } else if(!strcmp(cursor, "echo")){
			echo(command_list, &c_index, next_r_index);
		    } else {
			status = execute(command_list, &c_index, next_r_index, command_list.run_in_bg);
			if(status == E_BIN_MISSING){
			  fprintf(stderr, "Could not find %s\n", command_list.char_arr[c_index]);
			}
		    }
		    
		    if(command_list.r_length > 0 && r_index < command_list.r_length){
			close(pipe_fds[r_index][1]); // done writing to pipe, closing it up
			close(input_file); // if < was found, closes stdin for worst case
			if(next_r_index > 0){
			    close(pipe_fds[r_index - 1][0]); // done reading
			}
		    }
		    // child process is done!
		    free_str_arr(&command_list);
		    free(pipe_fds);
		    // returns 0 if executable was found otherwise
		    // 1 if it was missing
		    exit(status == E_BIN_MISSING ? 1 : 0); 
		} else if(!command_list.run_in_bg && waitpid(pid, &status, 0) == -1){
		    fprintf(stderr, "Process encountered an error. ERROR%d", errno);
		    terminate();
		    for(i = 0; i < command_list.r_length; i++){
			free(pipe_fds[i]);
		    }
		    free(pipe_fds);
		    free_str_arr(&command_list);

		    close(pipe_fds[r_index][1]);		    
		    exit(EXIT_FAILURE);
		} else if(!command_list.run_in_bg && WEXITSTATUS(status) > 0){
		    // do some cleanup then stop dealing with the command since something broke
		    if(command_list.r_length > 0){
			if(r_index > 0){
			    close(pipe_fds[r_index - 1][0]);
			}
			if(r_index < command_list.r_length){
			    close(pipe_fds[r_index][1]);
			}
		    }
		    break;
		} else { // successful
		    // log job if necessary
		    if(command_list.run_in_bg){
			printf("Running %s in background.\n", command_list.char_arr[c_index]);
			log_job(pid, command_list.char_arr[c_index]);
		    }

		    // not sure whethere the ...].r_index case should have -1 or - 0
		    c_index = r_index < command_list.r_length ? command_list.redirects[r_index].r_index - 1 : command_list.length;	     
		    // general redirect case 
		    if(command_list.r_length > 0){
			// done writing to this pipe
			if(r_index > 0){
			    // done reading from previous pipe
			    close(pipe_fds[r_index - 1][0]);
			}
			if(r_index < command_list.r_length){
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
		    } else if(redirect == READ_L && command_list.r_length > 0){
			c_index++; 			
		    }
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

int execute(str_arr command_list, int *start_index, int stop_index, bool run_in_bg){
    char* full_exec_path = malloc_command();
    pid_t exec_proc; 

    char** args;
    int j, k, status;
    
    args = (char**) malloc( (stop_index - *start_index + 1) * sizeof(char*));
    for(j = *start_index, k = 0; j < stop_index; j++, k++){
	args[k]  = malloc_command();
	expand_buff_with_vars(args[k], command_list.char_arr[j]);
	//fprintf(stderr, "arg: %d, arg_str: %s, in position: %d\n", k, args[k], j);
    }

    exec_proc = fork();
    
    if(exec_proc == 0){
	args[k] = NULL;       
	execvp(args[0], args);
    } else if(waitpid(exec_proc, &status, 0) == -1){	
	fprintf(stderr, "Error in process: %d\n", exec_proc);
    }
    // free all args

    for(j = 0; j < k; j++){
	free(args[j]);
    }
    free(args);
    
    free(full_exec_path);
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

void get_home_dir(char* buffer){    
    buffer = strcpy(buffer, getenv("HOME"));
}

str_arr mk_str_arr(command_t* cmd){
    str_arr commands;
    commands.run_in_bg = false; // default case
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
	    if(found_redirect == AWRITE_R){
		i++; // increment past the second > or <
	    }
	    last_space_was_delim = true;
	} else if(cmd->cmdstr[i] == '&'){ 
	    // wrap up last command if necessary
	    if(!last_space_was_delim){ 
		commands.char_arr[whitespace_count++][command_len] = '\0';
		command_len = 0;
	    }
	    last_space_was_delim = true;
	    commands.run_in_bg = true;
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
  
  if(isatty(fileno(stdin))){
      puts("Welcome to Quash!");
      printf("Type \"exit\" to quit\n%s", terminal_prompt);
  }

  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
      maintenance();
      handle_command(&cmd);
      if(is_running()){
	  printf("%s", terminal_prompt);
      }
  }

  if(isatty(fileno(stdin))){
      printf("Exiting Quash\n");
  }

  return EXIT_SUCCESS;
}
