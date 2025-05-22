/**
 * @file main/compat/zspec/zdo/definition/cpptypes.hpp
 */
#pragma once
#include "status.hpp"
#include <any>
#include <optional>

namespace zdo {
  struct GenericResponse {
    Status m_status;
    std::optional<std::any> m_data;
  };
} // namespace zdo

