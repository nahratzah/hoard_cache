# LibHoard

This is a library that allows for caching and memoidization.
It's named after a behaviour of dragons, keeping lots of shiny things.

This is a header-only library.

# Work in Progress

Still working on this, you could play with this.
The general shape of the code is pretty much stable, but some implementation is still shifting around.

# Quick Start

To include the cache in your code, you can simply add the `libhoard/include/` directory to your include-path.
Or if you use cmake, you can include the project (using `add_subdirectory(libhoard)` for example) and then link your project against the library: `target_link_library(your_target PUBLIC libhoard)`.

## The Most Trivial Cache

```
#include <libhoard/cache.h>

using key_type = int;
using mapped_type = std::string;
libhoard::cache<key_type, mapped_type> c;
```

This cache:
- will not expire any elements (and thus grow without bounds).
- you can add elements to it, and look them up.
- elements not in the cache cannot be resolved.

Looking up a value: `std::optional<std::string> = c.get(1);`

Adding/replacing a value:
- `c.emplace(17, "seventeen");`
- `c.emplace(std::piecewise_construct, std::make_tuple(17), std::make_tuple("seventeen"));`

Adding a value, if there isn't already one present:
- `std::string v = c.get_or_emplace(18, "eighteen");`
- `std::string v = c.get_or_emplace(std::piecewise_construct, std::make_tuple(18), std::make_tuple("eighteen"));`

Removing a value from the cache:
- `c.erase(17);`

Emptying the cache:
- `c.clear();`

# Policies

The cache accepts policies: `class cache<KeyType, MappedType, Policies...>`.

## Limiting the Size of the Cache

```
#include <libhoard/cache.h>
#include <libhoard/max_size_policy.h>

using key_type = int;
using mapped_type = std::string;

libhoard::cache<
    key_type, mapped_type,
    libhoard::max_size_policy
    > c(libhoard::max_size_policy(100));
```
Constructs a cache which holds up to 100 elements.

## Limiting the Age of Items in the Cache

```
#include <libhoard/cache.h>
#include <libhoard/max_age_policy.h>

using namespace std::literals::chrono_literals;
using key_type = int;
using mapped_type = std::string;

libhoard::cache<
    key_type, mapped_type,
    libhoard::max_age_policy
    > c(libhoard::max_age_policy(5m));
```
Constructs a cache where elements will be removed after 5 minutes.

## Thread-safe

Usually, when the cache is declared, it is thread-safe.
To make the cache not thread-safe, you'll want to add the `libhoard::thread_unsafe_policy`.
If you want to be explicit, you can add the `libhoard::thread_safe_policy`.

The refresh policy requires the `libhoard::thread_safe_policy` and introduces it as a dependency.
This ensures you can't accidentally introduce the `libhoard::thread_unsafe_policy` on a cache that requires thread safety to function correctly.

```
#include <libhoard/cache.h>
#include <libhoard/thread_safe_policy.h>

using key_type = int;
using mapped_type = std::string;

libhoard::cache<
    key_type, mapped_type,
    libhoard::thread_safe_policy // this is the default, you can omit this
    > c;

libhoard::cache<
    key_type, mapped_type,
    libhoard::thread_unsafe_policy
    > d;
```

## Smart Pointers and Singletons

When using smart pointers (such as [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr)) you can set up the cache to use weak pointers when shrinking the cache.
The advantage is that, as long as your weak-pointer element is still strongly referenced elsewhere in the code, the cache will keep returning it.

For example, you could do something like:
```
using file_contents_ptr = std::shared_ptr<const std::string>;

struct file_contents_resolver {
  auto operator()(std::string filename) -> file_contents_ptr {
    std::ifstream f(filename);
    std::ostringstream oss;
    oss << f.rdbuf();
    return std::make_shared<const std::string>(oss.str());
  }
};

using file_cache = libhoard::cache<
    std::string, file_contents_ptr,
    libhoard::pointer_policy<>, // tells the cache that we're using shared pointers
    libhoard::weaken_policy<>,  // tells the cache to use weak_ptr, when shrinking the cache
    libhoard::max_size_policy,  // tells the cache we'll have a size limit
    libhoard::resolver_policy<file_contents_resolver> // use the above resolver
    >;

int main() {
  file_cache fc(libhoard::max_size_policy(2));

  file_contents_ptr a_txt = fc.get("a.txt");

  // cause a_txt to expire because the cache grows too large
  fc.get("b.txt");
  fc.get("b.txt");
  fc.get("c.txt");
  fc.get("c.txt");

  // despite having expired a_txt, because it was still referenced in our code,
  // the entry was preserved in the cache
  assert(a_txt == fc.get("a.txt");
}
```

This behaviour can be used to create singletons.
Suppose we have a monitoring tool, that needs to keep track of metrics across a large group of machines.
Each machine has 1000 metrics, and they're all named the same.
We have 10k machines.
And each metric name takes up, say, 60 bytes.
So in total, we would need `60 bytes * 1k * 10k = 600 MB` just to store the metric names.

But if we use a cache instead, we can deduplicate duplicate names:
```
using metric_name_cache = libhoard::cache<
    std::string, std::shared_ptr<std::string>,
    libhoard::pointer_policy<>,
    libhoard::weaken_policy<>,
    libhoard::max_size_policy,
    libhoard::resolver_policy<std::shared_ptr<const std::string> (*)(std::string)>>;

metric_name_cache metric_names(
    libhoard::max_size_policy(100),
    libhoard::resolver_policy<std::shared_ptr<const std::string> (*)(std::string)>>([](std::string s) { return std::make_shared<const std::string>(s); }));

metric_names.get("my_metric_name"); // returns the same std::shared_ptr of "my_metric_name" each time
```

We changed the `std::string` to a `std::shared_ptr<std::string>`, and now we only need `60 bytes * 1k = 60 kB` of memory.

## Installing a Resolver

Instead of using `cache::emplace` to insert elements into the cache, you can equip the cache with a resolver function.
For each lookup in the cache, that it doesn't hold a value for, it'll invoke the resolver function to perform a lookup.

```
#include <libhoard/cache.h>
#include <libhoard/resolver_policy.h>

using key_type = int;
using mapped_type = std::string;

libhoard::cache<
    key_type, mapped_type,
    libhoard::resolver_policy<std::function<std::string(int)>>
    > c(libhoard::resolver_policy<std::function<std::string(int)>>(
        [](int key) -> {
          std::ostringstream s;
          s << key;
          return s.str();
        }));
```
Now, if a value isn't present, it'll invoke the function. :)

```
c.get(5); // std::string("5")
c.get(100); // std::string("100")
```

## Installing an Async Resolver

Instead of using a synchronous resolver, you can install an asynchronous resolver.
An asynchronous resolver is one that completes using a callback.

```
#include <libhoard/cache.h>
#include <libhoard/resolver_policy.h> // Yes, the regular and asynchronous resolver share a header file.

using key_type = int;
using mapped_type = std::string;

struct resolver {
  template<typename Callback>
  void operator()(std::shared_ptr<Callback> callback, int key) {
    std::async(std::launch_async,
        [callback, key]() {
          std::ostringstream s;
          s << key;
          callback->assign(s.str());
        });
  }
};

libhoard::cache<
    key_type, mapped_type,
    libhoard::async_resolver_policy<resolver>
    > c(libhoard::resolver_policy<resolver>(resolver()));
```

An async resolver allows other calls into the cache while the lookup is progressing.
If another lookup for the same key arrives, it'll share the existing lookup.

When using an async resolver, you'll also need to perform an asyc lookup:
```
using error_type = std::exception_ptr; // The error type is controlled using the `libhoard::error_policy` policy.

std::future<std::variant<mapped_type, error_type>> future_value = cache.get(3);
future_value.get(); // std::variant(std::in_place_index<0>, "3")
```

## Refreshing Values

You can set up the cache to periodically refresh values in the cache.

```
#include <libhoard/cache.h>
#include <libhoard/resolver_policy.h>
#include <libhoard/refresh_policy.h>

using namespace std::literals::chrono_literals;
using key_type = int;
using mapped_type = std::string;

libhoard::cache<
    key_type, mapped_type,
    libhoard::resolver_policy<std::function<std::string(int)>>,
    libhoard::refresh_policy<refresh_policy<std::chrono::steady_clock>>
    > c(
        libhoard::resolver_policy<std::function<std::string(int)>>(
            [](int key) -> {
              std::ostringstream s;
              s << key;
              return s.str();
            }),
	libhoard::refresh_policy<std::chrono::steady_clock>(
	    1m, 15m
	));
```

The refresh-delay (`1m` in this example) causes the cache to refresh the value every `1m`.
The idle-delay (`15m` in this example) makes it so the cache will stop refreshing and remove the value, if it has not been accessed in `15m`.

If the idle-delay is `0s`, then the cache will keep refreshing indefinitely (or until the value is removed for another reason).

The refresh policy is implemented by running a single thread that initiates resolution.
It'll work with both synchronous and asynchronous resolution.
Because the refresh policy requires that the cache is thread-safe, it'll pull in the thread-safe-policy automatically.

# In Combination with Asio

You can use this in combination with [asio](https://think-async.com/Asio/).

## Asio Resolver

The asio resolver is always async, and resembles the regular async resolver.
Except that it has an asio executor associated with it.

```
#include <libhoard/cache.h>
#include <libhoard/thread_safe_policy.h>
#include <libhoard/asio/resolver_policy.h>

using key_type = int;
using mapped_type = std::string;

struct resolver {
  template<typename Callback>
  void operator()(std::shared_ptr<Callback> callback, int key) {
    std::async(std::launch_async,
        [callback, key]() {
          std::ostringstream s;
          s << key;
          callback->assign(s.str());
        });
  }
};

asio::io_context ioctx;
libhoard::cache<
    key_type, mapped_type,
    libhoard::thread_safe_policy,
    libhoard::async_resolver_policy<resolver, asio::io_context::executor_type>
    > c(libhoard::resolver_policy<resolver>(resolver(), ioctx.get_executor()));
```

Note that the asio resolver-policy doesn't automatically mark the cache thread-safe.
This is to avoid mutex overhead if you're using your asio program with only a single thread.
(I may change that, no idea.)

## Asio Refresh Policy

When using asio, we can also install a refresh policy that uses asio.

```
#include <libhoard/cache.h>
#include <libhoard/thread_safe_policy.h>
#include <libhoard/asio/resolver_policy.h>

using key_type = int;
using mapped_type = std::string;

struct resolver {
  template<typename Callback>
  void operator()(std::shared_ptr<Callback> callback, int key) {
    std::async(std::launch_async,
        [callback, key]() {
          std::ostringstream s;
          s << key;
          callback->assign(s.str());
        });
  }
};

asio::io_context ioctx;
libhoard::cache<
    key_type, mapped_type,
    libhoard::thread_safe_policy,
    libhoard::async_resolver_policy<resolver, asio::io_context::executor_type>,
    libhoard::asio_refresh_policy<std::chrono::steady_clock, asio::io_context::executor_type>
    > c(libhoard::asio_resolver_policy<resolver>(resolver(), ioctx.get_executor()),
        libhoard::asio_refresh_policy<std::chrono::steady_clock, asio::io_context::executor_type>(ioctx.get_executor(), 1m, 15m));
```

Please refer to the regular resolver for the meaning of `1m` and `15m`.

The refresh-policy will run its timers on the associated executor.
But the resolver will run on its own executor.

You can mix-and-match regular and asio resolver-policy and refresh-policy.
This is very useful if you want to use a non-asynchronous resolver-policy.

## Asio Dynamic Refresh Policy

Instead of using a fixed refresh timer, you can also use a function to declare an expiry dependent on the lookup value.

```
#include <libhoard/cache.h>
#include <libhoard/thread_safe_policy.h>
#include <libhoard/asio/resolver_policy.h>

using key_type = int;
using mapped_type = std::string;

struct resolver {
  template<typename Callback>
  void operator()(std::shared_ptr<Callback> callback, int key) {
    std::async(std::launch_async,
        [callback, key]() {
          std::ostringstream s;
          s << key;
          callback->assign(s.str());
        });
  }
};

struct expire_functor;

asio::io_context ioctx;
libhoard::cache<
    key_type, mapped_type,
    libhoard::thread_safe_policy,
    libhoard::async_resolver_policy<resolver, asio::io_context::executor_type>,
    libhoard::asio_refresh_fn_policy<std::chrono::steady_clock, expire_functor, asio::io_context::executor_type>
    > c(libhoard::asio_resolver_policy<resolver>(resolver(), ioctx.get_executor()),
        libhoard::asio_refresh_fn_policy<std::chrono::steady_clock, expire_functor, asio::io_context::executor_type>(ioctx.get_executor(), expire_functor(), 15m));
```

The expire function must be a functor that takes a mapped-type as argument (`std::string` in our example) and returns either a `std::chrono::time_point` or a `std::chrono::duration`.

For example
```
struct expire_functor {
  auto operator()(const std::string& v) const -> std::chrono::steady_clock::duration {
    return 3m;
  }
};
```
would be the same as:
```
struct expire_functor {
  auto operator()(const std::string& v) const -> std::chrono::steady_clock::time_point {
    return std::chrono::steady_clock::now() + 3m;
  }
};
```
The code will figure out if a `time_point` or `duration` is used and automatically adapt accordingly.

# Extending

You can write your own policies to extend functionality.
For example you could use the asio code as a guide to adapt to your own favourite event-library.
Or for example create your own cost function to keep the cache size under control.

A lot of the documentation required to extend the code I'll need to write.
Please accept my apologies for the lack of documentation. <3
