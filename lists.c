#include <stddef.h>
#include <string.h>

#include "drv/drv_dmem.h"
#include "log.h"

#include "lists.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
t_LIST_NODE *list_new_node (uint8_t *data, uint32_t dataLength)
{
  t_LIST_NODE *n = NULL;

  if ((dataLength > 0) && data)
    {
      n = (t_LIST_NODE*) dmem_create (sizeof (t_LIST_NODE));
      if (n)
        {
          n->nodeSig = NULL;
          n->dataLength = dataLength;
          n->data = (uint8_t*) dmem_create_w_data (data, dataLength);
          if (n->data == NULL)
            {
              dmem_release (n);
              n = NULL;
            }
        }
    }
  else
    {
      _d ("LIST-NODE: data or data length is null\n");
    }

  return n;
}

void list_push_node (t_LIST_NODE **list, t_LIST_NODE *node, uint32_t order)
{
  t_LIST_NODE *aux;
  if (*list)
    {
      aux = *list;
      while (aux->nodeSig) aux = aux->nodeSig;
      aux->nodeSig = node;
    }
  else
    {
      *list = node;
    }
}

t_LIST_NODE *list_pull_node (t_LIST_NODE **list, uint32_t order)
{
  t_LIST_NODE *aux = NULL;
  if (*list)
    {
      aux = *list;
      *list = (*list)->nodeSig;
    }
  return aux;
}

void list_delete_node (t_LIST_NODE **node)
{
  if (*node)
    {
      if ((*node)->data)
        {
          dmem_release ((*node)->data);
          (*node)->data = NULL;
        }
      dmem_release (*node);
      *node = NULL;
    }
}

void list_delete (t_LIST_NODE **list)
{
  t_LIST_NODE *nodeAux;

  while (*list)
    {
      nodeAux = list_pull_node (list, 0);
      list_delete_node (&nodeAux);
    }
}

/* ************************************************************** End of File */
