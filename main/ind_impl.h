#pragma once
#include "zb_ncp.h"
#include <functional>

template<command_id_t CmdId, typename... TArgs>
struct default_unhandled_ind {
  using CallbackFunc = std::function<void(TArgs...)>;

  inline static CallbackFunc m_callback {
    [](TArgs...){
      ESP_LOGD("ind_impl", "Unhandled indication: %s", get_command_name(CmdId));
    }
  };

  static void connect(const CallbackFunc &callback) { m_callback = callback; }
};

template <command_id_t CmdId>
struct zb_ncp::ind_handle { };

#define DECLARE_IND_HANDLE(cmd_id, ...) \
  template<> \
  struct zb_ncp::ind_handle<cmd_id> \
    : public default_unhandled_ind<cmd_id, __VA_ARGS__> {}

DECLARE_IND_HANDLE(APSDE_DATA_IND, const zb_apsde_data_indication_t&);
DECLARE_IND_HANDLE(ZDO_DEV_ANNCE_IND, const zb_zdo_signal_device_annce_params_t&);
DECLARE_IND_HANDLE(NWK_LEAVE_IND, const zb_zdo_signal_leave_indication_params_t&);
DECLARE_IND_HANDLE(ZDO_DEV_UPDATE_IND, const zb_zdo_signal_device_update_params_t&);
DECLARE_IND_HANDLE(ZDO_DEV_AUTHORIZED_IND, const zb_zdo_signal_device_authorized_params_t&);

#undef DECLARE_IND_HANDLE
