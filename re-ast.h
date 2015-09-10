#ifndef _RE_AST_H
#define _RE_AST_H

#include <vector>
#include <memory>
#include <unordered_map>
//#include <typeinfo>

namespace Regex {

typedef unsigned int expression_t;

namespace detail {
  struct Node;
  using hash_result_t=size_t;

  struct LiteralBase {
    virtual ~LiteralBase() {}

    virtual hash_result_t hash() const =0;
    virtual bool operator==(const LiteralBase &rhs) const =0;

//    virtual int compare(const LiteralBase &) const =0;
  };

// FIXME
  /* [lib.comparisions]/8
    For templates greater, less, greater_equal, and less_equal, the specializations for any pointer type yield a total order,
    even if the built-in operators <, >, <=, >= do not.
  */
  template <typename T>  // ,typename Compare=std::less<T>>
  struct LiteralWrapper : LiteralBase {
    LiteralWrapper(T value) : value(value) {}

    hash_result_t hash() const override {
      return std::hash<T>()(value);
    }
    bool operator==(const LiteralBase &rhs) const override {
      if (auto rt=dynamic_cast<const LiteralWrapper *>(&rhs)) {
        return (value==rt->value);
      }
      return false;
    }
#if 0
    int compare(const LiteralBase &rhs) const override {
      const std::type_info &lt=typeid(*this),&rt=typeid(rhs);
      if (lt.before(rt)) {
        return -1;
      } else if (rt.before(lt)) {
        return 1;
      }
      const LiteralWrapper &other=static_cast<const LiteralWrapper &>(rhs);
      return value.compare(other.value);
    }
#endif

    T value;
  };
} // namespace detail

class ExpressionPool {
public:
  ExpressionPool();
  ~ExpressionPool();

  template <typename T>
  expression_t newLiteralValue(T&& lit) {
    return newLiteral(new detail::LiteralWrapper<T>(std::forward<T>(lit)));
  }
  expression_t newLiteral(detail::LiteralBase *lit);

  expression_t newSequence(expression_t a,expression_t b);
  expression_t newAlternative(expression_t a,expression_t b);
  expression_t newRepetition(expression_t a,int min=0,int max=-1);

protected:
  const detail::Node *get(expression_t a);
  expression_t add(std::unique_ptr<detail::Node> &&node);

private:
  std::vector<std::unique_ptr<detail::Node>> pool;
  std::unordered_map<const detail::Node *,expression_t,
                     detail::hash_result_t (*)(const detail::Node *),
                     bool (*)(const detail::Node *,const detail::Node *)> mapping;
};

} // namespace Regex

#endif
