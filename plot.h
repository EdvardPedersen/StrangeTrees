/* Authors:  Aage Kvalnes <aage@cs.uit.no> 
            Øyvind Nohr <oyvind.a.nohr@uit.no> */
#ifndef PLOT_H
#define PLOT_H

struct plot;
typedef struct plot plot_t;

/*  Creates a dotplot for visualisation of abstract data types
    Circular nodes and arrows between makes it ideal for trees */
plot_t *plot_create(char *name);

/*  Expanded linking for visualisation of trees */
void plot_addlink2(plot_t *plot, void *from, void *to, char *from_label, char *to_label);

/*   Create .dot file, using graphviz to create pdfs from the dotplot */
void plot_doplot(plot_t *plot);

/*   Frees memory and closes filepointers used for creating plots */
void plot_cleanup(plot_t *plot);

#endif  /* PLOT_H */