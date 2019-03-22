
// Copyright (C) 2018, Jaguar Land Rover
// This program is licensed under the terms and conditions of the
// Mozilla Public License, version 2.0.  The full text of the
// Mozilla Public License is at https://www.mozilla.org/MPL/2.0/
//
// Author: Magnus Feuer (mfeuer1@jaguarlandrover.com)
//
// Event publishing
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vehicle_signal_distribution.h"
#include "dstc.h"

void usage(char* prog)
{
        fprintf(stderr, "Usage: %s -d <signal_desc.csv> -p <signal->path> -s:<signal-path:value> ...\n", prog);
        fprintf(stderr, "  -d <signal_desc.csv>     Signal description from github.com/GENIVI/vehicle_signal_manager\n");
        fprintf(stderr, "  -p <signal->path>        The signal tree to publish\n");
        fprintf(stderr, "  <signal-path:value> ...  Signal-value pair to set before publish\n\n");
        fprintf(stderr, "Example: %s -d vss_rel_2.0.0-alpha+005.csv \\\n", prog);
        fprintf(stderr, "         %*c -p Vehicle.Drivetrain.InternalCombustionEngine \\\n",
                (int) strlen(prog), ' ');
        fprintf(stderr, "         %*c -s Vehicle.Drivetrain.InternalCombustionEngine.Engine.Power:230 \\\n",
                (int) strlen(prog), ' ');
        fprintf(stderr, "         %*c -s Vehicle.Drivetrain.InternalCombustionEngine.FuelType:gasoline\n",
                (int) strlen(prog), ' ');

        fprintf(stderr, "\nSignal descriptor CSV file is installed in the share directory under the\n");
        fprintf(stderr, "top-level installation directory.\n");
        exit(255);

}

int main(int argc, char* argv[])
{
    vsd_desc_t* desc;
    vsd_context_t* ctx = 0;
    int res;
    char opt = 0;
    char sig[1024];
    char *val;

    if (argc == 1)
        usage(argv[0]);

    vsd_context_create(&ctx);
    // First grab the -s arg so that we can load a file
    while ((opt = getopt(argc, argv, "s:p:d:")) != -1) {
        switch (opt) {
        case 'd':
            res = vsd_load_from_file(ctx, optarg);
            if (res) {
                fprintf(stderr, "Could not load %s: %s\n", optarg, strerror(res));
                exit(255);
            }
            break;

        default: /* '?' */
            break;
        }
    }

    if (!ctx) {
        fprintf(stderr, "Please specify -s <signal_desc.csv>\n");
        exit(255);
    }

    optind = 1;
    while ((opt = getopt(argc, argv, "s:p:d:")) != -1) {
        switch (opt) {

        case 's':
            strcpy(sig, optarg);
            val = strchr(sig, ':');
            if (!val) {
                fprintf(stderr, "Missing colon. Please signal values to set as -s <path>:<value>\n");
                exit(255);
            }
            *val++ = 0;

            res = vsd_set_value_by_path_convert(ctx, sig, val);
            if (res) {
                fprintf(stderr, "Could not set %s to %s: %s\n", sig, val, strerror(res));
                exit(255);
            }
            break;

        case 'p':
            fprintf(stderr, "Publishing %s\n", optarg);
            res = vsd_find_desc_by_path(ctx, 0, optarg, &desc);
            if (res) {
                fprintf(stderr, "Could not use publish path %s: %s\n", optarg, strerror(res));
                exit(255);
            }
            break;

        default: /* '?' */
            break;
        }
    }

    // Did we specify the signal tree to publish.
    if (!desc) {
        fprintf(stderr, "Please specify the signal subtree to publish using -p <signal->path>\n");
        exit(255);
    }

    // Ensure that the pub-sub relationships have been setup
    dstc_process_events(400000);

    // Send publish command to update
    res = vsd_publish(desc);

    if (res) {
        printf("Cannot publish signal %s %s\n", argv[2], strerror(res));
        exit(255);
    }

    // Process signals for another 100 msec to ensure that the call gets out.
    dstc_process_events(100000);
}
