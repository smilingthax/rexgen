#ifndef _RE_DERIV_INT_H
#define _RE_DERIV_INT_H

#include "re-ast.h"

#include <assert.h>

namespace Regex {

template <typename Predicate>
class Derivative {
public:
  Derivative(ExpressionPool &out,Predicate pred={}) : out(out),pred(pred) {}

  void empty();

  void literal(const detail::LiteralBase *lb);

//  struct Group;
  struct Repetition;
  struct Sequence;
  struct Alternative;

  operator expression_t() const {
    return (expression_t)out;
  }

private:
  ExpressionPool::Builder out;
  Predicate pred;

  bool nullable=false; // parallels Builder (out.current)
};

template <typename Predicate>
void Derivative<Predicate>::empty()
{
  nullable=true;
  out.expression(detail::none);
}

template <typename Predicate>
void Derivative<Predicate>::literal(const detail::LiteralBase *lb)
{
  nullable=false;
  if (pred(lb)) {
    out.empty();
  } else {
    out.expression(detail::none);
  }
}

/*
template <typename Predicate>
struct Derivative<Predicate>::Group {
  Group(Derivative &outer) : outer(outer),out(outer.out) {}

  void begin(expression_t cid) {}
  void end() {}

private:
  Derivative &outer;
  ExpressionPool::Builder &out;
};
*/

// δ a{n,m}  ->  δ(a) SEQ a{max(n-1,0),(m>0) ? m-1 : -1}   (m==0: None)
// nullable(a{n,m}) = (n==0) || nullable(a)
template <typename Predicate>
struct Derivative<Predicate>::Repetition {
  Repetition(Derivative &outer) : outer(outer),out(outer.out) {}

  bool begin(int _min,int _max,expression_t _cid) {
    if (_max==0) {
assert(false);
      outer.nullable=true; // (min==0)
      out.expression(detail::none);
      return false;
    }
    outer.nullable=false;  // esp. nullable(None{n,m}); further handled in end()
    out.begin(seq);
    min=_min; // tie(min,max,cid)=tie(_min,_max,_cid);
    max=_max;
    cid=_cid;
    return true;
  }

  void end() {
    if (min==0) {
      outer.nullable=true;
    } // else outer.nullable=nullable;

    out.seq(seq);
    if (min>0) {
      min--;
    }
    if (max>0) {
      max--;
    }
    out.expression(cid);
    out.rep(min,max);
    out.end(seq);
  }

private:
  Derivative &outer;
  ExpressionPool::Builder &out;

  int min,max;
  expression_t cid;
  BuilderBase::sequence_t seq;
};

// δ(a SEQ b SEQ c ...) = δ(a) SEQ b SEQ c ...  ALT  nullable(a) SEQ δ(b) SEQ c ...  ALT  nullable(a SEQ b) SEQ δ(c) SEQ ...  ALT ...
// nullable(a SEQ b) = nullable(a) && nullable(b)
template <typename Predicate>
struct Derivative<Predicate>::Sequence {
  Sequence(Derivative &outer) : outer(outer),out(outer.out) {}

  void begin() {
    outer.nullable=false;
    active=true;
  }
  bool next(int idx,expression_t cid) {
    assert(childs.size()==(size_t)idx);
    childs.push_back(cid);
    if (idx==0) {
      return true;
    } else if (!active) { // not calculating derivatives any more
      assert(idx>1);
      assert(!outer.nullable);
      return false;
    }
    dchilds.push_back(out.release());
    active=outer.nullable;
    return active; // derivative still needed?
  }
  void end() {
    if (active) {
      dchilds.push_back(out.release());
    }

    BuilderBase::alternative_t alt;
    out.begin(alt);
    for (size_t i=0; i<dchilds.size(); i++) {
      out.alt(alt); // (i==0 is safely skipped by builder)
      BuilderBase::sequence_t seq;
      out.begin(seq);
      out.expression(dchilds[i]);
      for (size_t j=i+1; j<childs.size(); j++) {
        out.seq(seq);
        out.expression(childs[j]);
      }
      out.end(seq);
    }
    out.end(alt);
  }

private:
  Derivative &outer;
  ExpressionPool::Builder &out;

  bool active;
  std::vector<expression_t> childs,dchilds;
};

// δ(a ALT b ALT c ...) = δ(a) ALT δ(b) ALT δ(c) ...
// nullable(a ALT b) = nullable(a) || nullable(b)
template <typename Predicate>
struct Derivative<Predicate>::Alternative {
  Alternative(Derivative &outer) : outer(outer),out(outer.out) {}

  void begin() {
    outer.nullable=false;
    nullable=false;
    out.begin(alt);
  }
  void next(int idx,expression_t cid) {
    if (idx>0) {
      nullable=nullable || outer.nullable;
      out.alt(alt);
      outer.nullable=false;
    }
  }
  void end() {
    outer.nullable=nullable || outer.nullable;
    out.end(alt);
  }

private:
  Derivative &outer;
  ExpressionPool::Builder &out;

  bool nullable;
  BuilderBase::alternative_t alt;
};

} // namespace Regex

#endif
