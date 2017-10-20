#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H 1

#include <inttypes.h>

enum sim_mode {
    SIM_CTRL = 0,
    EMU_CTRL = 1,
};

struct sim_config;

struct sim_config *sim_config_new(void);
void sim_config_set_mode(struct sim_config *conf, enum sim_mode mode);
void sim_config_set_end_time(struct sim_config *conf, uint64_t time);
void sim_config_set_ctrl_idle_interval(struct sim_config *conf, 
                                       uint64_t interval);
int sim_config_get_mode(struct sim_config *conf);
uint64_t sim_config_get_end_time(struct sim_config *conf);
uint64_t sim_config_get_ctrl_idle_interval(struct sim_config *conf);

#endif