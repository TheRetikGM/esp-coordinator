/**
 * @file main/compat/buffalo/zboss_buffalo_zcl.hpp
 */
#pragma once
#include "buffalo_zcl.hpp"
#include "../enums.hpp"
#include "../zspec/zcl/definition/enums.hpp"
#include <variant>

class ZBOSSBuffaloZcl : public BuffaloZcl {
public:
  ZBOSSBuffaloZcl() = default;
  ZBOSSBuffaloZcl(size_t reserve);
  ZBOSSBuffaloZcl(Buffer&& buf) : BuffaloZcl(std::move(buf)) {}

  void Write(const std::variant<zcl::DataType, zcl::BuffaloDataType, BuffaloZBOSSDataType>& type, std::any& value, BuffaloZclOptions& options) override;

  size_t WriteByDesc(Payload& payload, const std::vector<struct ParamsDesc>& params);
};
