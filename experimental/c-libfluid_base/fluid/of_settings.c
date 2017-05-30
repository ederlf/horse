#include "of_settings.h"
#include <stdlib.h>

static void add_version(struct of_settings *os, const uint8_t version)
{
    os->supported_versions |= (1 << version);

    unsigned int x = 0;
    os->max_supported_version = 0;
    for (x = (unsigned int) os->supported_versions;
         x > 0;
         x = x >> 1, os->max_supported_version++);
    os->max_supported_version--;
}

struct of_settings *of_settings_new()
{
    struct of_settings *os = malloc(sizeof(struct of_settings));
    os->supported_versions = 0;
    add_version(os, 1);
    os->version_set_by_hand = false;
    os->echo_interval = 15;
    os->liveness_check = true;
    os->handshake = true;
    os->dispatch_all_messages = false;
    os->use_hello_elements = false;
    os->keep_data_ownership = true;
    os->is_controller = true;
    os->datapath_id = 0;
    os->auxiliary_id = 0;
    os->n_buffers = 0;
    os->n_tables = 0;
    os->capabilities = 0;
    os->evloop = NULL;
} 

void supported_version(struct of_settings *os, const uint8_t version) {
    // If the user sets the version by hand, then all supported versions must
    // be explicitly declared.
    if (!os->version_set_by_hand) {
        os->version_set_by_hand = true;
        os->supported_versions = 0;
    }
    add_version(os, version);
}

bool is_controller(struct of_settings *os)
{
    return os->is_controller;
}
