/**
 * @file main/compat/driver.hpp
 */
#pragma once
#include "ZBOSSFrame.hpp"
#include "commands.hpp"
#include "enums.hpp"
#include "esp_log.h"
#include <array>

#define TAG "ZBOSSDriver"

class TestClass {
  static TestClass& instance() {
    static TestClass instance{};
    return instance;
  }
public:
  static esp_err_t init() {
    auto& i = instance();
    printf("Instance: %p\n", &i);
    return 0;
  }
};

class ZBOSSDriver {
private:
	static constexpr size_t BUF_SIZE = 1024;
	static constexpr size_t RINGBUF_SIZE = 1024*20;
	static constexpr size_t RINGBUF_TIMEOUT_MS = 50;
	static constexpr size_t TASK_STACK = 1024*4;
	static constexpr size_t TASK_PRIORITY = 18;

  ZBOSSDriver();
  static ZBOSSDriver& instance();
  esp_err_t init_int();
  esp_err_t start_int();
  StreamBufferHandle_t m_input_buf;

private:
  uint8_t m_tsn{ 1 };
  static constexpr size_t BUFFER_SIZE = 256 * 4;
  std::array<uint8_t, BUFFER_SIZE>  m_buffer{};

  void task_int();

	static void task(void *pvParameter) {
		static_cast<ZBOSSDriver*>(pvParameter)->task_int();
	}

  BasicResult<ZBOSSFrame> execCommand(CommandId command_id, const Payload& params);
  BasicResult<int> sendFrame(ZBOSSFrame& frame);

  void receive_int(const void* data, size_t size);

  void addEndpoint(uint8_t endpoint, uint16_t profileId, uint16_t deviceId,
                   const std::vector<uint16_t>& inputClusters,
                   const std::vector<uint16_t>& outputClusters);

public:
  static esp_err_t init() { return instance().init_int(); };
  static esp_err_t start() { return instance().start_int(); };
  static void receive(const void* data, size_t size) {
    instance().receive_int(data, size);
  }
};

#undef TAG
