#include <iostream>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <sensors/sensors.h>
#include "temp-sensors-table.h"

static const size_t kMaxSensorNameLength = 32;

void init_lmTempSensorsTable(void) { initialize_table_lmTempSensorsTable(); }

struct lmTempSensorsTable_entry {
    long lmTempSensorsIndex;
    char lmTempSensorsDevice[kMaxSensorNameLength];
    size_t lmTempSensorsDevice_len;
    u_long lmTempSensorsValue;
    struct lmTempSensorsTable_entry *next;
};

struct lmTempSensorsTable_entry *lmTempSensorsTable_head;

struct lmTempSensorsTable_entry *
lmTempSensorsTable_createEntry(long lmTempSensorsIndex) {
    struct lmTempSensorsTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(struct lmTempSensorsTable_entry);
    if (!entry)
        return NULL;

    entry->lmTempSensorsIndex = lmTempSensorsIndex;
    entry->next = lmTempSensorsTable_head;
    lmTempSensorsTable_head = entry;
    return entry;
}

void initialize_table_lmTempSensorsTable(void) {
    const oid lmTempSensorsTable_oid[] = {1, 3, 6, 1, 4, 1, 2021, 13, 16, 2};
    const size_t lmTempSensorsTable_oid_len =
        OID_LENGTH(lmTempSensorsTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    DEBUGMSGTL(
        ("lmTempSensorsTable:init", "initializing table lmTempSensorsTable\n"));

    reg = netsnmp_create_handler_registration(
        "lmTempSensorsTable", lmTempSensorsTable_handler,
        lmTempSensorsTable_oid, lmTempSensorsTable_oid_len, HANDLER_CAN_RONLY);

    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(
        table_info, 
        ASN_INTEGER,
        0);
    
    table_info->min_column = COLUMN_LMTEMPSENSORSINDEX;
    table_info->max_column = COLUMN_LMTEMPSENSORSVALUE;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = lmTempSensorsTable_get_first_data_point;
    iinfo->get_next_data_point = lmTempSensorsTable_get_next_data_point;

    iinfo->table_reginfo = table_info;

    netsnmp_register_table_iterator(reg, iinfo);
}

void lmTempSensorsTable_removeAllEntries() {
    while (lmTempSensorsTable_head) {
        auto next = lmTempSensorsTable_head->next;
        SNMP_FREE(lmTempSensorsTable_head);
        lmTempSensorsTable_head = next;
    }
}

void lmTempSensorsTable_addEntries() {
    sensors_chip_name const *cn;
    int c = 0;
    long index = 0;

    while ((cn = sensors_get_detected_chips(0, &c)) != 0) {
        if (cn->bus.type != SENSORS_BUS_TYPE_ISA) {
            continue;
        }

        sensors_feature const *feature;
        int f = 0;

        while ((feature = sensors_get_features(cn, &f)) != 0) {
            std::cout << sensors_get_label(cn, feature) << ": ";

            sensors_subfeature const *subfeature;
            int s = 0;

            while ((subfeature =
                        sensors_get_all_subfeatures(cn, feature, &s)) != 0) {
                if (subfeature->flags & SENSORS_MODE_R) {
                    if (SENSORS_SUBFEATURE_TEMP_INPUT == subfeature->type) {
                        double value;
                        if (0 ==
                            sensors_get_value(cn, subfeature->number, &value)) {
                            auto entry = lmTempSensorsTable_createEntry(index++);
                            strncpy(entry->lmTempSensorsDevice,
                                    sensors_get_label(cn, feature),
                                    kMaxSensorNameLength);
                            entry->lmTempSensorsDevice_len =
                                strlen(entry->lmTempSensorsDevice);
                            entry->lmTempSensorsValue = value;
                        }
                    }
                }
            }
        }
    }
}

netsnmp_variable_list *lmTempSensorsTable_get_first_data_point(
    void **my_loop_context, void **my_data_context,
    netsnmp_variable_list *put_index_data, netsnmp_iterator_info *mydata) {

    lmTempSensorsTable_removeAllEntries();
    lmTempSensorsTable_addEntries();

    *my_loop_context = lmTempSensorsTable_head;
    return lmTempSensorsTable_get_next_data_point(
        my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list *lmTempSensorsTable_get_next_data_point(
    void **my_loop_context, void **my_data_context,
    netsnmp_variable_list *put_index_data, netsnmp_iterator_info *mydata) {
    struct lmTempSensorsTable_entry *entry =
        (struct lmTempSensorsTable_entry *)*my_loop_context;

    if (entry) {
        snmp_set_var_typed_integer(put_index_data, ASN_INTEGER,
                                   entry->lmTempSensorsIndex);
        *my_data_context = (void *)entry;
        *my_loop_context = (void *)entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

int lmTempSensorsTable_handler(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests) {
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    struct lmTempSensorsTable_entry *table_entry;

    DEBUGMSGTL(("lmTempSensorsTable:handler", "Processing request (%d)\n",
                reqinfo->mode));

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (struct lmTempSensorsTable_entry *)
                netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_LMTEMPSENSORSINDEX:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           table_entry->lmTempSensorsIndex);
                break;
            case COLUMN_LMTEMPSENSORSDEVICE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                                         table_entry->lmTempSensorsDevice,
                                         table_entry->lmTempSensorsDevice_len);
                break;
            case COLUMN_LMTEMPSENSORSVALUE:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request,
                                              SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_GAUGE,
                                           table_entry->lmTempSensorsValue);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;
    }
    return SNMP_ERR_NOERROR;
}
