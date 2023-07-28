

#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // exit()
#include <fcntl.h>    // close(), open()
#include <signal.h>   // signal()
#include <sys/wait.h> // wait()
#include <unistd.h>   // exec(), fork(), getpid()

#define MAX 100000
#define CYAN "\x1B[36m"  // colours that can be used to print text
#define WHITE "\x1B[37m" // colours that can be used to print text

enum cases {SINGLE_COMMAND, SEQUENTIAL_COMMANDS, PARALLEL_COMMANDS, COMMAND_REDIRECTION};

// Function Declarations
int parseInput(char *inputs[MAX], char *user_input);                    // Parse 'user_input' by removing spaces
void execute_cd(char *inputs[MAX]);                                     // This function will execute the 'cd' command
void executeCommand(char *inputs[MAX]);                                 // This function is used to run a single command
void executeSequentialCommands(char *inputs[MAX], char *commands[MAX]); // This function will run multiple commands in a sequence (which are separated by ##)
void executeParallelCommands(char *inputs[MAX], char *commands[MAX]);   // This function will run multiple commands in parallel (which are separated by &&)
void executeCommandRedirection(char *inputs[MAX]);                      // This function will run a single command with output redirected to an output file given

int parseInput(char *inputs[MAX], char *user_input)
{
  // remove spaces from 'user_input' and store the strings in 'inputs'

  user_input[strlen(user_input) - 1] = '\0'; // removal of newline character and adding an end point

  int cases = SINGLE_COMMAND; // flag for different cases

  int i = 0;
  char *p = NULL; // traverse pointer
  while (1)
  {
    p = strsep(&user_input, " "); // used to split string & return pointer to it, using the delimiter ' '
    if (p == NULL)
      break;

    inputs[i++] = p;          // update each command string in 'inputs'
    if (strcmp(p, "##") == 0) // case of 'executeSequentialCommands'
      cases = SEQUENTIAL_COMMANDS;
    else if (strcmp(p, "&&") == 0) // case of 'executeParallelCommands'
      cases = PARALLEL_COMMANDS;
    else if (strcmp(p, ">") == 0) // case of 'executeCommandRedirection'
      cases = COMMAND_REDIRECTION;
  }
  inputs[i++] = strsep(&user_input, " "); // update last command
  inputs[i + 1] = NULL;                   // used as end point while traversing 'inputs'

  // Uncomment while debugging
  // for(int i=0; inputs[i]!=NULL; i++) {
  //   printf("%s ", inputs[i]);
  // }

  return cases;
}

void execute_cd(char *inputs[MAX])
{
  // This function will execute the 'cd' command
  int res;

  // execute 'chdir' and store return value
  if (inputs[1] == NULL) // when user enters only 'cd'
    res = chdir(getenv("HOME"));
  else
    res = chdir(inputs[1]);

  if (inputs[2] != NULL) // handling case of too many arguments
  {
    printf("bash: cd: too many arguments\n");
    return;
  }

  if (res == -1) // handling case of directory not present
    printf("bash: cd: %s: No such file or directory\n", inputs[1]);
}

void executeCommand(char *inputs[MAX])
{
  // This function is used to run a single command
  if (strcmp(inputs[0], "cd") == 0) // check for 'cd' case
  {
    execute_cd(inputs);
    return;
  }

  int p_id = fork();
  if (p_id < 0) // when fork fails
    exit(1);
  else if (p_id > 0) // parent process
  {
    int status, cWait;
    cWait = waitpid(WAIT_ANY, &status, WUNTRACED);  // wait for all child processes to exit/terminate/stop
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP) // if child process has stopped due to SIGTSTP signal
    {
      kill(cWait, SIGKILL);  // kill the stopped process
      return;
    }
    else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) // if child process has stopped due to SIGINT signal
    {
      return;
    }
  }
  else // child process
  {
    signal(SIGINT, SIG_DFL);  // enable SIGINT
    signal(SIGTSTP, SIG_DFL); // enable SIGTSTP

    execvp(inputs[0], inputs);            // execute the given command
    printf("Shell: Incorrect command\n"); // this gets printed if 'execvp' fails
    exit(1);
  }
}

void executeSequentialCommands(char *inputs[MAX], char *commands[MAX])
{
  // This function will run multiple commands in a sequence (which are separated by ##)

  int i = 0;
  // traverse through 'inputs' and add the strings of that particular command to 'commands'
  // here, 'two pointer' method is used
  while ((i == 0 && inputs[i] != NULL) || (inputs[i - 1] != NULL && inputs[i] != NULL)) // add until '##' is encountered
  {
    int j = i, curr = 0;
    while (inputs[j] != NULL && strcmp(inputs[j], "##") != 0)
    {
      commands[curr++] = inputs[j];
      j++;
    }
    commands[curr] = NULL; // end point is made NULL

    i = j + 1;

    // Uncomment while debugging
    // for(int k=0; commands[k]!=NULL; k++) {
    //   printf("%s ", commands[k]);
    // }

    // execute the given command
    if (strcmp(commands[0], "cd") == 0) // check for 'cd' case
    {
      execute_cd(commands);
      continue;
    }
    else if(strcmp(commands[0], "exit") == 0) { // check for 'exit' case
      printf("Exiting shell...\n");
      exit(0);
    }

    int p_id = fork();
    if (p_id < 0) // when fork fails
      exit(1);
    else if (p_id > 0) // parent process
    {
      int status, cWait;
      cWait = waitpid(WAIT_ANY, &status, WUNTRACED); // wait for all child processes to exit/terminate/stop
      if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP)  // if child process has stopped due to SIGTSTP signal
      {
        kill(cWait, SIGKILL);  // kill the stopped process
        return;
      }
      else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) // if child process has stopped due to SIGINT signal
      {
        return;
      }
    }
    else // child process
    {
      signal(SIGINT, SIG_DFL);  // enable SIGINT
      signal(SIGTSTP, SIG_DFL); // enable SIGTSTP

      execvp(commands[0], commands);        // execute the given command
      printf("Shell: Incorrect command\n"); // this gets printed if 'execvp' fails
      exit(1);
    }
  }
}

void executeParallelCommands(char *inputs[MAX], char *commands[MAX])
{
  // This function will run multiple commands in parallel (which are separated by &&)

  int i = 0, p_id;
  // traverse through 'inputs' and add the strings of that particular command to 'commands'
  // here, 'two pointer' method is used
  while ((i == 0 && inputs[i] != NULL) || (inputs[i - 1] != NULL && inputs[i] != NULL))
  {
    int j = i, curr = 0;
    while (inputs[j] != NULL && strcmp(inputs[j], "&&") != 0) // add until '&&' is encountered
    {
      commands[curr++] = inputs[j];
      j++;
    }
    commands[curr] = NULL; // end point is made NULL

    i = j + 1;

    // Uncomment while debugging
    // for(int k=0; commands[k]!=NULL; k++) {
    //   printf("%s ", commands[k]);
    // }

    if (strcmp(commands[0], "cd") == 0) // check for 'cd' case
    {
      execute_cd(commands);
      continue;
    }

    p_id = fork();
    if (p_id < 0) // when fork fails
      exit(1);
    else if (p_id > 0) // parent process
    {
    }
    else // child process
    {
      signal(SIGINT, SIG_DFL);  // enable SIGINT
      signal(SIGTSTP, SIG_DFL); // enable SIGTSTP

      execvp(commands[0], commands);        // execute the given command
      printf("Shell: Incorrect command\n"); // this gets printed if 'execvp' fails
      exit(1);
    }
  }

  int status, cWait;
  while ((cWait = waitpid(WAIT_ANY, &status, WUNTRACED)) > 0)   // parent waits for all child processes to exit/terminate/stop
  {
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP) // if child process has stopped due to SIGTSTP signal
    {                        
      kill(cWait, SIGKILL); // kill the stopped process
    }
  }
}

void executeCommandRedirection(char *inputs[MAX])
{
  // This function will run a single command with output redirected to an output file given
  // outputs > inputs
  char *outputs[MAX];
  int i = 0;
  for (i = 0; inputs[i] && strcmp(inputs[i], ">"); i++) // storing all commands before '>' in 'outputs'
  {
    outputs[i] = inputs[i];
  }
  outputs[i++] = NULL; // make end point as NULL

  if (strcmp(outputs[0], "cd") == 0) // check for 'cd' case
  {
    execute_cd(outputs);
    return;
  }

  int p_id = fork();
  if (p_id < 0) // when fork fails
    exit(1);
  else if (p_id > 0)
    wait(NULL); // parent process have to wait till child is finished
  else          // child process
  {
    signal(SIGINT, SIG_DFL);  // enable SIGINT
    signal(SIGTSTP, SIG_DFL); // enable SIGTSTP

    close(STDOUT_FILENO);                           // close the STDOUT file descriptor
    open(inputs[i], O_CREAT | O_WRONLY | O_APPEND); // open file descriptor to 'inputs[i]'

    execvp(outputs[0], outputs);          // execute the given command
    printf("Shell: Incorrect command\n"); // this gets printed if 'execvp' fails
    exit(1);
  }
}

int main()
{
  // Initial declarations
  char cwd[MAX];
  char *user_input;
  size_t chars, buffer_size = 0;
  char *inputs[MAX], *commands[MAX];

  // Memory allocation for 'user_input'
  user_input = (char *)malloc(buffer_size * sizeof(char));

  signal(SIGINT, SIG_IGN);  // Ignore SIGINT signal
  signal(SIGTSTP, SIG_IGN); // Ignore SIGTSTP signal
  signal(SIGCHLD, SIG_IGN); // Ignore SIGCHLD signal

  while (1) // This loop is used to run shell until the user exits
  {
    // Print the prompt in the format - currentWorkingDirectory$
    getcwd(cwd, sizeof(cwd));
    printf("%s$", cwd);

    // Input
    chars = getline(&user_input, &buffer_size, stdin);

    // Parse 'user_input' by removing spaces
    int pi = parseInput(inputs, user_input);
    // 'inputs' will contain strings of commands without spaces

    if (strcmp(inputs[0], "exit") == 0) // When user uses exit command
    {
      printf("Exiting shell...\n");
      exit(0);
    }
    else if (strcmp(inputs[0], "") == 0) // When user gives empty input
    {
      continue;
    }
    else
    {
      if (pi == SINGLE_COMMAND) // case of single command
      {
        executeCommand(inputs); // This function is used to run a single command
      }
      else if (pi == SEQUENTIAL_COMMANDS) // case of sequential commands
      {
        executeSequentialCommands(inputs, commands); // This function will run multiple commands in a sequence (which are separated by ##)
      }
      else if (pi == PARALLEL_COMMANDS) // case of parallel commands
      {
        executeParallelCommands(inputs, commands); // This function will run multiple commands in parallel (which are separated by &&)
      }
      else if (pi == COMMAND_REDIRECTION) // case of commands redirection
      {
        executeCommandRedirection(inputs); // This function will run a single command with output redirected to an output file given
      }
    }
  }

  // free space
  free(user_input);

  return 0;
}
