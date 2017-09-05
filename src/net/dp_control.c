/* Copyright (c) 2011, TrafficLab, Ericsson Research, Hungary
 * Copyright (c) 2012, CPqD, Brazil 
 * Copyright (c) 2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Ericsson Research nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

// #include "compiler.h"
// #include "dp_capabilities.h"

// #include "dp_actions.h"
// #include "dp_buffers.h"
// #include "dp_ports.h"
// #include "group_table.h"
// #include "meter_table.h"
// #include "packets.h"
// #include "pipeline.h"
// #include "oflib/ofl.h"
// #include "oflib/ofl-messages.h"
// #include "oflib/ofl-log.h"
// #include "openflow/openflow.h"

#include "dp_control.h"
#include <loci/loci.h>


// /* Handles features request messages. */
// static ofl_err
// handle_control_features_request(struct datapath *dp,
//           struct ofl_msg_header *msg, const struct sender *sender) {

//     struct ofl_msg_features_reply reply =
//             {{.type = OFPT_FEATURES_REPLY},
//              .datapath_id  = dp->id,
//              .n_buffers    = dp_buffers_size(dp->buffers),
//              .n_tables     = PIPELINE_TABLES,
//              .auxiliary_id = sender->conn_id,
//              .capabilities = DP_SUPPORTED_CAPABILITIES,
//              .reserved = 0x00000000};

//     dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

//     ofl_msg_free(msg, dp->exp);

//     return 0;
// }


// /* Handles get config request messages. */
// static ofl_err
// handle_control_get_config_request(struct datapath *dp,
//         struct ofl_msg_header *msg, const struct sender *sender) {

//     struct ofl_msg_get_config_reply reply =
//             {{.type = OFPT_GET_CONFIG_REPLY},
//              .config = &dp->config};
//     dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

//     ofl_msg_free(msg, dp->exp);
//     return 0;
// }

// /* Handles set config request messages. */
// static ofl_err
// handle_control_set_config(struct datapath *dp, struct ofl_msg_set_config *msg,
//                                                 const struct sender *sender UNUSED) {
//     uint16_t flags;

//     flags = msg->config->flags & OFPC_FRAG_MASK;
//     if ((flags & OFPC_FRAG_MASK) != OFPC_FRAG_NORMAL
//         && (flags & OFPC_FRAG_MASK) != OFPC_FRAG_DROP) {
//         flags = (flags & ~OFPC_FRAG_MASK) | OFPC_FRAG_DROP;
//     }

//     dp->config.flags = flags;
//     dp->config.miss_send_len = msg->config->miss_send_len;

//     ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
//     return 0;
// }

// /* Handles desc stats request messages. */
// static ofl_err
// handle_control_stats_request_desc(struct datapath *dp,
//                                   struct ofl_msg_multipart_request_header *msg,
//                                   const struct sender *sender) {
//     struct ofl_msg_reply_desc reply =
//             {{{.type = OFPT_MULTIPART_REPLY},
//               .type = OFPMP_DESC, .flags = 0x0000},
//               .mfr_desc   = dp->mfr_desc,
//               .hw_desc    = dp->hw_desc,
//               .sw_desc    = dp->sw_desc,
//               .serial_num = dp->serial_num,
//               .dp_desc    = dp->dp_desc};
//     dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

//     ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
//     return 0;
// }

// /* Dispatches statistic request messages to the appropriate handler functions. */
// static ofl_err
// handle_control_stats_request(struct datapath *dp,
//                                   struct ofl_msg_multipart_request_header *msg,
//                                                 const struct sender *sender) {
//     switch (msg->type) {
//         case (OFPMP_DESC): {
//             return handle_control_stats_request_desc(dp, msg, sender);
//         }
//         case (OFPMP_FLOW): {
//             return pipeline_handle_stats_request_flow(dp->pipeline, (struct ofl_msg_multipart_request_flow *)msg, sender);
//         }
//         case (OFPMP_AGGREGATE): {
//             return pipeline_handle_stats_request_aggregate(dp->pipeline, (struct ofl_msg_multipart_request_flow *)msg, sender);
//         }
//         case (OFPMP_TABLE): {
//             return pipeline_handle_stats_request_table(dp->pipeline, msg, sender);
//         }
//         case (OFPMP_TABLE_FEATURES):{
//             return pipeline_handle_stats_request_table_features_request(dp->pipeline, msg, sender);
//         }
//         case (OFPMP_PORT_STATS): {
//             return dp_ports_handle_stats_request_port(dp, (struct ofl_msg_multipart_request_port *)msg, sender);
//         }
//         case (OFPMP_QUEUE): {
//             return dp_ports_handle_stats_request_queue(dp, (struct ofl_msg_multipart_request_queue *)msg, sender);
//         }
//         case (OFPMP_GROUP): {
//             return group_table_handle_stats_request_group(dp->groups, (struct ofl_msg_multipart_request_group *)msg, sender);
//         }
//         case (OFPMP_GROUP_DESC): {
//             return group_table_handle_stats_request_group_desc(dp->groups, msg, sender);
//         }
//         case (OFPMP_GROUP_FEATURES):{
//             return group_table_handle_stats_request_group_features(dp->groups, msg, sender);            
//         }       
//         case (OFPMP_METER):{
//             return meter_table_handle_stats_request_meter(dp->meters,(struct ofl_msg_multipart_meter_request*)msg, sender);
//         }
//         case (OFPMP_METER_CONFIG):{
//             return meter_table_handle_stats_request_meter_conf(dp->meters,(struct ofl_msg_multipart_meter_request*)msg, sender);        
//         }
//         case OFPMP_METER_FEATURES:{
//             return meter_table_handle_features_request(dp->meters, msg, sender);
//         }
//         case OFPMP_PORT_DESC:{
//             return dp_ports_handle_port_desc_request(dp, msg, sender);        
//         }
//         case (OFPMP_EXPERIMENTER): {
//             return dp_exp_stats(dp, (struct ofl_msg_multipart_request_experimenter *)msg, sender);
//         }
//         default: {
//             return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_MULTIPART);
//         }
//     }
// }

/* Dispatches control messages to appropriate handler functions. */
of_object_t *
dp_control_handle_control_msg(struct datapath *dp, uint8_t *msg,
                              struct netflow *nf, size_t len, uint64_t time) {
    
    of_object_t *obj = of_object_new_from_message(msg, len);
    of_object_t* ret = NULL;
    switch (obj->object_id) {
        case OF_PACKET_OUT: {
            ret = dp_handle_pkt_out(dp, obj, nf, time);
            break;
        }
        case OF_FLOW_ADD: 
        case OF_FLOW_MODIFY:
        case OF_FLOW_MODIFY_STRICT: 
        case OF_FLOW_DELETE:
        case OF_FLOW_DELETE_STRICT: { 
            ret = dp_handle_flow_mod(dp, obj, time);
            break;
        }
        case OF_PORT_STATS_REQUEST: {
            ret = dp_handle_port_stats_req(dp, obj);
            break;
        }
        case OF_FLOW_STATS_REQUEST: {
            ret = dp_handle_flow_stats_req(dp, obj, time);
            break;
        }
        case OF_AGGREGATE_STATS_REQUEST: {

        }
        case OF_FEATURES_REQUEST: {

        }
        case OF_PORT_MOD: {

        }
        case OF_DESC_STATS_REQUEST: {

        }
        case OF_PORT_DESC_STATS_REQUEST: {
            ret = dp_handle_port_desc(dp, obj);
            break;
        }
        default: {
            break; /*ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);*/
        }
    }
    of_object_delete(obj);
    return ret;
}