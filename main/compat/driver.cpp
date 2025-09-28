/**
 * @file main/compat/driver.cpp
 */
#include "driver.hpp"
#include "commands.h"
#include "freertos/idf_additions.h"
#include <cstdint>
#include "../transport.h"

#define TAG "ZBOSSDriver"

#include "commands_impl.h"
#include "ind_impl.h"

#define ARR8_INIT(arr8) { arr8[0], arr8[1], arr8[2], arr8[3], arr8[4], arr8[5], arr8[6], arr8[7] }


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

  zb_ncp::cmd_handle<AF_SET_SIMPLE_DESC>::process_immediate_d({
      .endpoint = endpoint,
      .profileID = profileId,
      .deviceID = deviceId,
      .version = 0,
      .inputClusterCount = (uint8_t)inputClusters.size(),
      .outputClusterCount = (uint8_t)outputClusters.size(),
      // TODO: Add clusters
  });
}

void ZBOSSDriver::initCommunication() {
  auto _joined = zb_ncp::cmd_handle<GET_JOINED>::process_status_res_d();
  auto _role = zb_ncp::cmd_handle<GET_ZIGBEE_ROLE>::process_status_res_d();
  auto _ieee_addr = zb_ncp::cmd_handle<GET_LOCAL_IEEE_ADDR>::process_status_arg_res_d(0);
  auto _extended_pan_id = zb_ncp::cmd_handle<GET_EXTENDED_PAN_ID>::process_status_res_d();
  auto _pan_id = zb_ncp::cmd_handle<GET_PAN_ID>::process_status_res_d();
  auto _channel = zb_ncp::cmd_handle<GET_ZIGBEE_CHANNEL>::process_status_res_d();
  zb_ncp::cmd_handle<SET_ZIGBEE_ROLE>::process_status_arg_d(ZB_NWK_DEVICE_TYPE_COORDINATOR);
  zb_ncp::cmd_handle<SET_ZIGBEE_CHANNEL_MASK>::process_status_arg_d({
      .page = 0,
      .mask = 1 << 11
  });
  zb_ncp::cmd_handle<SET_PAN_ID>::process_status_arg_d(0x0e94);
  zb_ncp::cmd_handle<SET_NWK_KEY>::process_status_arg_d({
      .nwkKey = {
        0x1e, 0xc5, 0x82, 0x77, 0x7d, 0xcd, 0xfd, 0xdf,
        0xd0, 0x72, 0xf7, 0x99, 0x5f, 0xfd, 0x82, 0x4f
      },
      .index = 0
  });

  zb_ncp::cmd_handle<NWK_FORMATION>::process(
    NWK_FORMATION_arg_t{
      .channels = {
        { .page = 0, .mask = 1 << 11 },
      },
      .duration = 0x05,
      .distrib_flag = 0x00,
      .distrib_nwk = 0x0000,
      .extended_pan_id = ARR8_INIT(_extended_pan_id.value),
    },
    [](const NWK_FORMATION_resp_t& r) {
      ESP_LOGI(TAG, "NWK_FORMATION response: { 0x%x, 0x%x, 0x%x }", r.category, r.status, r.nwk);
    }
  );

  // Set policies
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = LINK_KEY_REQUIRED,
      .value = 0
  });
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = IC_REQUIRED,
      .value = 0
  });
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = TC_REJOIN_ENABLED,
      .value = 1
  });
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = IGNORE_TC_REJOIN,
      .value = 0
  });
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = APS_INSECURE_JOIN,
      .value = 0
  });
  zb_ncp::cmd_handle<SET_TC_POLICY>::process_status_arg_d(SET_TC_POLICY_arg_t{
      .type = DISABLE_NWK_MGMT_CHANNEL_UPDATE,
      .value = 0
  });

  addEndpoint(1, 260, 0xbeef, {0x0000, 0x0003, 0x0006, 0x000a, 0x0019, 0x001a, 0x0300},
              {
                0x0000, 0x0003, 0x0004, 0x0005, 0x0006, 0x0008, 0x0020, 0x0300, 0x0400,
                0x0402, 0x0405, 0x0406, 0x0500, 0x0b01, 0x0b03, 0x0b04, 0x0702, 0x1000,
                0xfc01, 0xfc02,
              });
  addEndpoint(242, 0xa1e0, 0x61, {}, { 0x0021 });

  zb_ncp::cmd_handle<SET_RX_ON_WHEN_IDLE>::process_status_arg_d(1);
}

// TODO: Refactor
void ZBOSSDriver::task_int() {
  uint16_t last_id = 0x0;

  initCommunication();

  zb_ncp::ind_handle<APSDE_DATA_IND>::connect([](const zb_apsde_data_indication_t& arg){
    ESP_LOGI(TAG, "APSDE_DATA_IND: ClusterId: 0x%x, EndpointId: 0x%x -> 0x%x",
        arg.clusterid, arg.src_endpoint, arg.dst_endpoint);

    ESP_LOGI(TAG, "<<<<< APSDE_DATA_IND");
    ESP_LOGI(TAG, "fc = 0x%i", arg.fc);
    ESP_LOGI(TAG, "src_addr = 0x%i", arg.src_addr);
    ESP_LOGI(TAG, "dst_addr = 0x%i", arg.dst_addr);
    ESP_LOGI(TAG, "group_addr = 0x%i", arg.group_addr);
    ESP_LOGI(TAG, "dst_endpoint = 0x%i", arg.dst_endpoint);
    ESP_LOGI(TAG, "src_endpoint = 0x%i", arg.src_endpoint);
    ESP_LOGI(TAG, "clusterid = 0x%i", arg.clusterid);
    ESP_LOGI(TAG, "profile_id = 0x%i", arg.profileid);
    ESP_LOGI(TAG, "aps_counter = 0x%i", arg.aps_counter);
    ESP_LOGI(TAG, "mac_src_addr = 0x%i", arg.mac_src_addr);
    ESP_LOGI(TAG, "mac_dst_addr = 0x%i", arg.mac_dst_addr);
    ESP_LOGI(TAG, "tsn = 0x%i", arg.tsn);
    ESP_LOGI(TAG, "block_num = 0x%i", arg.block_num);
    ESP_LOGI(TAG, "block_ack = 0x%i", arg.block_ack);
    ESP_LOGI(TAG, "radius = 0x%i", arg.radius);
    ESP_LOGI(TAG, "====");

    uint8_t dst_addr[8] = {0};
    *reinterpret_cast<uint16_t*>(dst_addr) = arg.src_addr;

    APSDE_DATA_REQ_max_arg_t req = {
      .hdr = {
        .paramLength = sizeof(apsde_data_req_base_t),
        .dataLength = 0,
        .base = {
          .addr_data = ARR8_INIT(dst_addr),
          .profile_id = arg.profileid,
          .cluster_id = arg.clusterid,
          .dst_endpoint = arg.src_endpoint,
          .src_endpoint = arg.dst_endpoint,
          .radius = arg.radius,
          .addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          .tx_options = ZB_TRUE,
          .use_alias = 0,
          .alias_src_addr = 0,
          .alias_seq_num = 0
        },
      },
      .data = { 0 }
    };

    zb_ncp::cmd_t cmd = {
      .version = 0,
      .type = zb_ncp::REQUEST,
      .command_id = APSDE_DATA_REQ,
      .tsn = 0,
    };

    zb_ncp::cmd_handle<APSDE_DATA_REQ>::process(cmd, &req, sizeof(req));
  });

  while (true) {
    auto recv_count = xStreamBufferReceive(m_input_buf, m_buffer.data(), m_buffer.size(), pdMS_TO_TICKS(RINGBUF_TIMEOUT_MS));

    if (recv_count == 0)
      continue;

    auto command_id = (command_id_t)(*((uint16_t*)(m_buffer.data() + 11)));
    ESP_LOGI(TAG, "Command: %s", get_command_name(command_id));

    uint16_t id = 0;
    switch (command_id) {
    case NWK_LEAVE_IND:
      break;
    case ZDO_DEV_UPDATE_IND:
      break;
    case ZDO_DEV_ANNCE_IND:
      break;
    case ZDO_DEV_AUTHORIZED_IND:
      break;
    case APSDE_DATA_IND:

      /*
       * TODO: Implement data requests (see dumps and z2mqtt logic).
       */

      // HACK: Guess button pressed by looking at the endpoint id.
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

void ZBOSSDriver::receive_int(const void* data, size_t size) {
  xStreamBufferSend(m_input_buf, data, size, 0);
}
