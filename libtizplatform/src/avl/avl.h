/*
 * Copyright (C) 1995 by Sam Rushing <rushing@nightmare.com>
 * Copyright (C) 2005 by Germanischer Lloyd AG
 * Copyright (C) 2001-2005 by IronPort Systems, Inc.
 */

/* $Id: avl.h,v 2.10 2005/06/02 01:06:28 rushing Exp $ */

#ifndef __AVL_H
#define __AVL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct avl_node_tag {
  void *                key;
  struct avl_node_tag * left;
  struct avl_node_tag * right;
  struct avl_node_tag * parent;
  /*
   * The lower 2 bits of <rank_and_balance> specify the balance
   * factor: 00==-1, 01==0, 10==+1.
   * The rest of the bits are used for <rank>
   */
  unsigned int          rank_and_balance;
} avl_node;

#define AVL_GET_BALANCE(n)      ((int)(((n)->rank_and_balance & 3) - 1))

#define AVL_GET_RANK(n) (((n)->rank_and_balance >> 2))

#define AVL_SET_BALANCE(n,b) \
  ((n)->rank_and_balance) = \
    (((n)->rank_and_balance & (~3)) | ((int)((b) + 1)))

#define AVL_SET_RANK(n,r) \
  ((n)->rank_and_balance) = \
    (((n)->rank_and_balance & 3) | ((r) << 2))

struct _avl_tree;

typedef int (*avl_key_compare_fun_type) (void * compare_arg, void * a, void * b);
typedef int (*avl_iter_fun_type)        (void * key, void * iter_arg);
typedef int (*avl_iter_index_fun_type)  (unsigned long index, void * key, void * iter_arg);
typedef int (*avl_free_key_fun_type)    (void * key);
typedef int (*avl_key_printer_fun_type) (char *, void *);

/*
 * <compare_fun> and <compare_arg> let us associate a particular compare
 * function with each tree, separately.
 */

typedef struct _avl_tree {
  avl_node *                    root;
  unsigned int                  height;
  unsigned int                  length;
  avl_key_compare_fun_type      compare_fun;
  void *                        compare_arg;
} avl_tree;

avl_tree * avl_new_avl_tree (avl_key_compare_fun_type compare_fun, void * compare_arg);
avl_node * avl_new_avl_node (void * key, avl_node * parent);

void avl_free_avl_tree (
  avl_tree *            tree,
  avl_free_key_fun_type free_key_fun
  );

int avl_insert_by_key (
  avl_tree *            ob,
  void *                key,
  unsigned long *        index
  );

int avl_remove_by_key (
  avl_tree *            tree,
  void *                key,
  avl_free_key_fun_type free_key_fun
  );

int avl_get_item_by_index (
  avl_tree *            tree,
  unsigned long          index,
  void **               value_address
  );

int avl_get_item_by_key (
  avl_tree *            tree,
  void *                key,
  void **               value_address
  );

int avl_iterate_inorder (
  avl_tree *            tree,
  avl_iter_fun_type     iter_fun,
  void *                iter_arg
  );

int avl_iterate_index_range (
  avl_tree *            tree,
  avl_iter_index_fun_type iter_fun,
  unsigned long          low,
  unsigned long          high,
  void *                iter_arg
  );

int avl_get_span_by_key (
  avl_tree *            tree,
  void *                key,
  unsigned long *        low,
  unsigned long *        high
  );

int avl_get_span_by_two_keys (
  avl_tree *            tree,
  void *                key_a,
  void *                key_b,
  unsigned long *        low,
  unsigned long *        high
  );

int avl_verify (avl_tree * tree);

void avl_print_tree (
  avl_tree *            tree,
  avl_key_printer_fun_type key_printer
  );

avl_node * avl_get_predecessor (avl_node * node);

avl_node * avl_get_successor (avl_node * node);

/* These two are from David Ascher <david_ascher@brown.edu> */

int avl_get_item_by_key_most (
  avl_tree *            tree,
  void *                key,
  void **               value_address
  );

int avl_get_item_by_key_least (
  avl_tree *            tree,
  void *                key,
  void **               value_address
  );

#ifdef __cplusplus
}
#endif

#endif /* __AVL_H */
