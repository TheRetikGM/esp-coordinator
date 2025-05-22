/**
 * @file main/compat/buffalo/buffalo.hpp
 */
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include "../enums.hpp"
#include "esp_log.h"
#include "esp_log.h"
#include "../../utils.h"

#define TAG "Buffalo"

class Buffalo {
protected:
  size_t m_position{ 0 };
  Buffer m_buffer{};

public:

  Buffalo() = default;
  Buffalo(Buffer&& buf) : m_buffer(buf) {}

  size_t GetPosition() const { return m_position; }

  const Buffer& GetBuffer() const { return m_buffer; }
  Buffer& GetBuffer() { return m_buffer; }

  void WriteBuffer(const Buffer& buffer, size_t length) {
    if (buffer.size() != length) {
      ESP_LOGE(TAG, "Buffer size doesn't correspond to the length");
      return;
    }

    m_buffer.insert(m_buffer.begin() + m_position, buffer.begin(), buffer.end());
    m_position += length;
  }

  void WriteListUInt16(const std::vector<uint16_t>& values) {
    for (const auto& value : values) {
      this->WriteUInt16(value);
    }
  }

  void WriteListUInt8(const std::vector<uint8_t>& values) {
    for (const auto& value : values) {
      WriteUInt8(value);
    }
  }

  void WriteUInt32(uint32_t value) {
    auto bytes = reinterpret_cast<uint8_t*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
    m_buffer.push_back(bytes[2]);
    m_buffer.push_back(bytes[3]);
    m_position += 4;
  }

  void WriteUInt16(uint16_t value) {
    auto bytes = reinterpret_cast<uint8_t*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
    m_position += 2;
  }

  void WriteUInt8(uint8_t value) {
    m_buffer.push_back(value);
    m_position += 1;
  }

  void WriteIeeeAddr(const std::string& addr) {
    auto p1 = std::stoul(addr.substr(10), nullptr, 16);
    auto p2 = std::stoul(addr.substr(2, 8), nullptr, 16);

    this->WriteUInt32(p1);
    this->WriteUInt32(p2);
  }

  uint8_t ReadUInt8() {
    auto value = m_buffer[m_position];
    m_position += 1;
    return value;
  }

  uint16_t ReadUInt16() {
    auto p_value = (uint16_t*)(m_buffer.data() + m_position);
    m_position += 2;
    return *p_value;
  }

  uint32_t ReadUInt32() {
    auto p_value = (uint32_t*)(m_buffer.data() + m_position);
    m_position += 4;
    return *p_value;
  }
};

#undef TAG
