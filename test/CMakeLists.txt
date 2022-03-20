find_package(UnitTest++)

if (UnitTest++_FOUND)
  include(CTest)
  include(CheckIncludeFileCXX)

  check_include_file_cxx(asio.hpp HAS_ASIO_HEADERS)

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
      test_main.cc)
  target_link_libraries (tests UnitTest++ libhoard)
  target_include_directories(tests PUBLIC ${UTPP_INCLUDE_DIRS})
  add_test (libhoard_tests tests)
  set_property (TARGET tests PROPERTY CXX_STANDARD 17)
  set_property (TARGET tests PROPERTY CXX_STANDARD_REQUIRED 17)

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
