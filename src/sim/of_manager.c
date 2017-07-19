#include "of_manager.h"
#include "lib/util.h"
#include "lib/openflow.h"
#include <loci/loci.h>


struct of_manager *of_manager_new(struct scheduler *sch)
{
    struct of_manager *om = xmalloc(sizeof(struct of_manager));
    om->of = of_client_new(0);
    om->of->owner = om;
    om->of->message_callback = of_manager_message_cb;
    om->sch = sch;
    return om;
}

void of_manager_send(struct of_manager *om, uint64_t dpid, 
                     uint8_t *buf, size_t len)
{
    /* Retrieve the respective dpid connection to send */
    struct of_conn *conn;
    HASH_FIND(hh, om->of->active_conns, &dpid, sizeof(uint64_t), conn);
    if (conn != NULL) {
        of_conn_send(conn, buf, len);
    }
}

void of_manager_message_cb(struct of_conn* conn, uint8_t type, 
                             void *data, size_t len)
{
    /* Operate directly in the target dp node */
    struct of_client *of = (struct of_client*) conn->conn->owner;
    struct of_manager *om = (struct of_manager*) of->owner;  
    
    uint64_t time = om->sch->clock;
    uint64_t dp_id = conn->id;

    struct sim_event_of *msg = sim_event_of_msg_in_new(time, dp_id, 
                                                       data, len);

    printf("Recebi Mensagem ahoi\n");
    /* need to protect with mutex */
    pthread_mutex_lock(&om->sch->sch_mutex);
    scheduler_insert(om->sch, (struct sim_event*) msg);
    pthread_mutex_unlock(&om->sch->sch_mutex);
    printf("Inseri Mensagem ahoi\n");
    /* We do not need the type but it is here 
     * because libfluid callback needs it */
    UNUSED(type);
    // struct datapath *dp = topology_datapath_by_dpid(om->topo, conn->id);
    // if (type == OFPT_FLOW_MOD) {
    //      Goes to unpack
    //     // of_message_t msg = OF_BUFFER_TO_MESSAGE(data);
    //     // of_object_t *ofo = of_object_new_from_message(msg, len);
        
    //     printf("%p\n", ofo);
    //     /* Handle flow mod   
        
    //     // Needs to protect the scheduler with a mutex? 
    // }
    // else if (type == OFPT_PACKET_OUT){
    
    //     /* Handle packet out */
    // }
}