#ifndef _RE_BUILDER_BASE_H
#define _RE_BUILDER_BASE_H

#include "opaque.h"
#include "re-literal.h"

namespace Regex {

class Builder_base {
public:
  struct group_t : Opaque {
    group_t() =default;
    group_t(Opaque &&o) : Opaque(std::move(o)) { }
  };
  struct sequence_t : Opaque {
    sequence_t() =default;
    sequence_t(Opaque &&o) : Opaque(std::move(o)) { }
  };
  struct alternative_t : Opaque {
    alternative_t() =default;
    alternative_t(Opaque &&o) : Opaque(std::move(o)) { }
  };

  virtual ~Builder_base() =default;

  virtual void empty() =0;

  template <typename T>
  void literalValue(T&& lit) {
    literal(new detail::LiteralWrapper<T>(std::forward<T>(lit)));
  }
  virtual void literal(detail::LiteralBase *lb) =0;

  // caller has to provide alternative_t/sequence_t/group_t storage
  virtual group_t grp() =0;
  virtual void end(group_t &g) =0;

  virtual void rep(int min=0,int max=-1) =0;

  virtual void begin(alternative_t &a) =0;
  virtual void alt(alternative_t &a) =0;
  virtual void end(alternative_t &a) =0;

  virtual void begin(sequence_t &s) =0;
  virtual void seq(sequence_t &s) =0;
  virtual void end(sequence_t &s) =0;

  virtual void end() =0; // TODO or error: bool  ??
};

} // namespace Regex

#endif
