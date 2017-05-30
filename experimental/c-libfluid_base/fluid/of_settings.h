#ifndef OF_SETTINGS_H
#define OF_SETTINGS_H 1

#include <stdint.h>
#include "fluid/base/evloop.h"

struct of_settings {
        uint32_t supported_versions;
        uint8_t max_supported_version;
        bool version_set_by_hand;
        int echo_interval;
        bool liveness_check;
        bool handshake;
        bool dispatch_all_messages;
        bool use_hello_elements;
        bool keep_data_ownership;
        bool is_controller;
        uint64_t datapath_id;
        uint32_t n_buffers;
        uint8_t n_tables;
        uint8_t auxiliary_id;
        uint32_t capabilities;
        struct evloop *evloop;
};

struct of_settings *of_settings_new(); 

/**
    Return true if instance represents an OpenFlow controller, false for switch.
*/
bool is_controller(struct of_settings *os);

#endif