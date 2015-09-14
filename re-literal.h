#ifndef _RE_LITERAL_H
#define _RE_LITERAL_H

#include <functional>

namespace Regex {

namespace detail {
  using hash_result_t=size_t;

  struct LiteralBase {
    virtual ~LiteralBase() {}
    virtual LiteralBase *clone() const =0;

    virtual hash_result_t hash() const =0;
    virtual bool operator==(const LiteralBase &rhs) const =0;
  };

  template <typename T>  // ,typename Compare=std::less<T>>
  struct LiteralWrapper : LiteralBase {
    LiteralWrapper(T value) : value(value) {}
    LiteralWrapper *clone() const override {
      return new LiteralWrapper(value);
    }

    hash_result_t hash() const override {
      return std::hash<T>()(value);
    }
    bool operator==(const LiteralBase &rhs) const override {
      if (auto rt=dynamic_cast<const LiteralWrapper *>(&rhs)) {
        return (value==rt->value);
      }
      return false;
    }

    T value;
  };

} // namespace detail

} // namespace Regex

#endif
