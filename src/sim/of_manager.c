#include "of_manager.h"
#include "lib/util.h"
#include "lib/openflow.h"
#include <loci/loci.h>


struct of_manager *of_manager_new(void)
{
    struct of_manager *om = xmalloc(sizeof(struct of_manager));
    /* TODO: The addition of a connection needs to be independent */
    struct of_settings *ofsc = of_settings_new();
    ofsc_supported_version(ofsc, 0x04);
    ofsc->is_controller = false;
    om->of = of_client_new(0, "127.0.0.1", 6653, 1, ofsc);
    om->of->owner = om;
    om->of->message_callback = of_manager_message_cb;
    return om;
}

void of_manager_send(struct of_manager *om, struct sim_event *ev)
{
    /* Need to pick the right connection to send */
    
    UNUSED(om);
    UNUSED(ev);
}

void of_manager_message_cb(struct of_conn* conn, uint8_t type, 
                             void *data, size_t len)
{
    /* Need of_manager to add event to scheduler queue */
    struct of_client *of = (struct of_client*) conn->conn->owner;
    struct of_manager *om = (struct of_manager*) of->owner;  
    if (type == OFPT_FLOW_MOD) {
        of_message_t msg = OF_BUFFER_TO_MESSAGE(data);
        of_object_t *ofo = of_object_new_from_message(msg, len);
        printf("%p\n", ofo);
        /* Handle flow mod */  
        // Convert to event and add to scheduler. 
        // Needs to protect the scheduler with a mutex 
    }
    else if (type == OFPT_PACKET_OUT){
    
        /* Handle packet out */
    }
    printf("%p %p\n", of, om);
    UNUSED(data);
    UNUSED(len);
}