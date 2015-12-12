// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
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

#include <utility>
#include <type_traits>
#include <stdexcept>

#ifndef __host__
#  define __host__
#  define OPTIONAL_UNDEF_HOST
#endif

#ifndef __device__
#  define __device__
#  define OPTIONAL_UNDEF_DEVICE
#endif

namespace std
{
namespace experimental
{


struct nullopt_t {};
constexpr nullopt_t nullopt{};


class bad_optional_access : public std::logic_error
{
  public:
    explicit bad_optional_access(const std::string& what_arg) : logic_error(what_arg) {}
    explicit bad_optional_access(const char* what_arg) : logic_error(what_arg) {}
};


namespace detail
{


__host__ __device__
inline void throw_bad_optional_access(const char* what_arg)
{
#ifdef __CUDA_ARCH__
  printf("bad_optional_access: %s\n", what_arg);
  assert(0);
#else
  throw bad_optional_access(what_arg);
#endif
}


#pragma nv_exec_check_disable
template<class T>
__host__ __device__
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


} // end detail


template<class T>
class optional : public detail::optional_base<T>
{
  public:
    __host__ __device__
    optional(nullopt_t) : contains_value_{false} {}

    __host__ __device__
    optional() : optional(nullopt) {}

    __host__ __device__
    optional(const optional& other)
      : contains_value_(other.contains_value_)
    {
      emplace(*other);
    }

    __host__ __device__
    optional(optional&& other)
      : contains_value_(false)
    {
      if(other)
      {
        emplace(std::move(*other));
      }
    }

    __host__ __device__
    optional(const T& value)
      : contains_value_(false)
    {
      emplace(value);
    }

    __host__ __device__
    optional(T&& value)
      : contains_value_(false)
    {
      emplace(std::move(value));
    }

    __host__ __device__
    ~optional()
    {
      clear();
    }

    __host__ __device__
    optional& operator=(nullopt_t)
    {
      clear();
      return *this;
    }

    template<class U,
             class = typename std::enable_if<
               std::is_same<typename std::decay<U>::type,T>::value
             >::type>
    __host__ __device__
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

    __host__ __device__
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

    __host__ __device__
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

    template<class... Args>
    __host__ __device__
    void emplace(Args&&... args)
    {
      clear();

      new (operator->()) T(std::forward<Args>(args)...) ;
      contains_value_ = true;
    }

    __host__ __device__
    explicit operator bool() const
    {
      return contains_value_;
    }

    __host__ __device__
    T& value() &
    {
      if(!*this)
      {
        detail::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return **this;
    }

    __host__ __device__
    const T& value() const &
    {
      if(!*this)
      {
        detail::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return **this;
    }

    __host__ __device__
    T&& value() &&
    {
      if(!*this)
      {
        detail::throw_bad_optional_access("optional::value(): optional does not contain a value");
      }

      return std::move(**this);
    }

    template<class U>
    __host__ __device__
    T value_or(U&& default_value) const &
    {
      return bool(*this) ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template<class U>
    __host__ __device__
    T value_or(U&& default_value) &&
    {
      return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    __host__ __device__
    void swap(optional& other)
    {
      if(*other)
      {
        if(*this)
        {
          detail::optional_swap(**this, *other);
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

    __host__ __device__
    const T* operator->() const
    {
      return reinterpret_cast<const T*>(this);
    }

    __host__ __device__
    T* operator->()
    {
      return reinterpret_cast<T*>(this);
    }

    __host__ __device__
    const T& operator*() const &
    {
      return *operator->();
    }

    __host__ __device__
    T& operator*() &
    {
      return *operator->();
    }

    __host__ __device__
    const T&& operator*() const &&
    {
      return *operator->();
    }

    __host__ __device__
    T&& operator*() &&
    {
      return *operator->();
    }

  private:
    __host__ __device__
    void clear()
    {
      if(*this)
      {
        (**this).~T();
        contains_value_ = false;
      }
    }

    bool contains_value_;
};


template<class T>
__host__ __device__
optional<typename std::decay<T>::type> make_optional(T&& value)
{
  return optional<typename std::decay<T>::type>(std::forward<T>(value));
}


} // end experimental
} // end std

#ifdef OPTIONAL_UNDEF_HOST
#  undef __host__
#  undef OPTIONAL_UNDEF_HOST
#endif

#ifdef OPTIONAL_UNDEF_DEVICE
#  undef __device__
#  undef OPTIONAL_UNDEF_DEVICE
#endif

