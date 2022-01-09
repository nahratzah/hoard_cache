#pragma once

#include <cstddef>
#include <tuple>

#include "linked_list.h"

namespace libhoard::detail {


///\brief Common type-agnostic part of queue.
class basic_queue {
  template<typename HashTable, typename ValueType> friend class queue;

  public:
  class value_base
  : public linked_list_link<basic_queue>
  {
    friend basic_queue;
    template<typename HashTable, typename ValueType> friend class queue;

    protected:
    template<typename HashTable>
    explicit value_base([[maybe_unused]] const HashTable& table) noexcept
    {}

    value_base(const value_base&) = default;
    value_base(value_base&&) = default;
    auto operator=(const value_base&) -> value_base& = default;
    auto operator=(value_base&&) noexcept -> value_base& = default;
    ~value_base() noexcept = default;

    private:
    bool hot_;
  };

  protected:
  using callback = void(value_base*) noexcept;

  basic_queue() noexcept = default;
  basic_queue(const basic_queue&) noexcept = delete;
  basic_queue(basic_queue&&) noexcept = default;
  auto operator=(const basic_queue&) -> basic_queue& = delete;
  auto operator=(basic_queue&&) noexcept -> basic_queue& = delete;
  ~basic_queue() noexcept = default;

  /**
   * \brief On-create event acceptor.
   * \details
   * Inserts the element into the queue.
   * The element will be inserted at the top of the cold zone.
   */
  auto on_create_(value_base* v, callback* strengthen) noexcept -> void;

  /**
   * \brief On-hit event acceptor.
   * \details
   * The element \p v is moved to the front of the queue.
   * The element will be marked hot.
   */
  auto on_hit_(value_base* v, callback* strengthen) noexcept -> void;

  public:
  /**
   * \brief On-unlink event acceptor.
   * \details
   * Removes the element from the queue.
   */
  auto on_unlink_(value_base* v) noexcept -> void;

  ///\brief Invariant maintained by the queue.
  auto invariant() const noexcept -> bool;

  private:
  linked_list<value_base, basic_queue> q_;
  linked_list<value_base, basic_queue>::iterator midpoint_ = q_.begin();
  bool odd_sized_ = false;
};


/**
 * \brief A queue to allow selecting an element for removal from the cache.
 * \details
 * This queue has a hot zone and a cold zone.
 *
 * Whenever an element is created, it gets added to the top of the cold zone.
 * Elements that have a cache-hit, are moved into top of the hot zone.
 * The hot and cold zones have the same size, thus a cache-hit
 * may cause another element to become cold.
 */
template<typename HashTable, typename ValueType>
class queue
: private basic_queue
{
  protected:
  template<typename... Args, typename Alloc>
  explicit queue([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] Alloc&& alloc) noexcept;

  ~queue() noexcept;

  /**
   * \brief Expire (or weaken) up to \p count elements.
   * \details
   * This will expire up to \p count elements, stopping once it would
   * have to expire hot elements.
   *
   * \note If the hashtable has the weaken_policy instead of expiring elements
   * they'll be weakened instead.
   */
  auto lru_expire_(std::size_t count) noexcept -> void;

  public:
  using basic_queue::on_unlink_;
  auto on_create_(value_base* v) noexcept -> void;
  auto on_hit_(value_base* v) noexcept -> void;

  using basic_queue::invariant;
};

/**
 * \brief Policy that ensures a queue is maintained.
 */
struct queue_policy {
  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = queue<HashTable, ValueType>;

  using value_base = basic_queue::value_base;
};


} /* namespace libhoard::detail */

#include "queue.ii"
