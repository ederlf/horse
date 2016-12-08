#include <inttypes.h>
#include <stddef.h>

// Max size of keyword
#define MAX_KEY_LEN 10

#define NO_KEY "NoKey"
#define DPS_KEY "dps"
#define LINKS_KEY "links"
#define SWITCHX "switchX"
#define SWITCHY "switchY"
#define PORTX "portX" 
#define PORTY "portY"
#define DELAY "delay"
#define BW    "bw"

struct link_spec {
    uint64_t switchX;
    uint64_t switchY;
    uint32_t portX;
    uint32_t portY;
    uint32_t delay;
    uint32_t bw;
};

struct parsed_topology {
    uint64_t dps[10000];
    size_t ndps;
    struct link_spec links[10000];
    size_t nlinks;
};

enum json_error {
    INVALID_JSON = 1,
    INVALID_KEYWORD = 2,
    INVALID_LINK_DP = 3
};

void parse_topology(char *json, size_t s, struct parsed_topology *ptopo);