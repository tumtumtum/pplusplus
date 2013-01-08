/*
 * This file contains definitions for use in a hashtable.
 * Almost all of this was written for my COSC 204 assignment.
 */

#include "types.h"

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

/* * * * * * * *
 * Macro definitions
 */

#define TRUE	1
#define FALSE	0

#define min(a,b)    (((a) < (b)) ? (a) : (b))
/* 
 * The default number of linked lists that make up the hashtable.
 */
#define DEFAULT_HASHTABLE_SIZE	257 /* 257, 101 */
/*
 * Allocates enough cleared (zeroed) memory for n numbers of a type.
 */
#define CALLOC(type, n) ((type*)calloc(sizeof(type), n))

/*
 * Allocates enough memory for n numbers of a type.
 */
#define MALLOC(type, n) ((type*)malloc(sizeof(type) * (n)))

/*
 * Tests to see if a varaint_t is empty. (holds nothing)
 */
#define VAR_EMPTY(x) (x.vt == VT_EMPTY)

/* * * * * * * *
 * Type definitions.
 */



/*
 * The following typedefs are for structures used throughout
 * the assignment.  These are defined later on in this file.
 */
typedef struct tag_variant variant_t;
typedef struct tag_hashitem hashitem_t;
typedef struct tag_assessment_item assessment_item_t;
typedef struct tag_assessment_mark assessment_mark_t;
typedef struct tag_student student_t;
typedef struct tag_list_node list_node_t;
typedef struct tag_list list_t;
typedef struct tag_hashtable hashtable_t;

/* * * * * * * *
 * The following typedefs are for functions.
 */

/*
 * Destroys a variant_t by freeing any dynamically allocated
 * memory the variant_t may point to.
 *
 * @param variant	The variant to destroy
 */
typedef void (*variant_destructor_fn) (variant_t* variant);

/*
 * Compares two variant_ts.
 * 
 * @param v1	The first variant_t to compare.
 * @param v2	The second variant_t to compare.
 * @returns		The function should return a negative number if
 *				v1 is less than v2, a positive number of v1 is
 *				more than v2, and zero if v1 is the same as v2.
 */
typedef int (*variant_comparator_fn) (variant_t* v1, variant_t* v2);


/* * * * * * * *
 * Enumeration definitions
 */

/*
 * Enumeration of the types that a variant_t can store.
 */
typedef enum
{
	VT_EMPTY = 0x0,			/* Nothing				*/
	VT_NULL	= 0x1,			/* NULL					*/
	VT_PVOID = 0x2,			/* void*				*/
	VT_I4 = 0x6,			/* int					*/
	VT_PSTR	= 0xa0,			/* char* or LPSTR		*/	
	VT_PHASHITEM = 0xa2		/* hashitem_t*			*/
}
variant_types_e;



/* * * * * * * *
 * Structure definitions
 */

/*
 * "Variant" data type.
 * Used in many places to store different types of values.
 */
struct tag_variant
{
	variant_types_e vt;	

	union
	{
		long val_i4;								/* VT_I4		*/
		void* val_void_p;							/* VT_PVOID		*/
		char* val_sz;								/* VT_PSTR		*/				
		hashitem_t* val_hashitem_p;					/* VT_PHASHITEM */		
	}
	val;
	/* ANSI C doesn't support anonymous inner unions */

	variant_destructor_fn destructor;
};

/*
 * Hashtables store hashitems in linkedlists.
 * A hashitem_t stores a variant_t that is the key for the value
 * and also a variant_t that is the actual value.
 *
 * @see variant_t
 */
struct tag_hashitem
{
	variant_t key;
	variant_t var;
};

/*
 * A node in a linked list.
 * Each node stores a value as a variant_t.
 */
struct tag_list_node
{
	variant_t var;
	list_node_t *next_p;	
};

/*
 * A linked list made up of zero or more list_node_t nodes.
 *
 * @see list_node_t
 */
struct tag_list
{
	UINT size;

	list_node_t *first_p;	/* First node of the linked list	*/
	list_node_t *last_p;	/* Last node of the linked list		*/
};

/*
 * A hashtable made up of a table (array) of linked lists.
 *
 * @see list_t
 */
struct tag_hashtable
{
	UINT size;
	UINT tblsize;		/* size of the hashtable's table	*/

	list_t ** table;	/* array of linked list pointers (the table)*/
};

/* * * * * * * *
 * The following structs are anonymous, and typedefed immediately.
 * No other structs refer to them, so it's tidier this way.
 */

/*
 * Used to help iterate through a list.
 *
 * @see list_make_iterator(list_iterator_t*)
 */
typedef struct
{	
	list_t *list_p;					/* Pointer to the list being iterated */	
	list_node_t *current_node_p;	/* Pointer to the current node */
	variant_t *var_p;				/* Pointer to the current value */
}
list_iterator_t;

/*
 * Used to help iterate through a hashtable.
 *
 * @see hashtable_make_iterator(hashtable_iterator_t*)
 */
typedef struct
{	
	UINT current_list_index;			/* The current list index in the hashtable */

	hashtable_t *hashtable_p;			/* Pointer to the hashtable being iterated */
	list_iterator_t list_iterator_p;	/* The list iterator for the current list being iterated */	
	variant_t* var_p;					/*Pointer to the current value */
}
hashtable_iterator_t;







typedef enum
{
	std_success,
	std_error,
	std_exists,
	std_malloc_fail
}
std_error_t;


/*
 * Functions used for linked lists.
 */

void list_node_delete(list_node_t *node_p);
list_t *list_new();
void list_delete(list_t *list_p);
void list_print(list_t *list_p);
list_node_t *list_node_new(variant_t var);
void list_node_delete(list_node_t *node_p);
variant_t list_find(list_t *list_p, variant_t var);
variant_t list_find_with(list_t *list_p, variant_t var, variant_comparator_fn comparator);
std_error_t list_add(list_t *list_p, variant_t var);
void list_init_iterator(list_t *list_p, list_iterator_t *iterator_p);
BOOL list_hasmore(list_iterator_t *list_iterator_p);

/*
 * Functions used for hashtables.
 */

hashtable_t *hashtable_new(int tblsize);
void hashtable_delete(hashtable_t *hashtable_p);
hashitem_t *hashitem_new(variant_t var, variant_t key);
void hashitem_delete(hashitem_t *hashcell_p);
std_error_t hashtable_put(hashtable_t *hashtable_p, variant_t var, variant_t key);
std_error_t hashtable_put_fast(hashtable_t *hashtable_p, variant_t var, variant_t key);
variant_t hashtable_get(hashtable_t *hashtable_p, variant_t key);
variant_t hashtable_get_stringkey(hashtable_t *hashtable_p, const char* key);
void hashtable_init_iterator(hashtable_t *hashtable_p, hashtable_iterator_t *iterator_p);
BOOL hashtable_hasmore(hashtable_iterator_t *hashtable_iterator_p);
UINT hash_string(const char *key_sz);
UINT hashtable_tblindex(hashtable_t *hashtable_p, variant_t key);
BOOL hashtable_exists(hashtable_t *hashtable_p, variant_t key);

/*
 * Functions for variants and structures stored within.
 */

void variant_init(variant_t *var_p, variant_types_e type);
void variant_init_i4(variant_t *var_p, long i4);
void variant_init_pointer(variant_t *var_p, variant_types_e type, void* pointer_p);
void variant_destruct(variant_t *var_p);
void variant_print(variant_t *var_p);
void variant_assign_default_destructor(variant_t *var_p);
void variant_string_destructor(variant_t *var_p);
void variant_hashitem_destructor(variant_t *var_p);
void variant_pointer_destructor(variant_t *var_p);
void variant_dynamic_destructor(variant_t *var_p);
int variant_hashitem_comparator(variant_t *v1_p, variant_t *v2_p);
int variant_dynamic_comparator(variant_t *v1_p, variant_t *v2_p);

#endif