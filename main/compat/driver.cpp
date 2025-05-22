/**
 * @file main/compat/driver.cpp
 */
#include "driver.hpp"
#include "enums.hpp"
#include "freertos/idf_additions.h"
#include <algorithm>
#include <cstdint>
#include "../transport.h"

#define TAG "ZBOSSDriver"

ZBOSSDriver::ZBOSSDriver() {
}

ZBOSSDriver& ZBOSSDriver::instance() {
  static ZBOSSDriver s_driver{};
  return s_driver;
}

esp_err_t ZBOSSDriver::init_int() {
  ESP_LOGI(TAG, "init");
  m_input_buf = xStreamBufferCreate(RINGBUF_SIZE, 8);
  if (!m_input_buf) {
    ESP_LOGE(TAG, "Input buffer create error");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

void ZBOSSDriver::addEndpoint(uint8_t endpoint, uint16_t profileId, uint16_t deviceId,
                 const std::vector<uint16_t>& inputClusters,
                 const std::vector<uint16_t>& outputClusters) {
  execCommand(CommandId::AF_SET_SIMPLE_DESC, {
      { "endpoint", endpoint },
      { "profileID", profileId },
      { "deviceID", deviceId },
      { "version", (uint8_t)0 },
      { "inputClusterCount", (uint8_t)inputClusters.size() },
      { "outputClusterCount", (uint8_t)outputClusters.size() },
      { "inputClusters", inputClusters },
      { "outputClusters", outputClusters },
  });
}

void ZBOSSDriver::task_int() {
  int i = 0;
  uint16_t last_id = 0x0;
  while (true) {
    if (i == 0) {
      i = 1;
      auto result = execCommand(CommandId::GET_JOINED, {});
      if (result.IsErr()) {
        ESP_LOGE(TAG, "execCommand failed: %s", result.Err().c_str());
      }

      // Get network info
      result = execCommand(CommandId::GET_ZIGBEE_ROLE, {});
      bool joined = m_buffer[16] == 1;

      result = execCommand(CommandId::GET_LOCAL_IEEE_ADDR, Payload{ { "mac", (uint8_t)0 } });
      uint8_t ieee_addr[8];
      std::copy(m_buffer.data() + 16 + 1, m_buffer.data() + 16 + 1 + 8, ieee_addr);

      result = execCommand(CommandId::GET_EXTENDED_PAN_ID, {});
      uint8_t extended_pan_id[8] = { 0 };
      std::copy(m_buffer.data() + 16, m_buffer.data() + 16 + 8, extended_pan_id);

      result = execCommand(CommandId::GET_PAN_ID, {});
      uint8_t pan_id[8] = { 0 };
      std::copy(m_buffer.data() + 16, m_buffer.data() + 16 + 8, pan_id);

      result = execCommand(CommandId::GET_ZIGBEE_CHANNEL, {});
      uint8_t channel = m_buffer[16 + 1];

      // Form network
      result = execCommand(CommandId::SET_ZIGBEE_ROLE, {{"role", (uint8_t)DeviceType::COORDINATOR}});
      result = execCommand(CommandId::SET_ZIGBEE_CHANNEL_MASK, {
          {"page", (uint8_t)0},
          {"mask", (uint32_t)(1 << 11)}
      });
      result = execCommand(CommandId::SET_PAN_ID, {{"panID", (uint16_t)0x0e94}});
      result = execCommand(CommandId::SET_NWK_KEY, {
          {"nwkKey", Buffer{
            0x1e, 0xc5, 0x82, 0x77, 0x7d, 0xcd, 0xfd, 0xdf,
            0xd0, 0x72, 0xf7, 0x99, 0x5f, 0xfd, 0x82, 0x4f
          }},
          {"index", (uint8_t)0}
      });
      result = execCommand(CommandId::NWK_FORMATION, {
          {"len", (uint8_t)1},
          {"channels", std::vector<Payload>{{ {"page", (uint8_t)0}, {"mask", (uint32_t)(1 << 11)} }}},
          {"duration", (uint8_t)0x05},
          {"distribFlag", (uint8_t)0x00},
          {"distribNwk", (uint16_t)0x0000},
          {"extendedPanID", Buffer(extended_pan_id, extended_pan_id + 8)}
      });

      // Set policies
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::LINK_KEY_REQUIRED},
          {"value", (uint8_t)0}
      });
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::IC_REQUIRED},
          {"value", (uint8_t)0}
      });
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::TC_REJOIN_ENABLED},
          {"value", (uint8_t)1}
      });
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::IGNORE_TC_REJOIN},
          {"value", (uint8_t)0}
      });
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::APS_INSECURE_JOIN},
          {"value", (uint8_t)0}
      });
      execCommand(CommandId::SET_TC_POLICY, {
          {"type", (uint16_t)PolicyType::DISABLE_NWK_MGMT_CHANNEL_UPDATE},
          {"value", (uint8_t)0}
      });
      addEndpoint(1, 260, 0xbeef, {0x0000, 0x0003, 0x0006, 0x000a, 0x0019, 0x001a, 0x0300},
                  {
                    0x0000, 0x0003, 0x0004, 0x0005, 0x0006, 0x0008, 0x0020, 0x0300, 0x0400,
                    0x0402, 0x0405, 0x0406, 0x0500, 0x0b01, 0x0b03, 0x0b04, 0x0702, 0x1000,
                    0xfc01, 0xfc02,
                  });
      addEndpoint(242, 0xa1e0, 0x61, {}, { 0x0021 });
      execCommand(CommandId::SET_RX_ON_WHEN_IDLE, {{"rxOn", (uint8_t)1}});
    }

    auto recv_count = xStreamBufferReceive(m_input_buf, m_buffer.data(), m_buffer.size(), pdMS_TO_TICKS(RINGBUF_TIMEOUT_MS));

    if (recv_count == 0)
      continue;

    CommandId command_id = (CommandId)(*((uint16_t*)(m_buffer.data() + 11)));
    ESP_LOGI(TAG, "Command: %s", COMMAND_ID_NAME(command_id).c_str());

    uint16_t id = 0;
    switch (command_id) {
    case CommandId::NWK_LEAVE_IND:
      break;
    case CommandId::ZDO_DEV_UPDATE_IND:
      break;
    case CommandId::ZDO_DEV_ANNCE_IND:
      break;
    case CommandId::ZDO_DEV_AUTHORIZED_IND:
      break;
    case CommandId::APSDE_DATA_IND:
      // utils::hex_dump(m_buffer.data(), recv_count);
      id = *(uint16_t*)(m_buffer.data() + 38);
      if (m_buffer[14] == 04 && id != last_id) {   // Button press
        last_id = id;
        printf("==================\nBUTTON PRESS: %i\n==================\n", m_buffer[24]);
      }

      break;
    default:
      break;
    }
  }
}

esp_err_t ZBOSSDriver::start_int() {
  if (!m_input_buf) {
    ESP_LOGE(TAG, "need init");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "start");
  return (xTaskCreate(&task, "ZBOSSDriver",TASK_STACK * 4, this,TASK_PRIORITY, NULL) == pdTRUE) ? ESP_OK : ESP_FAIL;
}

BasicResult<ZBOSSFrame> ZBOSSDriver::execCommand(CommandId command_id, const Payload& params) {
  auto frame_res = ZBOSSFrame::From(FrameType::REQUEST, command_id, params);
  if (frame_res.IsErr()) {
    return { "ZBOSSDriver: Failed to create frame: " + frame_res.Err() };
  }
  auto frame = frame_res.unwrap();

  frame.m_tsn = m_tsn;
  m_tsn = (m_tsn + 1) % 255;

  auto res = sendFrame(frame);
  if (res.IsErr()) {
    return { "Failed to send frame: " + res.Err() };
  }

  return { "TODO: return response frame" };
}

BasicResult<int> ZBOSSDriver::sendFrame(ZBOSSFrame& frame) {
  auto buffalo = frame.Write();
  const auto& buf = buffalo->GetBuffer();
  transport::receive(buf.data(), buf.size());

  // FIXME: We need to check the received size in the future. It would be ideal
  // to pass the size through the events in APP, receive the data THERE with the
  // known size and pass them here.
  auto recv_size = xStreamBufferReceive(m_input_buf, m_buffer.data(), m_buffer.size(), pdMS_TO_TICKS(10'000));

  if (recv_size == 0) {
    return { "Failed to receive response frame. (timeout)" };
  }

  // ESP_LOGI(TAG, "Received: %i bytes.", recv_size);
  // utils::hex_dump(m_buffer.data(), recv_size);

  // auto res = ZBOSSFrame::Read(Buffer(m_buffer.data() + 9, m_buffer.data() + m_buffer.size()));


  // return { "ZBOSSDriver: TODO: response handling Not done yet" };
  return 0;
}

void ZBOSSDriver::receive_int(const void* data, size_t size) {
  xStreamBufferSend(m_input_buf, data, size, 0);
}
