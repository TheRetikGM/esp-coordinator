#pragma once
#include "commands_list.h"
#include <cstdint>

enum command_id_t : uint16_t {
#define COMMAND(name,val) name = val,
  COMMANDS_LIST
#undef COMMAND
};

const char* get_command_name(unsigned command_id);

