


typedef struct list_node list_node_t;
typedef struct sdl_data sdl_data_t;
typedef struct person person_t;
typedef struct binary_tree binary_tree_t;

struct list_node {
    list_node_t *next;
    person_t *value;
};

struct binary_tree {
    binary_tree_t *parent;
    binary_tree_t *left;
    binary_tree_t *right;
    int value_x;
    int value_y;
    int x, y, w, h;
    list_node_t *content;
};