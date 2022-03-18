add_executable (fibonacci fibonacci.cc)
target_link_libraries (fibonacci libhoard)
set_property (TARGET fibonacci PROPERTY CXX_STANDARD 17)
set_property (TARGET fibonacci PROPERTY CXX_STANDARD_REQUIRED 17)
