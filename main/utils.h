#pragma once
#include <cstdint>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>
#include <variant>

namespace utils {

	uint8_t crc8(const void* data,size_t size);
	uint16_t crc16(const void* data,size_t size);

	class sem_lock {
		SemaphoreHandle_t m_sem;
	public:
		sem_lock(SemaphoreHandle_t sem) : m_sem(sem) {
			xSemaphoreTake(m_sem, portMAX_DELAY);
		}
		~sem_lock() {
			xSemaphoreGive(m_sem);
		}
	};

	const char* get_zdp_status_str(uint8_t status);
	const char* get_nlme_status_str(uint8_t status);


#define IEEE_ADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x"
#define IEEE_ADDR_PRINT(a) \
    static_cast<unsigned int>(a[0]),static_cast<unsigned int>(a[1]),static_cast<unsigned int>(a[2]),static_cast<unsigned int>(a[3]), \
    static_cast<unsigned int>(a[4]),static_cast<unsigned int>(a[5]),static_cast<unsigned int>(a[6]),static_cast<unsigned int>(a[7])

  void hex_dump(const void* data, size_t size);
}

template<class R, class E>
class Result {
  std::variant<R, E> m_res;
  bool m_isOk{ false };

public:
  using TOk = R;
  using TErr = E;

  static Result<R, E> Ok(const R& r) {
    Result<R, E> res(r);
    return res;
  }

  static Result<R, E> Err(const E& e) {
    Result<R, E> res(e);
    res.m_isOk = false;
    return res;
  }

  Result(const R& r) {
    m_res = r;
    m_isOk = true;
  }

  template<typename U = E, typename = typename std::enable_if<!std::is_same<R, U>::value>::type>
  Result(const E& e) {
    m_res = e;
    m_isOk = false;
  }

  R& Ok() { return std::get<0>(m_res); }
  E& Err() { return std::get<1>(m_res); }

  bool IsOk() { return m_isOk; }
  bool IsErr() { return !m_isOk; }

  R unwrap_or(const R& or_value) { return m_isOk ? unwrap() : or_value; }
  R unwrap() { return std::move(std::get<0>(m_res)); }
};

template<class T>
using BasicResult = Result<T, std::string>;

#define RESULT_TRY_ASSIGN(var, result) \
  if (result.IsErr()) { return result.Err(); } var = result.unwrap()

#define RESULT_MATCH(result, body_ok, body_err) \
  result.IsOk() \
  ? [&](decltype(result)::TOk& ok)body_ok(result.Ok()) \
  : [&](decltype(result)::TErr& err)body_err(result.Err())
