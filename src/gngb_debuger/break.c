#include <glib.h>
#include "break.h"
#include "../cpu.h"

#define BREAK_BP 0
#define BREAK_INT 1

typedef struct {
  char type;
  union {
    UINT16 add;
    UINT16 int_type;
  }data;
}BREAK;

GSList *break_list=NULL;

BREAK *create_break(char type) {
  BREAK *b=(BREAK *)malloc(sizeof(BREAK));
  if (!b) return NULL;
  b->type=type;
  return b;
}

void add_break(BREAK *b) {
  break_list=g_slist_prepend(break_list,(gpointer)b);
}

void remove_break(BREAK *b) {
  break_list=g_slist_remove(break_list,(gpointer)b);
}

// Classic Break Point

BREAK *find_break_point(UINT16 add) {
  GSList *node;
  for(node=break_list;node;node=node->next)
    if ((((BREAK *)node->data)->type==BREAK_BP) && 
	(((BREAK *)node->data)->data.add==add)) return (BREAK *)node->data;
  return NULL;
}

void add_break_point(UINT16 add) {
  BREAK *b=create_break(BREAK_BP);
  b->data.add=add;
  add_break(b);
}

void del_break_point(UINT16 add) {
  BREAK *b=find_break_point(add);
  if (b) remove_break(b);
}

char is_break_point(UINT16 add) {
  return (find_break_point(add)?TRUE:FALSE);
}

void add_break_int(UINT8 int_type) {
  //break_list=g_slist_add(break_list,GINT_TO_POINTER((int)int_type));
}

char test_all_break(void) {
  GSList *node;
  BREAK *b;
  for(node=break_list;node;node=node->next) {
    b=(BREAK *)node->data;
    switch(b->type) {
    case BREAK_BP:
      if (gbcpu->pc.w==b->data.add) return 1;
      break;
    }
  }
  return 0;
}

