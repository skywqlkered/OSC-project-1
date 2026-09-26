/**
  * Shell framework
  * course Operating Systems
  * Radboud University
  * v22.09.05

  Student names:
  - ...
  - ...
*/

/**
 * Hint: in most IDEs (Visual Studio Code, Qt Creator, neovim) you can:
 * - Control-click on a function name to go to the definition
 * - Ctrl-space to auto complete functions and variables
 */

// function/class definitions you are going to use
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <list>
#include <optional>

// although it is good habit, you don't have to type 'std' before many objects by including this line
using namespace std;

struct Command
{
  vector<string> parts = {};
};

struct Expression
{
  vector<Command> commands;
  string inputFromFile;
  string outputToFile;
  bool background = false;
};

// Parses a string to form a vector of arguments. The separator is a space char (' ').
vector<string> split_string(const string &str, char delimiter = ' ')
{
  vector<string> retval;
  for (size_t pos = 0; pos < str.length();)
  {
    // look for the next space
    size_t found = str.find(delimiter, pos);
    // if no space was found, this is the last word
    if (found == string::npos)
    {
      retval.push_back(str.substr(pos));
      break;
    }
    // filter out consequetive spaces
    if (found != pos)
      retval.push_back(str.substr(pos, found - pos));
    pos = found + 1;
  }
  return retval;
}
// wrapper around the C execvp so it can be called with C++ strings (easier to work with)
// always start with the command itself
// DO NOT CHANGE THIS FUNCTION UNDER ANY CIRCUMSTANCE :3
int execvp(const vector<string> &args)
{
  // build argument list
  const char **c_args = new const char *[args.size() + 1];
  for (size_t i = 0; i < args.size(); ++i)
  {
    c_args[i] = args[i].c_str();
  }
  c_args[args.size()] = nullptr;
  // replace current process with new process as specified
  int rc = ::execvp(c_args[0], const_cast<char **>(c_args));
  // if we got this far, there must be an error
  int error = errno;
  // in case of failure, clean up memory (this won't overwrite errno normally, but let's be sure)
  delete[] c_args;
  errno = error;
  return rc;
}

// Executes a command with arguments. In case of failure, returns error code.
int execute_command(const Command &cmd)
{
  auto &parts = cmd.parts;
  if (parts.size() == 0)
    return EINVAL;
  // execute external commands

  int retval = execvp(parts);

  return retval ? errno : 0;
}

void display_prompt()
{
  char buffer[512];
  char *dir = getcwd(buffer, sizeof(buffer));
  if (dir)
  {
    cout << "\e[32m" << dir << "\e[39m"; // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
  }
  cout << "$ ";
  flush(cout);
}

string request_command_line(bool showPrompt)
{
  if (showPrompt)
  {
    display_prompt();
  }
  string retval;
  getline(cin, retval);
  return retval;
}

// note: For such a simple shell, there is little need for a full-blown parser (as in an LL or LR capable parser).
// Here, the user input can be parsed using the following approach.
// First, divide the input into the distinct commands (as they can be chained, separated by `|`).
// Next, these commands are parsed separately. The first command is checked for the `<` operator, and the last command for the `>` operator.
Expression parse_command_line(string commandLine)
{
  Expression expression;
  vector<string> commands = split_string(commandLine, '|');
  for (size_t i = 0; i < commands.size(); ++i)
  {
    string &line = commands[i];
    vector<string> args = split_string(line, ' ');
    if (i == commands.size() - 1 && args.size() > 1 && args[args.size() - 1] == "&")
    {
      expression.background = true;
      args.resize(args.size() - 1);
    }
    if (i == commands.size() - 1 && args.size() > 2 && args[args.size() - 2] == ">")
    {
      expression.outputToFile = args[args.size() - 1];
      args.resize(args.size() - 2);
    }
    if (i == 0 && args.size() > 2 && args[args.size() - 2] == "<")
    {
      expression.inputFromFile = args[args.size() - 1];
      args.resize(args.size() - 2);
    }
    expression.commands.push_back({args});
  }
  return expression;
}

// // framework for executing "date | tail -c 5" using raw commands
// // two processes are created, and connected to each other
// int pipethatshit(vector<int*> fildes, Command cmd1, Command cmd2)
// {
//   // create communication channel shared between the two processes
//   // ...

//   pid_t child1 = fork();
//   if (child1 == 0)
//   {
//     // redirect standard output (STDOUT_FILENO) to the input of the shared communication channel
//     // free non used resources (why?)

//     dup2(fildes[1], STDOUT_FILENO);

//     close(fildes[0]);
//     close(fildes[1]);
//     execute_command(cmd1);
//     printf("uhoh");
//     // display nice warning that the executable could not be found
//     abort(); // if the executable is not found, we should abort. (why?)
//   }

//   pid_t child2 = fork();
//   if (child2 == 0)
//   {
//     // redirect the output of the shared communication channel to the standard input (STDIN_FILENO).
//     // free non used resources (why?)

//     dup2(fildes[0], STDIN_FILENO);

//     close(fildes[0]);
//     close(fildes[1]);
//     execute_command(cmd2);
//     printf("uhoh");
//     abort(); // if the executable is not found, we should abort. (why?)
//   }

//   close(fildes[0]);
//   close(fildes[1]);

//   // free non used resources (why?)
//   // wait on child processes to finish (why both?)
//   waitpid(child1, nullptr, 0);
//   waitpid(child2, nullptr, 0);
//   return 0;
// }

int make_pipez(int (*fds_arr)[2], int commandamount)
{
  for (int i = 0; i < commandamount - 1; i++)
  {
    if (pipe(fds_arr[i]) < 0)
    {
      return {};
    }
  }
  return 0;
}

int forkengo(Expression &expression, int (*filedes_arr)[2])
{
  int commandamount = expression.commands.size();
  pid_t fork_arr[commandamount];

  for (int i = 0; i < commandamount; i++)
  {
    if (i == 0)
    {
      pid_t child = fork();
      fork_arr[i] = child;
      if (child == 0)
      {
        dup2(filedes_arr[i][1], STDOUT_FILENO);

        for (int i = 0; i < commandamount; i++)
        {
          if (i != commandamount - 1)
          {
            close(filedes_arr[i][0]);
            close(filedes_arr[i][1]);
          }
        }
        execute_command(expression.commands[i]);
        printf("uhoh");
        // display nice warning that the executable could not be found
        abort(); // if the executable is not found, we should abort. (why?)
      }
    }
    else if (i < commandamount - 1)
    {
      pid_t child = fork();
      fork_arr[i] = child;
      if (child == 0)
      {
        dup2(filedes_arr[i - 1][0], STDIN_FILENO);
        dup2(filedes_arr[i][1], STDOUT_FILENO);

        for (int i = 0; i < commandamount; i++)
        {
          if (i != commandamount - 1)
          {
            close(filedes_arr[i][0]);
            close(filedes_arr[i][1]);
          }
        }
        execute_command(expression.commands[i]);
        printf("uhoh");
        // display nice warning that the executable could not be found
        abort();
      }
    }
    else
    {

      pid_t child = fork();
      fork_arr[i] = child;
      if (child == 0)
      {
        dup2(filedes_arr[i - 1][0], STDIN_FILENO);

        for (int i = 0; i < commandamount; i++)
        {
          if (i != commandamount - 1)
          {
            close(filedes_arr[i][0]);
            close(filedes_arr[i][1]);
          }
        }
        // close(filedes_arr[i - 1][1]);
        // close(filedes_arr[i - 1][0]);

        execute_command(expression.commands[i]);
        printf("uhoh");
        // display nice warning that the executable could not be found
        abort();
      }
    }
  }

  for (int i = 0; i < commandamount; i++)
  {
    if (i != commandamount - 1)
    {
      close(filedes_arr[i][0]);
      close(filedes_arr[i][1]);

      // free non used resources (why?)
      // wait on child processes to finish (why both?)
      // wow these are uselessright, (why?)
    }
    waitpid(fork_arr[i], nullptr, 0);
  }
  return 0;
}

int handle_ch(Expression expression)
{
  size_t cmdsize = expression.commands[0].parts.size();

  string fullargument = "";

  char firstchar = expression.commands[0].parts[1].front();
  char lastchar = expression.commands[0].parts.back().back();

  if ((firstchar == (char)34) && (firstchar == lastchar))
  {
    for (int i = 1; i < cmdsize; i++)
    {
      if (i != 1)
      {
        fullargument.append(" ");
      }
      fullargument.append(expression.commands[0].parts[i]);
    }
    fullargument.erase(0, 1);
    fullargument.erase(fullargument.size() - 1, 1);

    size_t idx = fullargument.find("./");
    if ((idx != -1) && cmdsize > 2)
    {
      fullargument.erase(idx, 2);
    }
  }
  else
  {
    fullargument = expression.commands[0].parts[1].c_str();
  }

  chdir(fullargument.c_str());
  return errno;
}

int execute_expression(Expression &expression)
{
  // Check for empty expression
  if (expression.commands.size() == 0)
    return EINVAL;

  // Handle intern commands (like 'cd' and 'exit')

  if (expression.commands[0].parts.size() > 1 && (!strcmp(expression.commands[0].parts[0].c_str(), (const char *)"cd")))
  {
    int rc_ch = handle_ch(expression);
    return rc_ch;
  }

  // External commands, executed with fork():
  // Loop over all commandos, and connect the output and input of the forked processes

  // For now, we just execute the first command in the expression. Disable.
  // execute_command(expression.commands[0]);

  int commandamount = expression.commands.size();

  int fds_arr[commandamount - 1][2];
  make_pipez(fds_arr, commandamount);

  forkengo(expression, fds_arr);
  return 0;
}

int shell(bool showPrompt)
{
  //* <- remove one '/' in front of the other '/' to switch from the normal code to step1 code
  while (cin.good())
  {
    string commandLine = request_command_line(showPrompt);
    Expression expression = parse_command_line(commandLine);

    int rc = execute_expression(expression);

    if (rc != 0)
      cerr << strerror(rc) << endl;
  }
  return 0;
}
