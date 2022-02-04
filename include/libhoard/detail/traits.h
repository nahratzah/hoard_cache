#pragma once

#include <type_traits>

#include "../allocator.h"
#include "../equal.h"
#include "../hash.h"
#include "../resolver_policy.h"

namespace libhoard::detail {


template<typename>
struct has_equal
: public std::false_type
{};

template<typename Equal>
struct has_equal<equal<Equal>>
: public std::true_type
{};


template<typename>
struct has_hash
: public std::false_type
{};

template<typename Hash>
struct has_hash<hash<Hash>>
: public std::true_type
{};


template<typename>
struct has_allocator
: public std::false_type
{};

template<typename Allocator>
struct has_allocator<allocator<Allocator>>
: public std::true_type
{};


template<typename>
struct has_resolver
: public std::false_type
{};

template<typename Functor>
struct has_resolver<resolver_policy<Functor>>
: public std::true_type
{};


template<typename>
struct has_async_resolver
: public std::false_type
{};

template<typename Functor>
struct has_async_resolver<async_resolver_policy<Functor>>
: public std::true_type
{};


} /* namespace libhoard::detail */
