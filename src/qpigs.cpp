#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <strings.h>
#include <time.h>

#include "solarmon.h"

const char *mpptChargerStatus[] = {
    "Abnormal",
    "Not Charging",
    "Charging"};

const char *PowerDirection[] = {
    "None",
    "Charging",
    "Discharging"};
const char *LineDirection[] = {
    "None  ---",
    "Input <<<",
    "Output >>>"};

int generalStatusDisplay(void)
{
  int value_grid_voltage_, value_grid_frequency_, value_ac_output_voltage_, 
      value_ac_output_frequency_, value_ac_output_apparent_power_,          
      value_ac_output_active_power_, value_output_load_percent_,            
      value_battery_voltage_, value_battery_voltage_scc1_,
      value_battery_voltage_scc2_, value_battery_discharge_current_,
      valuebattery_charging_current_, value_battery_capacity_percent_,
      value_inverter_heat_sink_temperature_, value_mppt1_charger_temperature_,
      value_mppt2_charger_temperature_, value_pv1_input_power_,
      value_pv2_input_power_, value_pv1_input_voltage_, value_pv2_input_voltage_,
      value_setting_value_configuration_state_, value_mppt1_charger_status_,
      mppt2_charger_status_, value_load_connection_, value_battery_power_direction_,
      value_dc_ac_power_direction_, value_line_power_direction_, value_local_parallel_id_;
  char tmpBuf[254]; //sometimes the device sends long junk causing the system to crash
  debugPort.println("sending ^P005GS");
  if (sendCommand("^P005GS") != TRUE)
    return FALSE;
  debugPort.println("sent ^P005GS");
  if (readport((uint8_t *)tmpBuf) != TRUE)
    return FALSE;
  debugPort.printf("%s\n", tmpBuf);
  if (!strstr(tmpBuf, "^D106"))
    return FALSE;

  sscanf(tmpBuf,                                                                                     // NOLINT
         "^D106%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", // NOLINT
         &value_grid_voltage_, &value_grid_frequency_, &value_ac_output_voltage_,                    // NOLINT
         &value_ac_output_frequency_, &value_ac_output_apparent_power_,                              // NOLINT
         &value_ac_output_active_power_, &value_output_load_percent_,                                // NOLINT
         &value_battery_voltage_, &value_battery_voltage_scc1_, &value_battery_voltage_scc2_,        // NOLINT
         &value_battery_discharge_current_, &valuebattery_charging_current_,                         // NOLINT
         &value_battery_capacity_percent_, &value_inverter_heat_sink_temperature_,                   // NOLINT
         &value_mppt1_charger_temperature_, &value_mppt2_charger_temperature_,                       // NOLINT
         &value_pv1_input_power_, &value_pv2_input_power_, &value_pv1_input_voltage_,                // NOLINT
         &value_pv2_input_voltage_, &value_setting_value_configuration_state_,                       // NOLINT
         &value_mppt1_charger_status_, &mppt2_charger_status_, &value_load_connection_,              // NOLINT
         &value_battery_power_direction_, &value_dc_ac_power_direction_,
         &value_line_power_direction_, &value_local_parallel_id_);
  //convert to ascii and send MQTT messages
  sprintf(tmpBuf, "%5.1f", (float)value_grid_voltage_ / 10);
  sendMQTTmessage("AC_grid_voltage", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_grid_frequency_ / 10);
  sendMQTTmessage("AC_grid_frequency", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_ac_output_voltage_ / 10);
  sendMQTTmessage("AC_out_voltage", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_ac_output_frequency_ / 10);
  sendMQTTmessage("AC_out_frequency", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_pv1_input_voltage_ / 10);
  sendMQTTmessage("PV_in_voltage", tmpBuf);
  sprintf(tmpBuf, "%d", value_pv1_input_power_);
  sendMQTTmessage("PV_in_power", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_battery_voltage_scc1_ / 10);
  sendMQTTmessage("SCC_voltage", tmpBuf);
  sprintf(tmpBuf, "%d", value_output_load_percent_);
  sendMQTTmessage("Load_pct", tmpBuf);
  sprintf(tmpBuf, "%d", value_ac_output_active_power_);
  sendMQTTmessage("Load_watt", tmpBuf);
  sprintf(tmpBuf, "%d", value_ac_output_apparent_power_);
  sendMQTTmessage("Load_va", tmpBuf);
  sprintf(tmpBuf, "%d", value_inverter_heat_sink_temperature_);
  sendMQTTmessage("Heatsink_temperature", tmpBuf);
  sprintf(tmpBuf, "%d", value_mppt1_charger_temperature_);
  sendMQTTmessage("MPPT_temperature", tmpBuf);
  sprintf(tmpBuf, "%d", value_battery_capacity_percent_);
  sendMQTTmessage("Battery_capacity", tmpBuf);
  sprintf(tmpBuf, "%5.1f", (float)value_battery_voltage_ / 10);
  sendMQTTmessage("Battery_voltage", tmpBuf);
  sprintf(tmpBuf, "%d", valuebattery_charging_current_);
  sendMQTTmessage("Battery_charge_current", tmpBuf);
  sprintf(tmpBuf, "%d", value_battery_discharge_current_);
  sendMQTTmessage("Battery_discharge_current", tmpBuf);
  sendMQTTmessage("Load_status_on", (value_load_connection_)?"Connected":"Disconnected");
  sendMQTTmessage("SCC_charge_on", mpptChargerStatus[value_mppt1_charger_status_]);
  sendMQTTmessage("AC_Power_dir", LineDirection[value_line_power_direction_]);
  sendMQTTmessage("DC_AC_dir", PowerDirection[value_dc_ac_power_direction_]);
  return true;
}
