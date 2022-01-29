/* Authors: Aage Kvalnes <aage@cs.uit.no> 
            Øyvind Nohr <oyvind.a.nohr@uit.no> */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "plot.h"

struct plot {
    FILE *f;
    char filename[128];
};


plot_t *plot_create(char *name)
{
    plot_t *plot;
    char cmd[256];

    plot = malloc(sizeof(plot_t));
    if (plot == NULL)
        goto error;
    
    sprintf(plot->filename, "%s", name);
    plot->f = fopen(plot->filename, "w");
    if (plot->f == NULL)
        goto error;

    fprintf(plot->f, "digraph structs {\n");
    fprintf(plot->f, "rankdir=TB;\n");
    fprintf(plot->f, "node [shape=circle];\n");
    fprintf(plot->f, "size=\"7.5,10\";\n");
    fflush(plot->f);
    
    return plot;
    
 error:
    if (plot != NULL) {
        if (plot->f != NULL)
            fclose(plot->f);
        sprintf(cmd, "rm -f %s", plot->filename);
        system(cmd);
        fclose(plot->f);
        free(plot);
    }
        
    return NULL;
}

void plot_addlink2(plot_t *plot, void *from, void *to, char *from_label, char *to_label)
{
	fprintf(plot->f, "%ld [label=\"%s\"];\n", (long)from, from_label);
    fprintf(plot->f, "%ld [label=\"%s\"];\n", (long)to, to_label);
    fprintf(plot->f, "%ld->%ld;\n", (long)from, (long)to);
    fflush(plot->f);
}

void plot_cleanup(plot_t *plot)
{
    fclose(plot->f);
    free(plot);
}

void plot_doplot(plot_t *plot)
{
    char cmd[512];

    fprintf(plot->f,"}\n");
    fflush(plot->f);

    sprintf(cmd, " dot -T pdf %s -o %s.pdf", plot->filename, plot->filename);
    system(cmd);

}