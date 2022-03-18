#include <libhoard/cache.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <tuple>

struct fib_resolver {
  auto operator()(std::uint32_t v) const -> std::tuple<std::uintmax_t>;
};

using fib_cache_type = libhoard::cache<std::uint32_t, std::uintmax_t, libhoard::resolver_policy<fib_resolver>>;

inline auto fib_resolver::operator()(std::uint32_t v) const -> std::tuple<std::uintmax_t> {
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
}

fib_cache_type fib_cache = fib_cache_type(libhoard::resolver_policy<fib_resolver>());

inline void example_lookup(std::uint32_t v) {
  const auto b = std::chrono::steady_clock::now();
  try {
    std::get<std::uintmax_t>(fib_cache.get(v));
    const auto v_fib = std::visit([](const auto& v) -> std::uintmax_t {
          using v_type = std::decay_t<decltype(v)>;
          if constexpr(std::is_same_v<std::monostate, v_type>) {
            throw std::logic_error("no value");
          } else if constexpr(std::is_same_v<std::uintmax_t, v_type>) {
            return v;
          } else {
            std::ostringstream oss;
            oss << "error code " << v << " result";
            throw std::runtime_error(std::move(oss).str());
          }
        },
        fib_cache.get(v));
    std::chrono::duration<double, std::micro> lookup_duration = std::chrono::steady_clock::now() - b;
    std::cout << v << "! = " << v_fib << " (lookup took " << lookup_duration.count() << " microseconds)" << std::endl;
  } catch (const std::exception& ex) {
    std::cerr << "error looking up fibonacci(" << v << "): " << ex.what() << std::endl;
  }
}

int main() {
  std::cout << "with empty cache:" << std::endl;
  for (int i = 0; i <= 90; ++i) example_lookup(i);
  example_lookup(200);

  std::cout << "------------------------------------------------------------------------\n";
  std::cout << "with primed cache:" << std::endl;
  for (int i = 0; i <= 90; ++i) example_lookup(i);
  example_lookup(200);
}
