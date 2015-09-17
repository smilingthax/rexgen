#include "re-ast.h"
#include <stdio.h>

namespace Regex {

class Dumper : public ExpressionPool::Visitor {
public:
  void literal(const detail::LiteralBase *lb) override {
// FIXME
if (auto s=dynamic_cast<const detail::LiteralWrapper<std::string> *>(lb)) {
    printf("%d:[%s]",id(),s->value.c_str());
} else if (auto c=dynamic_cast<const Regex::detail::LiteralWrapper<char> *>(lb)) {
    printf("%d:[%c]",id(),c->value);
} else
    printf("%d:[]",id());
  }

  bool preRepetition(int min,int max,expression_t cid) override {
    printf(" (");
    return true;
  }

  void postRepetition(int min,int max) override {
    printf(")%d{%d,%d}",id(),min,max);
  }

  bool preSequence() override {
    printf("%d ",id());
    return true;
  }

  bool nextSequence(int idx,expression_t cid) override {
    if (idx>0) printf(" ");
    return true;
  }

  void postSequence() override {
  }

  bool preAlternative() override {
    printf("%d:(",id());
    return true;
  }

  bool nextAlternative(int idx,expression_t cid) override {
    if (idx>0) {
      printf(" | ");
    }
    return true;
  }

  void postAlternative() override {
    printf(")");
  }

  void end() override {
    printf("\n");
  }
};

void dump(ExpressionPool &pool,expression_t a)
{
  Dumper d;
  pool.visit(a,d);
}

} // namespace Regex
