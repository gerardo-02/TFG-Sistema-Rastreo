#ifndef LISTS_H
#define	LISTS_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */
    #define LIST_ORDER_AT_FIRST     0
    #define LIST_ORDER_AT_THE_END   -1
    
    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */
    /* ************************************************************************** */
    typedef struct _t_LIST_NODE
    {
        struct _t_LIST_NODE *nodeSig;
        uint16_t dataAttrib;
        uint16_t dataLength;
        uint8_t *data;
    }t_LIST_NODE;
    
    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    /**
     *
     * @param data:         contenido de data en el nodo 
     * @param dataLength:   n bytes que contendra data en el interior del nodo
     * @return              ptr al nodo creado
     */
    t_LIST_NODE *list_new_node (uint8_t *data, uint32_t dataLength);
    /**
     * 
     * @param list:      lista en donde se introduce el nodo
     * @param node:      nodo a introducir
     * @param order:     NO IMPLEMENTDO!!!
     */
    void list_push_node(t_LIST_NODE **list, t_LIST_NODE *node, uint32_t order);
    /**
     * 
     * @param list:      lista de donde se extrae el nodo
     * @param order:     NO IMPLEMENTADO!!!
     * @return          nodo extraido
     */
    t_LIST_NODE *list_pull_node(t_LIST_NODE **list, uint32_t order);
    /**
     * 
     * @param node:      nodo que se va ha liberar
     */
    void list_delete_node(t_LIST_NODE **node);
    void list_delete (t_LIST_NODE **list);

#ifdef	__cplusplus
}
#endif

#endif	/* LISTS_H */

/* ************************************************************** End of File */
