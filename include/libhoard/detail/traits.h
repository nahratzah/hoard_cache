#pragma once

#include <type_traits>

#include "../allocator.h"
#include "../equal.h"
#include "../hash.h"

namespace libhoard::detail {


template<typename>
struct has_equal
: std::false_type
{};

template<typename Equal>
struct has_equal<equal<Equal>>
: std::true_type
{};


template<typename>
struct has_hash
: std::false_type
{};

template<typename Hash>
struct has_hash<hash<Hash>>
: std::true_type
{};


template<typename>
struct has_allocator
: std::false_type
{};

template<typename Allocator>
struct has_allocator<allocator<Allocator>>
: std::true_type
{};


template<typename>
struct has_resolver
: std::false_type
{};


template<typename>
struct has_async_resolver
: std::false_type
{};


} /* namespace libhoard::detail */
