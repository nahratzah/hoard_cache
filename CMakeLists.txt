cmake_minimum_required(VERSION 3.3)
project(libhoard)

include(CheckCXXCompilerFlag)

set(LIBHOARD_VERSION_MAJOR 0 CACHE STRING "major version" FORCE)
set(LIBHOARD_VERSION_MINOR 1 CACHE STRING "minor version" FORCE)
set(LIBHOARD_VERSION ${MONSOON_CACHE_VERSION_MAJOR}.${MONSOON_CACHE_VERSION_MINOR} CACHE STRING "version" FORCE)

# If using ninja and clang, enable colours.
if (UNIX AND
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_GENERATOR STREQUAL "Ninja")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

check_cxx_compiler_flag(-Wall cxx_wall_flag)
if (cxx_wall_flag)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()
check_cxx_compiler_flag(-Wextra cxx_wextra_flag)
if (cxx_wextra_flag)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra")
endif()

enable_testing()

set(headers
    include/libhoard/allocator.h
    include/libhoard/cache.h
    include/libhoard/doc_.h
    include/libhoard/equal.h
    include/libhoard/error_policy.h
    include/libhoard/expire_at_policy.h
    include/libhoard/expire_at_policy.ii
    include/libhoard/hash.h
    include/libhoard/max_age_policy.h
    include/libhoard/max_age_policy.ii
    include/libhoard/max_size_policy.h
    include/libhoard/max_size_policy.ii
    include/libhoard/negative_cache_policy.h
    include/libhoard/pointer_policy.h
    include/libhoard/policies.h
    include/libhoard/refresh_policy.h
    include/libhoard/refresh_policy.ii
    include/libhoard/resolver_policy.h
    include/libhoard/resolver_policy.ii
    include/libhoard/shared_from_this_policy.h
    include/libhoard/thread_safe_policy.h
    include/libhoard/thread_safe_policy.ii
    include/libhoard/thread_unsafe_policy.h
    include/libhoard/weaken_policy.h
    )
set(detail_headers
    include/libhoard/detail/async_resolver_callback.h
    include/libhoard/detail/basic_hashtable.h
    include/libhoard/detail/basic_hashtable.ii
    include/libhoard/detail/cache_async_get.h
    include/libhoard/detail/cache_get.h
    include/libhoard/detail/function_ref.h
    include/libhoard/detail/function_ref.ii
    include/libhoard/detail/hashtable.h
    include/libhoard/detail/hashtable.ii
    include/libhoard/detail/identity.h
    include/libhoard/detail/identity_fn.h
    include/libhoard/detail/identity_fn.ii
    include/libhoard/detail/linked_list.h
    include/libhoard/detail/linked_list.ii
    include/libhoard/detail/mapped_type.h
    include/libhoard/detail/mapped_type.ii
    include/libhoard/detail/meta.h
    include/libhoard/detail/notes.txt
    include/libhoard/detail/pending.h
    include/libhoard/detail/pending.ii
    include/libhoard/detail/queue.h
    include/libhoard/detail/queue.ii
    include/libhoard/detail/refcount.h
    include/libhoard/detail/refcount.ii
    include/libhoard/detail/refresh_impl_policy.h
    include/libhoard/detail/refresh_impl_policy.ii
    include/libhoard/detail/traits.h
    include/libhoard/detail/value_type.h
    include/libhoard/detail/value_type.ii
    )
set(asio_headers
    include/libhoard/asio/refresh_policy.h
    include/libhoard/asio/refresh_policy.ii
    include/libhoard/asio/resolver_policy.h
    )

add_library(libhoard INTERFACE)
set_property (TARGET libhoard PROPERTY CXX_STANDARD 17)
set_property (TARGET libhoard PROPERTY CXX_STANDARD_REQUIRED 17)

find_package (Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(libhoard INTERFACE ${CMAKE_THREAD_LIBS_INIT})
endif()

target_include_directories(libhoard INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    )

install(FILES ${headers} DESTINATION "include/libhoard")
install(FILES ${detail_headers} DESTINATION "include/libhoard/detail")
install(FILES ${asio_headers} DESTINATION "include/libhoard/asio")
install(TARGETS libhoard EXPORT libhoard DESTINATION "lib")
install(EXPORT libhoard DESTINATION "lib/cmake/libhoard")

configure_file(libhoard-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/libhoard-config-version.cmake @ONLY)
install(FILES libhoard-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libhoard-config-version.cmake DESTINATION "lib/cmake/libhoard")

add_subdirectory(test)
add_subdirectory(examples)

find_package(Doxygen COMPONENTS mscgen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
  doxygen_add_docs(libhoard-doc include)
endif()
