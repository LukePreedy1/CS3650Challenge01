#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokenizer.h"
#include "svec.h"

void op_check(svec*);
void execute(svec*);
void execute_exclusive(svec*);
void execute_cd(svec*);

void
check_rv(int rv) {
  if (rv == -1) {
    perror("fail");
    exit(1);
  }
}

// executes the given tokens, if a paren is found
void
execute_parens(svec* tokens, int index) {
  int paren_count = 1;
  int close_index = index+1;

  while(paren_count > 0) {
    // if the next token is a ( then increment the paren_count
    if (strcmp(svec_get(tokens, close_index), "(") == 0) {
      paren_count++;
      close_index++;
    }
    // if the next token is a ( then decrement the paren_count
    else if (strcmp(svec_get(tokens, close_index), ")") == 0) {
      paren_count--;
      if (paren_count == 0) { // if this closes the original paren, break the loop
        break;
      }
      close_index++;
    }
    else {
      close_index++;
    }
  }

  svec* inside = make_svec();

  // adds all the tokens inside the parens to the svec inside
  for (int ii = 1; ii < close_index; ii++) {
    svec_push_back(inside, svec_get(tokens, ii));
  }

  // run the op_check on the tokens inside the parens
  op_check(inside);
  free_svec(inside);
}

// executes the given tokens in the background
void
execute_background(svec* tokens) {
  int cpid;

  if ((cpid = fork())) {
    return;
  }
  else {
    svec* left_tokens = make_svec();

    for (int ii = 0; ii < tokens->size - 1; ++ii) { // copies all tokens in tokens before the & to left_tokens
      char* temp = svec_get(tokens, ii);
      svec_push_back(left_tokens, temp);
    }

    if (svec_get(left_tokens, 0)[0] == '(') op_check(left_tokens);
    else execute_exclusive(left_tokens);

    free_svec(left_tokens);
    exit(0);
  }
}

// will execute the given tokens, without forking, so the process will end after its done
void
execute_exclusive(svec* tokens) {
  if (strcmp(svec_get(tokens, 0), "cd") == 0) {
    execute_cd(tokens);
  }

  char* cmd_name = svec_get(tokens, 0);
  if (tokens->size == 1) {
    char* args[] = {cmd_name, 0};
    execvp(cmd_name, args);
  }
  char* args[tokens->size+1];

  for (int ii = 0; ii < tokens->size; ++ii) {
    args[ii] = svec_get(tokens, ii);
  }

  args[tokens->size] = 0; // args is now null terminated

  execvp(cmd_name, args);
}

// executes the 'or' command
void
execute_or(svec* tokens, int ii) {
  svec* left_tokens = make_svec();
  svec* right_tokens = make_svec();

  for (int jj = 0; jj < ii; ++jj) { // copies all tokens in tokens before || to left_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(left_tokens, temp);
  }

  for (int jj = ii + 1; jj < tokens->size; ++jj) { // copies all tokens in tokens after || to right_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(right_tokens, temp);
  }

  int cpid;

  if ((cpid = fork())) {
    int status;
    waitpid(cpid, &status, 0);
    if (status != 0) { op_check(right_tokens); }  // only difference between this and 'and', will check if the left is not true, then run the right
    free_svec(left_tokens);
    free_svec(right_tokens);
  }
  else {
    if (left_tokens->size > 1) {
      char* cmd_name = svec_get(left_tokens, 0);
      char* args[256];
      for (int jj = 0; jj < left_tokens->size; ++jj) {
        args[jj] = svec_get(left_tokens, jj);
      }
      execvp(cmd_name, args);
    }
    else {
      char* cmd_name = svec_get(left_tokens, 0);
      execlp(cmd_name, cmd_name, (char*)NULL);
    }
  }
}


// executes the 'and' command
void
execute_and(svec* tokens, int ii) {
  svec* left_tokens = make_svec();
  svec* right_tokens = make_svec();

  for (int jj = 0; jj < ii; ++jj) { // copies all tokens in tokens before && to left_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(left_tokens, temp);
  }

  for (int jj = ii + 1; jj < tokens->size; ++jj) { // copies all tokens in tokens after && to right_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(right_tokens, temp);
  }

  int cpid;

  if ((cpid = fork())) {
    int status;
    waitpid(cpid, &status, 0);
    if (status == 0) {
      if (strcmp(svec_get(left_tokens, 0), "cd") == 0) execute_cd(left_tokens);
      op_check(right_tokens);
    }
  }
  else {
    execute_exclusive(left_tokens);
  }
}

void
execute_cd(svec* tokens) {
  char* dest = svec_get(tokens, 1);
  chdir(dest);
}

void
execute_redirect_input(svec* tokens, int ii) {
  char* input_location = svec_get(tokens, ii + 1);  // the location of where to get the input from;

  int cpid;

  if ((cpid = fork())) {
    int stdin_copy = dup(0);
    waitpid(cpid, 0, 0);  // wait for the child
    close(0);
    dup(stdin_copy);
  }
  else {
    char* cmd_name = svec_get(tokens, 0);

    close(0); // closes stdin
    int il = fopen(input_location, "r");
    check_rv(il);
    dup(il);  // sets stdin to the given txt file

    execlp(cmd_name, cmd_name, (char*)NULL);  // executes the given command, which will take input from stdin, which is now the given txt file
  }
}

// executes a command that has a redirect output operator in it
void
execute_redirect_output(svec* tokens, int ii) {
  char* output_location = svec_get(tokens, ii + 1); // the location of where the output will be printed

  svec* left_tokens = make_svec();

  for (int jj = 0; jj < ii; ++jj) { // copies all tokens in tokens before the > to left_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(left_tokens, temp);
  }

  int cpid;

  if ((cpid = fork())) {
    int stdout_copy = dup(1);
    waitpid(cpid, 0, 0);  // wait for child
    close(1);
    dup(stdout_copy); // puts the stdout back where it belongs
    free_svec(left_tokens);
  }
  else {
    close(1); // closes stdout

    FILE *ol = fopen(output_location, "w");
    dup(ol); // replaces it with the output file
    char* cmd_name = svec_get(left_tokens, 0);

    char* args[left_tokens->size+1];

    for (int jj = 0; jj < left_tokens->size; ++jj) {
      args[jj] = svec_get(left_tokens, jj);
    }

    args[left_tokens->size] = 0; // args is now null terminated

    execvp(cmd_name, args);
  }
}

void
execute_pipe(svec* tokens, int ii) {
  int rv;

  svec* left_tokens = make_svec();
  svec* right_tokens = make_svec();

  for (int jj = 0; jj < ii; ++jj) { // copies all tokens in tokens before | to left_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(left_tokens, temp);
  }

  if (tokens->size > ii) {  // must make sure that there is something to copy after the pipe
    for (int jj = ii+1; jj < tokens->size; ++jj) {  // copies all tokens in tokens after the | to right_tokens
      char* temp = svec_get(tokens, jj);
      svec_push_back(right_tokens, temp);
    }
  }
  else {  // if there is nothing after |, will exit with error
    perror("Must have something after the pipe\n");
    exit(1);
  }

  int pipes[2];
  rv = pipe(pipes);
  check_rv(rv);

  int cpid;

  if ((cpid = fork())) {
    // parent
    close(pipes[1]);

    int status;
    int stdout_copy = dup(1);
    waitpid(cpid, &status, 0);
    close(1);
    dup(stdout_copy);

    int stdin_copy = dup(0);
    close(0);
    dup(pipes[0]);
    op_check(right_tokens);
    close(0);
    dup(stdin_copy);
    free_svec(left_tokens);
    free_svec(right_tokens);
  }
  else {
    // child
    close(pipes[0]);
    close(1);

    rv = dup(pipes[1]);
    check_rv(rv);

    op_check(left_tokens);  // rather than printing output to stdout, will now print to pipes[1]
    exit(0);
  }
}

void
execute_semicolon(svec* tokens, int ii) {
  svec* left_tokens = make_svec();
  svec* right_tokens = make_svec();

  for (int jj = 0; jj < ii; ++jj) { // copies all tokens in tokens before ; to left_tokens
    char* temp = svec_get(tokens, jj);
    svec_push_back(left_tokens, temp);
  }

  for (int jj = ii + 1; jj < tokens->size; ++jj) {
    char* temp = svec_get(tokens, jj);
    svec_push_back(right_tokens, temp);
  }

  int cpid;

  if ((cpid = fork())) {
    int status;
    waitpid(cpid, &status, 0);
    int es = WEXITSTATUS(status);
    if (es == 0) {
      op_check(right_tokens);
    }

    free_svec(left_tokens);
    free_svec(right_tokens);
  }
  else {
    op_check(left_tokens);
    exit(0);
  }
}

void
op_check(svec* tokens) {
  int depth = 0;
  if (tokens->size > 0) { // make sure there is actually something to execute
    if (strcmp(svec_get(tokens, 0), "exit") == 0) { // if the first token is "exit", exits the program
      exit(66);
    }

    if (strcmp(svec_get(tokens, tokens->size - 1), "&") == 0) { // if the last token in tokens is '&', will run this program in the background
      execute_background(tokens);
      return;
    }

    for (int ii = 0; ii < tokens->size; ++ii) { // first check for pipes, if one exists, recur on that
      char* tok = svec_get(tokens, ii);
      if (strcmp(tok, "|") == 0) {  // if the ii-th token is pipe:
        execute_pipe(tokens, ii); // calls execute_pipe, passing the tokens and the index of the pipe
        return;
      }
    }

    depth = 0;
    for (int ii = 0; ii < tokens->size; ++ii) { // next check for semicolons.  if one exists, recur on that
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') {
        depth++;
      }
      else if (tok[0] == ')') {
        depth--;
      }
      else if (tok[0] == ';' && depth == 0) {
        execute_semicolon(tokens, ii);
        return;
      }
    }

    depth = 0;
    for (int ii = 0; ii < tokens->size; ++ii) { // next check for ... you get the pattern
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') depth++;
      else if (tok[0] == ')') depth--;
      else if (strcmp(tok, "&&") == 0 && depth == 0) {
        execute_and(tokens, ii);
        return;
      }
    }

    depth = 0;
    for (int ii = 0; ii < tokens->size; ++ii) {
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') depth++;
      else if (tok[0] == ')') depth--;
      else if (strcmp(tok, "||") == 0 && depth == 0) {
        execute_or(tokens, ii);
        return;
      }
    }

    depth = 0;
    for (int ii = 0; ii < tokens->size; ++ii) {
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') depth++;
      else if (tok[0] == ')') depth--;
      else if (tok[0] == '>' && depth == 0) {
        execute_redirect_output(tokens, ii);
        return;
      }
    }

    depth = 0;
    for (int ii = 0; ii < tokens->size; ++ii) {
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') depth++;
      else if (tok[0] == ')') depth--;
      else if (tok[0] == '<' && depth == 0) {
        execute_redirect_input(tokens, ii);
        return;
      }
    }

    for (int ii = 0; ii < tokens->size; ++ii) {
      char* tok = svec_get(tokens, ii);
      if (tok[0] == '(') {
        execute_parens(tokens, ii);
        return;
      }
    }

    if (strcmp(svec_get(tokens, 0), "cd") == 0) { // if the first token is "cd", execute the cd command
      execute_cd(tokens);
      return;
    }

    execute(tokens);
  }
}

void
execute(svec* tokens) {
  if (tokens->size == 0) exit(0);

  int cpid;

  if ((cpid = fork())) {
    waitpid(cpid, 0, 0);
    return;
  }
  else {
    char* cmd_name = svec_get(tokens, 0);
    if (tokens->size == 1) {
      char* args[] = {cmd_name, 0};
      execvp(cmd_name, args);
    }
    char* args[tokens->size+1];

    for (int ii = 0; ii < tokens->size; ++ii) {
      args[ii] = svec_get(tokens, ii);
    }

    args[tokens->size] = 0; // args is now null terminated

    execvp(cmd_name, args);
  }
}

int
main(int argc, char* argv[]) {

  char line[256];

  for (;;)
  {
    char* temp;

    if (argc == 1) {  // if no extra input is given
        printf("nush$ ");
        fflush(stdout);
        temp = fgets(line, 256, stdin);

        if (!temp) { break; }
    }
    else {  // if extra input is given, only operates on that given input, then returns 0
      fflush(stdout);
      temp = argv[1];

      char* temp2;

      FILE* in = fopen(temp, "r");

      char* new_line = malloc(256 * sizeof(char));
      svec* lines = make_svec();

      do {
        temp2 = fgets(new_line, 256, in);
        if (!temp2) { break; }

        svec_push_back(lines, new_line);

      } while (temp2);
      fclose(in);

      free(new_line);

      for (int ii = 0; ii < lines->size; ++ii) {
        char* new_new_line = svec_get(lines, ii);
        svec* tokens;
        tokens = tokenize(new_new_line);

        // if the tokens end with a \, then add the next line onto the end of this one
        if (strchr(svec_get(tokens, tokens->size - 1), '\\')) {
          // appends the current tokens with the next line, and increments the counter
          tokens = remove_svec(tokens, tokens->size - 1);
          append_svec(tokens, tokenize(svec_get(lines, ++ii)));
        }

        op_check(tokens);
        free_svec(tokens);
        free(new_new_line);
      }
      free(temp2);
      return 0;
    }

    if (strcmp(line, "exit") == 0) { return 0; }

    svec* tokens = tokenize(line); // converts the line into tokens
    op_check(tokens);
    free_svec(tokens);
  }

  return 0;
}
