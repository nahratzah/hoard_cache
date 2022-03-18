#include <libhoard/cache.h>
#include <libhoard/resolver_policy.h>
#include <libhoard/thread_unsafe_policy.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <tuple>

constexpr bool use_fast_fibonacci = false; // use slow algorithm

struct fib_resolver {
  auto operator()(std::uint32_t v) const -> std::tuple<std::uintmax_t>;
};

using fib_cache_type = libhoard::cache<std::uint32_t, std::uintmax_t, libhoard::resolver_policy<fib_resolver>, libhoard::thread_unsafe_policy>;

inline auto fib_resolver::operator()(std::uint32_t v) const -> std::tuple<std::uintmax_t> {
  if constexpr(use_fast_fibonacci) {
    std::uintmax_t a = 0, b = 1;
    for (std::uint32_t i = 0; i < v; ++i) {
      std::tie(a, b) = std::make_tuple(b, a + b);

      if (b < a) { // overflow
        std::ostringstream oss;
        oss << "fibonacci(" << v << ") too large for uintmax_t";
        throw std::range_error(std::move(oss).str());
      }
    }
    return std::make_tuple(b);
  } else {
    // Slow algorithm. Uses recursion.
    switch (v) {
      default:
        return std::get<0>((*this)(v - 2u)) + std::get<0>((*this)(v - 1u));
      case 0:
      case 1:
        return std::make_tuple(1);
    }
  }
}

fib_cache_type fib_cache = fib_cache_type(libhoard::resolver_policy<fib_resolver>());

void example_lookup(std::uint32_t v) {
  const auto b = std::chrono::steady_clock::now();
  try {
    const auto v_fib = fib_cache.get(v);
    std::chrono::duration<double, std::micro> lookup_duration = std::chrono::steady_clock::now() - b;
    std::cout << "fibonacci(" << std::setw(3) << v << ") = " << std::setw(9) << v_fib <<
        " (lookup took " << std::fixed << std::setprecision(3) << std::setw(11) << lookup_duration.count() << " microseconds)" << std::endl;
  } catch (const std::exception& ex) {
    std::cerr << "error looking up fibonacci(" << v << "): " << ex.what() << std::endl;
  }
}

int main() {
  constexpr std::uint32_t highest_input = (use_fast_fibonacci ? 90 : 40);

  std::cout << "with empty cache:" << std::endl;
  for (int i = 0; i <= highest_input; ++i) example_lookup(i);
  if constexpr(use_fast_fibonacci) example_lookup(200);

  std::cout << "------------------------------------------------------------------------\n";
  std::cout << "with primed cache:" << std::endl;
  for (int i = 0; i <= highest_input; ++i) example_lookup(i);
  if constexpr(use_fast_fibonacci) example_lookup(200);
}
