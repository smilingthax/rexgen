#include "stringparser.h"
#include <stdio.h>
#include <assert.h>

// TODO?  exceptions instead of false ?

bool ReParser::operator()()
{
  if (!alternative()) {
    return false;
  }
  if (*str) {
    fprintf(stderr,"Unsupported char '%c'\n",*str);
    return false;
  }
  out.end();
  return true;
}

bool ReParser::group()
{
  if (!alternative()) {
    return false;
  }
  if (!*str) {
    fprintf(stderr,"Unterminated group\n");
    return false;
  } else if (*str!=')') {
    fprintf(stderr,"Expected ')', got unsupported '%c'\n",*str);
    return false;
  }
  str++;
  return true;
}

static inline bool isRep(char c)
{
  return ( (c=='*')||(c=='+')||(c=='?')||(c=='{') );
}

bool ReParser::term()
{
  if ( (!*str)||(*str==')')||(*str=='|') ) {
    out.empty();
    return true;
  } else if (*str=='(') { // group
    str++;
    if (*str=='?') {
      str++;
      if (*str!=':') {
        fprintf(stderr,"Unsupported '(?' sequence\n");
        return false;
      }
      str++;
      // non-capturing group
      if (!group()) {
        return false;
      }
    } else { // capturing group
      Regex::BuilderBase::group_t g=out.grp();
      if (!group()) {
        return false;
      }
      out.end(g);
    }
#if 0
  } else if (*str=='\\') { // escaped literal
    str++;
    switch (*str) {
//    case 'n': out.literalValue('\n'); break;
    case '{': case '}':
    case '(': case ')':
    case '|': case '\\': case '*': case '+': case '?':
      out.literalValue(*str);
      str++;
      break;
    default:
      fprintf(stderr,"Unsupported escape sequence: '\\%c'\n",*str);
      return false;
    }
#endif
  } else if ( (*str>='a')&&(*str<='z') ) { // literal
    out.literalValue(*str);
    str++;
  } else {
    fprintf(stderr,"Unexpected '%c'\n",*str);
    return false; // or, for isRep(*str): just warning?  [str++?]
  }
  return true;
}

bool ReParser::repspec()
{
  switch (*str) {
  case '*':
    out.rep();
    break;
  case '+':
    out.rep(1);
    break;
  case '?':
    out.rep(0,1);
    break;
  case '{': {
    int min=0,max=-1;

    str++;
    if ( (*str>='0')&&(*str<='9') ) { // not: '-', ',', '}'
      min=strtoul(str,const_cast<char **>(&str),10);
    // } else if (*str==',') { // "gnu extension {,m} not supported" ?
    } // else: {} -> {0}
    if (*str==',') {
      str++;
      if ( (*str>='0')&&(*str<='9') ) { // not: '-', '}'
        max=strtoul(str,const_cast<char **>(&str),10);
      }
      // {,} -> {0,-1}
    } else { // (*str=='}')
      max=min;
    }
    if (*str!='}') {
      fprintf(stderr,"Could not parse {,}\n");
      return false;
    }
    out.rep(min,max);
  } break;
  default:
    assert(false);
  }
  str++;
  return true;
}

bool ReParser::repetition()
{
  if (!term()) {
    return false;
  }
  if (isRep(*str)) {
    if (!repspec()) {
      return false;
    }
    if (*str=='?') {
      fprintf(stderr,"Lazy repetition not supported\n");
      return false;
    } else if (isRep(*str)) {
      fprintf(stderr,"Duplicate repetition\n");
      return false;
    }
  }
  return true;
}

bool ReParser::sequence()
{
  Regex::BuilderBase::sequence_t s;
  out.begin(s);
  while (repetition()) {
    if ( (!*str)||(*str==')')||(*str=='|') ) {
      out.end(s);
      return true;
    }
    out.seq(s);
  }
  return false;
}

bool ReParser::alternative()
{
  Regex::BuilderBase::alternative_t a;
  out.begin(a);
  while (sequence()) {
    if (*str!='|') {
      out.end(a);
      return true;
    }
    out.alt(a);
    str++;
  }
  return false;
}

