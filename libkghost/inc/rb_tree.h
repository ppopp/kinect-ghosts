#ifndef _rb_tree_h_
#define _rb_tree_h_

#include "common.h"
#include "json.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct rb_tree_node_s* rb_tree_node_handle_t;
	typedef struct rb_tree_s* rb_tree_handle_t;
	typedef int (*rb_tree_compare_f)(const void*, const void*);
	typedef status_t (*rb_tree_alloc_copy_key_f)(void*, void**);
	typedef status_t (*rb_tree_alloc_copy_data_f)(void*, void**);
	typedef status_t (*rb_tree_release_key_f)(void*);
	typedef status_t (*rb_tree_release_data_f)(void*);
	typedef status_t (*rb_tree_json_key_create_f)(
		const void* key, 
		char** p_str);
	typedef status_t (*rb_tree_json_key_release_f)(char* str);
	typedef status_t (*rb_tree_json_dump_data_f)(
		const void* data, 
		json_t** pp_json);
	typedef status_t (*rb_tree_json_load_key_f)(
		const char* jsonKey,
		void** p_key);
	typedef status_t (*rb_tree_json_load_data_f)(
		json_t* p_json,
		void** p_data);

	status_t rb_tree_create(
		rb_tree_compare_f cmp,
		rb_tree_alloc_copy_key_f cpyKey,
		rb_tree_release_key_f dstryKey,
		rb_tree_alloc_copy_data_f cpyData,
		rb_tree_release_data_f dstryData,
		rb_tree_handle_t* p_tree);

	status_t rb_tree_create_json(
		json_t* json,
		rb_tree_json_load_key_f loadKey,
		rb_tree_json_load_data_f loadData,
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

	status_t rb_tree_export_json(
		rb_tree_handle_t handle,
		rb_tree_json_key_create_f jsonKeyCreateFunc,
		rb_tree_json_key_release_f jsonKeyReleaseFunc,
		rb_tree_json_dump_data_f jsonDataFunc,
		json_t* p_jsonObj);



#ifdef __cplusplus
}
#endif

#endif
