#ifndef _RE_AST_INT_H
#define _RE_AST_INT_H

#include "re-ast.h"
#include "hash-algo.h"

namespace Regex {

namespace detail {

constexpr expression_t none=-1;

using Visitor=ExpressionPool::Visitor;

struct Empty;
struct Literal;
struct Repetition;
struct Sequence;
struct Alternative;

struct Node {
  using HashAlgo=fnv1a;

  virtual ~Node()=default;

  virtual void addTo(Repetition &a) const;
  virtual void addTo(Sequence &s) const;
  virtual void addTo(Alternative &a) const;

  virtual bool operator==(const Node &rhs) const =0;
  bool operator!=(const Node &rhs) const { return !(*this==rhs); }

  // full hash (preorder)
//  virtual void hash_append(std::function<void(const void *,std::size_t)> &h) const =0;  // (HashAlgo &h)
  // "merkle" tree hash
  HashAlgo::result_type hash; // must match hash_result_t

  virtual void visit(Visitor &visitor) const =0;

  expression_t id=none;

protected:
  void setCurrent(Visitor &visitor) const {
    visitor.current=id;
  }
};

struct Empty : Node {
  Empty() { hash=0; }
  void addTo(Repetition &a) const override;
  void addTo(Sequence &s) const override;

  bool operator==(const Node &rhs) const override;

  void visit(Visitor &visitor) const override;
};

struct Literal : Node {
  Literal(std::unique_ptr<LiteralBase> &&data) : data(std::move(data)) { }

  std::unique_ptr<LiteralBase> data;

  void calculate_hash();
  bool operator==(const Node &rhs) const override;

  void visit(Visitor &visitor) const override;
};

struct Repetition : Node {
  Repetition(int min,int max) : child(nullptr),min(min),max(max) {}
  void addTo(Repetition &s) const override;

  const Node *child;
  int min,max;

  void calculate_hash();
  bool operator==(const Node &rhs) const override;

  void visit(Visitor &visitor) const override;
};

struct Sequence : Node {
  void addTo(Sequence &s) const override;

  std::vector<const Node *> childs;

  void calculate_hash();
  bool operator==(const Node &rhs) const override;

  void visit(Visitor &visitor) const override;
};

struct Alternative : Node {
  void addTo(Alternative &a) const override;

  std::vector<const Node *> childs;

  void calculate_hash();
  bool operator==(const Node &rhs) const override;

  void visit(Visitor &visitor) const override;
};

} // namespace detail

} // namespace Regex

#endif
