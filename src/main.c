#include "io.h"
#include "neighbour_joining.h"

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    char *files[] = {
            "examples/hello_world.in",
            "examples/evolution.in",
            "examples/wiki.in"
    };

    int n = sizeof(files) / sizeof(*files);

    for (int i = 0; i < n; i++) {
        dist_matrix *dmat = load_file(files[i]);

        if (!dmat) {
            continue;
        }

        dist_matrix_print(dmat);
        printf("\n");

        /* Ensure that cluster_name has enough space for the longest possible name */
        char cluster_name[2 + 3 * sizeof(dmat->species_count)];
        uint32_t cluster_id = 1;

        while (dmat->species_count >= 2) {
            unsigned long result = snprintf(cluster_name, sizeof(cluster_name), "C_%" PRIu32, cluster_id);
            assert(result > 0 && result < sizeof(cluster_name));
            
            uint32_t c1, c2;
            dist_matrix *joined = nj_join_nearest_clusters(dmat, cluster_name, &c1, &c2);

            printf("Joining clusters '%s' and '%s' in '%s'.\n\n", dmat->species_names[c1], dmat->species_names[c2], cluster_name);
            
            dist_matrix_free(dmat);
            dmat = joined;

            dist_matrix_print(dmat);
            printf("\n\n");

            cluster_id++;
        }

        dist_matrix_free(dmat);
    }

    return 0;
}
