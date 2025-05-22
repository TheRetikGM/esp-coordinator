/**
 * @file main/compat/ZBOSSFrame.hpp
 */
#pragma once
#include "../utils.h"
#include "buffalo/zboss_buffalo_zcl.hpp"
#include "commands.hpp"
#include "enums.hpp"
#include "zspec/zdo/definition/clusters.hpp"
#include "zspec/zdo/definition/cpptypes.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

enum class FrameType : uint8_t {
    REQUEST = 0,
    RESPONSE = 1,
    INDICATION = 2,
};

struct ZBOSSFramePayload {
  Payload m_parameters;
  std::optional<zdo::ClusterId>m_zdo_cluster{};
  std::optional<zdo::GenericResponse> m_zdo{};
};

#define TAG "ZBOSSFrame"

struct ZBOSSFrame {
  uint8_t m_version{ 0 };
  FrameType m_type{ FrameType::REQUEST };
  CommandId m_command_id{ CommandId::UNKNOWN_1 };
  uint8_t m_tsn{ 0 };
  ZBOSSFramePayload m_payload;

  std::unique_ptr<ZBOSSBuffaloZcl> Write() {
    auto buf = std::make_unique<ZBOSSBuffaloZcl>(247);

    buf->WriteUInt8(m_version);
    buf->WriteUInt8((uint8_t)m_type);
    buf->WriteUInt16((uint16_t)m_command_id);
    buf->WriteUInt8(m_tsn);

    auto desc_res = GetFrameDesc(m_type, m_command_id);
    if (desc_res.IsOk()) {
      buf->WriteByDesc(m_payload.m_parameters, *desc_res.unwrap());
    } else {
      ESP_LOGE(TAG, "Write failed: Failed to get frame description: %s", desc_res.Err().c_str());
    }

    return buf;
  }

  static BasicResult<std::unique_ptr<ZBOSSFrame>> Read(Buffer&& buffer) {
    auto buf = ZBOSSBuffaloZcl(std::move(buffer));
    auto version = buf.ReadUInt8();
    auto type = (FrameType)buf.ReadUInt8();
    auto command_id = (CommandId)buf.ReadUInt16();
    auto tsn = type == FrameType::REQUEST || type == FrameType::RESPONSE ? buf.ReadUInt8() : 0;

    ESP_LOGI(TAG, "Received: { v = %i, t = %i, cmd = %s, tsn = %i }", version, (int)type, COMMAND_ID_NAME(command_id).c_str(), tsn);

    std::optional<zdo::ClusterId> zdo_response_cluster_id =
        type == FrameType::RESPONSE || type == FrameType::INDICATION ? (std::optional<zdo::ClusterId>)get_cluster_id(command_id) : (std::optional<zdo::ClusterId>){};

    if (zdo_response_cluster_id.has_value()) {
      auto category = type == FrameType::RESPONSE ? buf.ReadUInt8() : 0;
      fixNonStandardZdoRspPayload(zdo_response_cluster_id.value(), buf.GetBuffer(), type == FrameType::RESPONSE ? 6 : 4);

        // const zdo = BuffaloZdo.readResponse(false, zdoResponseClusterId, zdoPayload);
        //
        // return {
        //     version,
        //     type,
        //     commandId,
        //     tsn,
        //     payload: {
        //         category,
        //         zdoClusterId: zdoResponseClusterId,
        //         zdo,
        //     },
        // };
        //
    }

    return {"ZBOSSFrame::read: Not implemented yet"};
  }

  static void _transformBuffer(std::vector<uint8_t>& buffer, size_t offset) {
    if (buffer.size() < 10) return; // Sanity check

    const size_t nwkAddressOffset = buffer.size() - 2;
    const size_t inClusterCountOffset = 7;
    const size_t inClusterCount = buffer[offset + inClusterCountOffset];
    const size_t inClusterListSize = inClusterCount * 2;

    const size_t outClusterCountOffset = offset + 8 + inClusterListSize;
    const size_t outClusterListOffset = offset + outClusterCountOffset + 1;
    const size_t outClusterListSize = nwkAddressOffset - outClusterListOffset;

    // Save data we'll move and insert
    uint8_t status = buffer[offset];
    uint8_t nwkAddress[2] = { buffer[nwkAddressOffset], buffer[nwkAddressOffset + 1] };
    uint8_t length = static_cast<uint8_t>(buffer.size() - 3);

    // Create a new buffer to store transformed data (or move in place with care)
    std::vector<uint8_t> result;
    result.reserve(buffer.size() + 1);

    // Copy everything until the offset
    if (offset != 0) {
      std::copy(buffer.begin(), buffer.begin() + offset, result.begin());
    }

    // Append in new order
    result.push_back(status);                              // [status]
    result.insert(result.end(), nwkAddress, nwkAddress+2); // [nwkAddress]
    result.push_back(length);                              // [length]
    result.insert(result.end(), buffer.begin() + 1, buffer.begin() + 8);  // [endpoint -> inClusterCount]
    result.insert(result.end(), buffer.begin() + 9, buffer.begin() + 9 + inClusterListSize); // [inClusterList]
    result.push_back(buffer[8]);                           // [outClusterCount]
    result.insert(result.end(), buffer.begin() + outClusterListOffset, buffer.begin() + nwkAddressOffset); // [outClusterList]

    // Replace original buffer
    buffer = std::move(result);
  }

  static void fixNonStandardZdoRspPayload(zdo::ClusterId cluster_id, Buffer& buffer, size_t offset) {
    uint8_t last2[2] = { 0 };
    switch (cluster_id) {
      case zdo::ClusterId::NODE_DESCRIPTOR_RESPONSE:
      case zdo::ClusterId::POWER_DESCRIPTOR_RESPONSE:
      case zdo::ClusterId::ACTIVE_ENDPOINTS_RESPONSE:
      case zdo::ClusterId::MATCH_DESCRIPTORS_RESPONSE:
        // flip nwkAddress from end to start
        last2[0] = buffer[buffer.size() - 2];
        last2[1] = buffer[buffer.size() - 1];
        std::move_backward(buffer.begin() + offset + 1, buffer.end() - 2, buffer.end());
        buffer[offset + 1] = last2[0];
        buffer[offset + 2] = last2[1];
        return;
      case zdo::ClusterId::SIMPLE_DESCRIPTOR_RESPONSE:
        _transformBuffer(buffer, offset);
        return;
      default:
        return;
    }
  }


  static BasicResult<ZBOSSFrame> From(FrameType type, CommandId command_id, const Payload& params) {
    auto res = GetFrameDesc(type, command_id);
    RESULT_TRY_ASSIGN(auto frame_desc, res);

    Payload payload{};
    for (auto& parameter : *frame_desc) {
      if (parameter.m_condition.has_value() && !parameter.m_condition.value()(&payload, nullptr)) {
        continue;
      }

      payload[parameter.m_name] = params.at(parameter.m_name);
    }

    return ZBOSSFrame{
      .m_version = 0,
      .m_type = type,
      .m_command_id = command_id,
      .m_tsn = 0,
      .m_payload = ZBOSSFramePayload { std::move(payload) }
    };
  }

  static BasicResult<const std::vector<ParamsDesc>*> GetFrameDesc(FrameType type, CommandId key) {
    if (get_frames().count(key) == 0)
      return { "Unrecognized frame type from FrameID " + COMMAND_ID_NAME(key) };

    const auto& frame_desc = get_frames().at(key);
    switch (type) {
    case FrameType::REQUEST:
      return &frame_desc.m_request;
    case FrameType::RESPONSE:
      return &frame_desc.m_response;
    case FrameType::INDICATION:
      return &frame_desc.m_indication;
    }

    return { "Invalid type" };
  }
};

#undef TAG

