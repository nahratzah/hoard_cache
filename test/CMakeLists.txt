find_package(UnitTest++)

if (UnitTest++_FOUND)
  include(CTest)

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
      test_main.cc)
  target_link_libraries (tests UnitTest++ libhoard)
  target_include_directories(tests PUBLIC ${UTPP_INCLUDE_DIRS})
  add_test (libhoard_tests tests)
  set_property (TARGET tests PROPERTY CXX_STANDARD 20)
  set_property (TARGET tests PROPERTY CXX_STANDARD_REQUIRED 20)
endif ()
