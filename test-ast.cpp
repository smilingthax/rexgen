#include "re-ast.h"

int main()
{
  Regex::ExpressionPool rex;

  auto x1=rex.newLiteralValue(std::string("test"));
  auto x2=rex.newLiteralValue(std::string("tost"));

  auto x3=rex.newAlternative(rex.newSequence(x1,x2),rex.newSequence(x2,x1));

  printf("%d %d %d\n",x1,x2,x3);

  return 0;
}

