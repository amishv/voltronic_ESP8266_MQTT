// Copyright (c) 2022 amish
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <strings.h>
#include <time.h>

#include "solarmon.h"


static const char *ChargeSource[] = {
    "Solar first",
    "Solar & Utility",
    "Only solar"
};
static const char *OutputPriority[] = {
    "S-U-B", //Solar-Utility-Battery",
    "S-B-U"  //Solar-Battery-Utility"
};
#if 0  //for future use
static const char *BatteryType[] = {
    "AGM",
    "Flooded",
    "User"
};
static const char *OutputSetting[] = {
    "  Single",
    "Parallel",
    " Phase 1",
    " Phase 2",
    " Phase 3"
};
#endif //for future use
int ratedInformation(void)
{
  int value_AC_input_rating_voltage_, value_AC_input_rating_current_, value_AC_output_rating_voltage_,
    value_AC_output_rating_frequency_, value_AC_output_rating_current_, 
    value_AC_output_rating_apparent_power_, value_AC_output_rating_active_power_, 
    value_Battery_rating_voltage_, value_Battery_recharge_voltage_, value_Battery_redischarge_voltage_,
    value_Battery_under_voltage_, value_Battery_bulk_voltage_, value_Battery_float_voltage_, 
    value_Battery_type_, value_Max_AC_charging_current_, value_Max_charging_current_, 
    value_Input_voltage_range_, value_Output_source_priority_, value_Charger_source_priority_, 
    value_Parallel_max_num_, value_Machine_type_, value_Topology_, value_Output_model_setting_, 
    value_Solar_power_priority_, value_MPPT_string_;
  
  char tmpBuf[255];  //sometimes the device sends long junk causing the system to crash
  if (sendCommand("^P007PIRI")!=TRUE)
      return FALSE;
  if (readport((uint8_t *)tmpBuf)!=TRUE)
      return FALSE;
  debugPort.printf("%s\n", tmpBuf);
  if (!strstr( tmpBuf, "^D088"))
      return FALSE;

  sscanf( tmpBuf,                                                                               // NOLINT
      "^D088%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", 
      &value_AC_input_rating_voltage_, &value_AC_input_rating_current_, &value_AC_output_rating_voltage_,
      &value_AC_output_rating_frequency_, &value_AC_output_rating_current_, 
      &value_AC_output_rating_apparent_power_, &value_AC_output_rating_active_power_, 
      &value_Battery_rating_voltage_, &value_Battery_recharge_voltage_, &value_Battery_redischarge_voltage_,
      &value_Battery_under_voltage_, &value_Battery_bulk_voltage_, &value_Battery_float_voltage_, 
      &value_Battery_type_, &value_Max_AC_charging_current_, &value_Max_charging_current_, 
      &value_Input_voltage_range_, &value_Output_source_priority_, &value_Charger_source_priority_, 
      &value_Parallel_max_num_, &value_Machine_type_, &value_Topology_, &value_Output_model_setting_, 
      &value_Solar_power_priority_, &value_MPPT_string_);

  sendMQTTmessage( "Out_source_priority", OutputPriority[value_Output_source_priority_]);
  sendMQTTmessage("Charger_source_priority", ChargeSource[value_Charger_source_priority_]);

  return TRUE;
}