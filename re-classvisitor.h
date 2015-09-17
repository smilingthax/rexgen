#ifndef _RE_CLASSVISITOR_H
#define _RE_CLASSVISITOR_H

/* Usage:

struct MyVClass {
  // void empty() {...}
  // void literal(lb)

  struct Repetition {
    Repetition(MyVClass &outer) : outer(outer) {}  // or just default ctor

    void begin(int min,int max,Regex::expression_t cid); // or bool
//    void end();

  private:
    MyVClass &outer;
  };

  // ...  Group [begin(cid)/end]       (...if desired...)
  // ...  Sequence, Alternative [begin/next(idx,cid)/end]

  // void end()

};

Regex::ClassVisitor<MyVClass> cv;
visit(...,cv);

  or
Regex::ClassVisitor<MyVClass &> cv(...);
*/

#include "re-ast.h"

namespace Regex {

template <typename VClass>
class ClassVisitor : public ExpressionPool::Visitor {
public:
  template <typename... Args>
  ClassVisitor(Args&&... args) : outer(std::forward<Args>(args)...) {}

  void empty() override {
    empty_hlp(&outer);
  }

  void literal(const detail::LiteralBase *lb) override {
    literal_hlp(&outer,lb);
  }

/* TODO ... not yet
  bool preGroup(expression_t cid) override {
    grps.push(&outer);
    return beginGrp_hlp(&grps,cid);
  }
  void postGroup() override {
    endGRSA_hlp(&reps);
    grps.pop();
  }
*/

  bool preRepetition(int min,int max,expression_t cid) override {
    reps.push(&outer);
    return beginRep_hlp(&reps,min,max,cid);
  }
  void postRepetition(int min,int max) override {
    endGRSA_hlp(&reps);
    reps.pop();
  }

  bool preSequence() override {
    seqs.push(&outer);
    return beginSA_hlp(&seqs);
  }
  bool nextSequence(int idx,expression_t cid) override {
    return nextSA_hlp(&seqs,idx,cid);
  }
  void postSequence() override {
    endGRSA_hlp(&seqs);
    seqs.pop();
  }

  bool preAlternative() override {
    alts.push(&outer);
    return beginSA_hlp(&alts);
  }
  bool nextAlternative(int idx,expression_t cid) override {
    return nextSA_hlp(&alts,idx,cid);
  }
  void postAlternative() override {
    endGRSA_hlp(&alts);
  }

  void end() override {
    end_hlp(&outer);
  }

  // id() from base

  const VClass &get() const {
    return outer;
  }

private:
  template <typename T,typename X=decltype(&T::empty)>
  static void empty_hlp(T *t) {
    return t->empty();
  }
  static void empty_hlp(...) {}

  template <typename T,typename X=decltype(&T::literal)>
  static void literal_hlp(T *t,const detail::LiteralBase *lb) {
    return t->literal(lb);
  }
  static void literal_hlp(...) {}

  template <typename T>
  static bool beginGrp_hlp(T *vec,expression_t cid,char (*)[std::is_same<bool,decltype(vec->vec.back().begin(cid))>::value]=0) {
    return vec->vec.back().begin(cid);
  }
  template <typename T>
  static bool beginGrp_hlp(T *vec,expression_t cid,char (*)[std::is_same<void,decltype(vec->vec.back().begin(cid))>::value]=0) {
    vec->vec.back().begin(cid);
    return true;
  }
  static bool beginGrp_hlp(...) { return true; }

  template <typename T>
  static bool beginRep_hlp(T *vec,int min,int max,expression_t cid,char (*)[std::is_same<bool,decltype(vec->vec.back().begin(min,max,cid))>::value]=0) {
    return vec->vec.back().begin(min,max,cid);
  }
  template <typename T>
  static bool beginRep_hlp(T *vec,int min,int max,expression_t cid,char (*)[std::is_same<void,decltype(vec->vec.back().begin(min,max,cid))>::value]=0) {
    vec->vec.back().begin(min,max,cid);
    return true;
  }
  static bool beginRep_hlp(...) { return true; }

  template <typename T>
  static bool beginSA_hlp(T *vec,char (*)[std::is_same<bool,decltype(vec->vec.back().begin())>::value]=0) {
    return vec->vec.back().begin();
  }
  template <typename T>
  static bool beginSA_hlp(T *vec,char (*)[std::is_same<void,decltype(vec->vec.back().begin())>::value]=0) {
    vec->vec.back().begin();
    return true;
  }
  static bool beginSA_hlp(...) { return true; }

  template <typename T>
  static bool nextSA_hlp(T *vec,int idx,expression_t cid,char (*)[std::is_same<bool,decltype(vec->vec.back().next(idx,cid))>::value]=0) {
    return vec->vec.back().next(idx,cid);
  }
  template <typename T>
  static bool nextSA_hlp(T *vec,int idx,expression_t cid,char (*)[std::is_same<void,decltype(vec->vec.back().next(idx,cid))>::value]=0) {
    vec->vec.back().next(idx,cid);
    return true;
  }
  static bool nextSA_hlp(...) { return true; }

  template <typename T>
  static void endGRSA_hlp(T *vec,char (*)[sizeof(vec->vec.back().end(),0)]=0) {
    vec->vec.back().end();
  }
  static void endGRSA_hlp(...) {}

  template <typename T,typename X=decltype(&T::end)>
  static void end_hlp(T *t) {
    return t->end();
  }
  static void end_hlp(...) {}


  template <typename T>
  using group_t=typename T::Group;

  template <typename T>
  using repetition_t=typename T::Repetition;

  template <typename T>
  using sequence_t=typename T::Sequence;

  template <typename T>
  using alternative_t=typename T::Alternative;

  template <typename T,template <typename> class Inner,typename Enable=void>
  struct Vec {
    void push(T *outer) {}
    void pop() {}
  };

  template <typename T,template <typename> class Inner>
  struct Vec<T,Inner,decltype((void)(Inner<T> *)0)> {
    template <typename S>
    void push(S *outer,char (*)[sizeof(Inner<T>(*outer))]=0) {
      vec.emplace_back(*outer);
    }
    void push(...) {
      vec.emplace_back();
    }
    void pop() {
      vec.pop_back();
    }
    std::vector<Inner<T>> vec;
  };

  VClass outer;

  typedef typename std::decay<VClass>::type VCt;
  Vec<VCt,group_t> grps;
  Vec<VCt,repetition_t> reps;
  Vec<VCt,sequence_t> seqs;
  Vec<VCt,alternative_t> alts;
};

} // namespace Regex

#endif
