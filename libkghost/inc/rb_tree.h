#ifndef _rb_tree_h_
#define _rb_tree_h_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup rb_tree
	 * @{
	 */

	typedef struct rb_tree_node_s* rb_tree_node_handle_t;
	typedef struct rb_tree_s* rb_tree_handle_t;
	typedef int (*rb_tree_compare_f)(const void*, const void*);
	typedef status_t (*rb_tree_alloc_copy_key_f)(void*, void**);
	typedef status_t (*rb_tree_alloc_copy_data_f)(void*, void**);
	typedef status_t (*rb_tree_release_key_f)(void*);
	typedef status_t (*rb_tree_release_data_f)(void*);

	status_t rb_tree_create(
		rb_tree_compare_f cmp,
		rb_tree_alloc_copy_key_f cpyKey,
		rb_tree_release_key_f dstryKey,
		rb_tree_alloc_copy_data_f cpyData,
		rb_tree_release_data_f dstryData,
		rb_tree_handle_t* p_tree);

	status_t rb_tree_release(rb_tree_handle_t tree);

	status_t rb_tree_insert(
		rb_tree_handle_t tree, 
		void* key, 
		void* data,
		rb_tree_node_handle_t* p_node);

	status_t rb_tree_remove(
		rb_tree_handle_t tree, 
		rb_tree_node_handle_t node);

	status_t rb_tree_find(
		rb_tree_handle_t tree, 
		void* key,
		rb_tree_node_handle_t* p_node);

	status_t rb_tree_enumerate(
		rb_tree_handle_t tree,
		void* lowKey, 
		void* highKey,
		rb_tree_node_handle_t last,
		rb_tree_node_handle_t* p_next);

	status_t rb_tree_node_key(
		rb_tree_node_handle_t node,
		void** p_key);
	status_t rb_tree_node_data(
		rb_tree_node_handle_t node,
		void** p_data);

	/** @} */
#ifdef __cplusplus
}
#endif

#endif
