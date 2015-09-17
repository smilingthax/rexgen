#ifndef _RE_STACKVISITOR_H
#define _RE_STACKVISITOR_H

#include "re-ast.h"

#include <vector>

namespace Regex {

template <typename T>
class StackVisitor : public ExpressionPool::Visitor {
public:
  virtual void beginSequence() {}
  virtual void endSequence() {}
  virtual void beginAlternative() {}
  virtual void endAlternative() {}

  bool preSequence() override {
    stack.emplace_back();
    beginSequence();
    return true;
  }
  void postSequence() override {
    endSequence();
    stack.pop_back();
  }
  bool preAlternative() override {
    stack.emplace_back();
    beginAlternative();
    return true;
  }
  void postAlternative() override {
    endAlternative();
    stack.pop_back();
  }

  T &current() { return stack.back(); }
  size_t depth() const { return stack.size(); }

private:
  std::vector<T> stack;
};

class BuilderVisitor : public StackVisitor<Opaque> {
public:
  BuilderVisitor(BuilderBase &builder) : builder(builder) {}

  void empty() override {
    builder.empty();
  }
  void literal(const Regex::detail::LiteralBase *lb) override {
    builder.literal(lb->clone());
  }

/* TODO ... not yet
  bool preGroup(expression_t cid) override { return true; }
  void postGroup() override {}
*/

  void postRepetition(int min,int max) override {
    builder.rep(min,max);
  }

  void beginSequence() override {
    BuilderBase::sequence_t s;
    builder.begin(s);
    current()=std::move(s);
  }
  bool nextSequence(int idx,expression_t cid) override {
    if (idx>0) {
      BuilderBase::sequence_t s(std::move(current()));
      builder.seq(s);
      current()=std::move(s);
    }
    return true;
  }
  void endSequence() override {
    // AST guarantee: there has been at least one nextSequence before this
    BuilderBase::sequence_t s(std::move(current()));
    builder.end(s);
  }

  void beginAlternative() override {
    BuilderBase::alternative_t a;
    builder.begin(a);
    current()=std::move(a);
  }
  bool nextAlternative(int idx,expression_t cid) override {
    if (idx>0) {
      BuilderBase::alternative_t a(std::move(current()));
      builder.alt(a);
      current()=std::move(a);
    }
    return true;
  }
  void endAlternative() override {
    BuilderBase::alternative_t a(std::move(current()));
    builder.end(a);
  }

  void end() override {
    builder.end();
  }

private:
  BuilderBase &builder;
};

} // namespace Regex

#endif
