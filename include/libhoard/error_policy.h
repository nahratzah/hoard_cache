#pragma once

#include <exception>

namespace libhoard {


template<typename ErrorType = std::exception_ptr>
class error_policy {
  public:
  using error_type = ErrorType;
};


} /* namespace libhoard */
