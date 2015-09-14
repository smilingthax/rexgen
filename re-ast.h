#ifndef _RE_AST_H
#define _RE_AST_H

#include "re-literal.h"
#include "re-builder_base.h"

#include <vector>
#include <memory>
#include <unordered_map>

namespace Regex {

typedef unsigned int expression_t;

namespace detail {
  struct Node;
} // namespace detail

class ExpressionPool {
public:
  ExpressionPool();
  ~ExpressionPool();

  template <typename T>
  expression_t newLiteralValue(T&& lit) {
    return newLiteral(new detail::LiteralWrapper<T>(std::forward<T>(lit)));
  }
  expression_t newLiteral(detail::LiteralBase *lb);

  expression_t newRepetition(expression_t a,int min=0,int max=-1);
  expression_t newSequence(expression_t a,expression_t b);
  expression_t newAlternative(expression_t a,expression_t b);

  class Visitor;
  void visit(expression_t a,Visitor &visitor) const;

  class Builder;

protected:
  const detail::Node *get(expression_t a) const;
  expression_t add(std::unique_ptr<detail::Node> &&node);
  bool isEmpty(expression_t a) const;

private:
  std::vector<std::unique_ptr<detail::Node>> pool;
  std::unordered_map<const detail::Node *,expression_t,
                     detail::hash_result_t (*)(const detail::Node *),
                     bool (*)(const detail::Node *,const detail::Node *)> mapping;
};

// no-one except the real AST can drive the visitor (e.g. see friends)
class ExpressionPool::Visitor {
public:
  virtual ~Visitor() {}

  virtual void empty() {}
  virtual void literal(const detail::LiteralBase *lb) {}

/* TODO ... not yet
  virtual bool preGroup() { return true; }
  virtual void postGroup() {}
*/

  virtual bool preRepetition(int min,int max) { return true; }
  virtual void postRepetition(int min,int max) {}

  virtual bool preSequence() { return true; }
  virtual bool nextSequence(int idx) { return true; }
  virtual void postSequence() {}

  virtual bool preAlternative() { return true; }
  virtual bool nextAlternative(int idx) { return true; }
  virtual void postAlternative() {}

  virtual void end() {} // convenience

  expression_t id() const {
    return current;
  }
private:
  friend class detail::Node;
  friend class ExpressionPool;
  expression_t current=-1;
};

class ExpressionPool::Builder : public Builder_base {
public:
  Builder(ExpressionPool &pool); // must be out-of-line
//Builder(Builder &&rhs); // FIXME?
  ~Builder();

  void empty() override;
  void literal(detail::LiteralBase *lb) override;

// TODO?!   void expression(expression_t e);  // [non-override]

  // caller has to provide alternative_t/sequence_t/group_t storage
// FIXME: not supported in AST yet
  group_t grp() override;
  void end(group_t &g) override;

  void rep(int min=0,int max=-1) override;

  void begin(sequence_t &s) override;
  void seq(sequence_t &s) override;
  void end(sequence_t &s) override;

  void begin(alternative_t &s) override;
  void alt(alternative_t &a) override;
  void end(alternative_t &a) override;

  void end() override;

  operator expression_t() {
    return current;
  }

protected:
  void ensureCurrent(bool value=true) const;

private:
  ExpressionPool &pool;
  expression_t current;
};

} // namespace Regex

#endif
