#include "re-ast_int.h"
#include <stdio.h>
#include <assert.h>

// exceptions. (?)

namespace Regex {

namespace {
//ExpressionPool::Builder::group_t::Accessor<detail::Group *> access_grp;
ExpressionPool::Builder::sequence_t::Accessor<detail::Sequence *> access_seq;
ExpressionPool::Builder::alternative_t::Accessor<detail::Alternative *> access_alt;
} // namespace

ExpressionPool::Builder::Builder(ExpressionPool &pool)
  : pool(pool),current(detail::none)
{
}

ExpressionPool::Builder::~Builder() =default;

void ExpressionPool::Builder::empty()
{
  ensureNone();
  current=0;
}

void ExpressionPool::Builder::literal(detail::LiteralBase *lb)
{
  ensureNone();
  current=pool.newLiteral(lb);
}

void ExpressionPool::Builder::expression(expression_t e)
{
  ensureNone();
  current=e;
}

// FIXME: not in ast yet
ExpressionPool::Builder::group_t ExpressionPool::Builder::grp()
{
  ensureNone();
  group_t ret;
//  ret.set(access_grp,new detail::Group);
  return ret;
}

void ExpressionPool::Builder::end(group_t &g)
{
// g.release(access_grp);
  // if (isNone()) discard; else begin/endGroup;
}

void ExpressionPool::Builder::rep(int min,int max)
{
  if (!isNone()) {
    current=pool.newRepetition(current,min,max);
  } else {
    if (min==0) {
      empty();
    } // else: discard
  }
}

void ExpressionPool::Builder::begin(sequence_t &s)
{
  ensureNone();
assert(!s.is(access_seq));
  auto node=new detail::Sequence; // ? unique_ptr?
  s.set(access_seq,node);
}

void ExpressionPool::Builder::seq(sequence_t &s)
{
assert(s.is(access_seq));
  if (isNone()) {
    s.set(access_seq,nullptr); // -> set all discarded
    return; // current=none
  }
  auto node=s.get(access_seq);
  if ( (node)&&(!isEmpty()) ) {
    pool.get(current)->addTo(*node);
  }
  current=detail::none;
}

void ExpressionPool::Builder::end(sequence_t &s)
{
assert(s.is(access_seq));
  std::unique_ptr<detail::Sequence> node(s.release(access_seq));
  if ( (!node)||(isNone()) ) { // discard all
    current=detail::none;
    return; // unused node is freed by unique_ptr
  }

  if (node->childs.empty()) {
    return; // current="child" (maybe empty)
  } else if (isEmpty()) {
    if (node->childs.size()==1) {
      current=node->childs.front()->id;
      return;
    }
  } else { // !isEmpty()
    pool.get(current)->addTo(*node);
  }
assert(node->childs.size()>=2);

  node->calculate_hash();
  current=pool.add(std::move(node));
}

void ExpressionPool::Builder::begin(alternative_t &a)
{
  ensureNone();
  a.set(access_alt,nullptr); // initial value: discarded
}

void ExpressionPool::Builder::alt(alternative_t &a)
{
  if (isNone()) {
    return; // current=none
  }
  auto node=a.get(access_alt);
  if (!node) {
assert(a.is(access_alt));
    node=new detail::Alternative;
    a.set(access_alt,node); // not discarded any more
  }
  pool.get(current)->addTo(*node);
  current=detail::none;
}

void ExpressionPool::Builder::end(alternative_t &a)
{
assert(a.is(access_alt));
  std::unique_ptr<detail::Alternative> node(a.release(access_alt));
  if (!node) {
    return; // current=none / "child";
  }

  pool.get(current)->addTo(*node);
  if (node->childs.size()==1) {
    return; // dup (against current!)
  }
assert(node->childs.size()>=2);

  node->calculate_hash();
  current=pool.add(std::move(node));
}

void ExpressionPool::Builder::end()
{
  if (isNone()) {
    fprintf(stderr,"Warning(Builder): Unexpected end\n");
  }
// ... ? mark as ended?
}

expression_t ExpressionPool::Builder::release()
{
  expression_t ret=current;
  current=detail::none;
  return ret;
}

bool ExpressionPool::Builder::isNone() const
{
  return (current==detail::none);
}

bool ExpressionPool::Builder::isEmpty() const
{
  return pool.isEmpty(current);
}

void ExpressionPool::Builder::ensureNone() const
{
  if (!isNone()) {
    fprintf(stderr,"Warning(Builder): Overwriting old current\n");
  }
}

} // namespace Regex

