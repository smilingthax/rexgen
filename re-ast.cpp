#include "re-ast.h"
#include <stdio.h>
#include <assert.h>
#include "hash-algo.h"
#include "merge-sorted.h"

namespace Regex {

namespace detail {

struct Literal;
struct Sequence;
struct Alternative;
struct Repetition;

struct Node {
  using HashAlgo=fnv1a;

  virtual ~Node()=default;

  virtual void addTo(Sequence &s) const;
  virtual void addTo(Alternative &a) const;
  virtual void addTo(Repetition &a) const;

  virtual bool operator==(const Node &rhs) const =0;
  bool operator!=(const Node &rhs) const { return !(*this==rhs); }

  // full hash (preorder)
//  virtual void hash_append(std::function<void(const void *,std::size_t)> &h) const =0;  // (HashAlgo &h)
  // "merkle" tree hash
  HashAlgo::result_type hash; // must match hash_result_t

//  expression_t id=-1;
};

struct Literal : Node {
  Literal(std::unique_ptr<LiteralBase> &&data) : data(std::move(data)) { }

  std::unique_ptr<LiteralBase> data;

  void calculate_hash() {
    HashAlgo h;
    h("Literal",8);
    auto val=data->hash();
    h(&val,sizeof(val));
    hash=(HashAlgo::result_type)h;
  }
  bool operator==(const Node &rhs) const override {
    if (auto rt=dynamic_cast<const Literal *>(&rhs)) {
      return (*data==*rt->data);
    }
    return false;
  }
};

struct Sequence : Node {
  void addTo(Sequence &s) const override;

  std::vector<const Node *> childs;

/*
  void hash_append(HashAlgo &h) const override {
    h("Sequence",9);
    for (auto child : childs) {
      child->hash_append(h);
    }
  }
*/
  void calculate_hash() {
    HashAlgo h;
    h("Sequence",9);
    for (auto child : childs) {
      h(&child->hash,sizeof(child->hash));
    }
    hash=(HashAlgo::result_type)h;
  }
  bool operator==(const Node &rhs) const override {
    if (auto rt=dynamic_cast<const Sequence *>(&rhs)) {
      if (childs.size()!=rt->childs.size()) {
        return false;
      }
      for (size_t i=0; i<childs.size(); i++) {
        if (childs[i]!=rt->childs[i]) { // by ptr
          return false;
        }
      }
      return true;
    }
    return false;
  }
};

struct Alternative : Node {
  void addTo(Alternative &a) const override;

  std::vector<const Node *> childs;

  void calculate_hash() {
    HashAlgo h;
    h("Alternative",12);
    for (auto child : childs) {
      h(&child->hash,sizeof(child->hash));
    }
    hash=(HashAlgo::result_type)h;
  }
  bool operator==(const Node &rhs) const override {
    if (auto rt=dynamic_cast<const Alternative *>(&rhs)) {
      if (childs.size()!=rt->childs.size()) {
        return false;
      }
      for (size_t i=0; i<childs.size(); i++) {
        if (childs[i]!=rt->childs[i]) { // by ptr
          return false;
        }
      }
      return true;
    }
    return false;
  }
};

struct Repetition : Node {
  Repetition(int min,int max) : child(nullptr),min(min),max(max) {}
  void addTo(Repetition &s) const override;

  const Node *child;
  int min,max;

  void calculate_hash() {
    HashAlgo h;
    h("Repetition",11);
    h(&min,sizeof(min));
    h(&max,sizeof(max));
    h(&child->hash,sizeof(child->hash));
    hash=(HashAlgo::result_type)h;
  }
  bool operator==(const Node &rhs) const override {
    if (auto rt=dynamic_cast<const Repetition *>(&rhs)) {
      return (min==rt->min)&&(max==rt->max)&&(child==rt->child);
    }
    return false;
  }
};

void Node::addTo(Sequence &s) const
{
  s.childs.push_back(this);
}

void Sequence::addTo(Sequence &s) const
{
  s.childs.insert(s.childs.end(),childs.begin(),childs.end());
}

void Node::addTo(Alternative &a) const
{
//  auto pos=std::lower_bound(a.childs.begin(),a.childs.end(),this, cmp);
  auto pos=std::lower_bound(a.childs.begin(),a.childs.end(),this);
  if ( (pos!=a.childs.end())&&(*pos==this) ) { // dup (by ptr)
    return;
  }
  a.childs.insert(pos,this);
}

void Alternative::addTo(Alternative &a) const
{
  // merge sorted...  - filter dupes!
  std::vector<const Node *> result;
  result.reserve(a.childs.size()+childs.size());

  merge_sorted(a.childs.begin(),a.childs.end(), childs.begin(),childs.end(),
               [&result](const Node *a) { // only first
                 result.push_back(a);
               },
               [&result](const Node *b) { // only second
                  result.push_back(b);
               },
               [&result](const Node *a,const Node *b) { // both
                 result.push_back(a);
               }); // ,cmp);

  // result.shrink_to_fit();
  a.childs=std::move(result);
}

void Node::addTo(Repetition &r) const
{
  r.child=this;
}

void Repetition::addTo(Repetition &r) const
{
  r.child=this; // unmerged

  // merge them, when possible
  // assert( (min>=0)&&(r.min>=0) );
  if (max==0) { // assert(min==0);
    r.child=child;
    r.min=0;
    r.max=0;
  } else if (r.max==0) { // assert(r.min==0);
    r.child=child;
  } else if ( (min<=1)&&(r.min<=1) ) {
    if ( (max==-1)||(r.max==-1) ) {
      r.child=child;
      if (min<r.min) {
        r.min=0;
      }
      r.max=-1;
    } else if (max==1) {
      r.child=child;
      if (min<r.min) {
        r.min=0;
      }
    } else if (r.max==1) {
      r.child=child;
      if (min<r.min) {
        r.min=0;
      }
      r.max=max;
    }
  }
}

} // namespace detail


ExpressionPool::ExpressionPool()
  : mapping(0,[](const detail::Node *node){ // hashFn
    return node->hash;
  },[](const detail::Node *a,const detail::Node *b){ // equalFn
    return *a==*b;
  })
{
}

ExpressionPool::~ExpressionPool() =default;

expression_t ExpressionPool::newLiteral(detail::LiteralBase *lb)
{
  auto node=std::make_unique<detail::Literal>(std::unique_ptr<detail::LiteralBase>(lb));
  node->calculate_hash();

  return add(std::move(node));
}

expression_t ExpressionPool::newSequence(expression_t a,expression_t b)
{
  auto node=std::make_unique<detail::Sequence>();
  get(a)->addTo(*node);
  get(b)->addTo(*node);
  node->calculate_hash();

  return add(std::move(node));
}

expression_t ExpressionPool::newAlternative(expression_t a,expression_t b)
{
  auto node=std::make_unique<detail::Alternative>();
  get(a)->addTo(*node);
  get(b)->addTo(*node);
  node->calculate_hash();

  return add(std::move(node));
}

expression_t ExpressionPool::newRepetition(expression_t a,int min,int max)
{
  if ( (min<0)||(max<-2)||(min>max) ) {
    throw std::invalid_argument("bad min/max values for repetition");
  }
//  if ( (min==1)&&(max==1) ) return a; // shortcut

  auto node=std::make_unique<detail::Repetition>(min,max);
  get(a)->addTo(*node);

  node->calculate_hash();

  return add(std::move(node));
}

const detail::Node *ExpressionPool::get(expression_t a)
{
  assert(a<pool.size());
  return pool[a].get();
}

expression_t ExpressionPool::add(std::unique_ptr<detail::Node> &&node)
{
  const int id=pool.size();

  auto res=mapping.insert(std::make_pair(node.get(),id));
  if (!res.second) { // found
    return res.first->second; // use already existing, discard node
  }

//  node->id=id; // res.first->second
  pool.emplace_back(std::move(node));
  return id;
}

} // namespace Regex

