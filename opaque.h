#ifndef _OPAQUE_H
#define _OPAQUE_H

#include <stdint.h>
#include <memory>

struct Opaque { // access-safe intptr_t
  Opaque()=default;

  ~Opaque() {
    if (access) {
      access->destroy(data);
    }
  }

  Opaque(Opaque &&o) {
    swap(o);
  }

  Opaque& operator=(Opaque &&o) {
    swap(o);
    return *this;
  }

  void swap(Opaque &o) {
    std::swap(data,o.data);
    std::swap(access,o.access);
  }

  template <typename T,typename Deleter=
            typename std::conditional<
              std::is_pointer<T>::value && !std::is_const<typename std::remove_pointer<T>::type>::value,
              std::default_delete<typename std::remove_pointer<T>::type>,
              std::nullptr_t
            >::type>
  class Accessor;

  template <typename Access>
  typename Access::value_type get(Access &_access) const {
    if (access!=&_access) { // or by type: dynamic_cast<Access *>(access) / typeid(*access)==typeid(Access)
      return {};
    }
    return _access(data);
  }

  template <typename Access>
  typename Access::value_type release(Access &_access) {
    if (access!=&_access) {
      return {};
    }
    auto ret=_access(data);
    data={};
    access={};
    return ret;
  }

  template <typename Access>
  void set(Access &_access,typename Access::value_type _data) {
    if ( (access==&_access)&&(_access(data)==_data) ) {
      return; // unchanged
    } else if (access) {
      access->destroy(data);
    }
    data=reinterpret_cast<intptr_t>(_data);
    access=&_access;
  }

  template <typename Access>
  bool is(Access &_access) const {
    return (access==&_access);
  }

  void reset() {
    if (access) {
      access->destroy(data);
    }
    data={};
    access={};
  }
private:
  class Accessor_base {
    virtual void destroy(intptr_t data) const =0;
    friend class Opaque;
  };

  intptr_t data={};
  const Accessor_base *access={};
};

// Deleter=std::default_delete<T>, unless explicitly specified
template <typename T,typename Deleter>
class Opaque::Accessor<T *,Deleter> : public Opaque::Accessor_base {
public:
  typedef T *value_type;

  value_type operator()(intptr_t data) const {
    return reinterpret_cast<value_type>(data);
  }
private:
  void destroy(intptr_t data) const override {
    del(operator()(data));
  }
  Deleter del;
};

// default for const T *: no Deleter (but can be explicitly given)
template <typename T>
class Opaque::Accessor<T *,std::nullptr_t> : public Opaque::Accessor_base {
public:
  typedef const T *value_type;

  value_type operator()(intptr_t data) const {
    return reinterpret_cast<value_type>(data);
  }
private:
  void destroy(intptr_t data) const override { }
};

// NOTE: empty int looks exactly like int(0)  [-> is()]

// int with explicit Deleter
template <typename Deleter>
class Opaque::Accessor<int,Deleter> : public Opaque::Accessor_base {
public:
  typedef int value_type;

  value_type operator()(intptr_t data) const {
    return data;
  }
private:
  void destroy(intptr_t data) const override {
    del(operator()(data));
  }
  Deleter del;
};

// default for int: no Deleter
template <>
class Opaque::Accessor<int,std::nullptr_t> : public Opaque::Accessor_base {
public:
  typedef int value_type;

  value_type operator()(intptr_t data) const {
    return data;
  }
private:
  void destroy(intptr_t data) const override { }
};

#endif
