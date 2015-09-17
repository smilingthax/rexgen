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
  constexpr expression_t none=-1;

  struct Node;
} // namespace detail

class ExpressionPool {
public:
  ExpressionPool();
  ~ExpressionPool();

  template <typename T>
  expression_t newLiteralValue(T&& lit) {
    return newLiteral(new detail::LiteralWrapper<typename std::decay<T>::type>(std::forward<T>(lit)));
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
  virtual bool preGroup(expression_t cid) { return true; }
  virtual void postGroup() {}
*/

  virtual bool preRepetition(int min,int max,expression_t cid) { return true; }
  virtual void postRepetition(int min,int max) {}

  // NOTE: AST will never call preXXX when there's no child before postXXX
  virtual bool preSequence() { return true; }
  virtual bool nextSequence(int idx,expression_t cid) { return true; }
  virtual void postSequence() {}

  virtual bool preAlternative() { return true; }
  virtual bool nextAlternative(int idx,expression_t cid) { return true; }
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

class ExpressionPool::Builder : public BuilderBase {
public:
  Builder(ExpressionPool &pool); // must be out-of-line
//Builder(Builder &&rhs); // FIXME?
  ~Builder();

  void empty() override;
  void literal(detail::LiteralBase *lb) override;
  void expression(expression_t e); // non-override; (and supports detail::none)

  // caller has to provide alternative_t/sequence_t/group_t storage
// FIXME: not supported in AST yet
  group_t grp() override;
  void end(group_t &g) override;

  void rep(int min=0,int max=-1) override;

  // as a special extension, this builder handles None/missing
  void begin(sequence_t &s) override;
  void seq(sequence_t &s) override;
  void end(sequence_t &s) override;

  void begin(alternative_t &a) override;
  void alt(alternative_t &a) override;
  void end(alternative_t &a) override;

  // NOTE: unfinished group_t/... will only be destroyed by ~Opaque
  void end() override;

  // more non-overrides:
  operator expression_t() const {
    return current;
  }

  expression_t release(); // or None;  can be used together with expression() to reuse that subtree

protected:
  bool isNone() const;
  bool isEmpty() const;
  void ensureNone() const;

private:
  ExpressionPool &pool;
  expression_t current;
};

} // namespace Regex

#endif
