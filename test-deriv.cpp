
#include "re-ast.h"

 // FIXME
 #include "re-deriv_int.h"
 #include "re-classvisitor.h"

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

  printf("%s\n",str);
  Regex::ExpressionPool::Builder b(rex);
  ReParser::run(b,str);

  Regex::expression_t e=b;

auto pred=[](const Regex::detail::LiteralBase *lb) {
  if (auto c=dynamic_cast<const Regex::detail::LiteralWrapper<char> *>(lb)) {
    if (c->value=='a') return true;
  }
  return false;
};
Regex::ClassVisitor<Regex::Derivative<decltype(pred)>> dv(rex,pred);   // or: e=Regex::derivative(rex,pred,b);
rex.visit(b,dv);
e=dv.get();

  Regex::dump(rex,e);

  if (e!=Regex::detail::none) {
    StringBuilder s;
    Regex::BuilderVisitor sv(s);
    rex.visit(e,sv);
  }

  return 0;
}

