#ifndef _STRINGPARSER_H
#define _STRINGPARSER_H

#include "re-builder_base.h"

class ReParser {
public:
  ReParser(Regex::BuilderBase &out,const char *str) : out(out),str(str) {}

  bool operator()();

  static bool run(Regex::BuilderBase &out,const char *str) {
    return ReParser(out,str)();
  }

protected:
  bool group();
  bool term();
  bool repspec();
  bool repetition();
  bool sequence();
  bool alternative();

private:
  Regex::BuilderBase &out;
  const char *str;
};

#endif
