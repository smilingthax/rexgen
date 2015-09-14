#include "re-ast_int.h"
#include "merge-sorted.h"

// #include <typeinfo>
#include <string.h>
#include <assert.h>

namespace Regex {

namespace detail {

bool Empty::operator==(const Node &rhs) const
{
  if (dynamic_cast<const Empty *>(&rhs)) {
    return true;
  }
  return false;
}

void Empty::visit(Visitor &visitor) const
{
  setCurrent(visitor);
  visitor.empty();
}

void Literal::calculate_hash()
{
  HashAlgo h;
  h("Literal",8);
  const char *name=typeid(*data).name();
  h(name,strlen(name)+1);
  auto val=data->hash();
  h(&val,sizeof(val));
  hash=(HashAlgo::result_type)h;
}

bool Literal::operator==(const Node &rhs) const
{
  if (auto rt=dynamic_cast<const Literal *>(&rhs)) {
    return (*data==*rt->data);
  }
  return false;
}

void Literal::visit(Visitor &visitor) const
{
  setCurrent(visitor);
  visitor.literal(data.get());
}

void Repetition::calculate_hash()
{
  HashAlgo h;
  h("Repetition",11);
  h(&min,sizeof(min));
  h(&max,sizeof(max));
  h(&child->hash,sizeof(child->hash));
  hash=(HashAlgo::result_type)h;
}

bool Repetition::operator==(const Node &rhs) const
{
  if (auto rt=dynamic_cast<const Repetition *>(&rhs)) {
    return (min==rt->min)&&(max==rt->max)&&(child==rt->child);
  }
  return false;
}

void Repetition::visit(Visitor &visitor) const
{
  setCurrent(visitor);
  if (visitor.preRepetition(min,max)) {
    child->visit(visitor);
    setCurrent(visitor);
    visitor.postRepetition(min,max);
  }
}

/*
void Sequence::hash_append(HashAlgo &h) const override
{
  h("Sequence",9);
  for (auto child : childs) {
    child->hash_append(h);
  }
}
*/
void Sequence::calculate_hash()
{
  HashAlgo h;
  h("Sequence",9);
  for (auto child : childs) {
    h(&child->hash,sizeof(child->hash));
  }
  hash=(HashAlgo::result_type)h;
}

bool Sequence::operator==(const Node &rhs) const
{
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

void Sequence::visit(Visitor &visitor) const
{
  setCurrent(visitor);
  if (visitor.preSequence()) {
    for (size_t i=0; i<childs.size(); i++) {
      if (visitor.nextSequence(i)) {
        childs[i]->visit(visitor);
        setCurrent(visitor);
      }
    }
    visitor.postSequence();
  }
}

void Alternative::calculate_hash()
{
  HashAlgo h;
  h("Alternative",12);
  for (auto child : childs) {
    h(&child->hash,sizeof(child->hash));
  }
  hash=(HashAlgo::result_type)h;
}

bool Alternative::operator==(const Node &rhs) const
{
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

void Alternative::visit(Visitor &visitor) const
{
  setCurrent(visitor);
  if (visitor.preAlternative()) {
    for (size_t i=0; i<childs.size(); i++) {
      if (visitor.nextAlternative(i)) {
        childs[i]->visit(visitor);
        setCurrent(visitor);
      }
    }
    visitor.postAlternative();
  }
}


void Node::addTo(Repetition &r) const
{
  r.child=this;
}

void Empty::addTo(Repetition &r) const
{
  assert(false);
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

void Node::addTo(Sequence &s) const
{
  s.childs.push_back(this);
}

void Empty::addTo(Sequence &s) const
{
  assert(false);
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

} // namespace detail


ExpressionPool::ExpressionPool()
  : mapping(0,[](const detail::Node *node){ // hashFn
    return node->hash;
  },[](const detail::Node *a,const detail::Node *b){ // equalFn
    return *a==*b;
  })
{
  add(std::make_unique<detail::Empty>()); // ==0
//  assert(pool.size()==1);
}

ExpressionPool::~ExpressionPool() =default;

expression_t ExpressionPool::newLiteral(detail::LiteralBase *lb)
{
  auto node=std::make_unique<detail::Literal>(std::unique_ptr<detail::LiteralBase>(lb));
  node->calculate_hash();

  return add(std::move(node));
}

expression_t ExpressionPool::newRepetition(expression_t a,int min,int max)
{
  if (  (min<0)||(max<-2)||
        ( (max!=-1)&&(min>max) )  ) {
    throw std::invalid_argument("bad min/max values for repetition");
  }
  if (isEmpty(a)) {
    return a;
  }

  auto node=std::make_unique<detail::Repetition>(min,max);
  get(a)->addTo(*node);

  node->calculate_hash();

  return add(std::move(node));
}

expression_t ExpressionPool::newSequence(expression_t a,expression_t b)
{
  if (isEmpty(a)) {
    return b;
  } else if (isEmpty(b)) {
    return a;
  }
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
  if (node->childs.size()==1) { // a==b (dup)
    return a;
  }
  // assert(node->childs.size()>=2);
  node->calculate_hash();

  return add(std::move(node));
}

const detail::Node *ExpressionPool::get(expression_t a) const
{
  if (a>=pool.size()) {
    throw std::invalid_argument("bad expression_t handle");
  }
  return pool[a].get();
}

expression_t ExpressionPool::add(std::unique_ptr<detail::Node> &&node)
{
  const int id=pool.size();

  auto res=mapping.insert(std::make_pair(node.get(),id));
  if (!res.second) { // found
    return res.first->second; // use already existing, discard node
  }

  node->id=id; // res.first->second
  pool.emplace_back(std::move(node));
  return id;
}

bool ExpressionPool::isEmpty(expression_t a) const
{
  return (a==0);
}

void ExpressionPool::visit(expression_t a,Visitor &visitor) const
{
  get(a)->visit(visitor);
  visitor.current=-1;
  visitor.end();
}

} // namespace Regex

