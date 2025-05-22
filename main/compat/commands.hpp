/**
 * @file main/compat/ZBOSSFrame.hpp
 */
#pragma once
#include "buffalo/zboss_buffalo_zcl.hpp"
#include "enums.hpp"
#include "zspec/zcl/definition/enums.hpp"
#include "zspec/zdo/definition/clusters.hpp"
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

enum class ParamsDescTypedType {
  StatusCategory,
  MoreStates, // [StatusCodeGeneric, StatusCodeAPS, StatusCodeCBKE]
  ResetOptions,
  DeviceType,
  PolicyType,
};

using OptFunc = std::function<void(Payload*, BuffaloZclOptions*)>;

struct ParamsDesc {
  std::string m_name;
  std::variant<zcl::DataType, zcl::BuffaloDataType, BuffaloZBOSSDataType> m_data_type;

  std::optional<std::function<bool(Payload*, ZBOSSBuffaloZcl*)>> m_condition{};
  std::optional<std::any> m_typed{};
  std::optional<OptFunc> m_options{};
};

struct ZBOSSFrameDesc {
  std::vector<ParamsDesc> m_request{ };
  std::vector<ParamsDesc> m_response{ };
  std::vector<ParamsDesc> m_indication{ };
};

std::unordered_map<CommandId, ZBOSSFrameDesc>& get_frames();

CommandId get_command_id(zdo::ClusterId id);
zdo::ClusterId get_cluster_id(CommandId id);
