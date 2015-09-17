#ifndef _STRINGBUILDER_H
#define _STRINGBUILDER_H

#include "re-builder_base.h"

namespace {
Regex::BuilderBase::sequence_t::Accessor<std::string *> sb_seq;
Regex::BuilderBase::alternative_t::Accessor<std::string *> sb_alt;
}
class StringBuilder : public Regex::BuilderBase {
public:
  void empty() override {
    prec=PREC_TERM;
  }
  void literal(Regex::detail::LiteralBase *lb) override {
    std::unique_ptr<Regex::detail::LiteralBase> ptr(lb);
if (auto s=dynamic_cast<const Regex::detail::LiteralWrapper<std::string> *>(ptr.get())) {
//    current.assign(s->value);
    current.push_back('[');
    current.append(s->value); // grp() will not stash away current...
    current.push_back(']');
} else if (auto c=dynamic_cast<const Regex::detail::LiteralWrapper<char> *>(ptr.get())) {
//    current.assign(1,c->value);
    current.push_back(c->value); // grp() will not stash away current...
}
    prec=PREC_TERM;
  }

  group_t grp() override {
    // no real need to temporarily store (current,prec) in group_t  - TODO?
    current.push_back('(');
    prec=PREC_NONE;
    return {};
  }
  void end(group_t &g) override {
    current.push_back(')');
    prec=PREC_TERM;
  }
  void rep(int min=0,int max=-1) override {
    if (prec<PREC_REP) {
      current.insert(0,"(?:");
      current.push_back(')');
    }
    if ( (min==0)&&(max==-1) ) {
      current.push_back('*');
    } else if ( (min==1)&&(max==-1) ) {
      current.push_back('+');
    } else if ( (min==0)&&(max==1) ) {
      current.push_back('?');
    } else {
      current.push_back('{');
      if (min!=max) {
        if (min!=0) {
          current.append(std::to_string(min));
        }
        current.push_back(',');
      } // else assert(min!=-1);
      if (max!=-1) {
        current.append(std::to_string(max));
      }
      current.push_back('}');
    }
    prec=PREC_REP;
  }

  void begin(sequence_t &s) override {
    s.set(sb_seq,new std::string);
    prec=PREC_NONE;
  }
  void seq(sequence_t &s) override {
    auto pstr=s.get(sb_seq);
    currentTo(*pstr,PREC_SEQ);
  }
  void end(sequence_t &s) override {
    std::unique_ptr<std::string> pstr(s.release(sb_seq));
    currentWith(std::move(*pstr),PREC_SEQ);
  }

  void begin(alternative_t &a) override {
    a.set(sb_alt,new std::string);
    prec=PREC_NONE;
  }
  void alt(alternative_t &a) override {
    auto pstr=a.get(sb_alt);
    currentTo(*pstr,PREC_ALT);
    pstr->push_back('|');
  }
  void end(alternative_t &a) override {
    std::unique_ptr<std::string> pstr(a.release(sb_alt));
    currentWith(std::move(*pstr),PREC_ALT);
  }

  void end() override {
    printf("%s\n",current.c_str());
  }

private:
  enum prec_t { PREC_ALT, PREC_SEQ, PREC_REP, PREC_TERM, PREC_NONE };

  void currentTo(std::string &dst,prec_t _prec) {
    if (prec<_prec) {
      dst.append("(?:");
      dst.append(current);
      dst.push_back(')');
    } else {
      dst.append(current);
    }
    current.clear();
    // prec=_prec;
  }
  void currentWith(std::string &&src,prec_t _prec) {
    currentTo(src,_prec);
    current=std::move(src);
    prec=_prec;
  }

  std::string current;
  prec_t prec=PREC_NONE;
};

#endif
