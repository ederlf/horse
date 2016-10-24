/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE', which is part of this source code package.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "net/datapath.h"
#include "net/topology.h"
#include "net/flow_table.h"
#include <uthash/utlist.h>
static void 
display_help_message(void)
{
    printf("horse: SDN simulator\n");
    printf("Usage: horse [OPTIONS]\n");
    printf("\n");
    printf(" -h             display help message.\n");
    printf("\n");
}

static void 
display_horse(void)
{
    printf("     >>\\.\n");
    printf("    /_  )`.\n");
    printf("   /  _)`^)`.   _.---. _hjw\n");
    printf("  (_,' \\  `^-)""      `.\\\n");
    printf("        |              | \\\n");
    printf("        \\              / |\n");
    printf("       / \\  /.___.'\\  (\\ (_\n");
    printf("      < ,\"||     \\ |`. \\`-'\n");
    printf("       \\\\ ()      )|  )/\n");
    printf("        |_>|>     /_] //\n");
    printf("         /_]         /_]\n");
}

int 
main(int argc, char *argv[]){
    int c;
    while ((c = getopt (argc, argv, "ha")) != -1){
        switch (c){
            case 'h':{
                display_horse();
                display_help_message();
                break;
            }
            case 'a':{
                struct flow_table *ft = flow_table_new();
                struct flow *fl = new_flow();
                set_ip_proto(fl, 7);
                set_eth_type(fl, 0x800);
                fl->priority = 1;
                fl->action = 1;
                add_flow(ft, fl);
                struct flow *fl2 = new_flow();
                set_ipv4_dst(fl2, 21);
                set_eth_type(fl2, 0x800);
                set_ip_proto(fl2, 7);
                add_flow(ft, fl2);
                fl2->priority = 10;
                fl2->action = 2;
                add_flow(ft, fl2);
                struct flow *fl3 = new_flow();
                set_ip_proto(fl3, 7);
                set_eth_type(fl3, 0x806);
                fl3->action = 3;
                add_flow(ft, fl3);
                struct flow *fl4 = new_flow();
                set_ipv4_dst(fl4, 21);
                set_ip_proto(fl4, 7);
                set_eth_type(fl4, 0x800);
                // modify_flow(ft, fl3, false);
                // delete_flow(ft, fl3, true);
                struct mini_flow_table *elt;
                int count;
                DL_COUNT(ft->flows, elt, count);
                printf("%d\n", count); 
                struct flow *ret = flow_table_lookup(ft, fl4);
                if (ret){
                    printf("Action %d\n", ret->action);
                }
                // DL_FOREACH(ft->flows, elt) {
                //     struct flow *hash = elt->flows;
                //     struct flow *f = new_flow();
                //     set_eth_type(f, 0x800);
                //     set_ip_proto(f, 7);
                //     struct flow* ret;
                //     HASH_FIND(hh, hash, &f->key, sizeof(struct flow_key), ret);
                //     if (ret){
                //         printf("Modify first %d!\n", ret->action);
                //     }                                
                //     struct flow *f2 = new_flow();
                //     // set_ip_proto(f2, 17);
                //     set_ipv4_dst(f2, 21);
                //     set_eth_type(f2, 0x800);
                //     set_ip_proto(f2, 7);
                //     HASH_FIND(hh, hash, &f2->key, sizeof(struct flow_key), ret);
                //     if (ret){
                //         printf("Mod second %d!\n", ret->action);
                //     }
                //     unsigned int num_flows;
                //     num_flows = HASH_COUNT(hash);
                //     printf("%u\n", num_flows);
                // }               
                // struct topology *topo = new_topology();
                // struct datapath *dp =  dp_new(0x000000000001);
                // add_switch(topo, dp);
                // destroy_topology(topo);
                break;
            }
            case '?':{
                if (isprint (optopt)){
                  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                }
                else{
                  fprintf (stderr,
                           "Unknown option character `\\x%x'.\n",
                           optopt);
                }
                return 1;
            }
            default:{
                display_help_message();
                exit(EXIT_FAILURE);
            }
        }

    }
    return EXIT_SUCCESS;
}
