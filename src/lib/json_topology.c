#include "json_topology.h"
#include "util.h"
#include <json/json.h>
#include <string.h>

struct json_error_info {
    enum json_error type;
    size_t line;
    size_t row;
    char key[10];
};

static int 
cmpfunc(const void * a, const void * b){
   return ( *(uint64_t*) a - *(uint64_t*) b );
}

static void 
json_set_error_info(struct json_error_info *info, enum json_error err, size_t line, size_t row, char key[10]){
    info->type = err;
    info->line = line;
    info->row = row;
    memcpy(info->key, key, 10);
}

static void 
json_error(char* json, struct json_error_info err){
    switch (err.type){
        case INVALID_JSON:{
            fprintf(stderr, "JSON format error before line:%lu column:%lu.\n", err.line, err.row);
            break;
        }
        case INVALID_KEYWORD: {
            fprintf(stderr, "Keyword \"%s\" on line:%lu column:%lu is not allowed.\n", err.key, err.line, err.row);
            break;
        }
        case INVALID_LINK_DP: {
            fprintf(stderr, "Datapath \"%s\" on line:%lu column:%lu does not exist.\n", err.key, err.line, err.row);
            break;
        }
        default: {
            fprintf(stderr, "Unknown error in the json file.\n %s\n", json);
            break;
        }
    }
    free(json);
    exit(EXIT_FAILURE);
}

static void 
validate_keyword(char* expected_key, struct json_string_ex_s* key, char *json, struct json_error_info err_info){
    if (strcmp(expected_key, (char*) key->string.string) != 0){
        json_set_error_info(&err_info, INVALID_KEYWORD, key->line_no, key->row_no, (char*) key->string.string);
        json_error(json, err_info);
    }
}

static int 
validate_link_dp(uint64_t dpid, struct parsed_topology ptopo){
    uint64_t *res = bsearch (&dpid, ptopo.dps, ptopo.ndps, sizeof (uint64_t),cmpfunc);
    if (res == NULL){
        return -1;
    }
    else {
        return 0;
    }
}

void 
parse_topology(char *json, size_t s, struct parsed_topology *ptopo){
    struct json_parse_result_s result;
    struct json_value_s* root = json_parse_ex(json, s, json_parse_flags_allow_location_information, NULL, NULL, &result);
    struct json_object_s* obj;
    struct json_object_element_s* elements;
    struct json_string_ex_s* key;
    struct json_array_s* array;
    struct json_array_element_s* arr_elem;
    struct json_error_info err_info;
    size_t i;
    if (root == NULL) {
        json_set_error_info(&err_info, INVALID_JSON, result.error_line_no, result.error_row_no, NO_KEY);
        json_error(json, err_info);
    }
    // Parse datapaths
    obj = (struct json_object_s*)root->payload;
    elements = obj->start;
    // Validate dps key
    key = (struct json_string_ex_s*) elements->name;
    validate_keyword(DPS_KEY, key, json, err_info);
    // Get Values 
    array = (struct json_array_s*) elements->value->payload;
    arr_elem = array->start;
    // store dpids in array
    for(i = 0; i < array->length; ++i){
        struct json_number_s* dpid = arr_elem->value->payload;
        sscanf((char*) dpid->number, "%"PRIx64"", &ptopo->dps[i]);
        arr_elem = arr_elem->next; 
    }
    // Sorts the array so we can use bsearch
    qsort (ptopo->dps, array->length, sizeof(uint64_t), cmpfunc);
    ptopo->ndps = i;
    // Parse links
    elements = elements->next;
    if (elements != NULL){
        // Validate links key
        key = (struct json_string_ex_s*) elements->name;
        validate_keyword(LINKS_KEY, key, json, err_info);
        array = (struct json_array_s*) elements->value->payload;
        arr_elem = array->start;
        for(i = 0; i < array->length; ++i){
            struct json_number_s* value;
            struct json_object_s* links = (struct json_object_s*) arr_elem->value->payload;
            struct json_object_element_s* link_elem = links->start;
            // Validate SwitchX
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(SWITCHX, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx64"", &ptopo->links[i].switchX);
            if (validate_link_dp(ptopo->links[i].switchX, *ptopo) < 0) {
                struct json_value_ex_s *v = (struct json_value_ex_s *) link_elem->value;
                json_set_error_info(&err_info, INVALID_LINK_DP, v->line_no, v->row_no, (char*) value->number);
                json_error(json, err_info);
            }
            // Validate SwitchY
            link_elem = link_elem->next; 
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(SWITCHY, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx64"", &ptopo->links[i].switchY);
            if (validate_link_dp(ptopo->links[i].switchY, *ptopo) < 0) {
                struct json_value_ex_s *v = (struct json_value_ex_s *) link_elem->value;
                json_set_error_info(&err_info, INVALID_LINK_DP, v->line_no, v->row_no, (char*) value->number);
                json_error(json, err_info);
            }
            // Validate portX
            link_elem = link_elem->next;
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(PORTX, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx32"", &ptopo->links[i].portX);
            // Validate portY
            link_elem = link_elem->next;
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(PORTY, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx32"", &ptopo->links[i].portY);
            // Validate delay
            link_elem = link_elem->next;
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(DELAY, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx32"", &ptopo->links[i].delay);
            // Validate bw
            link_elem = link_elem->next;
            key = (struct json_string_ex_s*) link_elem->name;
            validate_keyword(BW, key, json, err_info);
            value = link_elem->value->payload;
            sscanf((char*) value->number, "%"PRIx32"", &ptopo->links[i].bw);
            arr_elem = arr_elem->next;
        }
        ptopo->nlinks = i;
    }
    free(root);
    free(json); 
}