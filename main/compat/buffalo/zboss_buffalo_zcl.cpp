/**
 * @file main/compat/buffalo/zboss_buffalo_zcl.cpp
 */
#include "../commands.hpp"
#include "buffalo_zcl.hpp"
#include <variant>
#include "zboss_buffalo_zcl.hpp"

#define VARIANT_IS(variant, datatype, value) \
  (std::holds_alternative<datatype>(variant) && std::get<datatype>(variant) == value)

ZBOSSBuffaloZcl::ZBOSSBuffaloZcl(size_t reserve) {
  m_buffer.reserve(reserve);
}

void ZBOSSBuffaloZcl::Write(const std::variant<zcl::DataType, zcl::BuffaloDataType, BuffaloZBOSSDataType>& type, std::any& value, BuffaloZclOptions& options) {
  if (VARIANT_IS(type, BuffaloZBOSSDataType, BuffaloZBOSSDataType::EXTENDED_PAN_ID)) {
    this->WriteBuffer(std::any_cast<Buffer>(value), 8);
    return;
  }

  BuffaloZcl::Write(type, value, options);
}

size_t ZBOSSBuffaloZcl::WriteByDesc(Payload& payload, const std::vector<ParamsDesc>& params) {
  auto start = this->GetPosition();
  for (const auto& parameter : params) {
    BuffaloZclOptions options{};

    if (parameter.m_condition.has_value() && !parameter.m_condition.value()(&payload, this)) {
      continue;
    }

    if (parameter.m_options.has_value()) {
      // parameter.m_options.value()(&payload, &options);
    }

    if (VARIANT_IS(parameter.m_data_type, BuffaloZBOSSDataType, BuffaloZBOSSDataType::LIST_TYPED)) {
      auto& internal_payload = std::any_cast<std::vector<Payload>&>(payload[parameter.m_name]);

      for (auto& value : internal_payload) {
        this->WriteByDesc(value, std::any_cast<const std::vector<ParamsDesc>&>(parameter.m_typed.value()));
      }
    } else {
      this->Write(parameter.m_data_type, payload[parameter.m_name], options);
    }
  }

  return this->GetPosition() - start;
}
