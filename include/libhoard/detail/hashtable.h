#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <utility>
#include <variant>

#include "basic_hashtable.h"
#include "function_ref.h"
#include "mapped_type.h"
#include "meta.h"
#include "traits.h"
#include "refcount.h"
#include "value_type.h"
#include "../allocator.h"
#include "../equal.h"
#include "../hash.h"
#include "../thread_safe_policy.h"
#include "../thread_unsafe_policy.h"
#include "../resolver_policy.h"
#include "../error_policy.h"
#include "../pointer_policy.h"

namespace libhoard::detail {


template<typename KeyType, typename T, typename... Policies>
class hashtable;


template<typename Policy> struct dependent_policies_; // Forward declaration.

template<typename Policy, typename = void>
struct dependent_policies_impl_ {
  using type = type_list<>;
};

template<typename Policy>
struct dependent_policies_impl_<Policy, std::void_t<typename Policy::dependencies>> {
  using type = type_list<>::extend_t<
      // Grab transitive dependencies (using recursion).
      typename Policy::dependencies::template transform_t<dependent_policies_>::template apply_t<type_list<>::extend_t>,
      // Grab dependencies.
      typename Policy::dependencies>;
};

template<typename Policy>
struct dependent_policies_
: dependent_policies_impl_<Policy>
{};


template<typename, typename = void>
struct figure_out_hashtable_value_base_impl_ {
  using type = void;
};

template<typename T>
struct figure_out_hashtable_value_base_impl_<T, std::void_t<typename T::value_base>> {
  using type = typename T::value_base;
};


template<typename T>
struct figure_out_hashtable_value_base_
: figure_out_hashtable_value_base_impl_<T>
{};


template<typename Policy, typename TableType, typename ValueType, typename Allocator, typename = void>
struct figure_out_hashtable_table_base_ {
  using type = void;
};

template<typename Policy, typename TableType, typename ValueType, typename Allocator>
struct figure_out_hashtable_table_base_<Policy, TableType, ValueType, Allocator, std::void_t<typename Policy::template table_base<TableType, ValueType, Allocator>>> {
  using type = typename Policy::template table_base<TableType, ValueType, Allocator>;
};


template<typename T, typename = void>
struct has_policy_removal_check_
: std::false_type
{};

template<typename T>
struct has_policy_removal_check_<T, std::void_t<decltype(std::declval<T>().policy_removal_check_())>>
: std::true_type
{};


template<typename T>
struct has_error_policy_
: std::false_type
{};

template<typename ErrorType>
struct has_error_policy_<error_policy<ErrorType>>
: std::true_type
{};


template<typename T>
struct is_pointer_policy_
: std::false_type
{};

template<typename WeakPointer, typename MemberPointer, typename MemberPointerConstructorArgs>
struct is_pointer_policy_<pointer_policy<WeakPointer, MemberPointer, MemberPointerConstructorArgs>>
: std::true_type
{};


///\brief Small adapter type to wrap elements that are default constructible.
template<typename T>
class hashtable_dfl_constructible_
: public T
{
  protected:
  template<typename HashTable>
  hashtable_dfl_constructible_(const HashTable& ht);
};


/**
 * \brief Class inheriting from the policy implementations.
 * \details
 * Handles dispatching events into the policies.
 */
template<typename ValueType, typename... PolicyMap>
class hashtable_policy_container
: public PolicyMap::table_base...
{
  template<typename, typename, typename> friend class async_resolver_callback; // Allow async_resolver_policy to emit the on_asign_ event.

  public:
  static constexpr bool has_policy_removal_check = std::disjunction_v<has_policy_removal_check_<typename PolicyMap::table_base>...>;

  protected:
  hashtable_policy_container() = delete;

  template<typename Alloc, typename... Args>
  hashtable_policy_container(std::allocator_arg_t aa, const Alloc& alloc, Args&&... args);

  ~hashtable_policy_container() noexcept;

  ///\brief Dispatch an on-create event.
  auto on_create_(ValueType* vptr) noexcept -> void;
  ///\brief Dispatch an on-assign event.
  auto on_assign_(ValueType* vptr, bool value, bool assigned_via_callback) noexcept -> void;
  ///\brief Dispatch an unlink event.
  auto on_unlink_(ValueType* vptr) noexcept -> void;
  ///\brief Dispatch an on-hit event.
  auto on_hit_(ValueType* vptr) noexcept -> void;
  ///\brief Dispatch an on-miss event.
  auto on_miss_() noexcept -> void;
  ///\brief Perform maintenance.
  auto on_maintenance_() noexcept -> void;

  ///\brief Check in with policies to figure out how many elements must be removed.
  ///\return Tuple with number of elements that is to be expired.
  template<bool Enable = has_policy_removal_check>
  auto policy_removal_check_() const noexcept -> std::enable_if_t<Enable, std::size_t>;

  public:
  ///\brief Initialization function.
  auto init() -> void;
  ///\brief Pre-destroy function.
  auto destroy() -> void;

  private:
  ///\brief Assign \p fn for each base type where it is invocable.
  template<typename Fn>
  static auto base_invoke_(Fn&& fn) noexcept -> void;
};


template<typename T, typename PoliciesTypeList, bool HasPointerPolicy>
struct select_mapper_impl_;

template<typename T, typename PoliciesTypeList>
struct select_mapper_impl_<T, PoliciesTypeList, false> {
  using allocator_policy = typename PoliciesTypeList::template filter_t<has_allocator>::template apply_t<select_single_element_t>;
  using allocator_type = typename allocator_policy::allocator_type;

  using error_policy = typename PoliciesTypeList::template filter_t<has_error_policy_>::template apply_t<select_single_element_t>;
  using error_type = typename error_policy::error_type;

  using type = mapped_value<T, allocator_type, error_type>;
};

template<typename T, typename PoliciesTypeList>
struct select_mapper_impl_<T, PoliciesTypeList, true> {
  using allocator_policy = typename PoliciesTypeList::template filter_t<has_allocator>::template apply_t<select_single_element_t>;
  using allocator_type = typename allocator_policy::allocator_type;

  using error_policy = typename PoliciesTypeList::template filter_t<has_error_policy_>::template apply_t<select_single_element_t>;
  using error_type = typename error_policy::error_type;

  using pointer_policy = typename PoliciesTypeList::template filter_t<is_pointer_policy_>::template apply_t<select_single_element_t>;
  using weak_pointer = typename pointer_policy::template weak_type<T>;
  using member_pointer = typename pointer_policy::template member_type<T>;

  using type = mapped_pointer<T, allocator_type, error_type, weak_pointer, member_pointer, typename pointer_policy::mapped_base>;
};

template<typename T, typename PoliciesTypeList>
using select_mapper_ = typename select_mapper_impl_<T, PoliciesTypeList, PoliciesTypeList::template transform_t<is_pointer_policy_>::template apply_t<std::disjunction>::value>::type;


template<typename Policy, typename TableBase>
struct policy_and_table_base {
  using policy = Policy;
  using table_base = TableBase;
};


template<typename KeyType, typename T, typename... Policies>
struct hashtable_helper_ {
  static inline constexpr bool is_identity_map = std::is_same_v<identity_t, KeyType>;
  static inline constexpr bool has_equal_policy = std::disjunction_v<has_equal<Policies>...>;
  static inline constexpr bool has_hash_policy = std::disjunction_v<has_hash<Policies>...>;
  static inline constexpr bool has_allocator_policy = std::disjunction_v<has_allocator<Policies>...>;
  static inline constexpr bool has_resolver_policy = std::disjunction_v<has_resolver<Policies>...>;
  static inline constexpr bool has_async_resolver_policy = std::disjunction_v<has_async_resolver<Policies>...>;

  using maybe_default_equal = std::conditional_t<
      has_equal_policy,
      type_list<>,
      type_list<equal<std::equal_to<std::conditional_t<is_identity_map, T, KeyType>>>>>;

  using maybe_default_hash = std::conditional_t<
      has_hash_policy,
      type_list<>,
      type_list<hash<std::hash<std::conditional_t<is_identity_map, T, KeyType>>>>>;

  using allocator_policy = typename std::conditional_t<
      has_allocator_policy,
      typename type_list<Policies...>::template filter_t<has_allocator>,
      type_list<allocator<std::allocator<int>>>
      >::template apply_t<select_single_element_t>;
  using maybe_extra_allocator_policy = std::conditional_t<
      has_allocator_policy,
      type_list<>,
      type_list<allocator_policy>>;

  ///\brief Figure out what dependency policies to pull in.
  ///\details
  ///Takes the dependencies type_list from the \p Policies.
  ///Ensures its set is distinct and none of the Policies are included.
  using policy_dependencies = typename type_list<>::extend_t<typename dependent_policies_<Policies>::type...>::template exclude_t<type_list<Policies...>>::distinct_t;

  static inline constexpr bool has_thread_safe_policy = std::disjunction_v<
      typename type_list<Policies...>::template has_type_t<thread_safe_policy>,
      typename policy_dependencies::template has_type_t<thread_safe_policy>,
      typename type_list<Policies...>::template has_type_t<thread_unsafe_policy>,
      typename policy_dependencies::template has_type_t<thread_unsafe_policy>>;
  using maybe_extra_mutex_policies = std::conditional_t<
      has_thread_safe_policy,
      type_list<>,
      type_list<thread_safe_policy>>;

  static inline constexpr bool has_error_policy = type_list<Policies...>::template extend_t<policy_dependencies>::template transform_t<has_error_policy_>::template apply_t<std::disjunction>::value;
  using maybe_extra_error_policies = std::conditional_t<
      has_error_policy,
      type_list<>,
      type_list<error_policy<>>>;

  using all_policies = typename type_list<>::extend_t<
      maybe_extra_mutex_policies,
      maybe_extra_allocator_policy,
      maybe_default_equal,
      maybe_default_hash,
      maybe_extra_error_policies,
      policy_dependencies,
      type_list<Policies...>>;

  using error_policy_type = typename all_policies::template filter_t<has_error_policy_>::template apply_t<select_single_element_t>;
  using mapper = select_mapper_<T, all_policies>;

  ///\brief List of types that the value type must inherit from according to policies.
  using vt_base_types = typename all_policies::template transform_t<figure_out_hashtable_value_base_>::template remove_all_t<void>;

  ///\brief The value type used in the hashtable.
  using value_type = typename type_list<KeyType, mapper, hashtable_dfl_constructible_<basic_hashtable_element>, hashtable_dfl_constructible_<refcount>>::template extend_t<vt_base_types>::template apply_t<libhoard::detail::value_type>;
  ///\brief Key type of the cache.
  using key_type = typename value_type::key_type;
  ///\brief Mapped type of the cache.
  using mapped_type = typename value_type::mapped_type;
  ///\brief Error type of the cache.
  using error_type = typename value_type::error_type;
  ///\brief Allocator used by the cache.
  using allocator_type = typename std::allocator_traits<typename value_type::allocator_type>::template rebind_alloc<value_type>;
  ///\brief Basic hashtable.
  using bht = basic_hashtable<typename std::allocator_traits<typename value_type::allocator_type>::template rebind_alloc<basic_hashtable_element*>>;

  ///\brief Helper to figure out table base.
  template<typename Policy>
  struct bound_figure_out_hashtable_table_base_ {
    private:
    using type_ = policy_and_table_base<Policy, typename figure_out_hashtable_table_base_<Policy, hashtable<KeyType, T, Policies...>, value_type, allocator_type>::type>;

    public:
    using type = std::conditional_t<std::is_void_v<typename type_::table_base>, void, type_>;
  };
  ///\brief List of types that the hashtable must derive from according to policies.
  using ht_base_types = typename all_policies::template transform_t<bound_figure_out_hashtable_table_base_>::template remove_all_t<void>;
  ///\brief Type that derives from ht_base_types.
  using ht_base = typename type_list<value_type>::template extend_t<ht_base_types>::template apply_t<hashtable_policy_container>;

  class iterator;
  class const_iterator;
  class range;
  class const_range;
};


template<typename KeyType, typename T, typename... Policies>
class hashtable_helper_<KeyType, T, Policies...>::iterator {
  template<typename HTKeyType, typename HTMappedType, typename... HTPolicies> friend class hashtable;
  friend hashtable_helper_::const_iterator;

  public:
  using difference_type = std::iterator_traits<basic_hashtable_algorithms::iterator>::difference_type;
  using value_type = hashtable_helper_::value_type;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::iterator_traits<basic_hashtable_algorithms::iterator>::iterator_category;

  iterator() noexcept;
  explicit iterator(basic_hashtable_algorithms::iterator iter) noexcept;

  auto operator++() noexcept -> iterator&;
  auto operator++(int) noexcept -> iterator;

  auto get() const noexcept -> value_type*;
  auto operator*() const noexcept -> value_type&;
  auto operator->() const noexcept -> value_type*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  basic_hashtable_algorithms::iterator iter_;
};


template<typename KeyType, typename T, typename... Policies>
class hashtable_helper_<KeyType, T, Policies...>::const_iterator {
  template<typename HTKeyType, typename HTMappedType, typename... HTPolicies> friend class hashtable;
  friend hashtable_helper_::iterator;

  public:
  using difference_type = std::iterator_traits<basic_hashtable_algorithms::iterator>::difference_type;
  using value_type = hashtable_helper_::value_type;
  using pointer = const value_type*;
  using reference = const value_type&;
  using iterator_category = std::iterator_traits<basic_hashtable_algorithms::iterator>::iterator_category;

  const_iterator() noexcept;
  const_iterator(const iterator& y) noexcept;
  explicit const_iterator(basic_hashtable_algorithms::const_iterator iter) noexcept;

  auto operator++() noexcept -> const_iterator&;
  auto operator++(int) noexcept -> const_iterator;

  auto get() const noexcept -> const value_type*;
  auto operator*() const noexcept -> const value_type&;
  auto operator->() const noexcept -> const value_type*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  basic_hashtable_algorithms::const_iterator iter_;
};


template<typename KeyType, typename T, typename... Policies>
class hashtable_helper_<KeyType, T, Policies...>::range {
  friend hashtable_helper_::const_range;

  public:
  using iterator = hashtable_helper_::iterator;

  range() noexcept;
  range(iterator b, iterator e) noexcept;

  auto begin() const noexcept -> iterator;
  auto end() const noexcept -> iterator;
  auto empty() const noexcept -> bool;

  private:
  iterator b, e;
};


template<typename KeyType, typename T, typename... Policies>
class hashtable_helper_<KeyType, T, Policies...>::const_range {
  friend hashtable_helper_::range;

  public:
  using iterator = hashtable_helper_::const_iterator;

  const_range() noexcept;
  const_range(const range& y) noexcept;
  const_range(iterator b, iterator e) noexcept;

  auto begin() const noexcept -> iterator;
  auto end() const noexcept -> iterator;
  auto empty() const noexcept -> bool;

  private:
  iterator b, e;
};


template<typename...>
struct arg_pack_starts_with_allocator_arg_
: std::false_type
{};

template<typename... Args>
struct arg_pack_starts_with_allocator_arg_<std::allocator_arg_t, Args...>
: std::true_type
{};

template<typename... Args>
inline constexpr bool arg_pack_starts_with_allocator_arg_v = arg_pack_starts_with_allocator_arg_<std::remove_cv_t<std::remove_reference_t<Args>>...>::value;


/**
 * \brief Hashtable that drives the cache primitives.
 * \details
 * Contains the base functions to make the cache function.
 *
 * \tparam KeyType The type of the key, or identity_t if this is a set.
 * \tparam T The mapped type of the hashtable.
 * \tparam Policies Additional types that value_type must inherit from.
 */
template<typename KeyType, typename T, typename... Policies>
class hashtable
: private basic_hashtable_allocator_member<typename hashtable_helper_<KeyType, T, Policies...>::allocator_type>,
  private hashtable_helper_<KeyType, T, Policies...>::bht,
  public hashtable_helper_<KeyType, T, Policies...>::ht_base
{
  private:
  using helper_type = hashtable_helper_<KeyType, T, Policies...>;
  using bht = typename helper_type::bht;

  public:
  using uses_resolver = std::integral_constant<bool, helper_type::has_resolver_policy || helper_type::has_async_resolver_policy>;
  using uses_async_resolver = std::integral_constant<bool, helper_type::has_async_resolver_policy>;
  using policy_type_list = typename helper_type::all_policies;
  using value_type = typename helper_type::value_type;
  using key_type = typename helper_type::key_type;
  using mapped_type = typename helper_type::mapped_type;
  using error_type = typename helper_type::error_type;
  using pending_type = typename value_type::pending_type;
  using allocator_type = typename helper_type::allocator_type;
  using size_type = typename bht::size_type;
  using value_pointer = refcount_ptr<value_type, allocator_type>;
  using callback_fn = typename value_type::callback_fn;
  using iterator = typename helper_type::iterator;
  using const_iterator = typename helper_type::const_iterator;

  private:
  struct disposer_impl;

  public:
  template<typename... Args>
  explicit hashtable(std::allocator_arg_t aa, allocator_type allocator, Args&&... args);
  template<typename... Args>
  explicit hashtable(std::allocator_arg_t aa, allocator_type allocator, float max_load_factor, Args&&... args);
  template<typename... Args, std::enable_if_t<!arg_pack_starts_with_allocator_arg_v<Args...>, int> = 0>
  explicit hashtable(Args&&... args); // also handles the max-load-factor case

  ~hashtable();

  using bht::is_hashtable_linked;
  using bht::empty;
  using bht::size;
  using bht::bucket_count;
  using bht::max_bucket_count;
  using bht::load_factor;
  using bht::max_load_factor;
  using basic_hashtable_allocator_member<allocator_type>::get_allocator;
  using helper_type::ht_base::init;

  auto reserve(size_type sz) -> void;

  private:
  /**
   * \brief Retrieve a value from the cache.
   * \details
   * A value is retrieved if it has been fully resolved (no pending values), and it has not expired.
   *
   * This function does not perform on-hit/on-miss events.
   *
   * \return Variant holding the monostate if no value was found, or one of the mapped type or error type, if a value is present.
   */
  auto get_if_exists_(std::size_t hash, function_ref<bool(const key_type&)> matcher) const -> std::variant<std::monostate, mapped_type, error_type>;

  public:
  /**
   * \brief Retrieve a value from the cache.
   * \details
   * A value is retrieved if it has been fully resolved (no pending value), and it has not expired.
   *
   * This function does not perform on-hit/on-miss events.
   *
   * \param keys Key lookup arguments. These are passed to the hash and equal functions.
   * \return Variant holding the monostate if no value was found, or one of the mapped type or error type, if a value is present.
   */
  template<typename... Keys>
  auto get_if_exists(const Keys&... keys) const noexcept(
      noexcept(std::invoke(std::declval<const typename helper_type::ht_base&>().equal, std::declval<const key_type&>(), std::declval<const Keys&>()...))
      &&
      noexcept(std::invoke(std::declval<const typename helper_type::ht_base&>().hash, std::declval<const Keys&>()...))
      &&
      std::is_nothrow_copy_constructible_v<mapped_type> && std::is_nothrow_copy_constructible_v<error_type>)
  -> std::variant<std::monostate, mapped_type, error_type>;

  private:
  /**
   * \brief Retrieve a value from the cache.
   * \details
   * Retrieved value shall not be expired, but it may be pending.
   *
   * Invokes the `on hit` and `on miss` events.
   *
   * \return Variant holding the monostate if no value was found, or one of the mapped type or error type, if a value is present.
   * If no value is present, but the lookup is pending, a pointer to the mapper will be returned.
   */
  template<bool IncludePending>
  auto get_(std::size_t hash, function_ref<bool(const key_type&)> matcher, std::integral_constant<bool, IncludePending> include_pending)
  -> std::conditional_t<
      IncludePending,
      std::variant<std::monostate, mapped_type, error_type, pending_type*>,
      std::variant<std::monostate, mapped_type, error_type>>;

  public:
  template<typename... Keys>
  auto get(const Keys&... keys) noexcept(
      noexcept(std::invoke(std::declval<const typename helper_type::ht_base&>().equal, std::declval<const key_type&>(), std::declval<const Keys&>()...))
      &&
      noexcept(std::invoke(std::declval<const typename helper_type::ht_base&>().hash, std::declval<const Keys&>()...))
      &&
      std::is_nothrow_copy_constructible_v<mapped_type> && std::is_nothrow_copy_constructible_v<error_type>)
  -> std::variant<std::monostate, mapped_type, error_type>;

  template<typename CompletionCallback, typename... Keys>
  auto async_get(CompletionCallback&& callback, const Keys&... keys) -> void;

  ///\brief Expire all elements in the cache.
  auto expire_all() noexcept -> void;

  private:
  /**
   * \brief Expire a specific key.
   * \details
   * Expires all lookups that match the given key.
   *
   * Pending lookups that are already in progress, will complete normally,
   * and may do so after the function has returned.
   * But new calls for that key won't join in the pending lookup.
   *
   * \note In identity-caches, pending lookups are not expired.
   */
  auto expire_(std::size_t hash, function_ref<bool(const key_type&)> matcher) -> void;

  public:
  template<typename... Keys>
  auto expire(const Keys&... keys) -> void;

  template<typename... Args>
  auto allocate_value_type(Args&&... args) -> value_pointer;
  auto value_to_refpointer(value_type* vptr) const -> value_pointer;

  auto link(std::size_t hash, value_pointer vptr) -> void;

  /**
   * \brief Create a callback that'll invoke the on_assign_ event.
   * \param vptr Pointer to the value type for which the callback will invoke the on-assign event.
   * \return Callback that'll invoke on-assign.
   */
  auto assign_value_callback(const value_pointer& vptr) -> callback_fn;

  auto begin() noexcept -> iterator;
  auto end() noexcept -> iterator;
  auto begin() const noexcept -> const_iterator;
  auto end() const noexcept -> const_iterator;
  auto cbegin() const noexcept -> const_iterator;
  auto cend() const noexcept -> const_iterator;

  template<typename... KeyArgs, typename... MappedArgs>
  auto emplace(std::piecewise_construct_t pc, std::tuple<KeyArgs...> key_args, std::tuple<MappedArgs...> mapped_args) -> void;

  template<typename KeyArg, typename MappedArg>
  auto emplace(KeyArg&& key_arg, MappedArg&& mapped_arg)
  -> std::enable_if_t<std::is_constructible_v<key_type, KeyArg> && std::is_constructible_v<mapped_type, MappedArg>>;

  template<typename... KeyArgs, typename... MappedArgs, bool IncludePending>
  auto get_or_emplace(std::piecewise_construct_t pc, std::tuple<KeyArgs...> key_args, std::tuple<MappedArgs...> mapped_args, std::integral_constant<bool, IncludePending> include_pending)
  -> std::conditional_t<
      IncludePending,
      std::variant<std::monostate, mapped_type, error_type, pending_type*>,
      std::variant<std::monostate, mapped_type, error_type>>;

  template<typename KeyArg, typename MappedArg, bool IncludePending>
  auto get_or_emplace(KeyArg&& key_arg, MappedArg&& mapped_arg, std::integral_constant<bool, IncludePending> include_pending)
  -> std::enable_if_t<std::is_constructible_v<key_type, KeyArg> && std::is_constructible_v<mapped_type, MappedArg>,
      std::conditional_t<
          IncludePending,
          std::variant<std::monostate, mapped_type, error_type, pending_type*>,
          std::variant<std::monostate, mapped_type, error_type>>>;

  ///\brief Count number of not-expired elements in the cache.
  auto count() const noexcept -> size_type;

  private:
  auto maintenance_() noexcept -> void;
  auto bucket_for_hash_(std::size_t hash) noexcept -> typename helper_type::range;
  auto bucket_for_hash_(std::size_t hash) const noexcept -> typename helper_type::const_range;

  /**
   * \brief Clean up expired elements.
   * \details
   * This method is intended to be run before a rehash.
   * If there are expired elements, it'll remove them, and maybe will
   * remove the need for a rehash operation.
   */
  auto unlink_expired_elements_() -> void;

  auto disposer_() noexcept -> disposer_impl;
};


template<typename KeyType, typename T, typename... Policies>
struct hashtable<KeyType, T, Policies...>::disposer_impl {
  explicit disposer_impl(hashtable& self) noexcept;
  auto operator()(basic_hashtable_element* base_elem) noexcept -> void;

  private:
  hashtable& self;
};


} /* namespace libhoard::detail */

#include "hashtable.ii"
