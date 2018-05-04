#include "conn_manager.h"
#include "lib/util.h"
#include "lib/openflow.h"

static void conn_manager_of_message_cb(struct of_conn* conn, uint8_t type,
                                       void *data, size_t len);

struct conn_manager *conn_manager_new(struct scheduler *sch)
{
    struct conn_manager *cm = xmalloc(sizeof(struct conn_manager));
    cm->of = of_client_new(0);
    cm->of->owner = cm;
    cm->of->message_callback = conn_manager_of_message_cb;
    cm->sch = sch;
    /* Create for now, decide later if creation will move */
    cm->srv = server_new("172.20.254.254", 6000);
    return cm;
}

void 
conn_manager_destroy(struct conn_manager *cm)
{
    of_client_stop(cm->of);
    of_client_destroy(cm->of);
    server_destroy(cm->srv);
    free(cm);
}

void 
conn_manager_send_of(struct conn_manager *cm, uint64_t dpid,
                     uint8_t *buf, size_t len)
{
    /* Retrieve the respective dpid connection to send */
    struct of_conn *conn;
    HASH_FIND(hh, cm->of->active_conns, &dpid, sizeof(uint64_t), conn);
    if (conn != NULL) {
        of_conn_send(conn, buf, len);
    }
    free(buf);
}

static void 
conn_manager_of_message_cb(struct of_conn* conn, uint8_t type,
                      void *data, size_t len)
{
    /* Operate directly in the target dp node */
    struct of_client *of = (struct of_client*) conn->conn->owner;
    struct conn_manager *cm = (struct conn_manager*) of->owner;  
    uint64_t time = cm->sch->clock;
    uint64_t dp_id = conn->id;

    /* PERF: Need to copy the data because libfluid 
        frees it after the callback returns */
    uint8_t *copy_data = xmalloc(len);
    memcpy(copy_data, data, len);
    struct sim_event_of *msg = sim_event_of_msg_in_new(time, dp_id, 
                                                       copy_data, len);
    scheduler_insert(cm->sch, (struct sim_event*) msg);
    /* We do not need the type but it is here 
     * because libfluid callback needs it */
    UNUSED(type);
}