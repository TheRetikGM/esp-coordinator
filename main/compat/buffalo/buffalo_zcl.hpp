/**
 * @file main/compat/buffalo/buffalo_zcl.hpp
 */
#pragma once
#include "buffalo.hpp"
#include <variant>
#include "../zspec/zcl/definition/enums.hpp"
#include <any>

class BuffaloZcl : public Buffalo {
public:
  BuffaloZcl() = default;
  BuffaloZcl(Buffer&& buf) : Buffalo(std::move(buf)) {}

  virtual void Write(const std::variant<zcl::DataType, zcl::BuffaloDataType, BuffaloZBOSSDataType>& type, std::any& value, BuffaloZclOptions& options) {
    if (std::holds_alternative<zcl::BuffaloDataType>(type)) {
      switch (std::get<zcl::BuffaloDataType>(type)) {
        case zcl::BuffaloDataType::LIST_UINT16:
          this->WriteListUInt16(std::any_cast<const std::vector<uint16_t>&>(value));
          return;
        case zcl::BuffaloDataType::LIST_UINT8:
          this->WriteListUInt8(std::any_cast<const std::vector<uint8_t>&>(value));
          return;
        default:
          break;
      }
    } else if (std::holds_alternative<zcl::DataType>(type)) {
      switch (std::get<zcl::DataType>(type)) {
        case zcl::DataType::IEEE_ADDR:
          this->WriteIeeeAddr(std::any_cast<const std::string&>(value));
          return;
        case zcl::DataType::SEC_KEY:
          this->WriteBuffer(std::any_cast<const Buffer&>(value), SEC_KEY_LENGTH);
          return;
        case zcl::DataType::UINT16:
        case zcl::DataType::DATA16:
        case zcl::DataType::BITMAP16:
        case zcl::DataType::ENUM16:
        case zcl::DataType::CLUSTER_ID:
        case zcl::DataType::ATTR_ID:
          this->WriteUInt16(std::any_cast<uint16_t>(value));
          return;
        case zcl::DataType::DATA32:
        case zcl::DataType::BITMAP32:
        case zcl::DataType::UINT32:
        case zcl::DataType::UTC:
        case zcl::DataType::BAC_OID:
          this->WriteUInt32(std::any_cast<uint32_t>(value));
          return;
        case zcl::DataType::DATA8:
        case zcl::DataType::BOOLEAN:
        case zcl::DataType::BITMAP8:
        case zcl::DataType::UINT8:
        case zcl::DataType::ENUM8:
          this->WriteUInt8(std::any_cast<uint8_t>(value));
          return;
        default:
          break;
      }
    }

    // In case the type is undefined, write it as a buffer to easily allow for custom types
    // e.g. for https://github.com/Koenkk/zigbee-herdsman/issues/127
    // if (Buffer.isBuffer(value) || isNumberArray(value)) {
    //  this.writeBuffer(value, value.length);
    //  break;
    // }

    // throw new Error(`Write for '${type}' not available`);
  }
};
