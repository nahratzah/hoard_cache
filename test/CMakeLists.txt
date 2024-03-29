find_package(UnitTest++)
find_package(cycle_ptr 0.4 QUIET)

if (UnitTest++_FOUND)
  set(extra_srcs "")
  include(CTest)
  include(CheckIncludeFileCXX)

  check_include_file_cxx(asio.hpp HAS_ASIO_HEADERS)

  if (cycle_ptr_FOUND)
    list(APPEND extra_srcs cycle_pointer.cc)
  endif ()

  add_executable (tests
      detail/basic_hashtable.cc
      detail/hashtable.cc
      detail/linked_list.cc
      detail/mapped_type.cc
      detail/meta.cc
      detail/pending.cc
      detail/queue.cc
      detail/refcount.cc
      cache.cc
      max_size_policy.cc
      resolver_policy.cc
      max_age_policy.cc
      refresh_policy.cc
      shared_pointer.cc
      ${extra_srcs}
      test_main.cc)
  target_link_libraries (tests UnitTest++ libhoard)
  target_include_directories(tests PUBLIC ${UTPP_INCLUDE_DIRS})
  add_test (libhoard_tests tests)
  set_property (TARGET tests PROPERTY CXX_STANDARD 17)
  set_property (TARGET tests PROPERTY CXX_STANDARD_REQUIRED 17)
  if (cycle_ptr_FOUND)
    target_link_libraries(tests cycle_ptr)
  endif ()

  if (HAS_ASIO_HEADERS)
    add_executable (asio_tests
        asio/resolver_policy.cc
        asio/refresh_policy.cc
        test_main.cc)
    target_link_libraries (asio_tests UnitTest++ libhoard)
    target_include_directories(asio_tests PUBLIC ${UTPP_INCLUDE_DIRS})
    add_test (libhoard_asio_tests asio_tests)
    set_property (TARGET asio_tests PROPERTY CXX_STANDARD 17)
    set_property (TARGET asio_tests PROPERTY CXX_STANDARD_REQUIRED 17)
  endif ()
endif ()
