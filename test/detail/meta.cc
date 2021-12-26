#include <libhoard/detail/meta.h>

#include <type_traits>

using libhoard::detail::type_list;

struct a;
struct b;
struct c;
struct d;

template<typename T>
struct is_same_predicate {
  template<typename U>
  using type = std::is_same<T, U>;
};

template<typename, typename> class two_types;

// type_list::add_type_t
static_assert(std::is_same_v<type_list<b>,          type_list<>::add_type_t<b>>,                            "adding a single element to an empty type_list");
static_assert(std::is_same_v<type_list<a, b>,       type_list<a>::add_type_t<b>>,                           "adding a single element to a type list");
static_assert(std::is_same_v<type_list<a, b, c>,    type_list<a>::add_type_t<b, c>>,                        "adding multiple elements to a type list");

// type_list::extend_t
static_assert(std::is_same_v<type_list<a, b, c, d>, type_list<a>::extend_t<type_list<b>, type_list<c, d>>>, "extending type list");

// type_list::has_type_t
static_assert(std::is_base_of_v<std::false_type,    type_list<a, b>::has_type_t<c>>,                        "type presence on the type list, when the type is absent");
static_assert(std::is_base_of_v<std::true_type,     type_list<a, b>::has_type_t<a>>,                        "type presence on the type list, when the type is present");

// type_list::has_type_v
static_assert(!type_list<a, b>::has_type_v<c>,                                                              "type presence on the type list, when the type is absent");
static_assert( type_list<a, b>::has_type_v<a>,                                                              "type presence on the type list, when the type is present");

// type_list::remove_t
static_assert(std::is_same_v<type_list<a, c>,       type_list<a, b, c>::remove_t<b>>,                       "removing a type from the type_list");
static_assert(std::is_same_v<type_list<a, b>,       type_list<a, b>::remove_t<d>>,                          "removing an absent type from the type_list");
static_assert(std::is_same_v<type_list<b, b, a>,    type_list<a, b, b, a>::remove_t<a>>,                    "removing from the type_list only once");

// type_list::remove_all_t
static_assert(std::is_same_v<type_list<a, c>,       type_list<a, b, c>::remove_all_t<b>>,                   "removing a type from the type_list");
static_assert(std::is_same_v<type_list<a, b>,       type_list<a, b>::remove_all_t<d>>,                      "removing an absent type from the type_list");
static_assert(std::is_same_v<type_list<b, b>,       type_list<a, b, b, a>::remove_all_t<a>>,                "removing multiple from the type_list");

// type_list::distinct_t
static_assert(std::is_same_v<type_list<a, b, c>,    type_list<a, b, b, c, c, b, a>::distinct_t>, "removing duplicates from the type list");

// type_list::reverse_t
static_assert(std::is_same_v<type_list<d, c, b, a>, type_list<a, b, c, d>::reverse_t>, "reversing the type list");

// type_list::filter_t
static_assert(std::is_same_v<type_list<a, a>,       type_list<a, b, a, c, d>::filter_t<is_same_predicate<a>::type>>, "filter predicate");

// type_list::apply_t
static_assert(std::is_same_v<two_types<a, b>,       type_list<a, b>::apply_t<two_types>>, "type_list apply");

// type_list::size
static_assert(0 == type_list<>::size);
static_assert(1 == type_list<a>::size);
static_assert(4 == type_list<a, b, b, a>::size);

// type_list::empty
static_assert(type_list<>::empty);
static_assert(!type_list<a>::empty);
static_assert(!type_list<a, b, b, a>::empty);
