#include "config.h"

#include "io.h"
#include "neighbour_joining.h"
#include "viz.h"
#include "cmdline.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    struct gengetopt_args_info args_info;
    
    /* Parse arguments from the command line. */
    if (cmdline_parser(argc, argv, &args_info) != 0) {
        return EXIT_FAILURE;
    }
    
    if (args_info.inputs_num == 0) {
        fprintf(stderr, "%s: no input files\n", CMDLINE_PARSER_PACKAGE);
        return EXIT_FAILURE;
    }
    
    /* Graphviz output requested but neigh compiled without Graphviz support. */
#if !HAS_GRAPHVIZ
    if (args_info.graphviz_tree_given && args_info.graphviz_tree_arg > graphviz_tree_arg_none) {
        fprintf(stderr, "%s: Graphviz support not available\n", CMDLINE_PARSER_PACKAGE);
    }
#endif
    
    for (unsigned i = 0; i < args_info.inputs_num; i++) {
        const char *input_file = args_info.inputs[i];
        dist_matrix *dmat = load_file(input_file);
        
        if (!dmat) {
            // Error, stop here
            return EXIT_FAILURE;
        }
        
        fprintf(stderr, "Processing file '%s'...\n\n", input_file);
        
        double u[dmat->species_count];
        
        /* Ensure that cluster_name has enough space for the longest possible name */
        char cluster_name[2 + 3 * sizeof(dmat->species_count) + 1];
        uint32_t cluster_id = 1;
        
        btree_node *partial_trees[dmat->species_count];
        btree_storage *tree_storage = nj_tree_init(dmat, partial_trees);
        
        while (dmat->species_count >= 2) {
            if (args_info.print_matrices_flag) {
                dist_matrix_print(dmat);
                printf("\n");
            }
            
            if (args_info.print_trees_arg == print_trees_arg_all) {
                btree_print_trees(partial_trees, dmat->species_count);
                printf("\n");
            }
            
#if HAS_GRAPHVIZ
            if (args_info.graphviz_tree_arg == graphviz_tree_arg_all) {
                viz_visualize_trees(partial_trees, dmat->species_count, "prova.part.pdf", "pdf");
            }
#endif
            
            /* Compute the average distance of each clusters from the others */
            dist_matrix_compute_avg_distances(dmat, u);
            
            /* Find the pair of nearest clusters */
            uint32_t c1, c2;
            nj_find_nearest_clusters(dmat, u, &c1, &c2);
            
            /* Generate a name for the new cluster */
            unsigned long result = snprintf(cluster_name, sizeof(cluster_name), "C_%" PRIu32, cluster_id);
            assert(result > 0 && result < sizeof(cluster_name));
            
            if (args_info.verbose_flag) {
                fprintf(stderr, "Joining clusters '%s' and '%s' in '%s'.\n\n", dmat->species_names[c1], dmat->species_names[c2], cluster_name);
            }

            /* Add a node for the new cluster to the array of partial trees */
            nj_tree_add_node(dmat, u, tree_storage, partial_trees, cluster_name, c1, c2);
            
            /* Create a new dist_matrix joining the specified clusters */
            dist_matrix *joined = nj_join_clusters(dmat, cluster_name, c1, c2);
            
            if (joined == NULL) {
                /* Error, stop here */
                break;
            }
            
            /* Release the old distance matrix */
            dist_matrix_free(dmat);
            dmat = joined;
            
            cluster_id++;
        }
        
        btree_node *phyl_tree = partial_trees[0];
        
        if (args_info.print_trees_arg >= print_trees_arg_final) {
            btree_print_tree(phyl_tree);
            printf("\n\n");
        }
        
#if HAS_GRAPHVIZ
         if (args_info.graphviz_tree_arg >= graphviz_tree_arg_final) {
             viz_visualize_tree(phyl_tree, "prova.pdf", "pdf");
         }
#endif
        
        dist_matrix_free(dmat);
        btree_storage_free(tree_storage);
    }
    
    return EXIT_SUCCESS;
}
