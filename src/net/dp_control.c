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

#include "dp_control.h"
#include <loci/loci.h>

/* Dispatches control messages to appropriate handler functions. */
of_object_t *
dp_control_handle_control_msg(struct datapath *dp, uint8_t *msg,
                              struct netflow *nf, size_t len, uint64_t time) {
    
    of_object_t *obj = of_object_new_from_message(msg, len);
    of_object_t* ret = NULL;
    if (obj != NULL) {
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
                ret = dp_handle_aggregate_stats_req(dp, obj, time);
                break;
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
    }
    return ret;
}