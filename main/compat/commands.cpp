/**
 * @file main/compat/commands.cpp
 */
#include "commands.hpp"
#include "enums.hpp"
#include <unordered_map>

static std::unordered_map<zdo::ClusterId, CommandId> ZDO_REQ_CLUSTER_ID_TO_ZBOSS_COMMAND_ID = {
    { zdo::ClusterId::NETWORK_ADDRESS_REQUEST, CommandId::ZDO_NWK_ADDR_REQ },
    { zdo::ClusterId::IEEE_ADDRESS_REQUEST, CommandId::ZDO_IEEE_ADDR_REQ },
    { zdo::ClusterId::POWER_DESCRIPTOR_REQUEST, CommandId::ZDO_POWER_DESC_REQ },
    { zdo::ClusterId::NODE_DESCRIPTOR_REQUEST, CommandId::ZDO_NODE_DESC_REQ },
    { zdo::ClusterId::SIMPLE_DESCRIPTOR_REQUEST, CommandId::ZDO_SIMPLE_DESC_REQ },
    { zdo::ClusterId::ACTIVE_ENDPOINTS_REQUEST, CommandId::ZDO_ACTIVE_EP_REQ },
    { zdo::ClusterId::MATCH_DESCRIPTORS_REQUEST, CommandId::ZDO_MATCH_DESC_REQ },
    { zdo::ClusterId::BIND_REQUEST, CommandId::ZDO_BIND_REQ },
    { zdo::ClusterId::UNBIND_REQUEST, CommandId::ZDO_UNBIND_REQ },
    { zdo::ClusterId::LEAVE_REQUEST, CommandId::ZDO_MGMT_LEAVE_REQ },
    { zdo::ClusterId::PERMIT_JOINING_REQUEST, CommandId::ZDO_PERMIT_JOINING_REQ },
    { zdo::ClusterId::BINDING_TABLE_REQUEST, CommandId::ZDO_MGMT_BIND_REQ },
    { zdo::ClusterId::LQI_TABLE_REQUEST, CommandId::ZDO_MGMT_LQI_REQ },
    { zdo::ClusterId::NWK_UPDATE_REQUEST, CommandId::ZDO_MGMT_NWK_UPDATE_REQ },
};

static std::unordered_map<CommandId, zdo::ClusterId> ZBOSS_COMMAND_ID_TO_ZDO_RSP_CLUSTER_ID = {
    { CommandId::ZDO_NWK_ADDR_REQ, zdo::ClusterId::NETWORK_ADDRESS_RESPONSE },
    { CommandId::ZDO_IEEE_ADDR_REQ, zdo::ClusterId::IEEE_ADDRESS_RESPONSE },
    { CommandId::ZDO_POWER_DESC_REQ, zdo::ClusterId::POWER_DESCRIPTOR_RESPONSE },
    { CommandId::ZDO_NODE_DESC_REQ, zdo::ClusterId::NODE_DESCRIPTOR_RESPONSE },
    { CommandId::ZDO_SIMPLE_DESC_REQ, zdo::ClusterId::SIMPLE_DESCRIPTOR_RESPONSE },
    { CommandId::ZDO_ACTIVE_EP_REQ, zdo::ClusterId::ACTIVE_ENDPOINTS_RESPONSE },
    { CommandId::ZDO_MATCH_DESC_REQ, zdo::ClusterId::MATCH_DESCRIPTORS_RESPONSE },
    { CommandId::ZDO_BIND_REQ, zdo::ClusterId::BIND_RESPONSE },
    { CommandId::ZDO_UNBIND_REQ, zdo::ClusterId::UNBIND_RESPONSE },
    { CommandId::ZDO_MGMT_LEAVE_REQ, zdo::ClusterId::LEAVE_RESPONSE },
    { CommandId::ZDO_PERMIT_JOINING_REQ, zdo::ClusterId::PERMIT_JOINING_RESPONSE },
    { CommandId::ZDO_MGMT_BIND_REQ, zdo::ClusterId::BINDING_TABLE_RESPONSE },
    { CommandId::ZDO_MGMT_LQI_REQ, zdo::ClusterId::LQI_TABLE_RESPONSE },
    { CommandId::ZDO_MGMT_NWK_UPDATE_REQ, zdo::ClusterId::NWK_UPDATE_RESPONSE },
    { CommandId::ZDO_DEV_ANNCE_IND, zdo::ClusterId::END_DEVICE_ANNOUNCE }
};

CommandId get_command_id(zdo::ClusterId id) {
  return ZDO_REQ_CLUSTER_ID_TO_ZBOSS_COMMAND_ID[id];
}

zdo::ClusterId get_cluster_id(CommandId id) {
  return ZBOSS_COMMAND_ID_TO_ZDO_RSP_CLUSTER_ID[id];
}

#define COMMON_RESPONSE \
    { .m_name = "category", .m_data_type = zcl::DataType::UINT8, .m_typed = ParamsDescTypedType::StatusCategory }, \
    { .m_name = "status", .m_data_type = zcl::DataType::UINT8, .m_typed = ParamsDescTypedType::MoreStates }


std::unordered_map<CommandId, ZBOSSFrameDesc>& get_frames() {
  static std::unordered_map<CommandId, ZBOSSFrameDesc> FRAMES = {
    // ------------------------------------------
    // NCP config
    // ------------------------------------------

    // Requests firmware, stack and protocol versions from NCP
    { CommandId::GET_MODULE_VERSION, ZBOSSFrameDesc {
      .m_response = std::vector<ParamsDesc>{
        COMMON_RESPONSE,
        { "fwVersion", zcl::DataType::UINT32 },
        { "stackVersion", zcl::DataType::UINT32 },
        { "protocolVersion", zcl::DataType::UINT32 },
      }
    }},

    // Force NCP module reboot
    { CommandId::NCP_RESET, {
      .m_request = {
        { .m_name = "options", .m_data_type = zcl::DataType::UINT8, .m_typed = ParamsDescTypedType::ResetOptions },
      },
      .m_response = { COMMON_RESPONSE }
    }},

    // Requests current Zigbee role of the local device
    { CommandId::GET_ZIGBEE_ROLE, {
      .m_response = {
        COMMON_RESPONSE,
        { .m_name = "role",
          .m_data_type = zcl::DataType::UINT8,
          .m_typed = ParamsDescTypedType::DeviceType
        }
      },
    } },
    // Set Zigbee role of the local device
    { CommandId::SET_ZIGBEE_ROLE, {
      .m_request = {
        { .m_name = "role",
          .m_data_type = zcl::DataType::UINT8,
          .m_typed = ParamsDescTypedType::DeviceType }
      },
      .m_response = { COMMON_RESPONSE },
    } },
    // Set Zigbee channels page and mask
    { CommandId::SET_ZIGBEE_CHANNEL_MASK, {
      .m_request = {
         { "page", zcl::DataType::UINT8 },
         { "mask", zcl::DataType::UINT32 },
      },
      .m_response = { COMMON_RESPONSE },
    } },
    // Get Zigbee channel
    { CommandId::GET_ZIGBEE_CHANNEL, {
      .m_response = {
        COMMON_RESPONSE,
        { "page", zcl::DataType::UINT8 },
        { "channel", zcl::DataType::UINT8 }
      },
    } },
    // Requests current short PAN ID
    { CommandId::GET_PAN_ID, {
      .m_response = { COMMON_RESPONSE, { "panID", zcl::DataType::UINT16 } },
    } },
    // Set short PAN ID
    { CommandId::SET_PAN_ID, {
      .m_request = { { "panID", zcl::DataType::UINT16 } },
      .m_response = { COMMON_RESPONSE },
    } },
    // Requests local IEEE address
    { CommandId::GET_LOCAL_IEEE_ADDR, {
      .m_request = { { "mac", zcl::DataType::UINT8 } },
      .m_response = {
        COMMON_RESPONSE,
        { "mac", zcl::DataType::UINT8 },
        { "ieee", zcl::DataType::IEEE_ADDR }
      },
    } },
    // Sets Rx On When Idle PIB attribute
    { CommandId::SET_RX_ON_WHEN_IDLE, {
      .m_request = { { "rxOn", zcl::DataType::UINT8 } },
      .m_response = { COMMON_RESPONSE },
    } },
    // Requests current join status of the device
    { CommandId::GET_JOINED, {
      .m_response = {COMMON_RESPONSE, { "joined", zcl::DataType::UINT8 } },
    } },
    // Set NWK Key
    { CommandId::SET_NWK_KEY, {
      .m_request = {
           { "nwkKey", zcl::DataType::SEC_KEY },
           { "index", zcl::DataType::UINT8 },
      },
      .m_response = { COMMON_RESPONSE },
    } },
    // Get Extended Pan ID
    { CommandId::GET_EXTENDED_PAN_ID, {
      .m_response = {COMMON_RESPONSE, { "extendedPanID", BuffaloZBOSSDataType::EXTENDED_PAN_ID } },
    } },
    // Get Coordinator version
    { CommandId::GET_COORDINATOR_VERSION, {
      .m_response = {COMMON_RESPONSE, { "version", zcl::DataType::UINT8 } },
    } },
    // Sets TC Policy
    { CommandId::SET_TC_POLICY, {
      .m_request = {
           { .m_name = "type",
             .m_data_type = zcl::DataType::UINT16,
             .m_typed = ParamsDescTypedType::PolicyType },
           { "value", zcl::DataType::UINT8 },
      },
      .m_response = { COMMON_RESPONSE },
    } },
    // Add or update Simple descriptor for a specified endpoint
    { CommandId::AF_SET_SIMPLE_DESC, {
      .m_request = {
        { "endpoint", zcl::DataType::UINT8 },
        { "profileID", zcl::DataType::UINT16 },
        { "deviceID", zcl::DataType::UINT16 },
        { "version", zcl::DataType::UINT8 },
        { "inputClusterCount", zcl::DataType::UINT8 },
        { "outputClusterCount", zcl::DataType::UINT8 },
        { .m_name = "inputClusters",
          .m_data_type = zcl::BuffaloDataType::LIST_UINT16,
          .m_options = [](Payload* payload, BuffaloZclOptions* options) {
            options->length = std::any_cast<uint8_t>(payload->at("inputClusterCount"));
          },
        },
        { .m_name = "outputClusters",
          .m_data_type = zcl::BuffaloDataType::LIST_UINT16,
          .m_options = [](Payload* payload, BuffaloZclOptions* options) {
            options->length = std::any_cast<uint8_t>(payload->at("outputClusterCount"));
          },
        },
      },
      .m_response = { COMMON_RESPONSE },
    } },

    // ------------------------------------------
    // Application Support Sub-layer
    // ------------------------------------------

    // APSDE-DATA.request
    { CommandId::APSDE_DATA_REQ, {
      .m_request = {
        { "paramLength", zcl::DataType::UINT8 },
        { "dataLength", zcl::DataType::UINT16 },
        { "addr", zcl::DataType::IEEE_ADDR },
        { "profileID", zcl::DataType::UINT16 },
        { "clusterID", zcl::DataType::UINT16 },
        { .m_name = "dstEndpoint", .m_data_type = zcl::DataType::UINT8,
          .m_condition = [](Payload* payload, auto _) {
            uint32_t val = std::any_cast<uint32_t>(payload->at("dstAddrMode"));
            return val == 2 || val == 3;
          }
        },
        //{name: 'dstEndpoint', type: DataType.UINT8},
        { "srcEndpoint", zcl::DataType::UINT8 },
        { "radius", zcl::DataType::UINT8 },
        { "dstAddrMode", zcl::DataType::UINT8 },
        { "txOptions", zcl::DataType::UINT8 },
        { "useAlias", zcl::DataType::UINT8 },
        //{name: 'aliasAddr', type: DataType.UINT16, condition: (payload) => payload.useAlias !== 0},
        { "aliasAddr", zcl::DataType::UINT16 },
        { "aliasSequence", zcl::DataType::UINT8 },
        { .m_name = "data",
          .m_data_type = zcl::BuffaloDataType::LIST_UINT8,
          .m_options = [](Payload* payload, BuffaloZclOptions* options) {
            options->length = std::any_cast<uint32_t>(payload->at("dataLength"));
          },
        },
      },
      .m_response = {
        COMMON_RESPONSE,
        { "ieee", zcl::DataType::IEEE_ADDR },
        { .m_name = "dstEndpoint",
          .m_data_type =zcl::DataType::UINT8,
          .m_condition = [](Payload* payload, auto _) {
            uint32_t val = std::any_cast<uint32_t>(payload->at("dstAddrMode"));
            return val == 2 || val == 3;
          }},
        { "srcEndpoint", zcl::DataType::UINT8 },
        { "txTime", zcl::DataType::UINT32 },
        { "dstAddrMode", zcl::DataType::UINT8 },
      },
    } },

    // ------------------------------------------
    // NWK Management API
    // ------------------------------------------

    // NLME-NETWORK-FORMATION.request
    { CommandId::NWK_FORMATION, {
      .m_request = {
        { "len", zcl::DataType::UINT8 },
        {
          .m_name = "channels",
          .m_data_type = BuffaloZBOSSDataType::LIST_TYPED,
          .m_typed = std::vector<ParamsDesc> {
            { "page", zcl::DataType::UINT8 },
            { "mask", zcl::DataType::UINT32 },
          },
        },
        { "duration", zcl::DataType::UINT8 },
        { "distribFlag", zcl::DataType::UINT8 },
        { "distribNwk", zcl::DataType::UINT16 },
        { "extendedPanID", BuffaloZBOSSDataType::EXTENDED_PAN_ID },
      },
      .m_response = { COMMON_RESPONSE, { "nwk", zcl::DataType::UINT16 } },
    } },
  };

  return FRAMES;
}

#undef COMMON_RESPONSE
