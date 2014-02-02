#include "rb_tree.h"

#include <stddef.h>
#include <stdlib.h>

typedef struct rb_tree_node_s {
	void* key;
	void* data;
	int red; /* if red=0 then the node is black */
	struct rb_tree_node_s* left;
	struct rb_tree_node_s* right;
	struct rb_tree_node_s* parent;
} rb_tree_node_t;

/* Compare(a,b) should return 1 if *a > *b, -1 if *a < *b, and 0 otherwise */
/* Destroy(a) takes a pointer to whatever key might be and frees it accordingly */
typedef struct rb_tree_s {
	rb_tree_compare_f compare;
	rb_tree_alloc_copy_key_f copy_key;
	rb_tree_alloc_copy_data_f copy_data;
	rb_tree_release_key_f release_key;
	rb_tree_release_data_f release_data;
	/*  A sentinel is used for root and for nil.  These sentinels are */
	/*  created when RBTreeCreate is caled.  root->left should always */
	/*  point to the node which is the root of the tree.  nil points to a */
	/*  node which should always be black but has aribtrary children and */
	/*  parent and no key or info.  The point of using these sentinels is so */
	/*  that the root and nil nodes do not require special cases in the code */
	rb_tree_node_t* root;             
	rb_tree_node_t* nil;              
} rb_tree_t;


static status_t _rb_tree_alloc_insert(
	rb_tree_handle_t tree, 
	void* key, 
	void* data,
	int doCopyKeyAndData,
	rb_tree_node_handle_t* p_node);
/* node creation/destruction */
static rb_tree_node_t* _rb_tree_create_node(
	rb_tree_t* tree,
	void* key,
	void* data,
	int doCopy);
static void _rb_tree_release_node(
	rb_tree_t* tree,
	rb_tree_node_t* node);

/* tree manimpulation */
static void _rb_tree_left_rotate(rb_tree_t* tree, rb_tree_node_t* x);
static void _rb_tree_right_rotate(rb_tree_t* tree, rb_tree_node_t* y);
static void _rb_tree_insert(rb_tree_t* tree, rb_tree_node_t* z);

/* tree access */
static rb_tree_node_t* _rb_tree_successor(rb_tree_t* tree,rb_tree_node_t* x);
static rb_tree_node_t* _rb_tree_predecessor(rb_tree_t* tree, rb_tree_node_t* x);

static void _rb_tree_release(rb_tree_t* tree, rb_tree_node_t* x);

status_t rb_tree_create( 
	rb_tree_compare_f cmp,
	rb_tree_alloc_copy_key_f cpyKey,
	rb_tree_release_key_f dstryKey,
	rb_tree_alloc_copy_data_f cpyData,
	rb_tree_release_data_f dstryData,
	rb_tree_handle_t* p_tree)
{
	rb_tree_t* tree = NULL;
	rb_tree_node_t* nil = NULL;
	rb_tree_node_t* root = NULL;

	if (
		(NULL == p_tree) ||
		(NULL == cmp) ||
		(NULL == cpyKey) ||
		(NULL == cpyData))
	{
		return ERR_NULL_POINTER;
	}

	tree = (rb_tree_t*)malloc(sizeof(rb_tree_t));
	if (NULL == tree) {
		return ERR_FAILED_ALLOC;
	}
	tree->compare = cmp;
	tree->copy_key = cpyKey;
	tree->release_key = dstryKey;
	tree->copy_data = cpyData;
	tree->release_data = dstryData;
	tree->nil = NULL;
	tree->root = NULL;

	nil = tree->nil = _rb_tree_create_node(tree, NULL, NULL, 0);
	if (NULL == nil) {
		rb_tree_release(tree);
		return ERR_FAILED_ALLOC;
	}
	nil->parent = nil->left = nil->right = nil;
	nil->red = 0;
	nil->key = NULL;

	root = tree->root = _rb_tree_create_node(tree, NULL, NULL, 0);
	if (NULL == root) {
		rb_tree_release(tree);
		return ERR_FAILED_ALLOC;
	}
	root->parent = root->left = root->right = tree->nil;
	root->key = NULL;
	root->red = 0;

	*p_tree = tree;
	return NO_ERROR;
}

status_t rb_tree_create_json(
	json_t* json,
	rb_tree_json_load_key_f loadKey,
	rb_tree_json_load_data_f loadData,
	rb_tree_compare_f cmp,
	rb_tree_alloc_copy_key_f cpyKey,
	rb_tree_release_key_f dstryKey,
	rb_tree_alloc_copy_data_f cpyData,
	rb_tree_release_data_f dstryData,
	rb_tree_handle_t* p_tree) 
{
	status_t err = NO_ERROR;
	rb_tree_t* tree = NULL;
	rb_tree_node_t* node = NULL;
	const char* jsonKey = NULL;
	json_t* jsonValue = NULL;
	void* key = NULL;
	void* data = NULL;
	void* iter = NULL;

	if ((NULL == json) ||
		(NULL == loadKey) ||
		(NULL == loadData) ||
		(NULL == cmp) ||
		(NULL == cpyKey) ||
		(NULL == dstryKey) ||
		(NULL == cpyData) ||
		(NULL == dstryData) ||
		(NULL == p_tree))
	{
		return ERR_NULL_POINTER;
	}

	err = rb_tree_create(
		cmp,
		cpyKey,
		dstryKey,
		cpyData,
		dstryData,
		&tree);
	if (NO_ERROR != err) {
		return err;
	}
	else if (NULL == tree) {
		return ERR_FAILED_CREATE;
	}

	iter = json_object_iter(json);
	while (iter) {
		jsonKey = json_object_iter_key(iter);
		jsonValue = json_object_iter_value(iter);
		
		err = loadKey(jsonKey, &key);
		if (NO_ERROR != err) {
			rb_tree_release(tree);
			return err;
		}
		err = loadData(jsonValue, &data);
		if (NO_ERROR != err) {
			rb_tree_release(tree);
			return err;
		}

		err = _rb_tree_alloc_insert(tree, key, data, 0, &node);
		if (NO_ERROR != err) {
			rb_tree_release(tree);
			return err;
		}
		iter = json_object_iter_next(json, iter);
	}

	*p_tree = tree;
	return NO_ERROR;
}

rb_tree_node_t* _rb_tree_create_node(
	rb_tree_t* tree,
	void* key,
	void* data,
	int doCopy) 
{
	rb_tree_node_t* node = NULL;
	status_t err = NO_ERROR;

	node = (rb_tree_node_t*)malloc(sizeof(rb_tree_node_t));
	if (NULL == node) {
		return node;
	}

	if (doCopy) {
		/* TODO: ignoring errors, need to refactor code */
		err = tree->copy_key(key, &(node->key));
		err = tree->copy_data(data, &(node->data));
	}
	else {
		node->key = key;
		node->data = data;
	}
	node->red = 0;
	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;

	return node;
}

void _rb_tree_release_node(
	rb_tree_t* tree,
	rb_tree_node_t* node) 
{
	if (NULL != tree->release_key) {
		tree->release_key(node->key);
	}
	if (NULL != tree->release_data) {
		tree->release_data(node->data);
	}
	free(node);
}

/* Rotates as described in _Introduction_To_Algorithms by Cormen, Leiserson, 
 * Rivest (Chapter 14).  Basically this makes the parent of x be to the left of 
 * x, x the parent of its parent before the rotation and fixes other pointers 
 * accordingly. */
void _rb_tree_left_rotate(rb_tree_t* tree, rb_tree_node_t* x) {
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* nil = tree->nil;

	y = x->right;
	x->right = y->left;

	if (y->left != nil) {
		y->left->parent = x; 
	}

	y->parent = x->parent; 

	if(x == x->parent->left) {
		x->parent->left = y;
	} else {
		x->parent->right = y;
	}

	y->left = x;
	x->parent = y;
}


/* Rotates as described in _Introduction_To_Algorithms by Cormen, Leiserson, 
 * Rivest (Chapter 14).  Basically this makes the parent of x be to the left of 
 * x, x the parent of its parent before the rotation and fixes other pointers 
 * accordingly. */
void _rb_tree_right_rotate(rb_tree_t* tree, rb_tree_node_t* y) {
	rb_tree_node_t* x;
	rb_tree_node_t* nil = tree->nil;

	x = y->left;
	y->left = x->right;

	if (nil != x->right)  {
		x->right->parent = y; 
	}

	x->parent = y->parent;
	if( y == y->parent->left) {
		y->parent->left = x;
	} else {
		y->parent->right = x;
	}

	x->right = y;
	y->parent = x;
}


void _rb_tree_insert(rb_tree_t* tree, rb_tree_node_t* z) {
	rb_tree_node_t* x = NULL;
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* nil = tree->nil;

	z->left = z->right = nil;
	y = tree->root;
	x = tree->root->left;

	while(x != nil) {
		y = x;
		if (0 < tree->compare(x->key, z->key)) { 
			x = x->left;
		} 
		else { 
			x = x->right;
		}
	}
	
	z->parent = y;
	if (
		(y == tree->root) ||
		(0 < tree->compare(y->key, z->key))) 
	{ 
		y->left = z;
	} 
	else {
		y->right = z;
	}
}

status_t rb_tree_insert(
	rb_tree_handle_t tree, 
	void* key, 
	void* data,
	rb_tree_node_handle_t* p_node)
{
	return _rb_tree_alloc_insert(tree, key, data, 1, p_node);
}

status_t _rb_tree_alloc_insert(
	rb_tree_handle_t tree, 
	void* key, 
	void* data,
	int doCopyKeyAndData,
	rb_tree_node_handle_t* p_node)
{
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* x = NULL;
	rb_tree_node_t* node = NULL;

	if (
		(NULL == tree) ||
		(NULL == key))
	{
		return ERR_NULL_POINTER;
	}

	node = _rb_tree_create_node(tree, key, data, doCopyKeyAndData);
	if (NULL == node) {
		return ERR_FAILED_ALLOC;
	}

	_rb_tree_insert(tree, node);
	x = node;
	x->red = 1;
	while(x->parent->red) { 
		if (x->parent == x->parent->parent->left) {
			y = x->parent->parent->right;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} 
			else {
				if (x == x->parent->right) {
					x = x->parent;
					_rb_tree_left_rotate(tree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				_rb_tree_right_rotate(tree, x->parent->parent);
			} 
		} else { 
			y = x->parent->parent->left;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} 
			else {
				if (x == x->parent->left) {
					x = x->parent;
					_rb_tree_right_rotate(tree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				_rb_tree_left_rotate(tree, x->parent->parent);
			} 
		}
	}

	tree->root->left->red = 0;

	if (NULL != p_node) {
		*p_node = node;
	}

	return NO_ERROR;
}
  
rb_tree_node_t* _rb_tree_successor(rb_tree_t* tree,rb_tree_node_t* x) { 
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* nil = tree->nil;
	rb_tree_node_t* root = tree->root;

	if (nil != (y = x->right)) { 
		while(y->left != nil) { 
			y = y->left;
		}
		return y;
	} 
	else {
		y = x->parent;
		while(x == y->right) { 
			x = y;
			y = y->parent;
		}
		if (y == root) {
			return nil;
		}
		return(y);
	}
}

rb_tree_node_t* _rb_tree_predecessor(rb_tree_t* tree, rb_tree_node_t* x) {
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* nil = tree->nil;
	rb_tree_node_t* root = tree->root;

	if (nil != (y = x->left)) {
		while(y->right != nil) {
			y = y->right;
		}
	} 
	else {
		y = x->parent;
		while(x == y->left) { 
			if (y == root) {
				return nil; 
			}
			x = y;
			y = y->parent;
		}
	}
	return y;
}

void _rb_tree_release(rb_tree_t* tree, rb_tree_node_t* x) {
	rb_tree_node_t* nil = tree->nil;
	if (x != nil) {
		_rb_tree_release(tree, x->left);
		_rb_tree_release(tree, x->right);
		tree->release_key(x->key);
		tree->release_data(x->data);
		free(x);
	}
}


status_t rb_tree_release(rb_tree_handle_t tree) {
	if (NULL == tree) {
		return NO_ERROR;
	}
	_rb_tree_release(tree, tree->root->left);
	free(tree->root);
	free(tree->nil);
	free(tree);
	return NO_ERROR;
}
  
status_t rb_tree_find(
	rb_tree_handle_t tree, 
	void* key,
	rb_tree_node_handle_t* p_node)
{
	rb_tree_node_t* x = NULL;
	rb_tree_node_t* nil = NULL;
	int compVal = 0;

	if (
		(NULL == tree) ||
		(NULL == key) ||
		(NULL == p_node))
	{
		return ERR_NULL_POINTER;
	}

	x = tree->root->left;
	nil = tree->nil;
	if (x == nil) {
		*p_node = NULL;
		return NO_ERROR;
	}

	compVal = tree->compare(x->key, key);
	while(0 != compVal) {
		if (0 < compVal) { 
			x = x->left;
		} 
		else {
			x = x->right;
		}
		if (x == nil) {
			*p_node = NULL;
			return NO_ERROR;
		}
		compVal = tree->compare(x->key, key);
	}
	*p_node = x;
	return NO_ERROR;
}

void _rb_tree_delete(rb_tree_t* tree, rb_tree_node_t* x) {
	rb_tree_node_t* root = tree->root->left;
	rb_tree_node_t* w = NULL;

	while((!x->red) && (root != x)) {
		if (x == x->parent->left) {
			w = x->parent->right;
			if (w->red) {
				w->red = 0;
				x->parent->red = 1;
				_rb_tree_left_rotate(tree, x->parent);
				w = x->parent->right;
			}
			if ((!w->right->red) && (!w->left->red)) { 
				w->red = 1;
				x = x->parent;
			} 
			else {
				if (!w->right->red) {
					w->left->red = 0;
					w->red = 1;
					_rb_tree_right_rotate(tree, w);
					w = x->parent->right;
				}
				w->red = x->parent->red;
				x->parent->red = 0;
				w->right->red = 0;
				_rb_tree_left_rotate(tree, x->parent);
				x = root; /* this is to exit while loop */
			}
		} 
		else { 
			/* the code below is has left and right switched from above */
			w = x->parent->left;
			if (w->red) {
				w->red = 0;
				x->parent->red = 1;
				_rb_tree_right_rotate(tree, x->parent);
				w = x->parent->left;
			}
			if ((!w->right->red) && (!w->left->red)) { 
				w->red = 1;
				x = x->parent;
			} 
			else {
				if (!w->left->red) {
					w->right->red = 0;
					w->red = 1;
					_rb_tree_left_rotate(tree, w);
					w = x->parent->left;
				}
				w->red = x->parent->red;
				x->parent->red = 0;
				w->left->red = 0;
				_rb_tree_right_rotate(tree, x->parent);
				x = root; /* this is to exit while loop */
			}
		}
	}
	x->red = 0;
}

status_t rb_tree_remove(
	rb_tree_handle_t tree, 
	rb_tree_node_handle_t node)
{
	rb_tree_node_t* y = NULL;
	rb_tree_node_t* x = NULL;
	rb_tree_node_t* nil = NULL;
	rb_tree_node_t* root = NULL;

	if (
		(NULL == tree) ||
		(NULL == node))
	{
		return ERR_NULL_POINTER;
	}

	nil = tree->nil;
	root = tree->root;
	if ((node->left == nil) || (node->right == nil)) {
		y = node;
	}
	else {
		y = _rb_tree_successor(tree, node);
	}

	if (y->left == nil) {
		x = y->right;
	}
	else {
		x = y->left;
	}

	x->parent = y->parent;
	if (root == x->parent) {
		root->left = x;
	} 
	else {
		if (y == y->parent->left) {
			y->parent->left = x;
		} 
		else {
			y->parent->right = x;
		}
	}

	if (y != node) { 
		// y should not be nil in this case 
		// y is the node to splice out and x is its child 

		if (!(y->red)) {
			_rb_tree_delete(tree, x);
		}

		y->left = node->left;
		y->right = node->right;
		y->parent = node->parent;
		y->red = node->red;
		
		node->left->parent = node->right->parent = y;
		if (node == node->parent->left) {
			node->parent->left=y; 
		} 
		else {
			node->parent->right = y;
		}
		_rb_tree_release_node(tree, node);
	} 
	else {
		if (!(y->red)) {
			_rb_tree_delete(tree, x);
		}
		_rb_tree_release_node(tree, y);
	}
	return NO_ERROR;
}


status_t rb_tree_enumerate(
	rb_tree_handle_t tree,
	void* lowKey, 
	void* highKey,
	rb_tree_node_handle_t last,
	rb_tree_node_handle_t* p_next)
{
	rb_tree_node_t* nil = NULL;
	rb_tree_node_t* x = NULL;
	rb_tree_node_t* next = NULL;

	if (
		(NULL == tree) ||
		(NULL == p_next))
	{
		return ERR_NULL_POINTER;
	}

	nil = tree->nil;
	x = tree->root->left;
	if (NULL == last) {
		// first time enumerate is called, find highest correct key
		next = nil;
		x = tree->root->left;

		if (NULL == highKey) {
			// No high key, so find the node with the highest key in entire 
			// tree.
			while(nil != x) {
				next = x;
				x = x->right;
			}
		}
		else {
			// High key given so find node equal to or greater than high key
			while(nil != x) {
				if (0 < tree->compare(x->key, highKey)) { 
					x = x->left;
				} 
				else {
					next = x;
					x = x->right;
				}
			}
		}
	}
	else {
		// this is called in a loop, so find next node
		next = _rb_tree_predecessor(tree, last);
	}

	if (next != nil) {
		if (NULL == lowKey) {
			*p_next = next;
		}
		else if (1 > tree->compare(lowKey, next->key)) {
			*p_next = next;
		}
		else {
			*p_next = NULL;
		}
	}
	else {
		*p_next = NULL;
	}
	return NO_ERROR;
}

status_t rb_tree_node_key(
	rb_tree_node_handle_t node,
	void** p_key)
{
	if ((NULL == node) || (NULL == p_key)) {
		return ERR_NULL_POINTER;
	}
	*p_key = node->key;
	return NO_ERROR;
}

status_t rb_tree_node_data(
	rb_tree_node_handle_t node,
	void** p_data)
{
	if ((NULL == node) || (NULL == p_data)) {
		return ERR_NULL_POINTER;
	}
	*p_data = node->data;
	return NO_ERROR;
}

status_t rb_tree_export_json(
	rb_tree_handle_t handle,
	rb_tree_json_key_create_f jsonKeyCreateFunc,
	rb_tree_json_key_release_f jsonKeyReleaseFunc,
	rb_tree_json_dump_data_f jsonDataFunc,
	json_t* p_jsonObj)
{
	rb_tree_node_handle_t last = NULL;
	rb_tree_node_handle_t next = NULL;
	status_t err = NO_ERROR;
	void* key = NULL;
	void* data = NULL;
	char* jsonKey = NULL;
	json_t* jsonData = NULL;
	int jsonErr = 0;

	if ((NULL == handle) || 
		(NULL == jsonKeyCreateFunc) || 
		(NULL == jsonKeyReleaseFunc) || 
		(NULL == jsonDataFunc) ||
		(NULL == p_jsonObj))
	{
		return ERR_NULL_POINTER;
	}

	if (!json_is_object(p_jsonObj)) {
		return ERR_INVALID_ARGUMENT;
	}

	err = rb_tree_enumerate(handle, NULL, NULL, last, &next);
	if (NO_ERROR != err) {
		return err;
	}

	while ((err == NO_ERROR) && (NULL != next)) {
		last = next;

		err = rb_tree_node_key(next, &key);
		if (NO_ERROR != err) {
			return err;
		}

		err = rb_tree_node_data(next, &data);
		if (NO_ERROR != err) {
			return err;
		}

		err = jsonKeyCreateFunc(key, &jsonKey);
		if (NO_ERROR != err) {
			return err;
		}

		err = jsonDataFunc(data, &jsonData);
		if (NO_ERROR != err) {
			jsonKeyReleaseFunc(jsonKey);
			return err;
		}

		if ((NULL != jsonKey) && (NULL != jsonData)) {
			jsonErr = json_object_set_new(p_jsonObj, jsonKey, jsonData);

			if (jsonErr) {
				jsonKeyReleaseFunc(jsonKey);
				return ERR_FAILED_EXPORT;
			}
		}

		err = jsonKeyReleaseFunc(jsonKey);
		if (NO_ERROR != err) {
			return err;
		}

		err = rb_tree_enumerate(handle, NULL, NULL, last, &next);
		if (NO_ERROR != err) {
			return err;
		}
	}

	return NO_ERROR;
}

