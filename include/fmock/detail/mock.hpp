// licence: GPLv3
// author: Maciej Chałapuk

#ifndef FMOCK_DETAIL_MOCK_HPP_
#define FMOCK_DETAIL_MOCK_HPP_

#include "fmock/detail/expectation.hpp"
#include "fmock/detail/call_check.hpp"
#include "fmock/expect_error.hpp"

#include <deque>
#include <sstream>
#include <cassert>

namespace fmock {
namespace detail {

class mock {
 public:
  mock() {}
  mock(mock const&) = delete;
  mock(mock &&) = delete;

  ~mock() throw(expect_error) {
    verify();
  }
 
  template <class return_t, class ...arg_ts>
  return_t call(arg_ts ...args) {
    if (!has_unsatisfied_expectations()) {
      throw make_unexpected_call_error<return_t, arg_ts...>();
    }
    auto const& exp = *expectations.front();

    call_check<return_t, arg_ts...> check;
    if (!check.return_type(exp)) {
      throw make_unexpected_call_error<return_t, arg_ts...>(exp);
    }
    if (!check.arg_count(exp)) {
      throw make_unexpected_call_error<return_t, arg_ts...>(exp);
    }
    if (!check.arg_types(exp)) {
      throw make_unexpected_call_error<return_t, arg_ts...>(exp);
    }

    typedef typed_expectation<return_t, arg_ts...> typed_expectation_type;
    auto const* typed_exp = dynamic_cast<typed_expectation_type const*>(&exp);
    assert(typed_exp != nullptr);

    auto args_tuple = std::forward_as_tuple(args...);
    size_t error_index = check.arg_values(*typed_exp, args_tuple);
    if (error_index != -1) {
      throw make_argument_mismatch_error(*typed_exp, args_tuple, error_index);
    }
    expectations.pop_front();
    return typed_exp->answer_function(std::forward<arg_ts>(args)...);
  }

  void verify() throw(expect_error) {
    if (!has_unsatisfied_expectations()) {
      return;
    }
    auto current_error = make_unsatisfied_error();
    if (!std::uncaught_exception()) {
      expectations.clear();
      throw current_error;
    }
    // error not thrown, can't do much about it (TODO stderr??)
  }

  void add_expectation(expectation *exp) {
    expectations.emplace_back(exp);
  }
 private:
  std::deque<std::unique_ptr<expectation>> expectations;

  expect_error make_unsatisfied_error() const {
    std::stringstream message_builder;
    message_builder << "unsatisfied expectations:" << std::endl;
    for (auto const& expect : expectations) {
      message_builder << "  " << expect->to_str() << std::endl;
    }
    return expect_error(message_builder.str());
  }
  template <class return_t, class ...arg_ts>
  expect_error make_unexpected_call_error() const {
    return expect_error("unexpected call");
  }
  template <class return_t, class ...arg_ts>
  expect_error make_unexpected_call_error(expectation const& exp) const {
    return expect_error("unexpected call");
  }
  template <class return_t, class ...arg_ts>
  expect_error make_argument_mismatch_error(
      typed_expectation<return_t, arg_ts...> const& exp,
      std::tuple<arg_ts &...> const& current_args,
      size_t arg_index
      ) const {
    return expect_error("argument not matching expectations");
  }

  bool has_unsatisfied_expectations() const noexcept(true) {
    return (expectations.size() != 0);
  }
}; // class mock

} // namespace detail
} // namespace fmock

#endif // include guard

