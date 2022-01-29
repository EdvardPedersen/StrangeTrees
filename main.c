#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "main.h"
#include "plot.h"

#define WIDTH 800
#define HEIGHT 800

int search_animation =0;


struct sdl_data {
    SDL_Renderer *renderer;
    SDL_Window *window;
};

struct person {
    float x;
    float y;
    float speed_x;
    float speed_y;
    Uint8 infected;
};


enum MOVE{RIGHT, LEFT, UP, DOWN};

list_node_t *add_person(person_t *person, list_node_t *list);
list_node_t *remove_person(person_t *person, list_node_t *list);
int draw_people_list(list_node_t *list, sdl_data_t *win);
binary_tree_t *find_directional_node(binary_tree_t *my_root, int x, int y, enum MOVE direction);
binary_tree_t *find_node(int choices, binary_tree_t *my_root);
void print_binary(int bin);
int reverse_bits(int target);
int interleave_choices(int x, int y, int choice);


/*
Binary tree structure:
Alternating X and Y values
Value is central point on this depth's axis
**  As of 29.01.2022 there is a bug when selecting  x>= y, giving opposite
    encoding on each half of the tree. EdvardP  assigned.
*/

sdl_data_t *win;

binary_tree_t *create_space_partition(binary_tree_t *parent, int depth, int x1, int x2, int y1, int y2, int val_x, int val_y) {
    binary_tree_t *node = malloc(sizeof(binary_tree_t));
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    if(val_x) {
        node->value_x = val_x;
    } else {
        node->value_x = 1;
    }
    if(val_y) {
        node->value_y = val_y;
    } else {
        node->value_y = 1;
    }
    node->x = x1;
    node->y = y1;
    node->w = x2-x1;
    node->h = y2-y1;

    node->content = NULL;
    if(depth == 0) {
        return node;
    }

    if(node->value_x >= node->value_y) {
        int central = (y2 + y1) / 2;
        node->left = create_space_partition(node, depth - 1, x1, x2, y1, central, node->value_x, node->value_y << 1);
        node->right = create_space_partition(node, depth - 1, x1, x2, central, y2, node->value_x, (node->value_y << 1) | 1);
    } else {
        int central = (x2 + x1) / 2;
        node->left = create_space_partition(node, depth - 1, x1, central, y1, y2, node->value_x << 1, node->value_y);
        node->right = create_space_partition(node, depth - 1, central, x2, y1, y2, (node->value_x << 1) | 1, node->value_y);
    }
    return node;
}


void move_nodes(binary_tree_t *my_root) {
    if(my_root == NULL)
        return;
    if(my_root->content == NULL) {
        move_nodes(my_root->left);
        move_nodes(my_root->right);
    } else {
        list_node_t *list_entry = my_root->content;
        while(list_entry) {
            person_t *my_person = list_entry->value;
            binary_tree_t *move_to = NULL;
            if(my_person->x > my_root->x + my_root->w) {
                move_to = find_directional_node(my_root, my_root->value_x, my_root->value_y, RIGHT);
            } else if (my_person->x < my_root->x) {
                move_to = find_directional_node(my_root, my_root->value_x, my_root->value_y, LEFT);
            } else if (my_person->y > my_root->y + my_root->h) {
                move_to = find_directional_node(my_root, my_root->value_x, my_root->value_y, UP);
            } else if (my_person->y < my_root->y) {
                move_to = find_directional_node(my_root, my_root->value_x, my_root->value_y, DOWN);
            }
            list_entry = list_entry->next;

            if(move_to) {
                my_root->content = remove_person(my_person, my_root->content);
                move_to->content = add_person(my_person, move_to->content);
            }
        }
    }
}

int put_person_in_tree(binary_tree_t *tree, person_t *person) {
    binary_tree_t *current_node = tree;
    while(current_node->left) {
        if(current_node->value_x >= current_node->value_y) {
            if(person->y > (current_node->y +(current_node->h / 2))) {
                current_node = current_node->right;
            } else {
                current_node = current_node->left;
            }
        } else {
            if(person->x > (current_node->x +(current_node->w / 2))) {
                current_node = current_node->right;
            } else {
                current_node = current_node->left;
            }
        }
    }
    current_node->content = add_person(person, current_node->content);
    return 1;
}

int put_people_in_tree(binary_tree_t *tree, list_node_t *people) {
    list_node_t *current = people;
    while(current) {
        put_person_in_tree(tree, current->value);
        current = current->next;
    }
}

binary_tree_t *find_directional_node(binary_tree_t *my_root, int x, int y, enum MOVE direction) {
    int choices = 0;
    int invert = 100;
    switch(direction){
    case RIGHT:
        choices = x;
        invert = 0;
        break;
    case LEFT:
        choices = x;
        invert = 1;
        break;
    case UP:
        choices = y;
        invert = 0;
        break;
    case DOWN:
        choices = y;
        invert = 1;
        break;
    default:
        return NULL;
    }

    int reversed_choices = 1;
    int swapped = 0;
    while(choices > 1) {
        reversed_choices = reversed_choices << 1;
        if(swapped == 0 && ((choices & 1) == invert)) {
            choices &= -2;
            choices |= !invert;
            swapped = 1;
        }
        if(!swapped) {
            reversed_choices |= !((choices & 1));
        } else {
            reversed_choices |= ((choices & 1));
        }
        choices = choices >> 1;
    }

    binary_tree_t *upper_root = my_root->parent;
    while(upper_root->parent) {
        upper_root = upper_root->parent;
    }

    int combined_choices = 0;

    binary_tree_t *node = find_node(interleave_choices(reversed_choices,reverse_bits(y),1),upper_root);
    return find_node(interleave_choices(reversed_choices, reverse_bits(y), 1), upper_root);
}

int reverse_bits(int target) {
    int ret = 1;
    while(target > 1) {
        ret = ret << 1;
        ret |= (target & 1);
        target = target >> 1;
    }
    return ret;
}

int interleave_choices(int x, int y, int choice) {
    if(choice == 0 && !((x >> 1 < 1) && (y >> 1 < 1))) {
        return (interleave_choices(x >> 1, y, 1) << 1) | (x & 1);
    } else if (!((x >> 1 < 1) && (y >> 1 < 1))) {
        return (interleave_choices(x, y >> 1, 0) << 1) | (y & 1);
    } else if ((x >> 1 < 1) && (y >> 1 < 1)) {
        return 1;
    }
}

binary_tree_t *find_node(int choices, binary_tree_t *my_root) {
    
    if(search_animation){
        SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 25);
        SDL_RenderFillRect(win->renderer, NULL);
        SDL_SetRenderDrawColor(win->renderer, 0xff, 0xff, 0xff, 0x10);
        SDL_Rect draw_rect;
        draw_rect.x = my_root->x;
        draw_rect.y = my_root->y;
        draw_rect.w = my_root->w;
        draw_rect.h = my_root->h;
        SDL_RenderDrawRect(win->renderer, &draw_rect);
        SDL_RenderPresent(win->renderer);
        SDL_Delay(200);
    }
    if(choices <= 1) {
        return my_root;
    }
    if(choices & 1) {
        // printf("R");
        find_node(choices >> 1, my_root->right);
    } else {
        // printf("L");
        find_node(choices >> 1, my_root->left);
    }
}

person_t *create_person() {
    person_t *new_person = malloc(sizeof(person_t));
    new_person->x = 10;
    new_person->y = 500;
    new_person->speed_x = ((rand() % 10) - 5) / 10.0;
    new_person->speed_y = ((rand() % 10) - 5) / 10.0;
    new_person->infected = 0;
    return new_person;
}

list_node_t *add_person(person_t *person, list_node_t *list) {
    list_node_t *new_entry = malloc(sizeof(list_node_t));
    new_entry->next = list;
    new_entry->value = person;
    return new_entry;
}

list_node_t *remove_person(person_t *person, list_node_t *list) {
    //printf("REMOVE PERSON %x from %x\n", person, list);
    list_node_t *head = list;
    list_node_t *prev = head;
    list_node_t *current = head->next;
    if(head->value == person) {
        free(head);
        return current;
    }
    while(current) {
        if(current->value == person) {
            prev->next = current->next;
            free(current);
            return head;
        }
        prev = current;
        current = current->next;
    }
}

int draw_people_list(list_node_t *list, sdl_data_t *win) {
    if(list == NULL)
        return 0;
    if(list->value == NULL)
        return 0;
    list_node_t *it = list;
    SDL_SetRenderDrawColor(win->renderer, 0xff, 0xff, 0xff, 0xff);
    while(it) {
        SDL_SetRenderDrawColor(win->renderer, it->value->infected, (255 - it->value->infected), 0xff, 0xff);
        SDL_RenderDrawPoint(win->renderer, (int)it->value->x, (int)it->value->y);
        it = it->next;
    }
    return 1;
}

float distance(person_t *p1, person_t *p2) {
    float dx = p1->x - p2->x;
    float dy = p1->y - p2->y;
    return sqrtf(dx*dx + dy*dy);
}

int update_people_position_list(list_node_t *list) {
    list_node_t *it = list;
    while(it) {
        it->value->x += it->value->speed_x;
        if(it->value->x > WIDTH) {
            it->value->x = 0;
        } else if (it->value->x < 0) {
            it->value->x = WIDTH;
        }
        it->value->y += it->value->speed_y;
        if(it->value->y > HEIGHT) {
            it->value->y = 0;
        } else if (it->value->y < 0) {
            it->value->y = HEIGHT;
        }
        it = it->next;
    }
    return 1;
}

int check_coll(list_node_t *list) {
    int count = 0;
    list_node_t *it = list;
    while(it->next) {
        count++;
        list_node_t *other_it = it->next;
        while(other_it) {
            float dist = distance(it->value, other_it->value);
            if(dist < 2) {
                if (it->value->infected < 255)
                    it->value->infected++;
                if (other_it->value->infected < 255)
                   other_it->value->infected++;
            }
            other_it = other_it->next;
       }
       it = it->next;
    }
    //printf("Checked coll for %i elements\n", count);
}

sdl_data_t *init_sdl(int width, int height) {
    sdl_data_t *ret = malloc(sizeof(sdl_data_t));
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return NULL;

    ret->window = SDL_CreateWindow("Simulation",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   width,
                                   height,
                                   0);
    if (!ret->window)
        return NULL;

    ret->renderer = SDL_CreateRenderer(ret->window,
                                       -1,
                                       0);
    if (!ret->renderer)
        return NULL;

    SDL_SetRenderDrawBlendMode(ret->renderer, SDL_BLENDMODE_BLEND);

    return ret;
}

int check_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT:
            exit(0);
        case SDL_KEYDOWN:
            printf("KEYDOWN\n");
            return 1;
        default:
            break;
        }
    }
    return 0;
}

void print_binary(int bin) {
    int temp = bin;
    int temp1 = bin;
    int temp2 = 0;
    while(temp) {
        temp2 |= temp & 1;
        temp = temp >> 1;
        temp2 = temp2 << 1;
    }
    temp2 = temp2 >> 1;
    while(temp1) {
        printf("%i", temp2 & 1);
        temp2 = temp2 >> 1;
        temp1 = temp1 >> 1;
    }
}

void print_tree(binary_tree_t *tree_to_print) {
    if(!tree_to_print){
        printf("LEAF!!\n");
        return;
    }
    printf("Node (");
    print_binary(tree_to_print->value_x);
    printf(", ");
    print_binary(tree_to_print->value_y);
    printf(") - x: %i, y: %i, width: %i, height %i\n", tree_to_print->x, tree_to_print->y, tree_to_print->w, tree_to_print->h);

    print_tree(tree_to_print->left);
    print_tree(tree_to_print->right);
}

void draw_people_tree(binary_tree_t *my_tree, sdl_data_t *win, int draw_node) {
    binary_tree_t *node = my_tree;
    if(node->left) {
        draw_people_tree(node->left, win, draw_node);
        draw_people_tree(node->right, win, draw_node);
    } 
    if(node->content) {
        if(draw_node) {
            SDL_SetRenderDrawColor(win->renderer, 0xff, 0xff, 0xff, 0x10);
            SDL_Rect draw_rect;
            draw_rect.x = my_tree->x;
            draw_rect.y = my_tree->y;
            draw_rect.w = my_tree->w;
            draw_rect.h = my_tree->h;
            SDL_RenderDrawRect(win->renderer, &draw_rect);
        }
        draw_people_list(node->content, win);
    }
}

void check_coll_tree(binary_tree_t *my_tree) {
    binary_tree_t *node = my_tree;
    if(node->left) {
        check_coll_tree(node->left);
        check_coll_tree(node->right);
    } 
    if(node->content) {
        check_coll(node->content);
    }
    
}

void print_binary2(char*tar, int bin,int bin2) {
    int temp = bin;
    int temp1 = bin;
    int temp2 = 0;
    while(temp) {
        temp2 |= temp & 1;
        temp = temp >> 1;
        temp2 = temp2 << 1;
    }
    temp2 = temp2 >> 1;
    while(temp1) {
        sprintf(tar,"%i", temp2 & 1);
        tar++;
        temp2 = temp2 >> 1;
        temp1 = temp1 >> 1;
    }
    sprintf(tar,"\n");
    tar++;
    temp = bin2;
    temp1 = bin2;
    temp2 = 0;
    while(temp) {
        temp2 |= temp & 1;
        temp = temp >> 1;
        temp2 = temp2 << 1;
    }
    temp2 = temp2 >> 1;
    while(temp1) {
        sprintf(tar,"%i", temp2 & 1);
        tar++;
        temp2 = temp2 >> 1;
        temp1 = temp1 >> 1;
    }

}

char *strnode(char *name,  binary_tree_t*n)
{
    print_binary2(name,n->value_x,n->value_y);

	
    return name;
}


void _tree_print(plot_t *plot, binary_tree_t *current)
{
    char from[500];
    char to[500];
	
    if (current == NULL)
        return;

    strnode(from, current);
    if (current->left != NULL) {
        plot_addlink2(plot, current, current->left, from, strnode(to, current->left));
    } 
	
    if (current->right != NULL) {
        plot_addlink2(plot, current, current->right, from, strnode(to, current->right));
    } 
    
    _tree_print(plot, current->left);
    _tree_print(plot, current->right);
}

void tree_visualize(binary_tree_t *parse)
{
    plot_t *plot;
    
    plot = plot_create("tree");
    _tree_print(plot, parse);
    plot_doplot(plot);
	plot_cleanup(plot);
}


int main(int argc, char * argv[]) {
    int c;
    int visualize = 0;
    if(argc >1){
        while(( c = getopt(argc,argv,"ad")) !=- 1){
            switch (c)
            {
            case 'd':
                visualize = 1;
                break;
            case 'a':
                search_animation = 1;
            default:
                break;
            }
        }
    }

    win = init_sdl(800, 800);
    if(!win) {
        SDL_Log("Big error: %s", SDL_GetError());
        SDL_Quit();
    }

    list_node_t *my_list = malloc(sizeof(list_node_t));

    my_list->next = NULL;
    my_list->value = create_person();

    for(int i = 0; i < 0; i++) {
        person_t *new_person = create_person();
        my_list = add_person(new_person, my_list);
    }

    int generation = 0;

    binary_tree_t *my_tree = create_space_partition(NULL, 10, 0, WIDTH, 0, HEIGHT, 0, 0);
    // print_tree(my_tree); 
    
    if(visualize)
        tree_visualize(my_tree);
    put_people_in_tree(my_tree, my_list);

    clock_t start = clock();
    clock_t diff;

    int type = 1;


    while(1) {
        SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 25);
        SDL_RenderFillRect(win->renderer, NULL);
        if(type == 0) {
            draw_people_list(my_list, win);
            update_people_position_list(my_list);
            check_coll(my_list);
        } else {
            move_nodes(my_tree);
            draw_people_tree(my_tree, win, 1);
            update_people_position_list(my_list);
            check_coll_tree(my_tree);
        }   
        SDL_RenderPresent(win->renderer);
        if(check_events() == 1) {
            type = !type;
        }
        generation++;
        SDL_Delay(10);
        if(generation % 1 == 0) {
            diff = clock() - start;
            float seconds = (float)diff / CLOCKS_PER_SEC;
            //printf("Generation %i complete in %f seconds\n", generation, seconds);
            start = clock();
        }
    }
}
