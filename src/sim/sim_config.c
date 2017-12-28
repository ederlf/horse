#include "sim_config.h"
#include "lib/util.h"
#include "log/log.h"

struct sim_config {
    enum sim_mode mode;
    uint64_t end_time;
    uint64_t ctrl_idle_interval;
};

struct sim_config *sim_config_new(void)
{
    struct sim_config *conf = xmalloc(sizeof(struct sim_config));
    /* Default values */
    conf->mode = EMU_CTRL;
    conf->end_time = UINT64_MAX;
    conf->ctrl_idle_interval = 100000; // in microseconds  
    return conf;
}

void 
sim_config_set_mode(struct sim_config *conf, enum sim_mode mode)
{
    conf->mode = mode;
}

void 
sim_config_set_end_time(struct sim_config *conf, uint64_t end_time)
{
    conf->end_time = end_time;
}

void 
sim_config_set_ctrl_idle_interval(struct sim_config *conf, uint64_t interval)
{
    conf->ctrl_idle_interval =  interval;
}

void sim_config_set_log_level(int level)
{
    log_set_level(level);
}

int 
sim_config_get_mode(struct sim_config *conf)
{
    return conf->mode;
}

uint64_t 
sim_config_get_end_time(struct sim_config *conf)
{
    return conf->end_time;
}

uint64_t sim_config_get_ctrl_idle_interval(struct sim_config *conf)
{
    return conf->ctrl_idle_interval;
}


