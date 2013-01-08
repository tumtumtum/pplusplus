#ifndef _PLZERO_H
#define _PLZERO_H

#include "stdio.h"
#include "hashtable.h"
#ifdef _WIN32
#include "direct.h"
#else
#include <unistd.h>
#endif

#if _WIN32 || _WIN64
#ifdef _WIN64
#define LONGLONG long long
#else
#define LONGLONG int
#endif
#else
#if __x86_64__ || __ppc64__
#define LONGLONG long long int
#else
#define LONGLONG int
#endif
#endif

#ifdef _DEBUG
#define ASSERT(a, b, c)	\
	if (!(a)) printf(b, c);
#define ASSERT1(a, b, c, d)	\
	if (!(a)) printf(b, c, d);
#else
#define ASSERT(a, b, c)
#define ASSERT1(a, b, c, d)
#endif

#define ZEROMEMORY(x) (memset(&(x), 0, sizeof(x)))

#define MAX_IDENT_LEN				0x100
#define MAX_WORD_NAME				0x100
#define MAX_NUMBER_DIGITS			0x20
#define MAX_INSTRUCTIONS			0x50000
#define MAX_IDENTS					0x5000
#define MAX_LEVEL					0x100
#define MAX_LITERAL_STRING_LEN		0x10000
#define MAX_PATH					0x1000

/*
 * ARRAY_HEADER_SIZE includes the MEMORY_HEADER_SIZE (2)
 */
#define ARRAY_HEADER_SIZE			0x4
#define ARRAY_LENGTH_OFFSET			0x2

#define EXIT_FOR			0x1
#define EXIT_WHICH			0x4
#define EXIT_REPEAT			0x5
#define EXIT_WHILE			0x6
#define EXIT_DO				0x7
#define EXIT_ANY			0x1ff

#define CONT_FOR			0x200
#define CONT_WHICH			0x201
#define CONT_REPEAT			0x202
#define CONT_WHILE			0x203
#define CONT_DO				0x204
#define CONT_ANY			0x2ff

#define JMP_RETURN			0x1000
#define GOTO_LABEL			0x2000
#define LITERAL_STRING			0x3000

typedef struct tag_word_sym word_t; 
typedef struct tag_instruction instruction_t;
typedef struct tag_indentinfo identinfo_t;
typedef struct tag_var var_t;
typedef struct tag_file_info file_info_t;
typedef struct tag_var_int var_int_t;
typedef struct tag_var_float var_float_t;
typedef struct tag_var_procedure var_procedure_t;
typedef struct tag_var_function var_function_t;
typedef struct tag_var_array var_array_t;
typedef struct tag_paraminfo paraminfo_t;
typedef struct tag_tag tag_t;
typedef struct tag_options options_t;
typedef struct tag_compiler_results compiler_results_t;
typedef struct tag_exp_res exp_res_t;

compiler_results_t compile(const char *file);

#define OC_FN_CAL	-OC_CAL
#define OC_FN_LOD	-OC_LOD
#define OC_FN_LIT	-OC_LIT

struct tag_compiler_results
{
	int lines_compiled;
	int number_of_errors;
	int opcodes_generated;
	instruction_t *instructions;
};

struct tag_options
{	
	BOOL quiet;
	BOOL verbose;
	BOOL list_opcodes;
	BOOL supress_errors;	
	BOOL interpret;
	BOOL list_source;
	BOOL pause;
	BOOL trace_stack;
	BOOL array_check_boundaries;
	BOOL optimize;
	BOOL write_binary;

	char source_path[2048];
	char command_line[2048];
};

options_t m_options;

struct tag_file_info
{
	FILE *file;
	int line_index;
	int line_length;
	int line_count;
	char file_name[512];
};

struct tag_word_sym
{
	char name[MAX_WORD_NAME];
	UINT symbol;
};

struct tag_instruction
{
	int f;
	int l;
	LONGLONG a;
};

struct tag_var_int
{	
	int default_val;
	int is_handle;	
	int is_character;
	int is_boolean;
};

struct tag_var_float
{
	float default_val;
};

struct tag_var_procedure
{
	BOOL byref;

	int params_num;
	int param_types[32];
	int param_tags[32];
};

struct tag_paraminfo
{
	int type;
	BOOL byref;
};

struct tag_indentinfo
{
	char name[MAX_IDENT_LEN + 1];

	int kind;  	
	int level;
	int refed;
	BOOL constant;
	BOOL byref;
	BOOL inmemory;
	int address;
	BOOL is_this;
	BOOL is_param;
	BOOL was_null;
	BOOL has_default;
	BOOL clean;
	
	union
	{		
		var_int_t int_val;
		var_float_t float_val;
		var_function_t *function_val_p;
		var_array_t *array_val_p;
	}
	data;	
};


struct tag_exp_res
{	
	int type;	
	identinfo_t ident;
};

struct tag_var_function
{	
	BOOL byref;

	int params_num;
	identinfo_t *default_p;
	
	identinfo_t params[32];	
	identinfo_t return_var;
};

struct tag_var_array
{
	int size;

	identinfo_t contains;
	BOOL byref;	
	BOOL assigning_ref;
	BOOL is_literal;
	BOOL was_null;
	BOOL is_big_number;
	
	int *array_default;

	int num_dims;
	identinfo_t* dims[5];
};

struct tag_tag
{
	BOOL lvalue_nogc; /* l-value needed, don't garbage collect*/
	BOOL dont_getsymbol;
	BOOL no_logical_and_or;
	BOOL statement;
	identinfo_t *ident_p;
};

typedef exp_res_t (*processing_fn)(int level, int *ptx, tag_t tag);

/*
 * Kind definitions.
 */
#define KIND_UNKNOWN		0x0
#define KIND_NULL			0x1
#define KIND_VOID			0x2
#define KIND_INTEGER		0x3
#define KIND_PROCEDURE		0x4
#define KIND_FUNCTION		0x5
#define KIND_LABEL			0x6
#define KIND_FLOAT			0x7
#define KIND_STRING			0x8
#define KIND_ARRAY			0x100

char m_parsed_files[100][MAX_PATH];
int m_num_parsed_files;

char m_char;
int m_symbol;
int m_last_symbol;
int m_number_of_errors;
BOOL m_ignore_esc_chars;
identinfo_t m_literal_string;
int m_line_count;
char m_line[1024];
char m_last_line[1024];
int m_instructions_index;
float m_number;
tag_t m_empty_tag;
int m_last_symbol_len;
int m_number_kind;
BOOL m_is_char;
void statement_asm(int level, int *ptx);
char m_string[MAX_LITERAL_STRING_LEN];
int m_string_length;
char m_ident_name[MAX_IDENT_LEN + 1];
instruction_t m_instructions[MAX_INSTRUCTIONS];
identinfo_t m_idents[MAX_IDENTS];
identinfo_t *m_level_gcidents[MAX_IDENTS];
int m_num_level_gcidents;

exp_res_t exp_float;
exp_res_t exp_integer;
exp_res_t exp_unknown;
exp_res_t exp_string;
exp_res_t exp_bignumber;
exp_res_t m_empty_res;
exp_res_t m_exp_null;
char m_message[MAX_IDENT_LEN * 2];

hashtable_t *m_syms_p;
hashtable_t *m_types_p;
hashtable_t *m_nonalpha_syms_p;




BOOL load_file(const char *file);
void error(int number);
void get_symbol();
void error_extra(int number, char *extra, BOOL show_code, BOOL last_line);
void boolate_top(int level, int *ptx, exp_res_t exp_res);
void logical_not_top(int level, int *ptx, exp_res_t exp_res);
void array_load_offset(identinfo_t *ident_p, int level, int *ptx, BOOL aggregate);
void instructions_add_itof(int f, int l, int a, int kind);
void instructions_fix_jumps(int start, int end, int jmptype, int dest);
void instructions_fix_jump_decs(int start, int end, int decamount);
void instructions_add_itof(int f, int l, int a, int kind);
void check_expression_start();
void identinfo_delete(identinfo_t *ident_p);
void identinfo_delete(identinfo_t *ident_p);
void memory_addref(identinfo_t *ident_p, int level, int *ptx);
void memory_release(identinfo_t *ident_p, int level, int *ptx);
void ident_read_type(identinfo_t *ident_p, int level, int *ptx);
char escape_char(char ch, BOOL *read_char);
int condition_operator(int symbol, int kind, tag_t tag);
int process_args(int level, int *ptx, identinfo_t *ident_p, BOOL aggregate);
int process_params(int level, int *ptx, int *pdx, identinfo_t *ident_p);
void instructions_add_mos(int f, int l, int a, BOOL memory);

void statement(int level, int *ptx, int *pdx, identinfo_t *ident_p);
exp_res_t statement_becomes(identinfo_t *ident_p, int level, int *ptx, BOOL push, tag_t tag);
BOOL signatures_match(var_function_t *func_p1, var_function_t *func_p2);
identinfo_t *ident_add_any(identinfo_t *ident_p, int *ptx, int *pdx);
word_t *add_symbol(hashtable_t *hashtable_p, const char* name_p, UINT symbol);
var_function_t *var_function_clone(var_function_t *function_p);
identinfo_t *ident_process_var(int level, int *ptx, int *pdx, BOOL byref, BOOL statement);
var_array_t *var_array_clone(var_array_t *array_p);
BOOL can_be_string(identinfo_t *ident_p);
BOOL can_be_big_number(identinfo_t *ident_p);

exp_res_t do_static_cast(int level, int *ptx, exp_res_t from, exp_res_t to);
exp_res_t function_procedure_caller(int level, int *ptx, identinfo_t *ident_p, BOOL aggregate);



void memory_release_by_handle_off_stack(int level, int *ptx);
void memory_addref_by_handle_off_stack(int level, int *ptx);
void process_pragma(int level, int *ptx);
void fix_gotos(int start, int end, int level, int *ptx, BOOL warn);
void process_statement_block(int level, int *ptx, int *pdx, identinfo_t *func_ident_p);

void itos(int level, int *ptx);
void ctos(int level, int *ptx);
void btos(int level, int *ptx);
void ftos(int level, int *ptx);
void stoi(int level, int *ptx);
void stof(int level, int *ptx);

void unary_operation_function_caller(const char *function_name, int level, int *ptx);
void binary_operation_function_caller(const char *function_name, int level, int *ptx);
void xtos(int level, int *ptx, exp_res_t current_res);
void stox(int level, int *ptx, exp_res_t current_res);
int* static_arraydef(identinfo_t *ident_p, int level, int *ptx);
void load_array(const int *string, int level, int *ptx);
void load_new_array(const int *string, int level, int *ptx);
void block_var(int level, int *ptx, int *pdx, BOOL statement);
void set_defaults(int level, int *ptx, int sx, int ex);
void instructions_add(int f, int l, LONGLONG a);
void pop_kind(int kind, int level, int *ptx);
identinfo_t *ident_find(char *name, int *ptx);
void pass_or_return_function(identinfo_t *ident_p, int level, int *ptx);
void load_array_memory_address(identinfo_t *ident_p, int level, int *ptx);
void load_ident(identinfo_t* ident_p, int level, int *ptx);
void instructions_add_f(int f, float l, float a);
identinfo_t *type_find(const char* name);
int get_kind();
int *chars2ints(const char *c_p);
exp_res_t load_number(int level, int *ptx);
void absolute_top(int kind);
exp_res_t expression_lambda(int level, int *ptx);



#define MAX(a, b)	((a > b) ? a : b)
#define MIN(a, b)	((a < b) ? a : b)

#define SET_EXP_RES(er, id) er.type = (id).kind; er.ident = id;


#endif

