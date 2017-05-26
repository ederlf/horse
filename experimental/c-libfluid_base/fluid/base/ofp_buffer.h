#ifndef OFP_BUFFER
#define OFP_BUFFER 1

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define OF_HEADER_LENGTH 8

struct ofp_buffer {
    uint8_t* data;
    int init;

    uint8_t header[OF_HEADER_LENGTH];
    uint16_t header_pos;

    uint16_t pos;
    uint16_t len;
};

static inline uint16_t get_read_len(struct ofp_buffer *ofb) {
    if (ofb->init)
        return ofb->len - ofb->pos;
    else
        return OF_HEADER_LENGTH - ofb->header_pos;
}

static inline uint8_t* get_read_pos(struct ofp_buffer *ofb) {
    if (ofb->init)
        return ofb->data + ofb->pos;
    else
        return ofb->header + ofb->header_pos;
}

static inline void read_notify(struct ofp_buffer *ofb, uint16_t read) {
    if (ofb->init)
        ofb->pos += read;
    else {
        ofb->header_pos += read;
        if (ofb->header_pos == OF_HEADER_LENGTH) {
            ofb->len = htons(*((uint16_t*) ofb->header + 1));
            ofb->data = malloc(ofb->len);
            memcpy(ofb->data, ofb->header, OF_HEADER_LENGTH);
            ofb->pos += OF_HEADER_LENGTH;
            ofb->init = 1;
        }
    }
}

static inline int is_ready(struct ofp_buffer *ofb) {
    return (ofb->len != 0) && (ofb->pos == ofb->len);
}

/* Could use data directly but will keep it now 
   so porting is straight forward.
*/
static inline void* get_data(struct ofp_buffer *ofb) {
    return ofb->data;
}

static inline int get_len(struct ofp_buffer *ofb) {
        return ofb->len;
}

static inline void clear(struct ofp_buffer *ofb, int delete_data) {
    if (delete_data && ofb->data != NULL) {
        free(ofb->data);
    }
    ofb->data = NULL;
    ofb->init = 0;
    memset(ofb->header, 0, OF_HEADER_LENGTH);
    ofb->header_pos = 0;
    ofb->pos = 0;
    ofb->len = 0;
}

static inline struct ofp_buffer *ofp_buffer_new()
{   
    struct ofp_buffer *ofb = malloc(sizeof(struct ofp_buffer));
    clear(ofb, 0);
    return ofb;
}

static inline void ofp_buffer_destroy(struct ofp_buffer *ofb)
{
    if (ofb->data != NULL)
        free(ofb->data);
}

#endif