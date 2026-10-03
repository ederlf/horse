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
#include "sim/sim.h"
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
    while ((c = getopt (argc, argv, "ht")) != -1){
        switch (c){
            case 'h':{
                display_horse();
                display_help_message();
                break;
            }
            case 't':{ 
                struct topology *topo = from_json(argv[optind]);
                struct sim_config *conf = sim_config_new();
                // struct topology *topo = topology_new();
                start(topo, conf);
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
