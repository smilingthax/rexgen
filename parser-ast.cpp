
#include "re-ast.h"

#include "stringparser.h"

//#include "re-dump.h"
namespace Regex { void dump(ExpressionPool &pool,expression_t a); }

#include "re-stackvisitor.h"
#include "stringbuilder.h"

int main(int argc,char **argv)
{
//  const char *str="a(bc)*|a|b*";
  const char *str="a(bc)*(?:d{2,4}e){0,1}d|";

  if (argc==2) {
    str=argv[1];
  }

  Regex::ExpressionPool rex;
//auto&& k=rex.builder(); //...
//auto k=rex.builder();

  printf("%s\n",str);
  Regex::ExpressionPool::Builder b(rex);
  ReParser::run(b,str);

  Regex::dump(rex,b);

  StringBuilder s;
  Regex::BuilderVisitor sv(s);
  rex.visit((Regex::expression_t)b,sv);

  return 0;
}

