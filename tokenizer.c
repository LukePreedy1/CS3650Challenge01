// tokenize.c
// Author: Nat Tuck
// 3650F2017, Challenge01 Hints

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tokenizer.h"

typedef int (*char_pred)(char);

int
is_op_char(char cc)
{
    return cc == '<' || cc == '>'
        || cc == '|' || cc == '&'
        || cc == ';';
}

int
is_nop_char(char cc)
{
    return cc != 0 && !is_op_char(cc) && !isspace(cc);
}

char*
get_tok(char* text, char_pred cpred)
{
    char* tt = malloc(256);
    int ii = 0;
    for (; cpred(text[ii]); ++ii) {
      if (text[ii] == ')' || text[ii] == '(') {
        tt[ii] = text[ii];
        ii++;
        break;
      }
        tt[ii] = text[ii];
    }
    tt[ii] = 0;
    return tt;
}


// a special tokenize made to work if there are parens in the input
svec*
tokenize_parens(char* text) {
  svec* xs = make_svec();

  while(text[0]) {
    if (isspace(text[0])) {
      text++;
      continue;
    }
    char* tt;
    // if the first char is a ( or a ), treat it special
    if (text[0] == '(' || text[0] == ')') {
      tt = malloc(256);
      tt[0] = text[0];
      tt[1] = 0;
    }
    // otherwise, just do what regular tokenize does
    else if (is_op_char(text[0])) {
      tt = get_tok(text, is_op_char);
      if (tt[strlen(tt)-1] == ')') tt[strlen(tt)-1] = 0;
    }
    else {
      tt = get_tok(text, is_nop_char);
      if (tt[strlen(tt)-1] == ')') tt[strlen(tt)-1] = 0;
    }

    svec_push_back(xs, tt);
    text += strlen(tt);
    free(tt);
  }
  return xs;
}

// given text that starts at a ", find the end "
char*
get_quotes(char* text) {
  char* tt = malloc(256);
  int ii = 1;
  for (; text[ii] != '\"'; ++ii) {
    tt[ii-1] = text[ii];
  }
  tt[ii] = 0;
  return tt;
}


svec*
tokenize(char* text)
{
    svec* xs = make_svec();

    // if the first char is (, do something special
    if (text[0] == '(' || text[0] == ')') {
      free_svec(xs);
      return tokenize_parens(text);
    }

    while (text[0]) {
        if (isspace(text[0])) {
            text++;
            continue;
        }

        char* tt;

        // if the next token is in quotes, make everything in the quotes one token
        if (text[0] == '\"') {
          tt = get_quotes(text);
          text += 2;
        }
        // if the next char is ), do something else special
        else if (text[0] == ')') {
          tt = malloc(256);
          tt[0] = text[0];
          tt[1] = 0;
        }
        else if (is_op_char(text[0])) {
            tt = get_tok(text, is_op_char);
        }
        else {
            tt = get_tok(text, is_nop_char);
        }
        svec_push_back(xs, tt);
        text += strlen(tt);
        free(tt);
    }
    return xs;
}
