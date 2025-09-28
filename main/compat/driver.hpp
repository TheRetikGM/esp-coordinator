/**
 * @file main/compat/driver.hpp
 */
#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include <array>
#include <cstddef>
#include <vector>

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

  void receive_int(const void* data, size_t size);

  void addEndpoint(uint8_t endpoint, uint16_t profileId, uint16_t deviceId,
                   const std::vector<uint16_t>& inputClusters,
                   const std::vector<uint16_t>& outputClusters);

  void initCommunication();

public:
  static esp_err_t init() { return instance().init_int(); };
  static esp_err_t start() { return instance().start_int(); };
  static void receive(const void* data, size_t size) {
    instance().receive_int(data, size);
  }
};

