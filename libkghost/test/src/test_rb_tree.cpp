#include "gtest/gtest.h"
#include "common.h"
#include "rb_tree.h"

#include <stdio.h>
#include <list>
#include <map>

using namespace std;

typedef int key_t;
typedef double data_t;

status_t rb_tree_json_key_create(const void* key, char** p_str) {
	if (NULL == key) {
		return ERR_NULL_POINTER;
	}
	*p_str = (char*)malloc(64);
	snprintf(*p_str, 64, "%i", *((int*)key));
	return NO_ERROR;
}

status_t rb_tree_json_key_release(char* str) {
	free(str);
	return NO_ERROR;
}

status_t rb_tree_json_dump_data(const void* data, json_t** pp_json) {
	json_t* obj = NULL;
	if ((NULL == data) || (NULL == pp_json)) {
		return ERR_NULL_POINTER;
	}

	obj = json_real(*((double*)data));
	*pp_json = obj;
	return NO_ERROR;
}

status_t rb_tree_json_load_key(const char* jsonKey, void** p_key) {
	int* key = NULL;

	if ((NULL == jsonKey) || (NULL == p_key)) {
		return ERR_NULL_POINTER;
	}

	key = (int*)malloc(sizeof(int));
	if (NULL == key) {
		return ERR_FAILED_ALLOC;
	}
	*key = atoi(jsonKey);
	*p_key = (void*)key;
	return NO_ERROR;
}

status_t rb_tree_json_load_data(json_t* p_json, void** p_data) {
	double* data = NULL;

	if ((NULL == p_json) || (NULL == p_data)) {
		return ERR_NULL_POINTER;
	}

	data = (double*)malloc(sizeof(double));
	*data = json_real_value(p_json);

	*p_data = (void*)data;

	return NO_ERROR;
}

int rb_tree_compare(const void* first, const void* second) {
	const key_t* firstId = (const key_t*)first;
	const key_t* secondId = (const key_t*)second;

	if (NULL == firstId) {
		if (NULL != secondId) {
			return -1;
		}
		else {
			return 0;
		}
	}
	else if (NULL == secondId) {
		return 1;
	}
	else if (*firstId < *secondId) {
		return -1;
	}
	else if (*firstId > *secondId) {
		return 1;
	}
	return 0;
}

status_t rb_tree_alloc_copy_key(void* in, void** copy) {
	const key_t* inKey = (const key_t*)in;
	key_t* copyKey = NULL;

	if (NULL == in) {
		*copy = (void*)in;
		return NO_ERROR;
	}

	copyKey = (key_t*)malloc(sizeof(key_t));
	*copyKey = *inKey;
	*copy = copyKey;
	return NO_ERROR;
}

status_t rb_tree_alloc_copy_data(void* in, void** copy) {
	const data_t* inData = (const data_t*)in;
	data_t* copyData = NULL;

	if (NULL == in) {
		*copy = in;
		return NO_ERROR;
	}

	copyData = (data_t*)malloc(sizeof(data_t));
	*copyData = *inData;
	*copy = copyData;
	return NO_ERROR;
}

status_t rb_tree_release_key(void* key) {
	free(key);
	return NO_ERROR;
}

status_t rb_tree_release_data(void* data) {
	free(data);
	return NO_ERROR;
}

TEST(RbTreeTest, CreateRelease) {
	rb_tree_handle_t tree;
	status_t err = NO_ERROR;

	err = rb_tree_create(
		rb_tree_compare,
		rb_tree_alloc_copy_key,
		rb_tree_release_key,
		rb_tree_alloc_copy_data,
		rb_tree_release_data,
		&tree);

	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(tree != NULL);

	err = rb_tree_release(tree);
	ASSERT_EQ(err, NO_ERROR);
}

TEST(RbTreeTest, InsertFindRemove) {
	rb_tree_handle_t tree;
	status_t err = NO_ERROR;
	list<key_t> keys;

	err = rb_tree_create(
		rb_tree_compare,
		rb_tree_alloc_copy_key,
		rb_tree_release_key,
		rb_tree_alloc_copy_data,
		rb_tree_release_data,
		&tree);

	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(tree != NULL);

	for (size_t i = 0; i < 100; i++) {
		key_t key = 0;
		key_t* nodeKey = 0;
		data_t data = 0;
		data_t* nodeData = 0;
		rb_tree_node_handle_t node = NULL;
		rb_tree_node_handle_t findNode = NULL;

		key = rand();
		data = rand();
		keys.push_back(key);

		/* test insert */
		err = rb_tree_insert(
			tree,
			&key,
			&data,
			&node);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(node != NULL);
		
		/* check data after insert */
		err = rb_tree_node_key(
			node,
			(void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeKey, key);
		err = rb_tree_node_data(
			node,
			(void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, data);

		/* test find */
		err = rb_tree_find(
			tree,
			&key,
			&findNode);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(findNode != NULL);
		ASSERT_EQ(findNode, node);
	}

	for (list<key_t>::iterator iter = keys.begin();
		iter != keys.end();
		iter++)
	{
		key_t* key = NULL;
		rb_tree_node_handle_t findNode;

		/* find node */
		err = rb_tree_find(
			tree,
			&(*iter),
			&findNode);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(findNode != NULL);
		err = rb_tree_node_key(
			findNode,
			(void**)&key);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*key, *iter);

		/* remove node */
		err = rb_tree_remove(
			tree,
			findNode);
		ASSERT_EQ(err, NO_ERROR);

		err = rb_tree_find(
			tree,
			&(*iter),
			&findNode);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(findNode == NULL);
	}

	err = rb_tree_release(tree);
	ASSERT_EQ(err, NO_ERROR);
}

TEST(RbTreeTest, Enumerate) {
	rb_tree_handle_t tree;
	status_t err = NO_ERROR;
	map<key_t, data_t> rbMap;

	err = rb_tree_create(
		rb_tree_compare,
		rb_tree_alloc_copy_key,
		rb_tree_release_key,
		rb_tree_alloc_copy_data,
		rb_tree_release_data,
		&tree);

	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(tree != NULL);

	for (size_t i = 0; i < 100; i++) {
		key_t key = i;
		data_t data = 0;
		rb_tree_node_handle_t node = NULL;

		data = rand();
		rbMap[key] = data;

		/* test insert */
		err = rb_tree_insert(tree, &key, &data, &node);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(node != NULL);
	}

	/* test enumerate between values */
	key_t low = 20;
	key_t high = 30;
	key_t cntr = high;
	rb_tree_node_handle_t last = NULL;
	rb_tree_node_handle_t next = NULL;

	/* test enumerate in range */
	err = rb_tree_enumerate(tree, &low, &high, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);
		cntr--;
		last = next;
		err = rb_tree_enumerate(tree, &low, &high, last, &next);
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, low - 1);

	/* test enumerate all */
	last = NULL;
	next = NULL;
	err = rb_tree_enumerate(tree, NULL, NULL, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	cntr = 99;
	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);
		cntr--;
		last = next;
		err = rb_tree_enumerate(tree, NULL, NULL, last, &next);
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, -1);

	/* test high limit */
	last = NULL;
	next = NULL;
	high = 40;
	err = rb_tree_enumerate(tree, NULL, &high, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	cntr = high;
	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);
		cntr--;
		last = next;
		err = rb_tree_enumerate(tree, NULL, &high, last, &next);
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, -1);

	/* test low limit */
	last = NULL;
	next = NULL;
	low = 40;
	err = rb_tree_enumerate(tree, &low, NULL, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	cntr = 99;
	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);
		cntr--;
		last = next;
		err = rb_tree_enumerate(tree, &low, NULL, last, &next);
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, low - 1);
	err = rb_tree_release(tree);
	ASSERT_EQ(err, NO_ERROR);
}

TEST(RbTreeTest, JsonExportCreate) {
	rb_tree_handle_t tree = NULL;
	rb_tree_handle_t tree2 = NULL;
	status_t err = NO_ERROR;
	map<key_t, data_t> rbMap;
	json_t* jsonObj = NULL;
	json_t* jsonObj2 = NULL;
	char* jsonStr = NULL;
	json_error_t jsonError;
	int cntr = 0;

	err = rb_tree_create(
		rb_tree_compare,
		rb_tree_alloc_copy_key,
		rb_tree_release_key,
		rb_tree_alloc_copy_data,
		rb_tree_release_data,
		&tree);

	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(tree != NULL);

	for (size_t i = 0; i < 100; i++) {
		key_t key = i;
		data_t data = 0;
		rb_tree_node_handle_t node = NULL;

		data = rand();
		rbMap[key] = data;

		/* test insert */
		err = rb_tree_insert(tree, &key, &data, &node);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_TRUE(node != NULL);
	}

	jsonObj = json_object();
	err = rb_tree_export_json(
		tree,
		&rb_tree_json_key_create,
		&rb_tree_json_key_release,
		&rb_tree_json_dump_data,
		jsonObj);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(jsonObj != NULL);

	jsonStr = json_dumps(jsonObj, JSON_COMPACT);
	ASSERT_TRUE(jsonObj != NULL);

	jsonObj2 = json_loads(jsonStr, 0, &jsonError);
	ASSERT_TRUE(jsonObj2 != NULL);

	free(jsonStr);
	json_decref(jsonObj);

	err = rb_tree_create_json(
		jsonObj2,
		&rb_tree_json_load_key,
		&rb_tree_json_load_data,
		&rb_tree_compare,
		&rb_tree_alloc_copy_key,
		&rb_tree_release_key,
		&rb_tree_alloc_copy_data,
		&rb_tree_release_data,
		&tree2);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(tree2 != NULL);
	json_decref(jsonObj2);


	/* test enumerate between values */
	rb_tree_node_handle_t last = NULL;
	rb_tree_node_handle_t next = NULL;
	cntr = 99;

	/* test enumerate in range */
	err = rb_tree_enumerate(tree, NULL, NULL, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);

		last = next;
		err = rb_tree_enumerate(tree, NULL, NULL, last, &next);
		if (next) {
			cntr--;
		}
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, 0);

	/* test enumerate between values */
	last = NULL;
	next = NULL;
	cntr = 99;

	/* test enumerate in range */
	err = rb_tree_enumerate(tree2, NULL, NULL, last, &next);
	ASSERT_EQ(err, NO_ERROR);

	while (next != NULL) {
		key_t* nodeKey = NULL;
		data_t* nodeData = NULL;

		err = rb_tree_node_key(next, (void**)&nodeKey);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(cntr, *nodeKey);

		err = rb_tree_node_data(next, (void**)&nodeData);
		ASSERT_EQ(err, NO_ERROR);
		ASSERT_EQ(*nodeData, rbMap[cntr]);
		
		last = next;
		err = rb_tree_enumerate(tree2, NULL, NULL, last, &next);
		if (next) {
			cntr--;
		}
		ASSERT_EQ(err, NO_ERROR);
	}
	ASSERT_EQ(cntr, 0);

	err = rb_tree_release(tree2);
	ASSERT_EQ(err, NO_ERROR);
	err = rb_tree_release(tree);
	ASSERT_EQ(err, NO_ERROR);
}
