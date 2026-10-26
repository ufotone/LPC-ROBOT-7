// Based on KORG SDK template. Copyright (c) 2020-2022 KORG Inc.
#include "unit.h"
const __unit_header unit_header_t unit_header = {
  .header_size=sizeof(unit_header_t),
  .target=UNIT_TARGET_PLATFORM | k_unit_module_synth,
  .api=UNIT_API_VERSION,
  .dev_id=0x5534544eU, // Private, unregistered development ID (U4TN).
  .unit_id=0x4c504337U,
  .version=0x00001100U,
  .name="LPC ROBOT 7",
  .num_presets=0,
  .num_params=11,
  .params={
    {0,57,0,0,k_unit_param_type_strings,0,0,0,{"WORD"}},
    {0,75,24,24,k_unit_param_type_strings,0,0,0,{"PITCH"}},
    {25,251,100,100,k_unit_param_type_strings,0,0,0,{"SPEED"}},
    {-1,101,-1,-1,k_unit_param_type_strings,0,0,0,{"INDEX"}},
    {0,100,0,80,k_unit_param_type_percent,0,0,0,{"LEVEL"}},
    {0,100,0,0,k_unit_param_type_percent,0,0,0,{"PAN DELAY"}},
    {40,750,250,250,k_unit_param_type_msec,0,0,0,{"DLY TIME"}},
    {0,75,0,35,k_unit_param_type_percent,0,0,0,{"DLY FB"}},
    {0,1,0,0,k_unit_param_type_onoff,0,0,0,{"DLY SYNC"}},
    {0,9,0,5,k_unit_param_type_strings,0,0,0,{"DLY DIV"}},
    {20,250,80,80,k_unit_param_type_msec,0,0,0,{"STTR TIME"}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
    {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
  }
};
