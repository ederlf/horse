#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void 
display_help_message(void)
{
    printf("horse: SDN simulator\n");
    printf("Usage: horse [OPTIONS]\n");
    printf("\n");
    printf("  -h             display help message.\n");
    printf("\n");
}

static void 
display_horse(void)
{
    printf("     >>\\.\n");
    printf("    /_  )`.\n");
    printf("   /  _)`^)`.   _.---. _\n");
    printf("  (_,' \\  `^-)""      `.\\\n");
    printf("        |              | \\\n");
    printf("        \\              / |\n");
    printf("       / \\  /.___.'\\  (\\ (_\n");
    printf("      < ,\"||     \\ |`. \\`-'\n");
    printf("       \\\\ ()      )|  )/\n");
    printf("        |_>|>     /_] //\n");
    printf("         /_]  hjw    /_]\n");
}

int 
main(int argc, char *argv[]){
    int c;
    display_horse();
    while ((c = getopt (argc, argv, "h")) != -1){
        switch (c){
            case 'h':{
                display_help_message();
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
