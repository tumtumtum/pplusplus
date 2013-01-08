#include "hashtable.h"
#include "memory.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/*
 * Creates a new list_t.  Sets all values to zero.
 * The list_t needs to later be explictly deleted.
 *
 * @see			list_delete(list_t*)
 * @returns		The list if successful, otherwise NULL.
 */
list_t *list_new()
{
	return CALLOC(list_t, 1);
}

/*
 * Deletes a list_t by removing it off the heap.
 * All the nodes in the list will also be deleted, this
 * also includes all the variants stored in these nodes.
 *
 * @see list_node_delete(list_node_t*)
 */
void list_delete(list_t *list_p)
{
	list_node_t *node1_p, *node2_p;

	node1_p = list_p->first_p;

	/*
	 * Go through every node.
	 */
	while (node1_p)
	{
		node2_p = node1_p->next_p;
		
		/*
		 * Delete each node.
		 */
		list_node_delete(node1_p);
		
		node1_p = node2_p;
	}

	free(list_p);

	/*
	 * Success.
	 */	
}

/*
 * Creates a new list_node_t.  Sets all values to zero.
 * The list_node_t needs to later be explictly deleted.
 *
 * @see			list_node_delete(list_node_t*)
 * @returns		The node if successful, otherwise NULL.
 */
list_node_t *list_node_new(variant_t var)
{
	list_node_t *node_p;

	/*
	 * If the list_node_t can't be allocated.
	 */
	if (!(node_p = CALLOC(list_node_t, 1)))
	{
		/*
		 * Fail.
		 */
		return NULL;
	}

	node_p->var = var;

	/*
	 * Success.
	 */
	return node_p;
}

/*
 * Deletes a list_node_t by removing it off the heap.
 * The variant_t being held in the node will also have it's
 * destructor called. 
 *
 * @see variant_destruct(variant_t*)
 */
void list_node_delete(list_node_t *node_p)
{
	variant_destruct(&node_p->var);
	
	free(node_p);

	/*
	 * Success.
	 */
}

/*
 * Finds an item on the list, by using the default dynamic comparator.
 * 
 * @see variant_dynamic_comparator
 */
variant_t list_find(list_t *list_p, variant_t var)
{
	/*
	 * Success.
	 */
	return list_find_with(list_p, var, variant_dynamic_comparator);
}

/*
 * Finds an item on the list, by using the supplied comparator.
 * 
 * @see variant_dynamic_comparator
 */
variant_t list_find_with(list_t *list_p, variant_t var, variant_comparator_fn comparator)
{
	variant_t retval;
	list_iterator_t iterator_p;

	/*
	 * Initialize an iterator for the list.
	 */
	list_init_iterator(list_p, &iterator_p);
	
	/*
	 * Iterate through all the variants in the list.
	 */
	while (list_hasmore(&iterator_p))
	{
		/*
		 * Does the current var match the one they're looking for?
		 */
		if (comparator(&var, iterator_p.var_p) == 0)
		{
			/*
			 * Return the current var.
			 */

			/*
			 * Success.
			 */
			return *iterator_p.var_p;
		}
	}

	/*
	 * Make the return value an empty variant.
	 */
	retval.vt = VT_EMPTY;
	
	/*
	 * Fail.
	 */
	return retval;
}

/*
 * Adds a variant to the end of a linked list.  The variant is owned by the list
 * and it's destructor will be called when the list is finished with it.
 *
 * @list	The linked list.
 * @var		The variant to add to the list.
 */
std_error_t list_add(list_t *list_p, variant_t var)
{
	list_node_t *node_p;

	/*
	 * If a new node couldn't be created.
	 */
	if (!(node_p = list_node_new(var)))
	{
		/*
		 * Destroy variant since we own it.
		 */
		variant_destruct(&var);

		/*
		 * Return the error code.
		 */
		return std_malloc_fail;
	}

	/*
	 * If there's a last node.
	 */
	if (list_p->last_p)
	{
		/*
		 * Make the new node be the next one after the last element.
		 */
		list_p->last_p->next_p = node_p;
	}
	
	/*
	 * Make the list's last node the new node.
	 */
	list_p->last_p = node_p;

	/*
	 * If there's no first node, then the new node is the first as well.
	 */
	if (list_p->first_p == NULL)
	{
		list_p->first_p = node_p;
	}

	list_p->size++;

	/*
	 * Success.
	 */
	return std_success;
}

/*
 * Initializes a list iterator.
 *
 * @param list		The list to iterate.
 * @param iterator	The iterator to initialize.
 */
void list_init_iterator(list_t *list_p, list_iterator_t *iterator_p)
{	
	iterator_p->list_p = list_p;
	iterator_p->current_node_p = NULL;
}

/*
 * Given a list iterator, it determines if there's any more elements
 * in the list to iterate.
 *
 * @param list_iterator	The hashtable iterator.
 *
 * @returns TRUE if there are more elements, FALSE if there aren't.
 */
BOOL list_hasmore(list_iterator_t *list_iterator_p)
{
	/*
	 * If this is the first time the iterator is used.
	 */
	if (list_iterator_p->current_node_p == NULL)
	{
		/*
		 * If the list has nothing in it.
		 */
		if (list_iterator_p->list_p->size <= 0)
		{
			/*
			 * Stop iterating.
			 */

			/*
			 * Success.
			 */
			return FALSE;
		}

		/*
		 * Set the current var and node to be the first on the list.
		 */
		list_iterator_p->current_node_p = list_iterator_p->list_p->first_p;
		list_iterator_p->var_p = &list_iterator_p->current_node_p->var;

		/*
		 * Success.
		 */
		return TRUE;
	}

	/*
	 * Move to the next node in the list.
	 */
	list_iterator_p->current_node_p = list_iterator_p->current_node_p->next_p;
	
	/*
	 * Is this the end of the list?
	 */
	if (list_iterator_p->current_node_p == NULL)
	{
		/*
		 * Success.
		 */
		return FALSE;		
	}
	else
	{
		/*
		 * Set the iterator's value pointer to point to the current node's value.
		 */
		list_iterator_p->var_p = &list_iterator_p->current_node_p->var;

		/*
		 * Success.
		 */
		return TRUE;
	}
}

/*
 * Creates a new hashtable_t.  Sets all values to zero.
 * The hashtable_t needs to later be explictly deleted.
 *
 * @see hashtable_delete(list_t*)
 */
hashtable_t *hashtable_new(int tblsize)
{
	UINT i;
	hashtable_t *hashtable_p;

	/*
	 * Could the hashtable_t not be allocated?
	 */
	if (!(hashtable_p = CALLOC(hashtable_t, 1)))
	{
		/*
		 * Fail.
		 */
		return NULL;
	}
		
	/*
	 * If a hashtable_t couldn't be allocated.
	 */
	if (!(hashtable_p->table = CALLOC(list_t*, tblsize)))
	{
		/*
		 * Deallocate what's already been allocated.
		 */
		free(hashtable_p);

		/*
		 * Fail.
		 */
		return NULL;
	}
	
	hashtable_p->tblsize = tblsize;

	/*
	 * Put a linked list inside each element in the table.
	 */
	for (i = 0; i < hashtable_p->tblsize; i++)
	{
		hashtable_p->table[i] = list_new();

		/*
		 * If the linked list couldn't be created.
		 */
		if (!hashtable_p->table[i])
		{
			/*
			 * Failed to create the whole hashtable.
			 * Deallocate what's been created and fail.
			 */
			hashtable_p->tblsize = i + 1;
			hashtable_delete(hashtable_p);

			/*
			 * Fail.
			 */
			return NULL;
		}
	}

	/*
	 * Success.
	 */
	return hashtable_p;
}

/*
 * Deletes a hashitem_t by removing it off the heap.
 * This will also free up any variant_ts held in the
 * hashtable.
 *
 * @see hashtable_delete(hashtable_t*)
 */
void hashtable_delete(hashtable_t *hashtable_p)
{
	UINT i;

	/*
	 * Go through the entire table.
	 */
	for (i = 0; i < hashtable_p->tblsize; i++)
	{
		/*
		 * Delete each linked list.
		 */
		list_delete(hashtable_p->table[i]);
	}

	free(hashtable_p->table);	
	free(hashtable_p);

	/*
	 * Success.
	 */
}

/*
 * Creates a new hashitem_t.  Sets the item's values from
 * the supplied arguments.
 * The hashitem_t needs to later be explictly deleted.
 *
 * @see hashitem_delete(list_node_t*)
 */
hashitem_t *hashitem_new(variant_t var, variant_t key)
{
	hashitem_t *hashitem_p;

	/*
	 * If a hashitem_t can't be allocated.
	 */
	if (!(hashitem_p = CALLOC(hashitem_t, 1)))
	{
		/*
		 * Fail.
		 */
		return NULL;
	}

	hashitem_p->var = var;
	hashitem_p->key = key;

	return hashitem_p;
}

/*
 * Deletes a hashitem_t by removing it off the heap.
 * The key and value of the hashitem will both be
 * destroyed.
 *
 * @see variant_destruct(variant_t*)
 */
void hashitem_delete(hashitem_t *hashitem_p)
{
	variant_destruct(&hashitem_p->key);
	variant_destruct(&hashitem_p->var);

	free(hashitem_p);
}

/*
 * Puts a variant into the hashtable with the spcified key.
 * The hashtable owns both the key and variant - so it will free them.
 * This function does not check to make sure the key hasn't already been used.
 *
 * @param hashtable		The hashtable.
 * @param var			The variant to put in the hashtable.
 * @param key			The key to be associated with the var in the hashtable.
 * @returns				std_exists if an item with the same key already exists.
 *						std_success otherwise.
 * @see hashtable_put(hashtable_t, variant_t, variant_t)
 */
std_error_t hashtable_put_fast(hashtable_t *hashtable_p, variant_t var, variant_t key)
{	
	hashitem_t *hashitem_p;
	variant_t var_hashitem;
	std_error_t retval;

	/*
	 * If a hashtiem containing the key and variant couldn't be allocated.
	 */
	if (!(hashitem_p = hashitem_new(var, key)))
	{
		/*
		 * Delete the var + key since we own it.
		 */
		variant_destruct(&var);
		variant_destruct(&key);

		/*
		 * Fail.
		 */
		return std_malloc_fail;
	}

	/*
	 * Initialize a variant 'var' that stores the hashitem.
	 */
	variant_init_pointer(&var_hashitem, VT_PHASHITEM, hashitem_p);
	
	/*
	 * Put the hashitem in the appropriate list of the hashtable.
	 */
	retval = list_add(hashtable_p->table[hashtable_tblindex(hashtable_p, key)], var_hashitem);
	
	/*
	 * If adding to the list failed.
	 */
	if (retval != std_success)
	{
		/*
		 * Delete hashitem (includes var + key) since we own them.
		 */			
		hashitem_delete(hashitem_p);

		/*
		 * Fail.
		 */
		return retval;
	}

	hashtable_p->size++;

	/*
	 * Success.
	 */
	return std_success;
}

/*
 * Puts a variant into the hashtable with the spcified key.
 * The hashtable owns both the key and variant - so it will free them.
 * This function checks to make sure the key hasn't already been used.
 *
 * @param hashtable		The hashtable.
 * @param var			The variant to put in the hashtable.
 * @param key			The key to be associated with the var in the hashtable.
 * @returns				std_exists if an item with the same key already exists.
 *						std_success otherwise.
 */
std_error_t hashtable_put(hashtable_t *hashtable_p, variant_t var, variant_t key)
{	
	/*
	 * If the hashtable already contains an item with the same key.
	 */
	if (hashtable_get(hashtable_p, key).vt != VT_EMPTY)
	{
		/*
		 * Delete the var + key since we own it.
		 */
		variant_destruct(&var);
		variant_destruct(&key);

		/*
		 * Fail.
		 */
		return std_exists;
	}

	return hashtable_put_fast(hashtable_p, var, key);
}

variant_t hashtable_get_stringkey(hashtable_t *hashtable_p, const char* key)
{
	variant_t var;
	
	variant_init_pointer(&var, VT_PSTR, (char*)key);

	return hashtable_get(hashtable_p, var);
}

/*
 * Checks to see if an element is in the hashtable by it's key.
 */
BOOL hashtable_exists(hashtable_t *hashtable_p, variant_t key)
{
	return hashtable_get(hashtable_p, key).vt != VT_EMPTY;
}

/*
 * Gets a variant from a hashtable given a key.
 *
 * @hashtable	The hashtable.
 * @key			The key of the variant to get.
 */
variant_t hashtable_get(hashtable_t *hashtable_p, variant_t key)
{
	UINT index;
	list_t *list;
	variant_t retval;
	
	/*
	 * Get the index of the list (of the hashtable) the varaint should be in.
	 */
	index = hashtable_tblindex(hashtable_p, key);

	list = hashtable_p->table[index];
	
	/*
	 * Search the list for the hashitem that has the same key as the key supplied.	 
	 */
	retval = list_find_with(list, key, variant_hashitem_comparator);

	/*
	 * If the item exists.
	 */
	if (retval.vt != VT_EMPTY)
	{
		/*
		 * Set the variant returned the the value contained in the hashitem.
		 */
		retval = retval.val.val_hashitem_p->var;
	}
	
	/*
	 * Success.
	 */
	return retval;
}

/*
 * Initializes a hashtable iterator.
 *
 * @param hashtable	The hashtable to iterate.
 * @param iterator	The iterator to initialize.
 */
void hashtable_init_iterator(hashtable_t *hashtable_p, hashtable_iterator_t *iterator)
{
	iterator->hashtable_p = hashtable_p;
	iterator->current_list_index = -1;
}

/*
 * Given a hashtable iterator, it determines if there's any more elements
 * in the hashtable to iterate.
 *
 * @param	hashtable_iterator	The hashtable iterator.
 * @returns TRUE if there are more elements, FALSE if there aren't.
 */
BOOL hashtable_hasmore(hashtable_iterator_t *hashtable_iterator_p)
{
	UINT i;

	/*
	 * If this is the first time the iterator has been used.
	 */
	if (hashtable_iterator_p->current_list_index == -1)
	{
		/*
		 * If the hashtable is empty.
		 */
		if (hashtable_iterator_p->hashtable_p->size <= 0)
		{
			/*
			 * Stop iterating.
			 */

			/*
			 * Success.
			 */
			return FALSE;
		}		
	}
	else
	{
		/*
		 * If sub list the hashtable is currently iterating has more.
		 */
		if (list_hasmore(&hashtable_iterator_p->list_iterator_p))
		{
			/*
			 * Get the next variant in the list.
			 */
			hashtable_iterator_p->var_p = &hashtable_iterator_p -> list_iterator_p.var_p -> val.val_hashitem_p -> var;

			/*
			 * Success.
			 */
			return TRUE;
		}
	}

	/*
	 * The last list in the hashtable has been fully iterated.
	 */

	/*
	 * Increment the list index so we can search the next list in the hashtable.
	 */
	hashtable_iterator_p->current_list_index++;

	/*
	 * If we've got to the end of size of the hashtable (no more lists).
	 */
	if (hashtable_iterator_p->current_list_index >= hashtable_iterator_p->hashtable_p->tblsize)
	{
		/*
		 * Stop iterating.
		 */

		/*
		 * Success.
		 */
		return FALSE;
	}

	/*
	 * Loops through all the lists in the hashtable from the current one to the end.
	 */
	for (i = hashtable_iterator_p->current_list_index; i < hashtable_iterator_p->hashtable_p->tblsize; i++)
	{
		/*
		 * If the current list isn't empty.
		 */
		if (hashtable_iterator_p->hashtable_p->table[i]->size > 0)
		{
			/*
			 * Set the hashtable's list iterator to iterate this list.
			 */
			list_init_iterator(hashtable_iterator_p->hashtable_p->table[i], &hashtable_iterator_p->list_iterator_p);
			
			/*
			 * Grab the first variant in the list.  It should store a hashitem.
			 */
			list_hasmore(&hashtable_iterator_p->list_iterator_p);

			/*
			 * Set the iterator's current variant (the thing the caller wants) to be the variant
			 * stored in the hashitem (stored in the variant we just got).
			 */
			hashtable_iterator_p->var_p = &hashtable_iterator_p->list_iterator_p.var_p->val.val_hashitem_p->var;
			
			/*
			 * Update the current list index in the iterator.
			 */
			hashtable_iterator_p->current_list_index = i;
			
			/*
			 * Stop iterating.
			 */

			/*
			 * Success.
			 */
			return TRUE;
		}
	}

	/*
	 * Stop iterating.
	 */

	/*
	 * Success.
	 */
	return FALSE;
}

/*
 * Determines the index of hashtable's table a given key would occupy.
 *
 * @param hashtable	The hashtable.
 * @param key		The key.
 *
 * @returns		The index of the table the key would occupy.
 */
UINT hashtable_tblindex(hashtable_t *hashtable_p, variant_t key)
{
	switch (key.vt)
	{
	case VT_PSTR:
		/*
		 * Key is a string, hash the string.
		 */
		return hash_string(key.val.val_sz) % hashtable_p->tblsize;	

	case VT_I4:
		/*
		 * Key is a number, hash the number.
		 */
		return key.val.val_i4 % hashtable_p->tblsize;	
	
	case VT_PVOID:
		/*
		 * Key is a void pointer, hash the pointer!
		 */
		return (UINT)key.val.val_void_p % hashtable_p->tblsize;

	default:
		/*
		 * Don't really know what in the variant they want as the key.
		 * Presume it's the lVal.
		 */
		return key.val.val_i4 % hashtable_p->tblsize;	

	}
}

/*
 * Hashes a string.
 */
UINT hash_string(const char *key_sz)
{
	UINT j = 0, h = 0;
	char *c = (char*)key_sz;
			
	/*
	 * Hashing strings by xoring every 4 bytes with the next 4 onto an integer.
	 */
	while(*c)
	{
		h ^= (*c++ << j);

		j += 8, j %= 32;
	}

	return h;
}

/*
 * Initialize a variant_t structure with the type specified.
 * Also assigns the variant a default destructor.
 *
 * @param var_p	Pointer to the variant to be initialized.
 * @param type	The type of the variant.
 *
 * @see typedef enum variant_types_e;
 * @see variant_assign_default_destructor(variant_t*)
 */
void variant_init(variant_t *var_p, variant_types_e type)
{
	var_p->vt = type;	

	variant_assign_default_destructor(var_p);
}

/*
 * Initialize a variant_t structure with the long specified.
 * Also assigns the variant a default destructor.
 *
 * @param var	Pointer to the variant to be initialized.
 * @param i4	The long to assign to the variant.
 *
 * @see typedef enum variant_types_e;
 * @see variant_assign_default_destructor(variant_t*)
 */
void variant_init_i4(variant_t *var_p, long i4)
{
	var_p->vt = VT_I4;	
	var_p->val.val_i4 = i4;

	variant_assign_default_destructor(var_p);
}

/*
 * Initialize a variant_t structure with the type and void* specified.
 * Also assigns the variant a default destructor.
 *
 * @param var		Pointer to the variant to be initialized.
 * @param type		The type of the variant.
 * @param pointer	The void pointer to assign to the variant.
 *
 * @see typedef		enum variant_types_e;
 * @see				variant_assign_default_destructor(variant_t*)
 */
void variant_init_pointer(variant_t *var_p, variant_types_e type, void *pointer)
{
	var_p->vt = type;	
	var_p->val.val_void_p = pointer;

	variant_assign_default_destructor(var_p);
}

/*
 * Calls a variant's destructor if it has one.
 *
 * @param var	The variant.
 */
void variant_destruct(variant_t *var_p)
{
	/*
	 * If the variant has a destructor
	 */
	if (var_p->destructor)
	{
		/*
		 * Call it.
		 */
		var_p->destructor(var_p);
	}
}

/*
 * Prints out a variant.
 *
 * @param var	The variant.
 */
void variant_print(variant_t *var_p)
{
	switch (var_p->vt)
	{
	case VT_PSTR:
		printf("%s", var_p->val.val_sz);
		break;
	
	case VT_NULL:
		printf("NULL");
		break;
	
	case VT_EMPTY:
		printf("EMPTY");
		break;
	
	default:
		printf("UNKOWN");
		break;

	}
}

/*
 * Assigns a default destructor to a variant.  If the variant contains a string,
 * then a string destructor will be associated etc.
 *
 * @param var	The variant.
 */
void variant_assign_default_destructor(variant_t *var_p)
{	
	switch (var_p->vt)	
	{
	case VT_PSTR:
		var_p->destructor = variant_string_destructor;
		break;

	case VT_PHASHITEM:
		var_p->destructor = variant_hashitem_destructor;
		break;

	case VT_PVOID:
		var_p->destructor = variant_pointer_destructor;

	default:
		var_p->destructor = variant_dynamic_destructor;	

	}	
}

/*
 * Frees up the string the variant holds.
 */
void variant_string_destructor(variant_t *var_p)
{
	if (var_p->vt != VT_PSTR)
	{
		variant_dynamic_destructor(var_p);
		return;
	}

	free(var_p->val.val_sz);
}

/*
 * Frees up the hashitem_t the variant holds.
 */
void variant_hashitem_destructor(variant_t *var_p)
{
	if (var_p->vt != VT_PHASHITEM)
	{
		variant_dynamic_destructor(var_p);
		return;
	}

	hashitem_delete(var_p->val.val_hashitem_p);
}

/*
 * Frees up the hashitem_t the variant holds.
 */
void variant_pointer_destructor(variant_t *var_p)
{
	if (var_p->vt != VT_PVOID)
	{
		variant_dynamic_destructor(var_p);
		return;
	}

	free(var_p->val.val_void_p);
}

/*
 * Dynamically destroy a variant based on it's type.
 *
 */
void variant_dynamic_destructor(variant_t *var_p)
{
	variant_t var_cp;

	/*
	 * Make a copy of the variant, and assign it the default destructor.
	 */
	var_cp = *var_p;	
	variant_assign_default_destructor(&var_cp);

	/*
	 * If the default destructor is found (eg. wasn't this dynamic one)
	 */
	if (var_cp.destructor != variant_dynamic_destructor)
	{
		/*
		 * Call that destructor.
		 */
		var_cp.destructor(var_p);
	}
}

/*
 * Comparator used to test a variant with a hashitem's key.
 *
 * @v1_p		variant_t that is the key to find.
 * @v2_p		variant_t that contains a hashitem.
 * @returns		0 if the key in v1 and key in v2 match.
 *				Nonzero otherwise.
 */
int variant_hashitem_comparator(variant_t *v1_p, variant_t *v2_p)
{	
	return variant_dynamic_comparator(v1_p, &v2_p->val.val_hashitem_p->key);
}

/*
 * Comparator that compares two void pointers.
 *
 * @param	variant_t that holds the first void pointer.
 * @param	variant_t that holds the second oid pointer.
 *
 * @returns	Negative if the first is less than the second.
 *			Positive if the first is more then the second.
 *			Zero if they are the same.
 */
int variant_pointer_comparator(variant_t *v1_p, variant_t *v2_p)
{
	return (char*)v1_p->val.val_void_p - (char*)v2_p->val.val_void_p;
}

/*
 * Dynamically compares two variants based on what they contain.
 *
 * @param	v1 First variant to compare.
 * @param	v2 Second variant to compare.
 * @returns	Negative if the first is less than the second,
 *			Positive if the first is more than the second,
 *			and 0 is they are equal.
 */
int variant_dynamic_comparator(variant_t *v1_p, variant_t *v2_p)
{
	/*
	 * If they are both longs.
	 */
	if (v1_p->vt == VT_I4 && v2_p->vt == VT_I4)
	{
		/*
		 * Compare the longs.
		 */
		return v1_p->val.val_i4 - v2_p->val.val_i4;
	}
	/*
	 * If they are both strings.
	 */
	else if (v1_p->vt == VT_PSTR && v2_p->vt == VT_PSTR)
	{
		/*
		 * Compare the strings.
		 */
		return strcmp(v1_p->val.val_sz, v2_p->val.val_sz);
	}
	/*
	 * If they are something else.
	 */
	else
	{
		/*
		 * Compare the void pointers.
		 */
		return (char*)v1_p->val.val_void_p - (char*)v2_p->val.val_void_p;
	}
}