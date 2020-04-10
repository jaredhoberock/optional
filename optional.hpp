// Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cassert>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>


// allow the user to define an annotation to apply to these functions
#ifndef OPTIONAL_ANNOTATION
#  if defined(__CUDACC__) && !(defined(__CUDA__) && defined(__clang__))
#    define OPTIONAL_ANNOTATION __host__ __device__
#  else
#    define OPTIONAL_ANNOTATION
#    define OPTIONAL_ANNOTATION_NEEDS_UNDEF
#  endif
#endif


// define the incantation to silence nvcc errors concerning __host__ __device__ functions
#if defined(__CUDACC__) && !(defined(__CUDA__) && defined(__clang__))
#  define OPTIONAL_EXEC_CHECK_DISABLE \
#  pragma nv_exec_check_disable
#else
#  define OPTIONAL_EXEC_CHECK_DISABLE
#endif

// allow the user to define a namespace for these functions
#if !defined(OPTIONAL_NAMESPACE)

#  if defined(OPTIONAL_NAMESPACE_OPEN_BRACE) or defined(OPTIONAL_NAMESPACE_CLOSE_BRACE)
#    error "All or none of OPTIONAL_NAMESPACE, OPTIONAL_NAMESPACE_OPEN_BRACE, and OPTIONAL_NAMESPACE_CLOSE_BRACE must be defined."
#  endif

#  define OPTIONAL_NAMESPACE
#  define OPTIONAL_NAMESPACE_OPEN_BRACE
#  define OPTIONAL_NAMESPACE_CLOSE_BRACE
#  define OPTIONAL_NAMESPACE_NEEDS_UNDEF

#else

#  if !defined(OPTIONAL_NAMESPACE_OPEN_BRACE) or !defined(OPTIONAL_NAMESPACE_CLOSE_BRACE)
#    error "All or none of OPTIONAL_NAMESPACE, OPTIONAL_NAMESPACE_OPEN_BRACE, and OPTIONAL_NAMESPACE_CLOSE_BRACE must be defined."
#  endif

#endif


// allow the user to define a singly-nested namespace for private implementation details
#if !defined(OPTIONAL_DETAIL_NAMESPACE)
#  define OPTIONAL_DETAIL_NAMESPACE detail
#  define OPTIONAL_DETAIL_NAMESPACE_NEEDS_UNDEF
#endif


OPTIONAL_NAMESPACE_OPEN_BRACE


struct nullopt_t {};
constexpr nullopt_t nullopt{};

struct in_place_t {};
constexpr in_place_t in_place{};


class bad_optional_access : public std::logic_error
{
  public:
    explicit bad_optional_access(const std::string& what_arg) : logic_error(what_arg) {}
    explicit bad_optional_access(const char* what_arg) : logic_error(what_arg) {}
};


namespace OPTIONAL_DETAIL_NAMESPACE
{


OPTIONAL_ANNOTATION
inline void throw_bad_optional_access(const char* what_arg)
{
#ifdef __CUDA_ARCH__
  printf("bad_optional_access: %s\n", what_arg);
  assert(0);
#else
  throw bad_optional_access(what_arg);
#endif
}


OPTIONAL_EXEC_CHECK_DISABLE
template<class T>
OPTIONAL_ANNOTATION
static void optional_swap(T& a, T& b)
{
  using std::swap;
  swap(a,b);
}


template<
  class T,
  bool use_empty_base_class_optimization =
    std::is_empty<T>::value
#if __cplusplus >= 201402L
    && !std::is_final<T>::value
#endif
>
struct optional_base
{
  typedef typename std::aligned_storage<
    sizeof(T)
  >::type storage_type;
  
  storage_type storage_;
};

template<class T>
struct optional_base<T,true> : T {};


} // end OPTIONAL_DETAIL_NAMESPACE


template<class T>
class optional : public OPTIONAL_DETAIL_NAMESPACE::optional_base<T>
{
  public:
    OPTIONAL_ANNOTATION
    optional(nullopt_t) : has_value_{false} {}

    OPTIONAL_ANNOTATION
    optional() : optional(nullopt) {}

    OPTIONAL_ANNOTATION
    optional(const optional& other)
      : has_value_(false)
    {
      if(other)
      {
        emplace(*other);
      }
    }

    OPTIONAL_ANNOTATION
    optional(optional&& other)
      : has_value_(false)
    {
      if(other)
      {
        emplace(std::move(*other));
      }
    }

    OPTIONAL_ANNOTATION
    optional(const T& value)
      : has_value_(false)
    {
      emplace(value);
    }

    OPTIONAL_ANNOTATION
    optional(T&& value)
      : optional(in_place, std::forward<T>(value))
    {
    }

    template<class... Args>
    OPTIONAL_ANNOTATION
    optional(in_place_t, Args&&... args)
      : has_value_(false)
    {
      emplace(std::forward<Args>(args)...);
    }

    template<class U, class... Args,
             class = typename std::enable_if<
               std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value
             >::type>
    OPTIONAL_ANNOTATION
    optional(in_place_t, std::initializer_list<U> ilist, Args&&... args)
      : has_value_(false)
    {
      emplace(ilist, std::forward<Args>(args)...);
    }

    OPTIONAL_ANNOTATION
    ~optional()
    {
      reset();
    }

    OPTIONAL_ANNOTATION
    optional& operator=(nullopt_t)
    {
      reset();
      return *this;
    }

    OPTIONAL_ANNOTATION
    optional& operator=(const optional& other)
    {
      if(other)
      {
        *this = *other;
      }
      else
      {
        *this = nullopt;
      }

      return *this;
    }

    OPTIONAL_ANNOTATION
    optional& operator=(optional&& other)
    {
      if(other)
      {
        *this = std::move(*other);
      }
      else
      {
        *this = nullopt;
      }

      return *this;
    }

    OPTIONAL_EXEC_CHECK_DISABLE
    template<class U,
             class = typename std::enable_if<
               std::is_same<typename std::decay<U>::type,T>::value
             >::type>
    OPTIONAL_ANNOTATION
    optional& operator=(U&& value)
    {
      if(*this)
      {
        **this = std::forward<U>(value);
      }
      else
      {
        emplace(std::forward<U>(value));
      }

      return *this;
    }

    OPTIONAL_EXEC_CHECK_DISABLE
    template<class... Args>
    OPTIONAL_ANNOTATION
    void emplace(Args&&... args)
    {
      reset();

      new (operator->()) T(std::forward<Args>(args)...);
      has_value_ = true;
    }

    template<class U, class... Args,
             class = typename std::enable_if<
               std::is_constructible<T,std::initializer_list<U>&, Args&&...>::value
             >::type>
    OPTIONAL_ANNOTATION
    void emplace(std::initializer_list<U> ilist, Args&&... args)
    {
      reset();

      new (operator->()) T(ilist, std::forward<Args>(args)...);
      has_value_ = true;
    }

    OPTIONAL_ANNOTATION
    bool has_value() const
    {
      return has_value_;
    }

    OPTIONAL_ANNOTATION
    explicit operator bool() const
    {
      return has_value();
    }

    OPTIONAL_ANNOTATION
    T& value() &
    {
      if(!*this)
      {
        OPTIONAL_DETAIL_NAMESPACE::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return **this;
    }

    OPTIONAL_ANNOTATION
    const T& value() const &
    {
      if(!*this)
      {
        OPTIONAL_DETAIL_NAMESPACE::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return **this;
    }

    OPTIONAL_ANNOTATION
    T&& value() &&
    {
      if(!*this)
      {
        OPTIONAL_DETAIL_NAMESPACE::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return std::move(**this);
    }

    OPTIONAL_ANNOTATION
    const T&& value() const &&
    {
      if(!*this)
      {
        OPTIONAL_DETAIL_NAMESPACE::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return std::move(**this);
    }

    template<class U>
    OPTIONAL_ANNOTATION
    T value_or(U&& default_value) const &
    {
      return bool(*this) ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template<class U>
    OPTIONAL_ANNOTATION
    T value_or(U&& default_value) &&
    {
      return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    OPTIONAL_ANNOTATION
    void swap(optional& other)
    {
      if(other)
      {
        if(*this)
        {
          OPTIONAL_DETAIL_NAMESPACE::optional_swap(**this, *other);
        }
        else
        {
          emplace(*other);
          other = nullopt;
        }
      }
      else
      {
        if(*this)
        {
          other.emplace(**this);
          *this = nullopt;
        }
        else
        {
          // no effect
        }
      }
    }

    OPTIONAL_ANNOTATION
    const T* operator->() const
    {
      return reinterpret_cast<const T*>(this);
    }

    OPTIONAL_ANNOTATION
    T* operator->()
    {
      return reinterpret_cast<T*>(this);
    }

    OPTIONAL_ANNOTATION
    const T& operator*() const &
    {
      return *operator->();
    }

    OPTIONAL_ANNOTATION
    T& operator*() &
    {
      return *operator->();
    }

    OPTIONAL_ANNOTATION
    const T&& operator*() const &&
    {
      return *operator->();
    }

    OPTIONAL_ANNOTATION
    T&& operator*() &&
    {
      return *operator->();
    }

    OPTIONAL_EXEC_CHECK_DISABLE
    OPTIONAL_ANNOTATION
    void reset()
    {
      if(*this)
      {
        (**this).~T();
        has_value_ = false;
      }
    }

  private:
    bool has_value_;
};


// comparison
template<class T>
OPTIONAL_ANNOTATION
bool operator==(const optional<T>& lhs, const optional<T>& rhs)
{
  if(lhs && rhs)
  {
    return *lhs == *rhs;
  }

  return bool(lhs) == bool(rhs);
}


template<class T>
OPTIONAL_ANNOTATION
bool operator<(const optional<T>& lhs, const optional<T>& rhs)
{
  if(lhs && rhs)
  {
    return *lhs < *rhs;
  }

  return !bool(lhs) && bool(rhs);
}


// comparison with nullopt_t
template<class T>
OPTIONAL_ANNOTATION
bool operator==(const optional<T>& lhs, nullopt_t)
{
  return lhs == optional<T>(nullopt);
}

template<class T>
OPTIONAL_ANNOTATION
bool operator==(nullopt_t, const optional<T>& rhs)
{
  return optional<T>(nullopt) == rhs;
}


template<class T>
OPTIONAL_ANNOTATION
bool operator<(const optional<T>& lhs, nullopt_t)
{
  return lhs < optional<T>(nullopt);
}


template<class T>
OPTIONAL_ANNOTATION
bool operator<(nullopt_t, const optional<T>& rhs)
{
  return optional<T>(nullopt) < rhs;
}


// comparison with T
template<class T>
OPTIONAL_ANNOTATION
bool operator==(const optional<T>& lhs, const T& rhs)
{
  if(lhs)
  {
    return *lhs == rhs;
  }

  return false;
}


template<class T>
OPTIONAL_ANNOTATION
bool operator==(const T& lhs, const optional<T>& rhs)
{
  if(rhs)
  {
    return lhs == *rhs;
  }

  return false;
}


template<class T>
OPTIONAL_ANNOTATION
bool operator<(const optional<T>& lhs, const T& rhs)
{
  if(lhs)
  {
    return *lhs < rhs;
  }

  return true;
}

template<class T>
OPTIONAL_ANNOTATION
bool operator<(const T& lhs, const optional<T>& rhs)
{
  if(rhs)
  {
    return lhs < *rhs;
  }

  return false;
}



template<class T>
OPTIONAL_ANNOTATION
optional<typename std::decay<T>::type> make_optional(T&& value)
{
  return optional<typename std::decay<T>::type>(std::forward<T>(value));
}


template<class T>
OPTIONAL_ANNOTATION
void swap(optional<T>& a, optional<T>& b)
{
  a.swap(b);
}


OPTIONAL_NAMESPACE_CLOSE_BRACE


namespace std
{

template<class T>
struct hash<OPTIONAL_NAMESPACE::optional<T>>
{
  OPTIONAL_EXEC_CHECK_DISABLE
  OPTIONAL_ANNOTATION
  size_t operator()(const OPTIONAL_NAMESPACE::optional<T>& key) const
  {
    return bool(key) ? std::hash<T>()(*key) : size_t(0);
  }
};

} // end std


#ifdef OPTIONAL_ANNOTATION_NEEDS_UNDEF
#  undef OPTIONAL_ANNOTATION
#  undef OPTIONAL_ANNOTATION_NEEDS_UNDEF
#endif

#ifdef OPTIONAL_NAMESPACE_NEEDS_UNDEF
#  undef OPTIONAL_NAMESPACE
#  undef OPTIONAL_NAMESPACE_OPEN_BRACE
#  undef OPTIONAL_NAMESPACE_CLOSE_BRACE
#  undef OPTIONAL_NAMESPACE_NEEDS_UNDEF
#endif

#ifdef OPTIONAL_DETAIL_NAMESPACE_NEEDS_UNDEF
#  undef OPTIONAL_DETAIL_NAMESPACE
#  undef OPTIONAL_DETAIL_NAMESPACE_NEEDS_UNDEF
#endif

#undef OPTIONAL_EXEC_CHECK_DISABLE

