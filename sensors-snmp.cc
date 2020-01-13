#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <sensors/sensors.h>
#include <signal.h>
#include "temp-sensors-table.h"

extern "C" void init_sensorsPlugin(void) {
    init_lmTempSensorsTable();
}

extern "C" void deinit_sensorsPlugin(void) {
    deinit_lmTempSensorsTable();
}