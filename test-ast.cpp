#include "re-ast.h"

#include "re-stackvisitor.h"

#include "stringbuilder.h"

namespace Regex {
void dump(ExpressionPool &pool,expression_t a);
}

int main()
{
  Regex::ExpressionPool rex;

  auto x1=rex.newLiteralValue(std::string("test"));
  auto x2=rex.newLiteralValue(std::string("tost"));

  auto x3=rex.newAlternative(rex.newSequence(x1,x2),rex.newRepetition(rex.newSequence(x2,x1)));

  printf("%d %d %d\n",x1,x2,x3);

  Regex::dump(rex,x3);

//  Regex::ExpressionPool::Builder b;
  StringBuilder b;
  Regex::BuilderVisitor bv(b);
  rex.visit(x3,bv);

  return 0;
}

