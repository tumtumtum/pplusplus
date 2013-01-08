/**
 * P++ Compiler & Interpretor.
 *
 * Copyright May 2000 Thong Nguyen (thongnguyen@mail.com).
 *
 * You may not use this work without prior permission from the author.
 *
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "compiler.h"
#include "symbols.h"
#include "compiler.h"
#include "math.h"
#include "ctype.h"
#include "time.h"
#include "interpretor.h"
#include "expressions.h"

static file_info_t m_files_stack[100];
static int m_files_stack_index = -1;

#ifndef WINDOWS
char *_strdup(const char *s)
{
	char *s2;

	s2 = (char *)malloc(strlen(s) + 1);

	if (!s2)
	{
		return NULL;
	}

	strcpy(s2, s);

	return s2;
}
#endif

#ifndef _WIN32
char* _fullpath(char *dest, const char *src, size_t max_length)
{
	return strncpy(dest, src, max_length);
}
#endif

void printout()
{
	print_out_code(m_instructions, m_instructions_index);
}

void error(int number)
{
	error_extra(number, "", TRUE, FALSE);
}

int sizeof_kind(int kind)
{
	switch (kind)
	{
	case KIND_INTEGER:
	case KIND_FLOAT:
	case KIND_ARRAY:

		return 1;
	}

	return 0;
}


void generate_stp(identinfo_t *ident_p)
{
	instructions_add(OC_STP, 0, 3 + sizeof_kind(ident_p->data.function_val_p->return_var.kind));
}

void generate_stp_aggregate(identinfo_t *ident_p)
{
	instructions_add(OC_STP, 0, 2 + sizeof_kind(ident_p->data.function_val_p->return_var.kind));
}

void generate_load(identinfo_t *ident_p, int level)
{
	if (ident_p->byref)
	{
		instructions_add(OC_LID, level - ident_p->level, ident_p->address);
	}
	else
	{
		instructions_add(OC_LOD, level - ident_p->level, ident_p->address);
	}
}

void generate_call_func(identinfo_t *ident_p, int level)
{
	instructions_add(OC_LIT, 0, 0);
	
	if (ident_p->address < 0)
	{
		instructions_add(OC_FN_CAL, level - ident_p->level, (LONGLONG)ident_p);
	}
	else
	{
		instructions_add(OC_CAL, level - ident_p->level, ident_p->address);
	}
}

void generate_call_sub(identinfo_t *ident_p, int level)
{
	generate_call_func(ident_p, level);

	instructions_add(OC_DEC, 0, 1);
}

void error_extra(int number, char *extra, BOOL show_code, BOOL last_line)
{
	char *line;
	int i = 0, j = 0, len;
	
	m_number_of_errors++;

	if (last_line)
	{
		line = m_last_line;
	}
	else
	{
		line = m_line;
	}


	if (m_options.supress_errors)
	{
		return;
	}

	len = strlen(line);

	for (i = i; i < len; i++)
	{
		if (m_line[i] != '\t')
		{
			printf("%c", line[i]);
		}
		else
		{
			j++;
		}
	}
	
	/* 
	 * Don't count the tabs, or the fact that we normally read ahead a char, and the
	 * counter is at the next char (eg. -2)
	 */

	if (m_files_stack_index >= 0)
	{
		if (show_code)
		{		
			len = m_files_stack[m_files_stack_index].line_index - j - m_last_symbol_len;
		
			for (i = 0; i < MAX(len, 0); i++)
			{
				printf(" ");
			}

			printf("^\n");
		}
		else
		{
		}

		printf("<error %d in file [%s], line ~%d>\n", number, m_files_stack[m_files_stack_index].file_name, m_files_stack[m_files_stack_index].line_count);
	}

	for (i = 0; i < COMPILER_ERRORS_SIZE; i++)
	{
		if (compiler_errors[i].number == number)
		{
			if (number == 4000)
			{
				strcpy(m_message, " but got <");
				strcat(m_message, m_ident_name);
				strcat(m_message, ">");

				extra = m_message;				
			}
			
			printf("[%s%s]\n\n", compiler_errors[i].description, extra);
			
			break;
		}		
	}
}

/*
 * Creates a new array.
 */
var_array_t *var_array_new()
{
	var_array_t *array;

	array = CALLOC(var_array_t, 1);
	
	return array;
}

/*
 * Deletes an array.
 */
void var_array_delete(var_array_t *array)
{
	int i;

	for (i = 0; i < array->num_dims; i++)
	{
		if (array->dims[i]->kind == KIND_ARRAY)
		{
			identinfo_delete(array->dims[i]);
		}
	}

	if (array->array_default)
	{
		free(array->array_default);
	}

	identinfo_delete(&array->contains);

	free(array);
}

/*
 * Creates a new identinfo.
 */
identinfo_t *identinfo_new()
{
	identinfo_t *ident_p;

	ident_p = CALLOC(identinfo_t, 1);

	return ident_p;
}

/*
 * Deletes an identinfo.
 */
var_function_t *var_function_new()
{
	var_function_t *function;

	function = CALLOC(var_function_t, 1);
		
	return function;
}

/*
 * Copies an identinfo,
 */
identinfo_t ident_copy(identinfo_t ident)
{
	identinfo_t ident2;

	ident2 = ident;
	
	switch (ident.kind)
	{
	case KIND_ARRAY:
		
		ident2.data.array_val_p = var_array_clone(ident.data.array_val_p);

		break;

	case KIND_FUNCTION:

		ident2.data.function_val_p = var_function_clone(ident.data.function_val_p);
		
		break;

	}

	return ident2;
}

/*
 * Clones an array.
 */
var_array_t *var_array_clone(var_array_t *array_p)
{
	var_array_t *array_p2;

	array_p2 = var_array_new();

	*array_p2 = *array_p;

	array_p2->contains = ident_copy(array_p->contains);

	if (array_p->array_default)
	{
		array_p2->array_default = MALLOC(int, *array_p->array_default + 1);
		memcpy(array_p2->array_default, array_p->array_default, sizeof(int) * (*array_p->array_default + 1));
		
	}

	return array_p2;
}

/*
 * Clones a function.
 */
var_function_t *var_function_clone(var_function_t *function_p)
{
	int i;
	var_function_t *function_p2 = var_function_new();

	function_p2 = CALLOC(var_function_t, 1);

	*function_p2 = *function_p;

	function_p2->byref = function_p->byref;	
	function_p2->params_num = function_p->params_num;

	function_p2->return_var = ident_copy(function_p->return_var);	

	function_p->default_p = function_p->default_p;
	
	for (i = 0; i < function_p2->params_num; i++)
	{
		function_p2->params[i] = ident_copy(function_p->params[i]);
	}

	return function_p2;
}

/*
 * Deletes a function.
 */
void var_function_delete(var_function_t *function_p)
{	
	int i;

	for (i = 0; i < function_p->params_num; i++)
	{
		identinfo_delete(&function_p->params[i]);
	}

	identinfo_delete(&function_p->return_var);

	free(function_p);
}

/*
 * Deletes an identinfo.
 */
void identinfo_delete(identinfo_t *ident_p)
{
	switch (ident_p->kind)
	{
	case KIND_FUNCTION:
		
		var_function_delete(ident_p->data.function_val_p);
		
		break;

	case KIND_ARRAY:

		var_array_delete(ident_p->data.array_val_p);
		
		break;
	}
}

/*
 * Creates a new word.
 */
word_t *word_new(const char* name_p, UINT symbol)
{
	word_t *word_p;

	word_p = MALLOC(word_t, 1);

	if (!word_p)
	{
		return NULL;
	}

	strncpy(word_p->name, name_p, MAX_WORD_NAME);

	word_p->symbol = symbol;

	return word_p;
}

/*
 * Adds a symbol to the symbol hashtable.
 */
word_t *add_symbol(hashtable_t *hashtable_p, const char* name_p, UINT symbol)
{
	word_t *word_p;
	variant_t var, key;

	word_p = word_new(name_p, symbol);

	variant_init_pointer(&var, VT_PVOID, (void*)word_p);
	variant_init_pointer(&key, VT_PSTR, (void*)_strdup(word_p->name));

	hashtable_put(hashtable_p, var, key);

	return word_p;
}

/* 
 * Initializes all the variables.
 */
void init()
{
	/*
	 * Default starting character is space so that the first character is read
	 *
	 * @see get_symbol()
	 */

	m_char = ' ';
	m_line_count = 0;
	m_num_level_gcidents = 0;
	m_instructions_index = 0;

	m_last_symbol_len = 0;
	m_num_parsed_files = 0;
	m_number_of_errors = 0;
	m_ignore_esc_chars = FALSE;
	m_last_symbol = SYM_NOSYM;
	
	ZEROMEMORY(m_instructions);
	ZEROMEMORY(m_parsed_files);
	ZEROMEMORY(m_literal_string);
	
	ZEROMEMORY(exp_integer);
	ZEROMEMORY(exp_unknown);
	ZEROMEMORY(exp_float);
	ZEROMEMORY(exp_string);
	ZEROMEMORY(exp_bignumber);
	ZEROMEMORY(m_exp_null);

	ZEROMEMORY(m_empty_tag);
	ZEROMEMORY(m_empty_res);

	exp_integer.type = KIND_INTEGER;
	exp_float.type = KIND_FLOAT;
	exp_unknown.type = KIND_UNKNOWN;
	
	/*
	 * Used to indicate the type of expression is a literal string.
	 */
	m_literal_string.kind = KIND_ARRAY;
	m_literal_string.data.array_val_p = var_array_new();
	m_literal_string.data.array_val_p->is_literal = TRUE;	
	m_literal_string.data.array_val_p->contains.kind = KIND_INTEGER;


	/*
	 * Used to indicate the type of expression is a literal string.
	 */
	exp_string.type = KIND_ARRAY;
	exp_string.ident.kind = KIND_ARRAY;	
	exp_string.ident.data.array_val_p = var_array_new();
	exp_string.ident.data.array_val_p->contains.kind = KIND_INTEGER;
	exp_string.ident.data.array_val_p->contains.data.int_val.is_character = TRUE;

	exp_bignumber.type = KIND_ARRAY;
	exp_bignumber.ident.kind = KIND_ARRAY;	
	exp_bignumber.ident.data.array_val_p = var_array_new();
	exp_bignumber.ident.data.array_val_p->contains.kind = KIND_INTEGER;
	exp_bignumber.ident.data.array_val_p->is_big_number = TRUE;
	
	
	m_exp_null.type = KIND_NULL;
	
		
	/*
	 * Initialize symbol hashtables.
	 */
	m_types_p = hashtable_new(DEFAULT_HASHTABLE_SIZE);
	m_syms_p = hashtable_new(DEFAULT_HASHTABLE_SIZE);
	m_nonalpha_syms_p = hashtable_new(DEFAULT_HASHTABLE_SIZE);
		
	/*
	 * Symbols for the P++ language.
	 */
	add_symbol(m_syms_p, "begin", SYM_BEGIN);
	add_symbol(m_syms_p, "end", SYM_END);	
	add_symbol(m_syms_p, "call", SYM_CALL);
	add_symbol(m_syms_p, "const", SYM_CONST);
	add_symbol(m_syms_p, "do", SYM_DO);	
	add_symbol(m_syms_p, "if", SYM_IF);
	add_symbol(m_syms_p, "else", SYM_ELSE);
	add_symbol(m_syms_p, "odd", SYM_ODD);
	add_symbol(m_syms_p, "procedure", SYM_PROCEDURE);
	add_symbol(m_syms_p, "then", SYM_THEN);
	add_symbol(m_syms_p, "var", SYM_VAR);
	add_symbol(m_syms_p, "while", SYM_WHILE);	
	add_symbol(m_syms_p, "integer", SYM_INTEGER);
	add_symbol(m_syms_p, "for", SYM_FOR);
	add_symbol(m_syms_p, "to", SYM_TO);
	add_symbol(m_syms_p, "step", SYM_STEP);
	add_symbol(m_syms_p, "exit", SYM_EXIT);
	add_symbol(m_syms_p, "function", SYM_FUNCTION);
	add_symbol(m_syms_p, "return", SYM_RETURN);
	add_symbol(m_syms_p, "goto", SYM_GOTO);
	add_symbol(m_syms_p, "include", SYM_INCLUDE);	
	add_symbol(m_syms_p, "using", SYM_INCLUDE);
	add_symbol(m_syms_p, "lambda", SYM_LAMBDA);
	add_symbol(m_syms_p, "repeat", SYM_REPEAT);
	add_symbol(m_syms_p, "which", SYM_WHICH);
	add_symbol(m_syms_p, "case", SYM_CASE);
	add_symbol(m_syms_p, "ref", SYM_REF);
	add_symbol(m_syms_p, "val", SYM_VAL);	
	add_symbol(m_syms_p, "asm", SYM_ASM);
	add_symbol(m_syms_p, "machine_code", SYM_ASM);	
	add_symbol(m_syms_p, "float", SYM_FLOAT);
	add_symbol(m_syms_p, "void", SYM_VOID);		
	add_symbol(m_syms_p, "handle", SYM_HANDLE);
	add_symbol(m_syms_p, "string", SYM_STRING);
	add_symbol(m_syms_p, "bignumber", SYM_BIGNUMBER);
	add_symbol(m_syms_p, "array", SYM_ARRAY);
	add_symbol(m_syms_p, "byte", SYM_INTEGER);
	add_symbol(m_syms_p, "biginteger", SYM_BIGNUMBER);
	add_symbol(m_syms_p, "character", SYM_CHARACTER);
	add_symbol(m_syms_p, "boolean", SYM_BOOLEAN);
	add_symbol(m_syms_p, "not", SYM_EXCLAMATION);	
	add_symbol(m_syms_p, "or", SYM_LOR);	
	add_symbol(m_syms_p, "and", SYM_LAND);	
	add_symbol(m_syms_p, "pragma", SYM_PRAGMA);
	add_symbol(m_syms_p, "option", SYM_PRAGMA);
	add_symbol(m_syms_p, "boundary_check", SYM_BOUNDARY_CHECK);
	add_symbol(m_syms_p, "optimize", SYM_OPTIMIZE);
	add_symbol(m_syms_p, "continue", SYM_CONTINUE);
	add_symbol(m_syms_p, "null", SYM_NULL);
	add_symbol(m_syms_p, "declare", SYM_DECLARE);
	add_symbol(m_syms_p, "type", SYM_TYPE);
	add_symbol(m_syms_p, "clean", SYM_CLEAN);
	add_symbol(m_syms_p, "abs", SYM_ABS);
			
	/*
	 * None alphabetal symbols (operators).
	 */
	add_symbol(m_nonalpha_syms_p, "||", SYM_LOR);
	add_symbol(m_nonalpha_syms_p, "!", SYM_EXCLAMATION);
	add_symbol(m_nonalpha_syms_p, "&&", SYM_LAND);
	add_symbol(m_nonalpha_syms_p, "[", SYM_LBRACKET);
	add_symbol(m_nonalpha_syms_p, "]", SYM_RBRACKET);
	add_symbol(m_nonalpha_syms_p, "~", SYM_TILD);
	add_symbol(m_nonalpha_syms_p, "?", SYM_QMARK);
	add_symbol(m_nonalpha_syms_p, "..", SYM_DOTDOT);
	add_symbol(m_nonalpha_syms_p, "=>", SYM_EQMORE);
	add_symbol(m_nonalpha_syms_p, "<-", SYM_LESSMINUS);	
	add_symbol(m_nonalpha_syms_p, "<<", SYM_LCHEVRON);
	add_symbol(m_nonalpha_syms_p, ">>", SYM_RCHEVRON);
	add_symbol(m_nonalpha_syms_p, "{", SYM_BEGIN);
	add_symbol(m_nonalpha_syms_p, "}", SYM_END);
	add_symbol(m_nonalpha_syms_p, "+", SYM_PLUS);
	add_symbol(m_nonalpha_syms_p, "++", SYM_PLUS_PLUS);
	add_symbol(m_nonalpha_syms_p, "--", SYM_MINUS_MINUS);
	add_symbol(m_nonalpha_syms_p, "+=", SYM_PLUS_EQL);
	add_symbol(m_nonalpha_syms_p, "-=", SYM_MINUS_EQL);
	add_symbol(m_nonalpha_syms_p, "*=", SYM_STAR_EQL);
	add_symbol(m_nonalpha_syms_p, "/=", SYM_SLASH_EQL);
	add_symbol(m_nonalpha_syms_p, "<<=", SYM_LCHEVRON_EQL);
	add_symbol(m_nonalpha_syms_p, ">>=", SYM_RCHEVRON_EQL);
	add_symbol(m_nonalpha_syms_p, "-", SYM_MINUS);
	add_symbol(m_nonalpha_syms_p, "*", SYM_STAR);
	add_symbol(m_nonalpha_syms_p, "/", SYM_SLASH);
	add_symbol(m_nonalpha_syms_p, "(", SYM_LPAREN);
	add_symbol(m_nonalpha_syms_p, ")", SYM_RPAREN);
	add_symbol(m_nonalpha_syms_p, "=", SYM_EQL);
	add_symbol(m_nonalpha_syms_p, ",", SYM_COMMA);
	add_symbol(m_nonalpha_syms_p, ".", SYM_PERIOD);
	add_symbol(m_nonalpha_syms_p, ";", SYM_SEMICOLON);
	add_symbol(m_nonalpha_syms_p, ":", SYM_COLON);
	add_symbol(m_nonalpha_syms_p, "&", SYM_AMPERSAND);
	add_symbol(m_nonalpha_syms_p, "!=", SYM_NOT_EQL);
	add_symbol(m_nonalpha_syms_p, "<", SYM_LESS);
	add_symbol(m_nonalpha_syms_p, ">", SYM_GREATER);
	add_symbol(m_nonalpha_syms_p, "<=", SYM_LES_EQL);
	add_symbol(m_nonalpha_syms_p, ">=", SYM_GRE_EQL);	
	add_symbol(m_nonalpha_syms_p, ">=", SYM_GRE_EQL);
	add_symbol(m_nonalpha_syms_p, ":=", SYM_BECOMES);	
	add_symbol(m_nonalpha_syms_p, "//", SYM_LINECOMMENT);
	add_symbol(m_nonalpha_syms_p, "/*", SYM_LBLOCKCOMMENT);
	add_symbol(m_nonalpha_syms_p, "*/", SYM_RBLOCKCOMMENT);
	add_symbol(m_nonalpha_syms_p, "'", SYM_APOS);
	add_symbol(m_nonalpha_syms_p, "^", SYM_HAT);
	add_symbol(m_nonalpha_syms_p, "|", SYM_PIPE);
	add_symbol(m_nonalpha_syms_p, "@", SYM_AT);	
	add_symbol(m_nonalpha_syms_p, "$=", SYM_STR_EQUALS);
	add_symbol(m_nonalpha_syms_p, "$!=", SYM_STR_NEQUALS);
	add_symbol(m_nonalpha_syms_p, "^=", SYM_HAT_EQL);
	add_symbol(m_nonalpha_syms_p, "|=", SYM_PIPE_EQL);
	add_symbol(m_nonalpha_syms_p, "&=", SYM_AMPERSAND_EQL);		
	add_symbol(m_nonalpha_syms_p, "%", SYM_PERCENT);
}

/* 
 * Cleans up all the variables.
 */
void deinit()
{
	/*
	 * Free hashtables.
	 */
	hashtable_delete(m_syms_p);
	hashtable_delete(m_types_p);
	hashtable_delete(m_nonalpha_syms_p);
	var_array_delete(exp_string.ident.data.array_val_p);
	var_array_delete(exp_bignumber.ident.data.array_val_p);
	var_array_delete(m_literal_string.data.array_val_p);	
}

/*
 * Reads the next line of the current file.
 */
std_error_t read_next_line()
{
	char c;
	static BOOL revert = FALSE;

	strcpy(m_last_line, m_line);

	if (revert)
	{
		m_files_stack_index--;

		revert = FALSE;

		if (m_options.list_source)
		{
			printf("FILE     :[%s]\n", m_files_stack[m_files_stack_index].file_name);		
		}
	}

	m_files_stack[m_files_stack_index].line_index = 0;
	m_files_stack[m_files_stack_index].line_length = 0;

	while (((c = getc(m_files_stack[m_files_stack_index].file)) != 10))
	{
		if (c == EOF)
		{
			m_line[m_files_stack[m_files_stack_index].line_length++] = '\n';
			m_line[m_files_stack[m_files_stack_index].line_length] = 0;
			m_files_stack[m_files_stack_index].line_count++;
			
			if (m_options.list_source)
			{
				printf("LINE %4d: %s", m_files_stack[m_files_stack_index].line_count, m_line);
			}

			if (m_files_stack_index == 0)
			{
				return std_error;
			}
			else
			{
				revert = TRUE;
				return std_success;
			}
		}
		
		m_line[m_files_stack[m_files_stack_index].line_length++] = c;
	}

	m_line[m_files_stack[m_files_stack_index].line_length++] = c;
	m_line[m_files_stack[m_files_stack_index].line_length] = 0;
	m_files_stack[m_files_stack_index].line_count++;
	m_line_count++;

	if (m_options.list_source)
	{
		printf("LINE %4d: %s", m_files_stack[m_files_stack_index].line_count, m_line);
	}

	return std_success;
}

/*
 * Gets the next character in the file.
 */
char get_next_char()
{
	if (m_files_stack[m_files_stack_index].line_index >= m_files_stack[m_files_stack_index].line_length)
	{
		if (read_next_line() != std_success && m_files_stack[m_files_stack_index].line_length <= 1)
		{
			/*
			 * End of source files.
			 */

			return -1;
		}
	}

	m_last_symbol_len++;

	return m_line[m_files_stack[m_files_stack_index].line_index++];
}

/*
 * Tests to see if a char is a valid char for a symbol/word.
 */
BOOL valid_word_char(char ch)
{
	/*
	 * Added support for underscores.
	 */
	return (ch >= '0' && ch <= '9') 
		|| (ch  >= 'A' && ch <= 'Z') 
		|| (ch >= 'a' && ch <= 'z')
		|| (ch == '_') || (ch == '`');
}

/*
 * Checks to see if a char is a valid non alpha char.
 */
BOOL valid_nonalpha_sym(char ch)
{
	return (ch >= 33 && ch <= 47) || (ch >=58 && ch <= 64)
		|| (ch >=91 && ch <=96) || (ch >= 123 && ch <= 126);
}

/*
 * Gets the integer symbol of a word symbol.
 */
int get_word_symbol(hashtable_t *hashtable_p, const char *name)
{
	variant_t var;

	var = hashtable_get_stringkey(hashtable_p, name);
	
	if (var.vt != VT_PVOID)
	{
		m_symbol = SYM_NOSYM;
	}
	else
	{
		m_symbol = ((word_t*)var.val.val_void_p)->symbol;
	}

	return m_symbol;
}

/*
 * Checks to see if a char is a non-symbol char.
 */
BOOL is_non_sym_char(char ch, BOOL includesemicolon)
{
	return ch == '\n' || ch == '\t' || ch == ' ' 
		|| (includesemicolon ? (ch == ';') : 0) 
		|| (ch >= 127) || (ch < 32);
}

/*
 * Tests to see if there are any reserved symbols that start with the supplied string.
 */
BOOL any_symbol_starts_with(hashtable_t *hashtable, const char* str)
{
	hashtable_iterator_t iterator;
	word_t *word_p;

	hashtable_init_iterator(hashtable, &iterator);

	for (; hashtable_hasmore(&iterator); )
	{
		word_p = (word_t*)iterator.var_p->val.val_void_p;

		if (!(strlen(word_p->name) == strlen(str)))
		{
			if (strncmp(word_p->name, str, strlen(str)) == 0)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
 * Reads away block comments.
 */
BOOL comments()
{
	char chars[3];

	memset(chars, 0, 3);

	for (;;)
	{
		m_char = get_next_char();

		if (m_char == -1)
		{
			return FALSE;
		}

		chars[0] = chars[1];
		chars[1] = m_char;
		
		if (strcmp(chars, "/*") == 0)
		{
			if (!comments())
			{
				return FALSE;
			}
		}
		else if (strcmp(chars, "*/") == 0)
		{
			return TRUE;

			break;
		}

	}
}

/*
 * Returns the next symbol in the source code.
 */
void get_symbol()
{	
	char c, c2;
	int k, b;
	BOOL read_char;
	char tmp[MAX_IDENTS + 1];
	
get_symbol_start:

	m_is_char = FALSE;
	m_last_symbol_len = 0;
	
	m_last_symbol = m_symbol;

	/*
	 * Forget all the new line, tab and spaces.
	 */
	while (is_non_sym_char(m_char, (BOOL)(m_last_symbol == SYM_SEMICOLON)) && m_char != -1)
		m_char = get_next_char();
	
	if (isalpha((int)m_char) || m_char == '_')
	{
		k = 0;

		do
		{
			if (k < MAX_IDENT_LEN)
			{
				m_ident_name[k++] = m_char;
			}

			m_char = get_next_char();
		}
		while(valid_word_char((m_char)));

		m_ident_name[k] = 0;

		m_symbol = get_word_symbol(m_syms_p, m_ident_name);	

		if (m_symbol == SYM_NOSYM)
		{
			m_symbol = SYM_IDENT;
		}
	}
	else if (((m_char >= '0') && (m_char <= '9')) || m_char == '.')
	{
		int num;
		k = 0;
		b = 10;
		m_number = 0;
		m_symbol = SYM_LITERAL_NUMBER;
		m_number_kind = KIND_INTEGER;
		
		/*
		 * @author Thong Nguyen
		 *
		 * Added support for hexadecimal & 32bit floating point numbers
		 */
		for (;;)
		{
			m_string[k] = m_char;

			if (m_char == '.')
			{
				m_number_kind = KIND_FLOAT;
			}
			
			m_char = get_next_char();
			k++;

			if (k == 1 && m_char == 'x' && m_number == 0)
			{
				b = 16;
			}

			
			if (b == 16)
			{
				if (!(m_char >= '0' && m_char <= '9'))
				{
					if (!((m_char >= 'a' && m_char <='f') || m_char == 'x'))
					{
						if (!(m_char >= 'A' && m_char <= 'F'))
						{
							break;
						}
					}
				}
			}
			else
			{
				if (((m_char < '0') || (m_char > '9')) && m_char != '.')
				{
					break;
				}
			}
		}

		m_string[k] = 0;

		if (k > MAX_NUMBER_DIGITS && m_char != '#')
		{
			error(1100);
		}

		if (m_char == '#')
		{
			m_char = get_next_char();

			m_symbol = SYM_LITERAL_BIGNUMBER;
		}
		else
		{
			if (m_number_kind == KIND_FLOAT)
			{	
				sscanf(m_string, "%f", &m_number);			
			}
			else if (b == 16)
			{
				sscanf(m_string, "%x", &num);
				m_number = (float)num;			
			}
			else
			{
				
				sscanf(m_string, "%d", &num);

				m_number = (float)num;				
			}
		}
	}
	else if (m_char == '\"' || m_char == '@')
	{		
		BOOL verbatim = m_char == '@';

		if (verbatim)
		{
			m_char = get_next_char();
		}

		m_symbol = SYM_LITERAL_STRING;
		
		b = FALSE;

		read_char = TRUE;

		for (k = 0; ; k++)
		{			
			if (read_char)
			{
				m_char = get_next_char();
			}

			if (m_char == -1)
			{
				error(2010);

				break;
			}

			if (k < MAX_LITERAL_STRING_LEN)
			{
				if (m_char == '\\')
				{
					if (!m_ignore_esc_chars && !verbatim)
					{
						if (b == FALSE)
						{
							b = TRUE;
							m_char = get_next_char();
						}
					}
				}				
			}

			c2 = m_char;

			if (b)
			{
				b = FALSE;

				c2 = escape_char(m_char, &read_char);
			}
			else
			{
				if (c2 == '\"')
				{
					if (verbatim)
					{
						m_char = get_next_char();

						if (m_char == '@')
						{
							m_char = get_next_char();

							break;
						}
						else
						{
							m_string[k] = '\"';
							read_char = FALSE;
						}
					}
					else
					{
						m_char = get_next_char();

						break;
					}
				}
				else
				{
					read_char = TRUE;
				}
			}

			m_string[k] = c2;
		}

		m_string[k] = '\0';
		m_string_length = k;

	}
	else if (valid_nonalpha_sym(m_char) && m_char != '\"' && m_char != '\'')
	{
		k = 0;

		/*
		 * My support for non-alphabet symbols that can be of any length
		 * as long as the combinations don't conflict.
		 *
		 * eg. > >= >=$ are all valid different symbols that can be differentiated.
		 */
		do
		{
			if (k < MAX_IDENT_LEN)
			{
				tmp[k++] = m_char;
			}

			tmp[k] = 0;

			if (!any_symbol_starts_with(m_nonalpha_syms_p, tmp))
			{
				m_symbol = get_word_symbol(m_nonalpha_syms_p, tmp);

				if (k >= 1 && m_symbol <= 0)
				{
					tmp[k - 1] = 0;
				}
				else
				{
					m_char = get_next_char();
				}

				break;
			}		
			
			m_char = get_next_char();

		}
		while (valid_nonalpha_sym(m_char));
		
		m_symbol = get_word_symbol(m_nonalpha_syms_p, tmp);
		
		if (m_symbol == SYM_LINECOMMENT)
		{
			m_char = ' ';
			m_files_stack[m_files_stack_index].line_length = -1;

			goto get_symbol_start;
		}
		else if (m_symbol == SYM_LBLOCKCOMMENT)
		{
			if (!comments())
			{
				error(2000); /* Unexpected end of file while processing comment block */
				m_symbol = SYM_EOFSYM;

				return;
			}
			
			m_char = ' ';
			get_symbol();
		}

		m_last_symbol_len++;
	}
	else if (m_char == '\'')
	{
		m_char = get_next_char();

		read_char = TRUE;

		if (m_char == '\\')
		{
			m_char = get_next_char();
			c = escape_char(m_char, &read_char);
		}
		else
		{
			c = m_char;
		}

		m_number = (float)c;

		m_is_char = TRUE;
		m_number_kind = KIND_INTEGER;
				
		if (read_char)
		{
			m_char = get_next_char();
		}

		if (m_char == '\'')
		{
			m_char = get_next_char();

			m_symbol = SYM_LITERAL_NUMBER;
		}
		else
		{
			error(7006);
		}
	}	
	else if (m_char == -1)
	{
		m_symbol = SYM_EOFSYM;
	}
}

/*
 * Returns the escape character given a character.
 * Can read hexadecimal escape characters too. (eg. '\x100').
 */
char escape_char(char ch, BOOL *read_char)
{
	int i, j;
	BOOL hex = FALSE;
	BOOL oct = FALSE;
	char number[4];
	
	if ((ch >= '0' && ch <= '9') || ch == 'x' || ch == 'o')
	{
		number[3] = 0;
		number[0] = ch;	

		if (ch == 'x')
		{
			hex = TRUE;
			
			j = 1;
			number[2] = 0;

			m_char = get_next_char();

			m_char = tolower(m_char);

			if ((m_char >= '0' && m_char <='9')
				|| (m_char >= 'a' && m_char <= 'f'))				
			{
				number[0] = m_char;
			}
			else
			{
				error(3000);
			}
		}
		else if (ch == 'o')
		{
			oct = TRUE;

			j = 2;

			number[2] = 0;

			m_char = get_next_char();

			m_char = tolower(m_char);

			if ((m_char >= '0' && m_char <='7'))
			{
				number[0] = m_char;
			}
			else
			{
				error(3000);
			}
		}
		else
		{
			j = 2;
		}

		*read_char = TRUE;

		for (i = 1; i <= j; i++)
		{
			m_char = get_next_char();

			if ((m_char >= '0' && m_char <='9')
				|| (hex && (m_char >= 'a' && m_char <='f')))
			{
				number[i] = m_char;
			}
			else
			{
				number[i] = 0;
				*read_char = FALSE;

				break;
			}
		}

		if (hex)
		{
			sscanf(number, "%x", &i);
		}
		else if (oct)
		{			
			sscanf(number, "%o", &i);
		}
		else
		{			
			i = atoi(number);		
		}

		if (i < 0 || i > 255)
		{
			error(3000);
		}

		return i;
	}

	*read_char = TRUE;

	switch (ch)
	{
	case 'n':
		
		return '\n';

	case 'r':
		
		return '\r';

	case '"':
		
		return '"';

	case '\\':
		return '\\';

	case 't':
		return '\t';

	case 'v':
		return '\v';

	case '?':
		return '\?';

	case 'a':
		return '\a';

	case 'b':
		return '\b';

	case '\'':
		return '\'';

	}

	m_last_symbol_len = 1;

	error(3000);

	return 0;
}

/*
 * The following is for the code optimizer.  It optimizes multiple instructions
 * into less instructions if possible. 
 *
 * e.g.
 *
 * LIT 0 0, LIT 0 0 will optimize to LIS 1 0.
 * LIT 0 2, LIT 0 2, OPR 0 OPR_ADD will optimize to LIT 0 4
 */

#define ODD(x)	\
	((x % 2) != 0);

#define OPTIMIZE_OPR(op)	\
	m_instructions[i - 2].f = OC_LIT;	\
	m_instructions[i - 2].l = 0;		\
	m_instructions[i - 2].a = m_instructions[i - 2].a op m_instructions[i - 1].a;	\
	m_instructions_index -= 2;

#define OPTIMIZE_U_OPR(op)	\
	m_instructions[i - 1].f = OC_LIT;	\
	m_instructions[i - 1].l = 0;		\
	m_instructions[i - 1].a = op(m_instructions[i - 1].a);	\
	m_instructions_index -= 1;

float m_f;

#define OPTIMIZE_FOP(op)	\
	m_instructions[i - 2].f = OC_LIT;	\
	m_instructions[i - 2].l = 0;		\
	m_f = *((float*)(&m_instructions[i - 2].a)) op *((float*)(&m_instructions[i - 1].a));	\
	m_instructions[i - 2].a = *((int*)(&m_f));	\
	m_instructions_index -= 2;\

void optimize()
{
	int i;
	instruction_t ti;

	if (!m_options.optimize)
	{
		return;
	}

	i = m_instructions_index;

	if (i >= 1 && m_instructions[i].f == OC_SLD && m_instructions[i].a == 0)
	{
		if (m_instructions[i - 1].f == OC_LIT && m_instructions[i - 1].l == 0)
		{
			m_instructions[i - 1].f = OC_LIS;
			m_instructions[i - 1].l = 2;

			m_instructions_index--;
		}
		else if (m_instructions[i - 1].f == OC_LIS)
		{
			m_instructions[i - 1].l++;
			m_instructions_index--;
		}
	}
	else if (i >= 0 && m_instructions[i].f == OC_JPC)
	{
		if (m_instructions[i - 1].f == OC_LIT)			
		{
			m_message[0] = 0;

			error_extra(11100, m_message, TRUE, TRUE);
		}
	}
	else if (m_instructions[i].f == OC_JMP)
	{
		/*
		 * No point in just jumping to the next line.
		 */

		if (m_instructions[i].a == i + 1)
		{
			m_instructions_index--;
		}		
	}
	else if (i >= 1 && m_instructions[i].f == OC_OPR
		&& m_instructions[i].a == 0)		
	{
		
	}
	else if (i >= 2 && m_instructions[i].f == OC_SWS)
	{
		/*
		 * Do static swapping rather than dynamic swapping if it's possible.
		 */
		if ((m_instructions[i - 1].f == OC_LOD
			|| m_instructions[i - 1].f == OC_LID
			|| m_instructions[i - 1].f == OC_LDA
			|| m_instructions[i - 1].f == OC_LIT			
			|| m_instructions[i - 1].f == OC_MLO
			|| m_instructions[i - 1].f == OC_MLI
			|| m_instructions[i - 1].f == OC_PUS)
			&&
			(m_instructions[i - 2].f == OC_LOD
			|| m_instructions[i - 2].f == OC_LID
			|| m_instructions[i - 2].f == OC_LDA
			|| m_instructions[i - 2].f == OC_LIT
			|| m_instructions[i - 2].f == OC_MLO
			|| m_instructions[i - 2].f == OC_MLI
			|| m_instructions[i - 2].f == OC_PUS))
		{			
			ti = m_instructions[i - 1];

			m_instructions[i - 1] = m_instructions[i - 2];
			m_instructions[i - 2] = ti;

			/*
			 * Remove the swap instruction.
			 */
			m_instructions_index--;
		}
	}
	else if (i >= 1 && m_instructions[i].f == OC_INC)
	{
		if (m_instructions[i - 1].f == OC_INC)
		{
			m_instructions[i - 1].a += m_instructions[i].a;
		
			m_instructions_index--;
		}
		else if (m_instructions[i - 1].f == OC_DEC)
		{
			/*
			 * DEC then INC by the same amount.
			 */
			if (m_instructions[i - 1].a == m_instructions[i].a)
			{
				m_instructions_index -= 2;
			}
			/*
			 * DEC then INC.  DEC > INC.
			 */
			else if (m_instructions[i - 1].a > m_instructions[i].a)
			{
				m_instructions[i - 1].a = m_instructions[i - 1].a - m_instructions[i].a;
				m_instructions_index -= 1;
			}
			/*
			 * DEC then INC.  DEC < INC.
			 */
			else if (m_instructions[i - 1].a < m_instructions[i].a)
			{
				m_instructions[i - 1].f = OC_INC
					;
				m_instructions[i - 1].a = m_instructions[i].a - m_instructions[i - 1].a;
				m_instructions_index -= 1;
			}
		}
	}
	else if (i >= 1 && m_instructions[i].f == OC_DEC)
	{
		if (m_instructions[i - 1].f == OC_DEC)
		{
			m_instructions[i - 1].a += m_instructions[i].a;
		
			m_instructions_index--;
		}
		else if (m_instructions[i - 1].f == OC_INC)
		{
			/*
			 * INC then DEC by the same amount.
			 */
			if (m_instructions[i - 1].a == m_instructions[i].a)
			{
				m_instructions_index -= 2;
			}
			/*
			 * INC then DEC.  INC > DEC.
			 */
			else if (m_instructions[i - 1].a > m_instructions[i].a)
			{
				m_instructions[i - 1].a = m_instructions[i - 1].a - m_instructions[i].a;
				m_instructions_index -= 1;
			}
			/*
			 * INC then DEC.  INC < DEC.
			 */
			else if (m_instructions[i - 1].a < m_instructions[i].a)
			{
				m_instructions[i - 1].f = OC_DEC;
				m_instructions[i - 1].a = m_instructions[i].a - m_instructions[i - 1].a;
				m_instructions_index -= 1;
			}
		}
	}
	else if (i >= 1 && m_instructions[i].f == OC_OPR)
	{
		if (m_instructions[i - 1].f == OC_LIT && m_instructions[i - 1].a == 0
			&& m_instructions[i - 1].f != OC_CAL)
		{
			switch (m_instructions[i].a)
			{
			case OPR_SUB:
			case OPR_ADD:
			
				m_instructions_index -= 2;

				break;

			case OPR_MUL:			
			case OPR_MOD:

				m_instructions[i - 2].f = OC_LIT;
				m_instructions[i - 2].l = 0;
				m_instructions[i - 2].a = 0;

				m_instructions_index -= 2;
				
				break;

			case OPR_DIV:
				
				error(11110);

				break;
			}
		}
		else if (i >= 2 && m_instructions[i - 2].f == OC_LIT && m_instructions[i - 2].a == 0
			&& m_instructions[i - 1].f != OC_CAL)
		{
			switch (m_instructions[i].a)
			{
			case OPR_SUB:
			case OPR_ADD:
			
				m_instructions[i - 2] = m_instructions[i - 1];

				m_instructions_index -= 2;

				break;

			case OPR_MUL:			
			case OPR_MOD:

				m_instructions[i - 2].f = OC_LIT;
				m_instructions[i - 2].l = 0;
				m_instructions[i - 2].a = 0;

				m_instructions_index -= 2;
				
				break;

			}
		}
		else if (i >= 1 && m_instructions[i - 1].f == OC_LIT && m_instructions[i - 1].l == 0)
		{
			switch (m_instructions[i].a)
			{
				case OPR_ODD:
					
					OPTIMIZE_U_OPR(ODD);

					break;

				case OPR_NEG:
					
					OPTIMIZE_U_OPR(-);

					break;

				case OPR_NOT:

					OPTIMIZE_U_OPR(!);

					break;

				default:

					if (i >= 2 && m_instructions[i - 2].f == OC_LIT && m_instructions[i - 2].l == 0)
					{
						switch (m_instructions[i].a)
						{
						case OPR_ADD:

							OPTIMIZE_OPR(+);
							
							break;

						case OPR_SUB:

							OPTIMIZE_OPR(-);
							
							break;

						case OPR_MUL:

							OPTIMIZE_OPR(*);
							
							break;

						case OPR_DIV:

							OPTIMIZE_OPR(/);
							
							break;

						case OPR_MOD:

							OPTIMIZE_OPR(%);
							
							break;

						case OPR_EQL:

							OPTIMIZE_OPR(=);

							break;

						case OPR_NEQ:

							OPTIMIZE_OPR(!=);

							break;

						case OPR_LES:

							OPTIMIZE_OPR(<);

							break;

						case OPR_LEQ:

							OPTIMIZE_OPR(<=);

							break;

						case OPR_GRE:

							OPTIMIZE_OPR(>);

							break;

						case OPR_GRQ:

							OPTIMIZE_OPR(>=);

							break;

						case OPR_SHL:

							OPTIMIZE_OPR(<<);

							break;

						case OPR_SHR:

							OPTIMIZE_OPR(>>);

							break;

						case OPR_BOR:

							OPTIMIZE_OPR(|);

							break;

						case OPR_AND:

							OPTIMIZE_OPR(&);

							break;

						case OPR_XOR:

							OPTIMIZE_OPR(^);

							break;
						}
					}
			}			
		}
	}
	else if (m_instructions[i].f == OC_FOP)
	{
		if (i >= 1 && m_instructions[i - 1].f == OC_LIT && m_instructions[i - 1].l == 0)
		{
			switch (m_instructions[i].a)
			{
			case FOP_INT:

				m_instructions[i - 1].f = OC_LIT;
				m_instructions[i - 1].l = 0;
				m_f = *((float*)&m_instructions[i - 1].a);
				m_instructions[i - 1].a = (LONGLONG)m_f;
				m_instructions_index -= 1;

				break;

			case FOP_FLO:

				m_instructions[i - 1].f = OC_LIT;
				m_instructions[i - 1].l = 0;
				m_f = (float)m_instructions[i - 1].a;
				memcpy(&m_instructions[i - 1].a, &m_f, sizeof(float));
				m_instructions_index -= 1;

				break;

			case FOP_NEG:

				m_instructions[i - 1].f = OC_LIT;
				m_instructions[i - 1].l = 0;
				m_f = -(*((float*)&m_instructions[i - 1].a));
				m_instructions[i - 1].a = *((int*)&m_f);
				m_instructions_index -= 1;

				break;

			default:
								
				if (i >= 2 && m_instructions[i - 2].f == OC_LIT && m_instructions[i - 2].l == 0)
				{
					switch (m_instructions[i].a)
					{
					case FOP_ADD:

						OPTIMIZE_FOP(+);

						break;

					case FOP_SUB:
					case FOP_MUL:
					case FOP_DIV:
					case FOP_EQL:
					case FOP_NEQ:
					case FOP_LES:
					case FOP_LEQ:
					case FOP_GRE:
					case FOP_GRQ:

						break;
					}
				}
			}
		
		}
	}
}

/*
 * Adds an instruction to the output opcodes.
 */
void instructions_add(int f, int l, LONGLONG a)
{
	if (m_instructions_index >= MAX_INSTRUCTIONS)
	{
		error(30010);		
	}
	else
	{
		m_instructions[m_instructions_index].f = f;
		m_instructions[m_instructions_index].l = l;
		m_instructions[m_instructions_index].a = a;
	}

	optimize();

	m_instructions_index++;
}

/*
 * Adds an instruction to the output opcodes.  Works with floats.
 */
void instructions_add_f(int f, float l, float a)
{
	if (m_instructions_index >= MAX_INSTRUCTIONS)
	{
		error(30010);
	}
	else
	{
		m_instructions[m_instructions_index].f = f;
		memcpy(&m_instructions[m_instructions_index].l, &l, sizeof(int));
		memcpy(&m_instructions[m_instructions_index].a, &a, sizeof(int));
	}

	optimize();

	m_instructions_index++;
}

/*
 * Returns the position of an identifier in the identifier stack.
 */
int ident_position(char *name, int *ptx)
{
	int i;
	identinfo_t *ident_p;

	for (i = *ptx; i >= 0; i--)
	{
		ident_p = &m_idents[i];

		if (strcmp(name, ident_p->name) == 0)
		{
			break;
		}
	}

	return i;
}

/*
 * Finds and returns an identifier in the identifier stack.
 */
identinfo_t *ident_find(char *name, int *ptx)
{
	int i;
	
	i = ident_position(name, ptx);

	if (i < 0)
	{
		return NULL;
	}
	else
	{
		return &m_idents[i];
	}
}

/*
 * Adds a constant to the identifier stack.
 */
void ident_add_const(int level, int *ptx, int *pdx)
{
	identinfo_t *ident_p;

	if (m_symbol != SYM_IDENT)
	{
		error(4000);

		return;
	}

	ident_p = ident_process_var(level, ptx, pdx, FALSE, FALSE);
	
	if ((ident_p->kind == KIND_INTEGER) || (ident_p->kind == KIND_FLOAT)
		|| (ident_p->kind == KIND_ARRAY))
	{
		ident_p->constant = TRUE;		
	}
	else
	{
		error(10350);
	}
	
	/*
	 * constants don't need to be stored.
	 */

	(*pdx)--;
}

/*
 * Returns the kind of the type, from the integer symbol.
 */
int get_kind()
{
	switch(m_symbol)
	{
	
	case SYM_STRING:
	case SYM_BIGNUMBER:
		
		return KIND_STRING;

	case SYM_INTEGER:
		
		return KIND_INTEGER;

	case SYM_HANDLE:

		return KIND_INTEGER;		

	case SYM_CHARACTER:

		return KIND_INTEGER;

	case SYM_BOOLEAN:

		return KIND_INTEGER;

	case SYM_FLOAT:
		
		return KIND_FLOAT;

	case SYM_VOID:
		
		return KIND_VOID;

	default:
		
		return KIND_UNKNOWN;
	}
}

/*
 * Processes an array declaration.
 */
void ident_array_process(identinfo_t *ident_p, int level, int *ptx, int *kind_p)
{
	if (ident_p->kind == KIND_STRING)
	{
		/*
		 * Strings have an initial size of 0 (eg. they are valid arrays and allocated
		 * automatically - they are not just null pointers initially).
		 */
		*kind_p = KIND_ARRAY;

		ident_p->kind = KIND_ARRAY;
		ident_p->data.array_val_p = var_array_new();
		ident_p->data.array_val_p->size = 0;
		ident_p->data.array_val_p->contains.kind = KIND_INTEGER;
		ident_p->data.array_val_p->contains.data.int_val.is_character = TRUE;

		ident_p->refed++;
		
		return;
	}

	if (m_symbol == SYM_LBRACKET)
	{
		get_symbol();

		ident_p->data.array_val_p = var_array_new();

		/*
		 * If no size is specified, then it must be a reference to an array.
		 */
		if (m_symbol == SYM_RBRACKET)
		{
			ident_p->data.array_val_p->size = -1;

			get_symbol();
		}		
		else if (m_symbol == SYM_LITERAL_NUMBER || m_symbol == SYM_DOTDOT)
		{
			if (m_symbol == SYM_LITERAL_NUMBER)
			{				
				ident_p->data.array_val_p->size = (int)m_number;
			}
			else
			{
				ident_p->data.array_val_p->size = 0;
			}

			get_symbol();

			if (m_symbol != SYM_RBRACKET)
			{
				error(6002); /* missing right bracket */
			}

			get_symbol();
		}
		else
		{
			error(1000);
		}

		if (*kind_p == KIND_UNKNOWN)
		{
			ident_p->data.array_val_p->contains.kind = KIND_INTEGER;				
		}
		else
		{
			ident_p->data.array_val_p->contains.kind = *kind_p;
		}

		*kind_p = KIND_ARRAY;

		ident_p->refed++;
	}
	else
	{
		/*
		 * Not a list
		 */	
	}
}

/*
 * Tests to see if an identifier is a function or a procedure.
 */
BOOL func_or_proc(identinfo_t *ident_p)
{
	return ident_p->kind == KIND_PROCEDURE || ident_p->kind == KIND_FUNCTION;
}

void type_add(identinfo_t *ident_p)
{
	variant_t var, varkey;

	variant_init_pointer(&var, VT_PVOID, ident_p);
	variant_init_pointer(&varkey, VT_PSTR, ident_p->name);
	varkey.destructor = NULL;

	hashtable_put(m_types_p, var, varkey);
}

identinfo_t *type_find(const char* name)
{
	variant_t var, varkey;

	variant_init_pointer(&varkey, VT_PSTR, (char*)name);

	var = hashtable_get(m_types_p, varkey);

	if (var.vt == VT_EMPTY)
	{
		return NULL;
	}

	return (identinfo_t*)var.val.val_void_p;
}

/*
 * Reads the type for an identifier.
 */
void ident_read_type(identinfo_t *ident_p, int level, int *ptx)
{
	int kind;
	BOOL handle = FALSE, character = FALSE, boolean = FALSE, biginteger = FALSE;
	identinfo_t *ident_p2;

	if (m_symbol != SYM_COLON)
	{
		ident_p->kind = KIND_INTEGER;
		
		return;
	}

	get_symbol();

	/*
	if (m_symbol == SYM_CLEAN)
	{
		ident_p->clean = TRUE;

		get_symbol();
	}
	*/
	
	if ((m_symbol >= SYM_TYPESTART && m_symbol <= SYM_TYPEEND))
	{	
		if (m_symbol == SYM_ARRAY)
		{
			get_symbol();

			/*
			 * This is a special kind to allow functions to return handles to any kind
			 * of array, and survive the current type checking.
			 */

			ident_p->kind = KIND_ARRAY;
			ident_p->data.array_val_p = var_array_new();
			ident_p->data.array_val_p->size = -1;
			ident_p->data.array_val_p->contains.kind = KIND_INTEGER;
						
			return;
		}
		
		ident_p->kind = get_kind();		

		if (m_symbol == SYM_HANDLE)
		{
			handle = TRUE;		
		}
		else if (m_symbol == SYM_CHARACTER)
		{
			character = TRUE;
		}
		else if (m_symbol == SYM_BOOLEAN)
		{
			boolean = TRUE;
		}
		else if (m_symbol == SYM_BIGNUMBER)
		{
			biginteger = TRUE;
		}

		get_symbol();
		
		ident_array_process(ident_p, level, ptx, &ident_p->kind);

		if (biginteger)
		{						
			ident_p->data.array_val_p->is_big_number = TRUE;
			ident_p->data.array_val_p->array_default = chars2ints("0");
		}

		if (ident_p->kind == KIND_ARRAY && ident_p->data.array_val_p->contains.kind == KIND_INTEGER)
		{
			ident_p->data.array_val_p->contains.data.int_val.is_handle = handle;
		}
		else if (ident_p->kind == KIND_INTEGER)
		{
			ident_p->data.int_val.is_handle = handle;
			ident_p->data.int_val.is_character = character;
			ident_p->data.int_val.is_boolean = boolean;
		}

		if (ident_p->kind == KIND_UNKNOWN)
		{
			error(9000); /* unknown type, presuming int */
			ident_p->kind = KIND_INTEGER;
		}
	}
	else
	{
		ident_p2 = ident_find(m_ident_name, ptx);
		
		if (!ident_p2)
		{
			/*
			 * The type comes from a type declaration.
			 */

			ident_p2 = type_find(m_ident_name);

			if (ident_p2)
			{
				get_symbol();

				ident_p->kind = ident_p2->kind;
				ident_p->data = ident_p2->data;
			}
		}
		else if (ident_p2)
		{
			get_symbol();

			/*
			 * It's an identifier that's the type.
			 * This should be a function, you're allowed to have vars of type function.
			 */
			kind = ident_p2->kind;			
			ident_array_process(ident_p, level, ptx, &kind);
									
			if (kind == KIND_FUNCTION || kind == KIND_PROCEDURE || kind == KIND_ARRAY)
			{
				if (kind == KIND_ARRAY)
				{
					ident_p->kind = kind;
					ident_p->data.array_val_p->contains = ident_copy(*ident_p2);
				}
				else if (kind == KIND_FUNCTION || kind == KIND_PROCEDURE)
				{	
					ident_p->kind = ident_p2->kind;
					ident_p->data.function_val_p = var_function_clone(ident_p2->data.function_val_p);						
					ident_p->data.function_val_p->byref = TRUE;
				}
			}
			else
			{
				error(4000); /* can't be of a type  that's not the basic type or a function */
			}
		}
		else
		{
			error(4000); /* expected identifier */
		}
		
		if (ident_p->kind == KIND_UNKNOWN)
		{
			/*
			 * If this ident isn't a function or procedure.
			 */
			if (func_or_proc(ident_p))
			{
				/*
				 * Set the default type to an integer. (they should have supplied one)
				 * cause they supplied an int)
				 */
				error(4002);
				ident_p->kind = KIND_INTEGER;
			}
		}
	}
}

int *chars2ints(const char *c_p)
{
	int *i_p, i, len;

	len = strlen(c_p);

	i_p = MALLOC(int, len + 2);

	for (i = 0; i < len; i++)
	{
		i_p[i + 1] = c_p[i];
	}

	*i_p = len;

	return i_p;
}

/*
 * Processes and adds a var to the identifier stack.
 */
identinfo_t *ident_process_var(int level, int *ptx, int *pdx, BOOL byref, BOOL statement)
{
	identinfo_t ident, *ident_p;
	BOOL kind_was_default = FALSE;
	BOOL becomes = FALSE;
	
	ident.kind = KIND_INTEGER;

	ZEROMEMORY(ident);
	
	if (m_symbol != SYM_IDENT)
	{
		error(4000);

		return NULL;		
	}
	
	strcpy(ident.name, m_ident_name);
	ident.level = level;
	ident.byref = byref;
	ident.constant = FALSE;
	ident.address = *pdx;

	get_symbol(); 

	if (m_symbol == SYM_COLON)
	{
		ident_read_type(&ident, level, ptx);
	}
	else
	{
		ident.kind = KIND_INTEGER;
		kind_was_default = TRUE;
	}
	
	if (m_symbol == SYM_EQL || m_symbol == SYM_BECOMES)
	{
		/*
		 * Need to one day add support for expressions as defaults (for non constants).
		 */
		
		if (statement)
		{
			becomes = TRUE;
		}
		else
		{
			/*
			 * If this is not a var declared inside a statement, then the default can only be
			 * done at compile time until the statement blocks are processed (at which time
			 * block() goes and sets the defaults for these vars).
			 * Reason is blocks can only have instructions after ALL vars and functions have
			 * been declared.
			 */

			ident.has_default = TRUE;

			get_symbol();
			
			if (can_be_string(&ident))
			{
				/*
				 * Default is a string.
				 */

				if (m_symbol == SYM_IDENT)
				{
					ident_p = ident_find(m_ident_name, ptx);

					if (ident_p && ident_p->constant)
					{
						if (can_be_string(ident_p))
						{
							ident.data.array_val_p->array_default = MALLOC(int, *ident_p->data.array_val_p->array_default);
							memcpy(ident.data.array_val_p->array_default, ident_p->data.array_val_p, *ident_p->data.array_val_p->array_default);
							
							m_symbol = SYM_LITERAL_STRING;
						}				
					}
					else
					{
						error(4200);
					}
				}				
				else if (m_symbol == SYM_BEGIN)
				{
					ident.data.array_val_p->array_default = static_arraydef(&ident, level, ptx);
				}
				else if ((m_symbol == SYM_LITERAL_STRING && !byref)
					|| m_symbol == SYM_LITERAL_BIGNUMBER)
				{
					get_symbol();

					ident.data.array_val_p->array_default = chars2ints(m_string);
				}
				else if (m_symbol == SYM_NULL)
				{
					get_symbol();

					ident.data.array_val_p->array_default = NULL;
					ident.data.array_val_p->size = -1;
				}
				else
				{
					error(10560);
				}
			}
			else if (ident.kind == KIND_FUNCTION)
			{
				if (m_symbol == SYM_IDENT)
				{
					ident_p = ident_find(m_ident_name, ptx);

					if (ident_p)
					{
						ident.data.function_val_p->default_p = ident_p;
					}
					
					get_symbol();
				}
			}
			else
			{
				/*
				 * Default is an integer or a float.
				 * We can use the expression() engine to process this and by using the optimizer
				 * can get the default value out at compile time!  See below.
				 */
				
				becomes = TRUE;
			}
		}
	}
	else
	{
		if (ident.kind == KIND_INTEGER)
		{
			ident.data.int_val.default_val = 0;
		}
		else if (ident.kind == KIND_FLOAT)
		{
			ident.data.float_val.default_val = 0;
		}
	}

	ident_p = ident_add_any(&ident, ptx, pdx);

	if (!becomes)
	{
		/*
		 * If there's no default specified, but this variable declaration is a statement.
		 * There has to be a default set.
		 */
		if (statement)
		{
			set_defaults(level, ptx, *ptx, *ptx);			
		}
	}
	else
	{	
		exp_res_t exp_res;

		ZEROMEMORY(exp_res);

		SET_EXP_RES(exp_res, (*ident_p));

		if (statement)
		{
			/*
			 * "var" used as a statement (e.g. inside a function) is done all at runtime.
			 */

			if (m_symbol == SYM_EQL)
			{
				m_symbol = SYM_BECOMES;
			}

			if (ident_p->kind == KIND_ARRAY)
			{
				ident_p->data.array_val_p->assigning_ref = TRUE;
				ident_p->refed = 0;
			}

			statement_becomes(ident_p, level, ptx, FALSE, m_empty_tag);

			if (ident_p->kind == KIND_ARRAY)
			{
				ident_p->refed = 1;
			}
		}
		else
		{
			/*
			 * 'var' declared outside a statement (and is an integer or float).
			 * The default can be calculated using the expression engine, then
			 * the value extracted from the instructions.
			 */

			int diff, index, optimize = m_options.optimize;

			index = m_instructions_index;
			m_options.optimize = TRUE;

			SET_EXP_RES(exp_res, *ident_p);
			
			expression(level, ptx, exp_res, m_empty_tag);

			m_options.optimize = optimize;

			diff = m_instructions_index - index;

			if (diff != 1 && m_instructions[m_instructions_index - 1].f != OC_LIT)
			{
				error(10560);
			}

			if (ident_p->kind == KIND_INTEGER)
			{
				ident_p->data.int_val.default_val = m_instructions[m_instructions_index - 1].a;
			}
			else if (ident_p->kind == KIND_FLOAT)
			{
				memcpy(&ident_p->data.float_val.default_val, &m_instructions[m_instructions_index - 1].a, sizeof(int));
			}

			m_instructions_index -= diff;
		}
	}

	return ident_p;
}

/*
 * Adds any identifier.
 */
identinfo_t *ident_add_any(identinfo_t *ident_p, int *ptx, int *pdx)
{
	if (*ptx >= 0)
	{
		
		m_idents[++(*ptx)] = *ident_p;
				
		(*pdx)++;		

		return &m_idents[(*ptx)];
	}
	else
	{
		identinfo_t *ident_p2 = identinfo_new();

		m_level_gcidents[m_num_level_gcidents++] = ident_p2;

		*ident_p2 = ident_copy(*ident_p);

		return ident_p2;
	}	
}

/*
 * Adds a function identifier.
 */
identinfo_t *ident_add_function_ex(int level, int *ptx, int *pdx, BOOL byref, BOOL procedure)
{
	identinfo_t *ident_p;

	ident_p = &m_idents[++(*ptx)];
	
	ZEROMEMORY(*ident_p);

	ident_p->kind = (procedure) ? KIND_PROCEDURE : KIND_FUNCTION;
	ident_p->data.function_val_p = var_function_new();
	ident_p->data.function_val_p->return_var.kind = KIND_INTEGER;
	ident_p->level = level;	
	ident_p->data.function_val_p->params_num = 0;
	ident_p->data.function_val_p->byref = byref;

	strcpy(ident_p->name, m_ident_name);

	if (ident_p->kind == KIND_PROCEDURE)
	{
		ident_p->data.function_val_p->return_var.kind = KIND_VOID;
	}

	return ident_p;
}

/*
 * Adds a goto label identifier.
 */
identinfo_t *ident_add_goto(int *ptx, int *address, int level)
{
	identinfo_t *ident_p;

	ident_p = &m_idents[++(*ptx)];
	strcpy(ident_p->name, m_ident_name);

	ident_p->kind = KIND_LABEL;	
	ident_p->level = level;
	ident_p->address = *address;

	return ident_p;
}

/*
 * Loads the default for an identifier.
 */
void load_ident_default(identinfo_t* ident_p, int level, int *ptx)
{
	int number;

	if (!ident_p)
	{
		error(4165);

		return;
	}

	if (ident_p->byref)
	{
		instructions_add(OC_LIT, 0, 0);

		return;
	}

	switch(ident_p->kind)
	{
	case KIND_FLOAT:
		
		number = *((int*)(float*)&ident_p->data.float_val.default_val);
		instructions_add(OC_LIT, level - ident_p->level, number);

		break;

	case KIND_INTEGER:

		instructions_add(OC_LIT, level - ident_p->level, ident_p->data.int_val.default_val);

		break;

	case KIND_ARRAY:

		if (ident_p->data.array_val_p->array_default)
		{
			load_new_array(ident_p->data.array_val_p->array_default, level, ptx);
		}
		else
		{
			/*
			 * Alternative default is null.
			 */

			instructions_add(OC_LIT, 0, 0);
		}

		break;

	case KIND_FUNCTION:

		if (ident_p->data.function_val_p->default_p->address < 0)
		{
			instructions_add(OC_FN_LIT, level - ident_p->level, (LONGLONG)ident_p->data.function_val_p->default_p);
		}
		else
		{
			instructions_add(OC_LIT, level - ident_p->level, ident_p->data.function_val_p->default_p->address);
		}
		
		break;

	default:

		error(10120);
	}
}

void pop_kind(int kind, int level, int *ptx)
{
	switch(kind)
	{
	case KIND_ARRAY:

		memory_release_by_handle_off_stack(level, ptx);

		break;

	default:

		instructions_add(OC_DEC, 0, sizeof_kind(kind));
		
	}
}

/*
 * Loads an identifier onto the stack.
 */
void load_ident(identinfo_t* ident_p, int level, int *ptx)
{
	int number;

	if (!ident_p)
	{
		error(4165);

		return;
	}
	
	switch(ident_p->kind)
	{		
	case KIND_INTEGER:
	case KIND_FLOAT:
	case KIND_ARRAY:

		if (ident_p->constant)
		{
			if (ident_p->kind == KIND_INTEGER)
			{
				instructions_add(OC_LIT, level - ident_p->level, ident_p->data.int_val.default_val);
			}
			else if (ident_p->kind == KIND_FLOAT)
			{
				number = *((int*)(float*)&ident_p->data.float_val.default_val);
				instructions_add(OC_LIT, level - ident_p->level, number);
			}				
		}
		else
		{
			if (ident_p->byref)
			{
				instructions_add(OC_LID, level - ident_p->level, ident_p->address);
			}
			else
			{					
				instructions_add(OC_LOD, level - ident_p->level, ident_p->address);
			}
		}

		break;

	default:

		error(4165);

		break;

	}
}

/*
 * Do garbage collection on all identifiers between the start and end.
 */
void do_garbage_collection(int level, int *ptx, int start, int end)
{	
	/*
	 * Do garbage collection.
	 */

	int i;

	for (i = start; i <= end; i++)
	{
		if (m_idents[i].level == level)
		{
			/*
			 * If the identifier is an array, and this function OWNs it.
			 * Arrays that this function get passed by reference are owned
			 * by the outer function, and will automatically be freeed by
			 * the outer function.  Freeing it in this funciton will cause
			 * negative/invalid reference counts and premature destruction.
			 */

			if (m_idents[i].kind == KIND_ARRAY && !m_idents[i].byref
				&& !m_idents[i].is_this
				&& !m_idents[i].constant

				/*
				 * @optimization
				 *
				 * Don't release array refs that aren't even used.
				 */
				&& !m_idents[i].refed == 0)				
			{
				/*
				 * Then release the array.
				 */
				memory_release(&m_idents[i], level, ptx);
			}
		}
	}
}

/*
 * Processes lambda expressions (anonymous functions).
 */
exp_res_t expression_lambda(int level, int *ptx)
{	
	int dx = 3, otx;	
	identinfo_t *ident_p;	
	int cx1, cx2, jmp1, num_params, params_byte_len;
	exp_res_t exp_res, exp_ret_res;

	ZEROMEMORY(exp_res);
	ZEROMEMORY(exp_ret_res);	

	ident_p = identinfo_new();
		
	ident_p->data.function_val_p = var_function_new();
	strcpy(ident_p->name, "lambda");
	
	otx = *ptx;

	if (m_symbol == SYM_LPAREN)
	{
		get_symbol();
	}
	else
	{
		error(6001);
	}
	
	/*
	 * Make the jump past the lambda expression.
	 */
	jmp1 = m_instructions_index;
	instructions_add(OC_JMP, 0, 0);

	/*
	 * Setup the default stack size of the function.
	 */
	cx1 = m_instructions_index;
	instructions_add(OC_INC, 0, 0);
	
	/*
	 * Enter in the parameter defnitions.
	 */
	cx2 = m_instructions_index;
	num_params = process_params(level + 1, ptx, &dx, ident_p);
	params_byte_len = m_instructions_index - cx2;

	/*
	 * Fix the function's stack size.
	 */
	m_instructions[cx1].a = dx;
	
	/*
	 * Right parenthesis of the parameter definitions.
	 */
	if (m_symbol == SYM_RPAREN)
	{
		get_symbol();
	}
	else
	{
		error(6002);
	}

	ident_p->kind = KIND_FUNCTION;

	if (m_symbol == SYM_COLON)
	{
		ident_read_type(&ident_p->data.function_val_p->return_var, level, ptx);
	}
	else
	{
		ident_p->data.function_val_p->return_var.kind = KIND_INTEGER;		
	}

	/*
	 * The => symbol that points to the definition of the function.
	 */
	if (m_symbol == SYM_EQMORE)
	{
		get_symbol();
	}
	else
	{
		error(6010);
	}

	/*
	 * Generate the instructions for the actual function.
	 */

	ident_p->address = cx1;
	ident_p->level = level;
	
	SET_EXP_RES(exp_res, ident_p->data.function_val_p->return_var);

	expression(level + 1, ptx, exp_res, m_empty_tag);

	do_garbage_collection(level + 1, ptx, otx, *ptx);
	
	instructions_add(OC_SRV, 0, 1);
	instructions_add(OC_OPR, 0, 0);

	/*
	 * Restore the variable top stack cause this function deallocates.
	 */
	*ptx = otx;

	/*
	 * Fix the jump past the lambda expression 'code'.
	 */
	m_instructions[jmp1].a = m_instructions_index;

	/*
	 * The <- symbol that redirects arguments.
	 */

	if (m_symbol == SYM_LESSMINUS)
	{
		get_symbol();
	}
	else
	{
		instructions_add(OC_LIT, 0, cx1);

		SET_EXP_RES(exp_res, (*ident_p));

		/*
		 * This is a bit of a hack to make sure this lambda expression's ident
		 * is freeed (it's not in the m_idents stack since it's anonymous).
		 * C has no garbage collection, and m_level_gcidents is freeed at the end of the
		 * compile function.
		 */
		
		m_level_gcidents[m_num_level_gcidents++] = ident_p;

		return exp_res;
	}

	/*
	 * The opening parenthesis of argument list.
	 */

	if (m_symbol != SYM_LPAREN)
	{	
		error(6001);
	}	

	function_procedure_caller(level, ptx, ident_p, FALSE);

	var_function_delete(ident_p->data.function_val_p);

	return exp_res;
}

/*
 * Checks to make sure two arrays contain the same types.
 */
void check_array_kinds(var_array_t *array1, var_array_t *array2)
{
	/* 
	 * If the arrays don't contain the same type then:
	 */
	if (!array1 || !array2)
	{
		/*
		 * Casting later on will try to turn this array into something else
		 * (eg. casting a string into an integer).
		 */
		return;
	}

	if (array1->contains.kind !=
		array2->contains.kind)
	{
		/*
		 * Assigning an array handle that holds handles to any kind of array
		 * is ok.
		 */

		/*
		
		if (array1->contains.kind == KIND_INTEGER
			&& (array1->contains.data.int_val.is_handle))
		{
			return;
		}

		if (array2->contains.kind == KIND_INTEGER
			&& (array2->contains.data.int_val.is_handle))
		{
			return;
		}

		*/
		
		error(10400);
	}
}

/*
 * Calls a function or procedure.
 *
 * @aggregate		TRUE if this is an aggregate call on a function.
 *			e.g. pok(10)(100)(10000).
 */
exp_res_t function_procedure_caller(int level, int *ptx, identinfo_t *ident_p, BOOL aggregate)
{		
	exp_res_t exp_res;

	ZEROMEMORY(exp_res);
	
	if (ident_p->kind == KIND_FUNCTION && ident_p->kind != KIND_VOID)
	{
		/*
		 * Allocate space for the return value (set the default to 0).
		 *
		instructions_add(OC_LIT, 0, 0);

		if (aggregate)
		{
			//
			// Have to swap the function pointer with this return space.
			// OC_CAS expects the function pointer on top.
			//
			instructions_add(OC_SWS, 0, 0);
		}
		*/
	}

	if (m_symbol == SYM_LPAREN)
	{
		get_symbol();

		process_args(level, ptx, ident_p, aggregate);
		
		/*
		 * Return space.
		 */
		instructions_add(OC_LIT, 0, 0);

		if (aggregate)
		{			
			instructions_add(OC_CAS, 0, 0);			
		}		
		/*
		 * If ident_p is actually only holding a reference to a function.
		 */
		else if (ident_p->data.function_val_p->byref)
		{			
			/*
			 * If the function reference we have is a reference to the reference
			 * that holds the actual function address.  Do a 2 level indirect call.
			 */
			if (ident_p->byref)
			{				
				instructions_add(OC_CII, level - ident_p->level, ident_p->address);				
			}
			else
			{
				/*
				 * Otherwise do a one level call (the address of the function is in
				 * the varible).
				 */
				instructions_add(OC_CAI, level - ident_p->level, ident_p->address);
			}
		}
		else
		{
			/*
			 * ident_p must hold a function, this address is the real address of the
			 * function.
			 */

			if (ident_p->address < 0)
			{
				instructions_add(OC_FN_CAL, level - ident_p->level, (LONGLONG)ident_p);
			}
			else
			{
				instructions_add(OC_CAL, level - ident_p->level, ident_p->address);
			}
		}
	
		if (m_symbol == SYM_RPAREN)
		{
			get_symbol();
		}
		else
		{
			error(6002);
		}

		SET_EXP_RES(exp_res, ident_p->data.function_val_p->return_var);				
		
		/*
		 * Support for calling functions from a functional return value.
		 *
		 * eg.  test()(100);
		 */
		if (m_symbol == SYM_LPAREN)
		{
			if (ident_p->data.function_val_p->return_var.kind == KIND_FUNCTION)
			{
				exp_res = function_procedure_caller(level, ptx, &ident_p->data.function_val_p->return_var, TRUE);
																
				if (exp_res.type == KIND_VOID)
				{
					error(4208);
				}
			}
			else
			{
				error(12010);
			}
		}
		else if (m_symbol == SYM_LBRACKET)
		{
			if (ident_p->data.function_val_p->return_var.kind == KIND_ARRAY)
			{					
				exp_res = expression_array_element(&ident_p->data.function_val_p->return_var, level, ptx, TRUE, TRUE);
			}
		}
	}
	else
	{
		/*
		 * No arguments, must be passing as a function pointer.
		 */

		instructions_add(OC_LOD, level - ident_p->level, ident_p->address);

		get_symbol();

		if (m_symbol == SYM_RPAREN)
		{
			get_symbol();
		}
		else
		{
			error(6002);
		}

		exp_res.ident = *ident_p;
	}

	return exp_res;
}

/*
 * Tests to see if an identifier can be a string.
 */
BOOL can_be_string(identinfo_t *ident_p)
{
	return ident_p && ident_p->kind == KIND_ARRAY
		&& ident_p->data.array_val_p->contains.kind == KIND_INTEGER;
}


/*
 * Tests to see if an identifier can be a string.
 */
BOOL can_be_big_number(identinfo_t *ident_p)
{
	return can_be_string(ident_p) && ident_p->data.array_val_p->is_big_number;
}

/*
 * Performs a static cast, from one type to another.
 */
exp_res_t do_static_cast(int level, int *ptx, exp_res_t from, exp_res_t to)
{	
	if (to.type == KIND_NULL)
	{
		from.ident.was_null = TRUE;

		return from;
	}
	else if (from.type == KIND_NULL)
	{
		to.ident.was_null = TRUE;

		return to;
	}

	
	if (can_be_string(&to.ident) && !can_be_big_number(&to.ident)
		&& can_be_big_number(&from.ident))
	{
		/*
		 * BigNumber -> String
		 */

		return exp_string;
	}
	else if (can_be_string(&from.ident) && !can_be_big_number(&from.ident)
		&& can_be_big_number(&to.ident))
	{
		/*
		 * String -> BigNumber
		 */

		return exp_bignumber;
	}



	if (from.type == to.type)
	{
		if (from.type == KIND_FUNCTION)
		{
			if (!signatures_match(from.ident.data.function_val_p, to.ident.data.function_val_p))
			{
				error(11010);
			}
		}

		return to;
	}

	if (to.type == KIND_UNKNOWN)
	{
		return from;
	}

	if (from.type == KIND_UNKNOWN)
	{
		return to;
	}

	if (from.type == KIND_FUNCTION || to.type == KIND_FUNCTION)
	{
		error(12000); /* no way to cast from non function to function. */

		return from;
	}
	
	if (can_be_string(&to.ident) && from.type == KIND_INTEGER && !from.ident.data.int_val.is_handle)
	{
		xtos(level, ptx, from);
	}
	else if (can_be_string(&to.ident) && from.type == KIND_FLOAT)		
	{
		xtos(level, ptx, from);
	}
	else if (can_be_string(&to.ident)
		&& can_be_big_number(&from.ident))
	{
		/*
		 * Casting from a BigNumber to a String is ok.
		 */
	}
	else if (can_be_string(&from.ident)
		&& can_be_big_number(&to.ident))
	{
		/*
		 * Casting from a String to a BigNumber is ok.
		 */
	}
	else if (from.type == KIND_FLOAT)
	{
		if (to.type == KIND_INTEGER)
		{
			instructions_add(OC_FOP, 0, FOP_INT);
		}
	}
	else if (from.type == KIND_INTEGER)
	{
		if (to.type == KIND_FLOAT)
		{
			instructions_add(OC_FOP, 0, FOP_FLO);
		}
		
		 /*
		  * Casting from an integer to an array is ok if the integer is a handle.
		  */
		 
		else if (from.type == KIND_INTEGER
			&& from.ident.data.int_val.is_handle)
		{			
		}
		else
		{
			error(11080);
		}
	}
	else if (to.type == KIND_INTEGER && from.type == KIND_ARRAY
		&& to.ident.data.int_val.is_handle)
	{
		if (from.ident.data.array_val_p->is_literal)
		{
			error(10550);
		}
	}
	else if (from.type == KIND_INTEGER && from.ident.data.int_val.is_handle
		&& to.type == KIND_ARRAY)
	{
	}
	else if (to.type == KIND_INTEGER && can_be_string(&from.ident))
	{
		stoi(level, ptx);
	}
	else if (to.type == KIND_FLOAT && can_be_string(&from.ident))
	{
		stof(level, ptx);
	}
	else
	{
		error(12001);
	}
	
	return to;
}

/*
 * Tests to see if two functions have the same signature.
 */
BOOL signatures_match(var_function_t *func_p1, var_function_t *func_p2)
{
	int i;

	if (!func_p1 || !func_p2)
	{
		return FALSE;
	}

	if (func_p1->params_num != func_p2->params_num)
	{
		return FALSE;
	}

	if (func_p1->return_var.kind != func_p2->return_var.kind)
	{
		return FALSE;
	}

	for (i = 0; i < func_p1->params_num; i++)
	{
		if (func_p1->params[i].kind != func_p2->params[i].kind)
		{
			return FALSE;
		}
		
		if (func_p1->params[i].kind == KIND_FUNCTION || func_p1->params[i].kind == KIND_PROCEDURE)
		{
			if (!signatures_match(func_p1->params[i].data.function_val_p, func_p2->params[i].data.function_val_p))
			{
				return FALSE;
			}
		}
		else if (func_p1->params[i].kind == KIND_ARRAY)
		{
			if (memcmp(func_p1->params[i].data.array_val_p, func_p2->params[i].data.array_val_p, sizeof(var_array_t)) != 0)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/*
 * Loads a function pointer onto the stack.
 */
void pass_or_return_function(identinfo_t *ident_p, int level, int *ptx)
{
	if (ident_p->data.function_val_p->byref)
	{
		if (ident_p->address < 0)
		{
			instructions_add(OC_FN_LOD, level - ident_p->level, (LONGLONG)ident_p);
		}
		else
		{
			instructions_add(OC_LOD, level - ident_p->level, ident_p->address);
		}
	}
	else
	{
		if (ident_p->address < 0)
		{
			instructions_add(OC_FN_LIT, level - ident_p->level, (LONGLONG)ident_p);
		}
		else
		{
			instructions_add(OC_LIT, level - ident_p->level, ident_p->address);
		}
	}
}

/*
 * Loads the handle/address of an array.
 */
void load_array_memory_address(identinfo_t *ident_p, int level, int *ptx)
{
	load_ident(ident_p, level, ptx);	
}

/*
 * Tests to see if an ident can be a handle.
 */
BOOL is_handle(identinfo_t *ident_p)
{
	return ident_p->kind == KIND_INTEGER && ident_p->data.int_val.is_handle;
}

/*
 * Handles unary expressions on identifiers, including arrays.
 */
exp_res_t expression_ident_unary(int level, int *ptx, exp_res_t current_res)
{
	int symbol;
	exp_res_t exp_res;
	
	exp_res = current_res;

	symbol = m_symbol;

	if  (symbol >= SYM_UNARY_START && symbol <= SYM_UNARY_END
			&& 	(current_res.type == KIND_ARRAY || current_res.type == KIND_FLOAT || current_res.type == KIND_INTEGER)
		)
	{
		/*
		 * ++ and -- work differently, when they are used, the original value of the ident
		 * is used.  When another unary operator is used (like *=) the new value is used.
		 */
		if (!(symbol == SYM_PLUS_PLUS || symbol == SYM_MINUS_MINUS))
		{
			if (current_res.type == KIND_ARRAY)
			{
				/*
				 * Make a copy of the array index.
				 */				
				instructions_add(OC_SLD, 0, 0);
			}
		}
		
		exp_res = expression_unary(&current_res.ident, m_symbol, level, ptx, exp_res, FALSE);

		if (!(symbol == SYM_PLUS_PLUS || symbol == SYM_MINUS_MINUS))
		{			
			if (current_res.ident.kind == KIND_ARRAY)
			{				
				expression_pure_array_element(&current_res.ident, level, ptx, FALSE);				
			}
			else
			{
				load_ident(&current_res.ident, level, ptx);
			}
		}
	}
	else
	{
		if (current_res.type == KIND_ARRAY)
		{
			SET_EXP_RES(exp_res, current_res.ident.data.array_val_p->contains);
		}
	}

	return exp_res;
}

/*
 * Loads a number onto the stack.
 */
exp_res_t load_number(int level, int *ptx)
{
	exp_res_t exp_res;

	/*
	 * If number is a float, and they want a float OR integer then treat this as a float.
	 */
	
	exp_res = (m_number_kind == KIND_FLOAT) ? exp_float : exp_integer;

	if (exp_res.type == KIND_FLOAT)
	{
		instructions_add_f(OC_LIT, 0, m_number);
	}
	else if (exp_res.type == KIND_INTEGER)
	{
		instructions_add(OC_LIT, 0, (LONGLONG)m_number);
		exp_res.ident.data.int_val.is_character = m_is_char;		
	}
	else
	{
		error(7500); 
	}

	get_symbol();

	return exp_res;
}


/*
 * Loads up a new literal string, the handle to the new string is loaded onto the stack.
 *
 */
void load_new_array(const int *array, int level, int *ptx)
{
	instructions_add(OC_LIT, 0, ARRAY_HEADER_SIZE);
	instructions_add(OC_LIT, 0, *array);
	instructions_add(OC_OPR, 0, OPR_ADD);
	instructions_add(OC_MAL, 0, 0);
	
	/*
	 * Create a copy of the string handle.
	 */
	instructions_add(OC_SLD, 0, 0);

	/*
	 * Addref the string.
	 */
	memory_addref_by_handle_off_stack(level, ptx);
	
	/*
	 * Load the literal string into the string handle.
	 */
	load_array(array, level, ptx);
}

/*
 * Loads up a literal string.
 *
 * The array handle for the string is ontop of the stack.
 */
void load_array(const int *array, int level, int *ptx)
{
	int i;

	for (i = 0; i < *array; i++)
	{
		instructions_add(OC_SRG, i + ARRAY_HEADER_SIZE, REG_CX);
		instructions_add(OC_LIT, 0, array[i + 1]);

		/*
		 * OC_MSS pops the value to be stored, but not the handle
		 * which sits underneath it.
		 */

		instructions_add(OC_MSS, 0, 1);
	}

	/*
	 * At this point the top of the stack should be the handle of the
	 * string.  Underneath should be the orignal DX register.
	 */
	
	/*
	 * Store the length of the string as the length of the array.
	 */
	instructions_add(OC_SRG, ARRAY_LENGTH_OFFSET, REG_CX);
	instructions_add(OC_LIT, 0, *array);
	instructions_add(OC_MSS, 0, 1);
}

/*
 * Makes the top absolute.
 */
void absolute_top(int kind)
{
	int cx1;

	instructions_add(OC_SLD, 0, 0);
	instructions_add(OC_LIT, 0, 0);

	instructions_add_itof(OC_OPR, 0, OPR_LES, kind);
	cx1 = m_instructions_index;
	instructions_add(OC_JPC, 0, 0);
	instructions_add_itof(OC_OPR, 0, OPR_NEG, kind);
	m_instructions[cx1].a = m_instructions_index;
}


/*
 * Automatically converts integer to floating point operations.
 */
void instructions_add_itof(int f, int l, int a, int kind)
{
	int aa = a;
	BOOL fop = FALSE;

	if (f == OC_OPR)
	{
		if (kind == KIND_FLOAT)
		{

			switch (a)
			{
			case OPR_MUL:
				
				aa = FOP_MUL;
				
				break;

			case OPR_DIV:
				
				aa = FOP_DIV;
				
				break;

			case OPR_MOD:
				
				error(13000); /* can't do mod on float */
				
				break;

			case OPR_ADD:
				
				aa = FOP_ADD;
				
				break;

			case OPR_SUB:
				
				aa = FOP_SUB;
				
				break;

			case OPR_NEG:

				aa = FOP_NEG;

				break;

			case OPR_LES:

				aa = FOP_LES;

				break;

			case OPR_LEQ:

				aa = FOP_LEQ;

				break;

			case OPR_GRE:

				aa = FOP_GRE;

				break;

			case OPR_GRQ:

				aa = FOP_GRQ;

				break;

			case OPR_EQL:

				aa = FOP_EQL;

				break;

			default:
				error(13090); /* unknown operation */

				return;
			}
		}

		instructions_add((kind == KIND_FLOAT) ? OC_FOP : OC_OPR, l, aa);
	}
	else
	{
		switch (f)
		{
		case OC_IAD:
		case OC_ISU:
		case OC_IMU:
		case OC_IDI:
		case OC_IIA:
		case OC_IIS:
		case OC_IIM:

			/*
			 * If the stack holds a floating point number.
			 */
			if (kind == KIND_FLOAT)
			{
				/*
				 * Turn floating point arithmatic on, save the orignal DX in AX.
				 */
				fop = TRUE;
				instructions_add(OC_PUS, 0, REG_DX);
				instructions_add(OC_POP, 0, REG_AX);
				instructions_add(OC_RIO, REG_FLOAT_FLAG, REG_DX);
			}

			break;
		}

		instructions_add(f, l, a);

		/*
		 * If the stack holds a floating point number.
		 */
		if (fop)
		{			
			/*
			 * Restore DX from AX.
			 */
			instructions_add(OC_MOV, REG_AX, REG_DX);
		}
	}
		
	return;
}

/*
 * Make the top of the stack 1 if it's logically TRUE (non zero) and 0 otherwise.
 */
void boolate_top(int level, int *ptx, exp_res_t exp_res)
{		
	do_static_cast(level, ptx, exp_res, exp_integer);
	
	/* Load 0 onto the stack */
	instructions_add(OC_LIT, 0, 0);
	
	/* Compare top and 0 */
	instructions_add(OC_OPR, 0, OPR_EQL);
	
	/*
	 * Do a bitwise not on the answer (the first bit will then be correct).
	 */
	instructions_add(OC_OPR, 0, OPR_NOT);

	/*
	 * Remove all the bits except for the first.
	 */
	instructions_add(OC_LIT, 0, 1);
	instructions_add(OC_OPR, 0, OPR_AND);
}

/*
 * Make the top of the stack 1 if it's logically TRUE (non zero) and 0 otherwise.
 */
void logical_not_top(int level, int *ptx, exp_res_t exp_res)
{	
	do_static_cast(level, ptx, exp_res, exp_integer);
	
	/* Load 0 onto the stack */
	instructions_add(OC_LIT, 0, 0);
	
	/* Compare top and 0 */
	instructions_add(OC_OPR, 0, OPR_EQL);
	
	/*
	 * Remove all the bits except for the first.
	 */
	instructions_add(OC_LIT, 0, 1);
	instructions_add(OC_OPR, 0, OPR_AND);
}

/*
 * Converts a string to an integer.
 */
void stoi(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("s2i", ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{
		/*
		 * Load the second argument (radix).
		 */
		instructions_add(OC_LIT, 0, funcident_p->data.function_val_p->params[1].data.int_val.default_val);

		/*
		 * Store the params.
		 */
		generate_stp(funcident_p);
		generate_stp(funcident_p);

		/*
		 * Convert the number to a string.
		 */		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16010);
	}	
}

/*
 * Converts a string to a float.
 */
void stof(int level, int *ptx)
{
	unary_operation_function_caller("s2f", level, ptx);	
}

/*
 * Converts anything into a string.
 */
void xtos(int level, int *ptx, exp_res_t exp_res)
{
	if (exp_res.type == KIND_INTEGER)
	{
		if (exp_res.ident.data.int_val.is_boolean)
		{
			btos(level, ptx);
		}
		else if (exp_res.ident.data.int_val.is_character)
		{
			ctos(level, ptx);
		}
		else
		{
			itos(level, ptx);
		}		
	}
	else if (exp_res.type == KIND_FLOAT)
	{
		ftos(level, ptx);
	}
}

/*
 * Converts a string into an appropriate type, given the exp_res that contains
 * the type needed.
 */
void stox(int level, int *ptx, exp_res_t exp_res)
{
	if (exp_res.type == KIND_INTEGER)
	{
		stoi(level, ptx);
	}
	else if (exp_res.type == KIND_FLOAT)
	{
		stof(level, ptx);
	}
}
/*
 * Converts a boolean into a string.
 *
 * The boolean must be ontop of the stack.
 */
void btos(int level, int *ptx)
{
	unary_operation_function_caller("b2s", level, ptx);
}

/*
 * Converts an integer into a string.
 *
 * The integer must be ontop of the stack.
 */
void itos(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("i2s", ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{
		/*
		 * Load the second argument (radix).
		 */
		instructions_add(OC_LIT, 0, funcident_p->data.function_val_p->params[1].data.int_val.default_val);

		/*
		 * Store the params.
		 */
		generate_stp(funcident_p);
		generate_stp(funcident_p);

		/*
		 * Convert the number to a string.
		 */		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16010);
	}	
}

/*
 * Converts a character into a string.
 *
 * The character must be ontop of the stack.
 */
void ctos(int level, int *ptx)
{
	unary_operation_function_caller("c2s", level, ptx);	
}

/*
 * Converts a floating point number into a string.
 *
 * The float must be ontop of the stack.
 */
void ftos(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("f2s", ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{
		/*
		 * Load the second argument (decimal places).
		 */
		instructions_add(OC_LIT, 0, funcident_p->data.function_val_p->params[1].data.int_val.default_val);

		/*
		 * Store the params.
		 */
		generate_stp(funcident_p);
		generate_stp(funcident_p);

		/*
		 * Convert the number to a string.
		 */		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16010);
	}	
}

/*
 * Adds two big integer numbers.
 *
 * The two array handles must be ontop of the stack.
 */
void unary_operation_function_caller(const char *function_name, int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find((char *)function_name, ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{	
		generate_stp(funcident_p);
		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16999);
	}
}

/*
 * Adds two big integer numbers.
 *
 * The two array handles must be ontop of the stack.
 */
void binary_operation_function_caller(const char *function_name, int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find((char *)function_name, ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{	
		generate_stp(funcident_p);
		generate_stp(funcident_p);		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16999);
	}
}

/*
 * Creates a new copy of an integer array.
 *
 * The integer array handle must be ontop of the stack.
 */
void new_integer_array_copy(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("new_integer_array_copy", ptx);

	if (funcident_p)
	{
		/*
		 * Load up the default length.
		 */
		instructions_add(OC_LIT, 0, funcident_p->data.function_val_p->params[1].data.int_val.default_val);

		generate_stp(funcident_p);
		generate_stp(funcident_p);	
		
		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16000);
	}
}

/*
 * The stuff inside brackets for a maths expression.
 */
exp_res_t expression_plus_minus(int level, int *ptx, tag_t tag)
{
	int symbol;
	exp_res_t exp_res;
	
	exp_res = expression_bitwise(level, ptx, tag);
	
	if (exp_res.type == KIND_FLOAT || exp_res.type == KIND_INTEGER || exp_res.type == KIND_UNKNOWN
		|| can_be_big_number(&exp_res.ident))
	{
		if ((exp_res.type == KIND_ARRAY && exp_res.ident.data.array_val_p->is_big_number))
		{
			/*int x = 0;*/
		}

		 /* Finally, handle addition and subtraction. */
		while ((m_symbol == SYM_PLUS) || (m_symbol == SYM_MINUS))
		{
			symbol = m_symbol;
				
			get_symbol();

			exp_res = expression_cast_op(expression_bitwise, level, ptx, symbol, exp_res, tag);

			if (exp_res.type == KIND_INTEGER || exp_res.type == KIND_FLOAT)
			{				 
				if (symbol == SYM_PLUS)
				{
					instructions_add_itof(OC_OPR, 0, OPR_ADD, exp_res.type);
				}
				else
				{
					instructions_add_itof(OC_OPR, 0, OPR_SUB, exp_res.type);
				}
			}
			else if (can_be_big_number(&exp_res.ident))
			{
				if (symbol == SYM_PLUS)
				{
					binary_operation_function_caller("big_number_add", level, ptx);
				}
				else
				{
					binary_operation_function_caller("big_number_sub", level, ptx);
				}
			}
			else if (exp_res.type == KIND_ARRAY)
			{
				binary_operation_function_caller("integer_array_append", level, ptx);

				break;
			}
		}
	}
	
	if (exp_res.type == KIND_ARRAY && !can_be_big_number(&exp_res.ident))
	{
		/*
		 * Adding of arrays.
		 */
		while (m_symbol == SYM_PLUS)
		{	
			/*
			 * Create a new copy of the left array, and then append the right
			 * array to it.  Using an integer array copy will work with 4 byte
			 * floats as well.
			 */
			
			get_symbol();
			
			/*
			 * If this is an array, make a new copy to append to.
			 * However if this is a literal string, there's no need, just append
			 * to the literal string, since they don't have a reference to this.
			 */
			if (exp_res.type == KIND_ARRAY			
				&& !exp_res.ident.data.array_val_p->is_literal)
			{
				new_integer_array_copy(level, ptx);
			}

			exp_res = expression_cast_op(expression_bitwise, level, ptx, SYM_PLUS, exp_res, tag);

			if (exp_res.type == KIND_INTEGER || exp_res.type == KIND_FLOAT)
			{
				instructions_add_itof(OC_OPR, 0, OPR_ADD, exp_res.type);
			}
			else
			{
				binary_operation_function_caller("integer_array_append", level, ptx);
			}			
		}
	}

	return exp_res;
}

/*
 * Makes sure that an expression starts properly (for additional error checking).
 * Invalid starts of expressions for example are expressions that start with a
 * infix operator like an _EQL sign.
 */
void check_expression_start()
{
	if		(
				(m_symbol >= SYM_CMP_START && m_symbol <= SYM_CMP_END) ||
				(m_symbol == SYM_BECOMES) ||
				(m_symbol == SYM_EQL)				
			)
	{
		error(4210);
	}
}

/*
 * Processes conditions, also used in which/case statements.
 */
exp_res_t expression_condition_worker(int level, int *ptx, exp_res_t exp_current, BOOL for_case, processing_fn pfn, tag_t tag)
{
	int op, symbol;
	exp_res_t exp_res, exp_res2, ret_exp_res;

	ZEROMEMORY(ret_exp_res);

	/*
	 * @author Thong Nguyen
	 *
	 * Added support for parenthesis around conditional expressions.
	 */

	if (!pfn)
	{
		pfn = expression_raw;
	}

	if (m_symbol == SYM_LPAREN)
	{
		get_symbol();

		exp_res = expression_condition_worker(level, ptx, exp_current, for_case, pfn, tag);

		if (m_symbol != SYM_RPAREN)
		{
			error(6002);
		}

		get_symbol();

		return exp_res;
	}

	/*
	 * If this is for a case expression (where there is no left operand - it's already on the stack).
	 */
	if (for_case)
	{
		/*
		 * Special case for case statements.  This occurs when a case just has a expression, no operators.
		 * eg. 10 opposed to >= 10.
		 */
		if (m_symbol < SYM_CMP_START || m_symbol > SYM_CMP_END)
		{		
			exp_res = expression_cast_op(pfn, level, ptx, 0, exp_current, tag);

			if (exp_res.type != KIND_ARRAY)
			{
				instructions_add(OC_OPR, 0, OPR_EQL);
			}
			else
			{
				binary_operation_function_caller("integer_array_compare", level, ptx);

				exp_res.type = KIND_INTEGER;
						
				instructions_add(OC_LIT, 0, 0);
				instructions_add(OC_OPR, 0, OPR_EQL);				
			}

			exp_res = exp_integer;
			exp_res.ident.data.int_val.is_boolean = TRUE;

			return exp_res;
		}
	}
	else	
	{
		tag_t empty_tag;
		ZEROMEMORY(empty_tag);
		
		exp_current = expression_raw(level, ptx, tag);
		ret_exp_res.type = exp_current.type;
	}

	if (m_symbol >= SYM_CMP_START && m_symbol <= SYM_CMP_END)
	{
		symbol = m_symbol;		
		get_symbol();

		/*
		 * Do supplied expression.
		 */

		check_expression_start();
		exp_res2 = expression_cast_op(pfn, level, ptx, 0, exp_current, tag);
				
		op = condition_operator(symbol, exp_res2.type, tag);
	
		if (exp_res2.type == KIND_ARRAY)
		{
			if (op == OPR_EQL)
			{
				/*
				 * Duplicate the two operands so the two arrays can be freeed later.
				 * Arrays compared with "=" compares array references (not values).
				 */

				instructions_add(OC_POP, 0, REG_AX);
				instructions_add(OC_POP, 0, REG_BX);

				instructions_add(OC_PUS, 0, REG_BX);
				instructions_add(OC_PUS, 0, REG_AX);
				instructions_add(OC_PUS, 0, REG_BX);
				instructions_add(OC_PUS, 0, REG_AX);		
			}
			else
			{
				error(13200);
			}
		}

		if (op >= 0)
		{
			if (exp_res2.type == KIND_FLOAT)
			{
				instructions_add(OC_FOP, 0, op);
				
				/*ret_exp_res.type = KIND_FLOAT;*/

				ret_exp_res = exp_integer;
				ret_exp_res.ident.data.int_val.is_boolean = TRUE;
			}	
			else if (exp_res2.type == KIND_INTEGER
				|| (exp_res2.type == KIND_ARRAY && op == OPR_EQL)
				|| (exp_res2.type == KIND_FUNCTION && op == OPR_EQL)
				|| (exp_res2.type == KIND_NULL && op == OPR_EQL))
			{
				/*
				 * Comparing integers, array handles or functions.
				 */

				instructions_add(OC_OPR, 0, op);
				
				/*ret_exp_res.type = KIND_INTEGER;*/
				
				ret_exp_res = exp_integer;
				ret_exp_res.ident.data.int_val.is_boolean = TRUE;
			}
			else
			{
				error(13300);
			}
		}

		if (exp_res2.type == KIND_ARRAY && op == OPR_EQL)
		{
			/*
			 * Save the conditional result.
			 */

			instructions_add(OC_POP, 0, REG_AX);
			
			/*
			 * Release the two arrays.
			 */

			memory_release_by_handle_off_stack(level, ptx);
			memory_release_by_handle_off_stack(level, ptx);

			/*
			 * Restore the conditional result.
			 */

			instructions_add(OC_PUS, 0, REG_AX);

			/*ret_exp_res.type = KIND_INTEGER;*/

			ret_exp_res = exp_integer;
			ret_exp_res.ident.data.int_val.is_boolean = TRUE;
		}

		
	}
	else
	{
		/*
		 * No operator, just use the value that's on the stack as the result.
		 * eg. it is if x then opposed to if x > 10 then.
		 */
	}

	return ret_exp_res;
}

/*
 * Works out conditional operator to use given the kind needed (integer or float).
 */
int condition_operator(int symbol, int kind, tag_t tag)
{
	switch (symbol)
	{
		case SYM_EQL:
			
			if (kind == KIND_FLOAT)
			{
				return FOP_EQL;
			}
			else
			{
				return OPR_EQL;
			}

		case SYM_NOT_EQL:
			
			if (kind == KIND_FLOAT)
			{
				return FOP_NEQ;
			}
			else
			{
				return OPR_NEQ;
			}

		case SYM_LESS:

			if (kind == KIND_FLOAT)
			{
				return FOP_LES;
			}
			else
			{
				return OPR_LES;
			}

		case SYM_LES_EQL:
			
			if (kind == KIND_FLOAT)
			{
				return FOP_LEQ;
			}
			else
			{
				return OPR_LEQ;
			}

		case SYM_GREATER:
			
			if (kind == KIND_FLOAT)
			{
				return FOP_GRE;
			}
			else
			{
				return OPR_GRE;
			}

		case SYM_GRE_EQL:

			if (kind == KIND_FLOAT)
			{
				return FOP_GRQ;
			}
			else
			{
				return OPR_GRQ;
			}
			
	}

	return -1;
}

void array_realloc_by_handle_off_stack(int level, int *ptx, int size)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__array_check", ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{
		/*
		 * Load the size.
		 */
		instructions_add(OC_LIT, 0, size);
		
		generate_stp(funcident_p);
		generate_stp(funcident_p);

		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16000);
	}	
}

/*
 * Reallocates an array reference given an array.
 */
void array_realloc(identinfo_t *ident_p, int level, int *ptx, int size)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__array_check", ptx);

	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{
		/*
		 * Load the the array handle.
		 */		
		generate_load(ident_p, level);
		
		/*
		 * Load the size.
		 */
		instructions_add(OC_LIT, 0, size);
		
		generate_stp(funcident_p);
		generate_stp(funcident_p);

		generate_call_func(funcident_p, level);
	}
	else
	{
		error(16000);
	}	
}

/*
 * Addrefs an array reference given an array.
 */
void memory_addref(identinfo_t *ident_p, int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__memory_addref", ptx);
	
	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{		
		generate_load(ident_p, level);
		
		generate_stp(funcident_p);

		generate_call_sub(funcident_p, level);
	}
	else
	{
		error(16000);
	}
}

/*
 * Releases an array reference given an array.
 */
void memory_release(identinfo_t *ident_p, int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__memory_release", ptx);
	
	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{		
		generate_load(ident_p, level);
		
		generate_stp(funcident_p);

		generate_call_sub(funcident_p, level);
	}
	else
	{
		error(16000);
	}
}

/*
 * Addrefs an array reference.
 *
 * The handle of the array should be on top of the stack.
 * The handle of the array will be popped off the stack.
 */
void memory_addref_by_handle_off_stack(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__memory_addref", ptx);
	
	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{	
		generate_stp(funcident_p);

		generate_call_sub(funcident_p, level);
	}
	else
	{
		error(16000);
	}
}

/*
 * Releases an array reference.
 *
 * The handle of the array should be on top of the stack.
 * The handle of the array will be popped off the stack.
 */
void memory_release_by_handle_off_stack(int level, int *ptx)
{
	identinfo_t *funcident_p;

	funcident_p = ident_find("__memory_release", ptx);
	
	if (funcident_p && funcident_p->kind == KIND_FUNCTION)
	{	
		generate_stp(funcident_p);

		generate_call_sub(funcident_p, level);
	}
	else
	{
		error(16000);
	}
}

/*
 * Loads the memory offset of an array given an array index, and also does array
 * boundary check.
 */
void array_load_offset(identinfo_t *ident_p, int level, int *ptx, BOOL aggregate)
{	
	int kind;
	identinfo_t *ident_p2;
	exp_res_t exp_res;

	if (ident_p->kind == KIND_ARRAY)
	{
		get_symbol();

		
		kind = KIND_INTEGER;

		/*
		 * Expression pushes the array index on the stack.
		 */

		exp_res = expression_raw(level, ptx, m_empty_tag);
		
		if (m_options.array_check_boundaries)
		{
			/*
			 * Copy the index for bounds checking.
			 */
			instructions_add(OC_SLD, 0, 0);

			/*
			 * Check the size of the array is ok.
			 */
			ident_p2 = ident_find("__array_check", ptx);

			if (ident_p2 && ident_p2->kind == KIND_FUNCTION)
			{				
				if (!aggregate)
				{
					/*
					 * Load the array handle.
					 */
					generate_load(ident_p, level);

					/*
					 * Swap the array handle and index.
					 */
					instructions_add(OC_SWS, 0, 0);

					/*
					 * Store the index parameter.
					 */
					generate_stp(ident_p2);
					/*
					 * Store the array handle as an argument.
					 */
					generate_stp(ident_p2);
				}
				else
				{
					/*
					 * Load the array handle (was on top stack before this function call).
					 */
					instructions_add(OC_SLD, 0, 2);

					/*
					 * Swap array handle and index.
					 */
					instructions_add(OC_SWS, 0, 0);

					/*
					 * Store index then array handle.
					 */
					generate_stp(ident_p2);
					generate_stp(ident_p2);
				}
				
				generate_call_sub(ident_p2, level);
			}
			else
			{
				error(16000);
			}
		}

		/*
		 * Add the header to the array index to get the memory offset.
		 */
		
		instructions_add(OC_LIT, 0, ARRAY_HEADER_SIZE);
		instructions_add(OC_OPR, 0, OPR_ADD);		

		if (m_symbol == SYM_RBRACKET)
		{
			get_symbol();
		}
		else
		{
			error(7003);
		}
	}
}

int* static_arraydef(identinfo_t *ident_p, int level, int *ptx)
{
	int i, oix, optimize;
	int *i_p;	
	exp_res_t exp_res;

	ZEROMEMORY(exp_res);

	i_p = MALLOC(int, 255);

	i = 1;

	do
	{
		get_symbol();

		if (m_symbol == SYM_END)
		{
			break;
		}

		optimize = m_options.optimize;
		m_options.optimize = TRUE;

		oix = m_instructions_index;

		SET_EXP_RES(exp_res, ident_p->data.array_val_p->contains);

		expression(level, ptx, exp_res, m_empty_tag);

		m_options.optimize = optimize;

		if (m_instructions_index - oix != 1
			|| m_instructions[m_instructions_index - 1].f != OC_LIT)
		{
			error(10350);
		}

		i_p[i] = m_instructions[m_instructions_index - 1].a;

		m_instructions_index = oix;
		
		i++;
	}
	while (m_symbol == SYM_COMMA);

	*i_p = i - 1;

	if (m_symbol == SYM_END)
	{
		get_symbol();
	}
	else
	{
		error(6204);
	}

	return i_p;	
}

exp_res_t expression_arraydef(int level, int *ptx, tag_t tag)
{
	int cx,  i = 0;
	identinfo_t *ident_p2;
	exp_res_t exp_res, exp_wanted;
	
	ZEROMEMORY(exp_res);

	if (m_symbol == SYM_BEGIN && tag.ident_p && tag.ident_p->kind == KIND_ARRAY)
	{
		cx = m_instructions_index;
		instructions_add(OC_LIT, 0, 0);		
		instructions_add(OC_MAL, 0, 0);
		
		/*
		 * Create a copy of the string handle.
		 */
		instructions_add(OC_SLD, 0, 0);

		/*
		 * Addref the string.
		 */
		memory_addref_by_handle_off_stack(level, ptx);

		ident_p2 = tag.ident_p;

		SET_EXP_RES(exp_wanted, tag.ident_p->data.array_val_p->contains);
		
		tag.ident_p = &tag.ident_p->data.array_val_p->contains;

		do
		{
			get_symbol();

			if (m_symbol == SYM_END)
			{
				break;
			}

			instructions_add(OC_SRG, i + ARRAY_HEADER_SIZE, REG_CX);
						
			exp_res = expression(level, ptx, exp_wanted, tag);

			instructions_add(OC_MSS, 0, 1);
			
			i++;
		}
		while (m_symbol == SYM_COMMA);

		if (m_symbol == SYM_END)
		{
			get_symbol();
		}
		else
		{
			error(6204);
		}

		instructions_add(OC_SRG, ARRAY_LENGTH_OFFSET, REG_CX);
		instructions_add(OC_LIT, 0, /*ARRAY_HEADER_SIZE + */ i);
		instructions_add(OC_MSS, 0, 1);

		m_instructions[cx].a = ARRAY_HEADER_SIZE + i;
	
		SET_EXP_RES(exp_res, *ident_p2);
	}
	
	return exp_res;
}

/*
 * Processes a becomes statements (eg. x := 10).
 */
exp_res_t statement_becomes(identinfo_t *ident_p, int level, int *ptx, BOOL push, tag_t tag)
{
	int tx = 0;
	BOOL ok = TRUE;
	BOOL in_memory_store = FALSE;
	exp_res_t exp_res, exp_current;
		
	ZEROMEMORY(exp_res);
	ZEROMEMORY(exp_current);
	
	if (ident_p->constant)
	{
		ok = FALSE;

		error(13010);
	}
	
	if (m_symbol == SYM_BECOMES)
	{
		get_symbol();		
	}
	else if (m_symbol == SYM_EQL)
	{
		get_symbol();

		error(4800);
	}
		
	if (ok && ident_p)
	{		
		tx = *ptx;
				
		if (ident_p->kind == KIND_ARRAY && !ident_p->data.array_val_p->assigning_ref)
		{
			SET_EXP_RES(exp_current, ident_p->data.array_val_p->contains);
		}
		else
		{
			SET_EXP_RES(exp_current, (*ident_p));
		}

		tag.ident_p = ident_p;
		exp_res = expression(level, ptx, exp_current, tag);

		/*
		 * If this is an assignment to a array.
		 */
		if (ident_p->kind == KIND_ARRAY)
		{
			/*
			 * If we should be assigning a reference to the array (not an element)
			 */
			if (ident_p->data.array_val_p->assigning_ref)
			{				
				/*
				 * Then release the current array being referenced.
				 */

				/*
				 * The last expression that was evaluated should come out to be an
				 * array reference, and would automatically be addrefed.
				 */						

				/*
				 * A little optimization thing.  No need to release what's in the var
				 * if we know for sure that this var hasn't been used yet.
				 */
				if (ident_p->refed > 0)					
				{
					memory_release(ident_p, level, ptx);
				}
			}
			else
			{
				/*
				 * This is an actual array element we want to store into 
				 */

				/*
				 * Swap the top two elements on the stack.
				 */
				instructions_add(OC_SWS, 0, 0);

				/*
				 * The index of the array is now on top, pop it off into the CX register.
				 * The top of the stack will now be the last expression result again.
				 */
				instructions_add(OC_POP, 0, REG_CX);
				
				in_memory_store = TRUE;
			}
		}
		
		ident_p->refed++;

		/*
		 * If the result of this operation needs to remain on the stack.
		 */
		if (push)
		{
			/*
			 * Duplicate this result on the stack before storing it.
			 */
			instructions_add(OC_SLD, 0, 0);
		}
		
		if (ident_p->byref)
		{	
			instructions_add_mos(OC_SID, level - ident_p->level, ident_p->address, in_memory_store);
		}
		else
		{
			instructions_add_mos(OC_STO, level - ident_p->level, ident_p->address, in_memory_store);
		}
	
		/*
		 * If this was an array reference assignment, and the result needs to be returned.
		 * then addref.
		 */
		
		if (ident_p->kind == KIND_ARRAY && ident_p->data.array_val_p->assigning_ref && push)
		{
			if (!(exp_res.type == KIND_ARRAY && exp_res.ident.was_null))
			{
				memory_addref(ident_p, level, ptx);
			}
		}				
	}
	else
	{
		error(11060);
	}

	return exp_res;
}

/*
 * Adds an instruction.  Automatically converts stack based stores and loads in memory
 * store and load instructions.
 */
void instructions_add_mos(int f, int l, int a, BOOL memory)
{
	switch (f)
	{
	case OC_STO:
		
		if (memory)
		{
			instructions_add(OC_MST, l, a);
		}
		else
		{
			instructions_add(OC_STO, l, a);
		}

		break;

	case OC_LOD:

		if (memory)
		{
			instructions_add(OC_MLO, l, a);
		}
		else
		{
			instructions_add(OC_LOD, l, a);
		}

		break;

	case OC_SID:

		if (memory)
		{
			instructions_add(OC_MSI, l, a);
		}
		else
		{
			instructions_add(OC_SID, l, a);
		}

		break;
		

	case OC_LID:

		if (memory)
		{
			instructions_add(OC_MLI, l, a);
		}
		else
		{
			instructions_add(OC_LID, l, a);
		}

		break;

	default:

		instructions_add(f, l, a);
	}
}

/*
 * Reads and generates code for a for loop.
 *
 * eg. for i := 1 to 10 step 2.
 * 
 * counts from 1 to 10 in steps of 2, while i <= 10.
 */
void statement_for(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{	
	identinfo_t *ident_p;	
	exp_res_t exp_res;
	int kind, cx1, cx2, cx3;
	int for_instructions_start, for_instructions_end;	
	int to_start = -1, to_end = -1, step_start = -1, step_end = -1;
	
	/*
	 * The symbol after a for should be a counter variable.
	 */
	if (m_symbol == SYM_IDENT)
	{		
		ident_p = ident_find(m_ident_name, ptx);
	}
	else
	{
		error(4002);

		get_symbol();

		return;
	}

	if (!ident_p)
	{
		error(4000);

		return;
	}

	if (!ident_p->kind == KIND_INTEGER || !ident_p->kind == KIND_FLOAT)
	{
		error(10300);
	}

	/*
	 * This is the part after the for.  It should be an assignment.
	 * eg.  i := 10
	 */
	get_symbol();

	statement_becomes(ident_p, level, ptx, FALSE, m_empty_tag);

	/*
	 * The next symbol should be a to.
	 */

	if (m_symbol == SYM_TO)
	{
		get_symbol();
	}
	else
	{
		error(6200);
	}		
	
	/*
	 * Evaluate the to expression, it should come out to the same type as
	 * the kind of the counter.
	 */	
	kind = ident_p->kind;
	
	/*
	 * to_start is the address of the start of the "to" instructions.
	 */
	
	to_start = m_instructions_index;	
	
	/*
	 * Skip all the "to" and "step" code for now.
	 */

	instructions_add(OC_JMP, 0, 0);

	/*
	 * Do the expression for the "to" part of the for loop, it's implemented as
	 * a function cause it needs to be used in several places.
	 */	

	ZEROMEMORY(exp_res);

	SET_EXP_RES(exp_res, (*ident_p));

	expression(level, ptx, exp_res, m_empty_tag);
	to_end = m_instructions_index;

	instructions_add(OC_JMP, 0, 0);

	/*
	 * If there is a "step" statement.
	 */
	if (m_symbol == SYM_STEP)
	{
		get_symbol();

		step_start = m_instructions_index;		
		/*
		 * Evaluate the step instructions.
		 */		
		expression(level, ptx, exp_res, m_empty_tag);
		step_end = m_instructions_index;
		instructions_add(OC_JMP, 0, 0);		
	}
	
	/*
	 * If the next symbol is a do.
	 */
	if (m_symbol == SYM_DO)
	{		
		get_symbol();
	}
	else
	{
		/*
		 * It's ok not to have a do as long as they have a begin.
		 */
		if (m_symbol != SYM_BEGIN)
		{
			error(6202);
		}
	}
	
	/*
	 * Fix the jump that skips the to and skip code.
	 */
	m_instructions[to_start].a = m_instructions_index;
	
	/*
	 * Store a copy of the orignal starting value.
	 */
	instructions_add(OC_LOD, level - ident_p->level, ident_p->address);

	/*
	 * cx1 is the address of the instructions for checking the counter.
	 */
	cx1 = m_instructions_index;
		
	instructions_add(OC_SLD, 0, 0);
	
	/*
	 * Evaluate the "to" expression.
	 */		
	instructions_add(OC_JMP, 0, to_start + 1);
	m_instructions[to_end].a = m_instructions_index;
	
	/*
	 * Pass a copy of the result to AX.
	 * AX is a free scratch pad register that isn't garunteed to be preserved
	 * across function calls, so no need to push/pop AX.
	 */
	instructions_add(OC_PAS, 0, REG_AX);

	/*
	 * If the "to" is more than the original starting value then.
	 */
	instructions_add(OC_OPR, 0, OPR_LEQ);
	instructions_add(OC_JPC, 0, m_instructions_index + 5);

	/*
	 * Do a <= comparison.
	 */
	instructions_add(OC_PUS, 0, REG_AX);
	load_ident(ident_p, level, ptx);	
	instructions_add(OC_OPR, 0, OPR_GRQ);
	instructions_add(OC_JMP, 0, m_instructions_index + 4);

	/*
	 * Otherwise do a >= comparison.
	 */
	instructions_add(OC_PUS, 0, REG_AX);
	load_ident(ident_p, level, ptx);	
	instructions_add(OC_OPR, 0, OPR_LEQ);
	
	/*
	 * cx2 is the address of the following conditional jump statement.
	 */
	cx2 = m_instructions_index;

	/*
	 * Do a jump out of the loop if the counter is too high or low.
	 */
	instructions_add(OC_JPC, 0, 0);
	
	/*
	 * Load a copy of the orignal starting value, and the current 'to' value.
	 */
	instructions_add(OC_SLD, 0, 0);
	instructions_add(OC_PUS, 0, REG_AX);

	/*
	 * for_instructions_start is the address of the first instruction inside the for.
	 */
	for_instructions_start = m_instructions_index;

	/*
	 * Do the stuff inside the for loop.
	 */
	statement(level, ptx, pdx, func_ident_p);

	/*
	 * for_instructions_end is the address of the first instruction after the for.
	 */
	for_instructions_end = m_instructions_index;
	
	/*
	 * If the original start value and 'to' value are not equal then continue the
	 * loop.  Otherwise skip out of the loop.
	 * This is actually needed for the special case:
	 *
	 * for i := 0 to 0 step -1. (which would loop forever since i = (0, -1, -2)).
	 */
	instructions_add(OC_OPR, 0, OPR_NEQ);
	cx3 = m_instructions_index;
	instructions_add(OC_JPC, 0, 0);
	
	/*
	 * If there is a step.
	 */
	if (step_start >= 0)
	{
		/*
		 * Evaluate the step code.
		 */
		instructions_add(OC_JMP, 0, step_start);
		m_instructions[step_end].a = m_instructions_index;		
	}
	else
	{
		/*
		 * Otherwise the default step is 1.
		 */
		instructions_add(OC_LIT, 0, 1);
	}

	/*
	 * Do an inline add with the "step".
	 */	
	instructions_add_itof(OC_IAD, level - ident_p->level, ident_p->address, kind);
		
	/*
	 * Jump backup to to start of the for loop (the part that checks the counter's range).
	 */
	instructions_add(OC_JMP, 0, cx1);

	/*
	 * Fix the destination of the address of the conditional jumps (above).
	 */
	m_instructions[cx2].a = m_instructions_index;
	m_instructions[cx3].a = m_instructions_index;
		
	/*
	 * Get rid of the starting value we loaded right at the top of the for loop.
	 */		
	instructions_add(OC_DEC, 0, 1);

	cx1 = m_instructions_index;
	instructions_add(OC_JMP, 0, 0);
		
	/*
	 * If there were any "exit for" statements inside the for loop, fix them up to jump here.
	 */	
	instructions_fix_jumps(for_instructions_start, for_instructions_end, CONT_FOR, for_instructions_end);
	instructions_fix_jumps(for_instructions_start, for_instructions_end, CONT_ANY, for_instructions_end);
	
	instructions_fix_jumps(for_instructions_start, for_instructions_end, EXIT_FOR, m_instructions_index);
	instructions_fix_jumps(for_instructions_start, for_instructions_end, EXIT_ANY, m_instructions_index);
	
	/*
	 * Need to DEC 3 for a local exit for too..
	 */
	instructions_add(OC_DEC, 0, 3);

	m_instructions[cx1].a = m_instructions_index;
	
	/*
	 * Each exit statement inside this for loop needs to decrement another 3 if it hasn't
	 * been 'fixed' yet.
	 * Decrement 3.  1 for the starting value, 1 for the to value, and one for current to value.
	 */
	instructions_fix_jump_decs(for_instructions_start, for_instructions_end, 3);
}

void instructions_fix_jump_decs(int start, int end, int decamount)
{
	int i;

	for (i = start; i <= end; i++)
	{
		if (m_instructions[i].f == OC_JMP || m_instructions[i].f == OC_JPC)
		{
			if (m_instructions[i].l 
				&& m_instructions[i].l != JMP_RETURN)				
			{
				m_instructions[i - 1].a += decamount;				
			}
		}
	}
}

/*
 * Scans instructoins from start to end and fixes the destination of JMP and JPC statements. 
 *
 * @param start		The start index of the instructions to search.
 * @param end		The end index of the instructions to search.
 * @param jmptype	The type of jump statement.
 * @param dest		The destination address for the jump statements.
 */
void instructions_fix_jumps(int start, int end, int jmptype, int dest)
{
	int i;

	for (i = start; i <= end; i++)
	{
		if (m_instructions[i].f == OC_JMP || m_instructions[i].f == OC_JPC)
		{
			if (m_instructions[i].l == jmptype)
			{
				if (m_instructions[i].a == 1)
				{
					m_instructions[i].l = 0;
					m_instructions[i].a = dest;
				}
				else
				{
					m_instructions[i].a--;
				}
			}
		}
	}
}

/*
 * Checks between a range, for JMP or JPC statements that haven't beed 'fixed'.
 * This happens when there's a goto or exit statement that doesn't have a matching
 * label.
 *
 * @param start		The start index of the instructions to search.
 * @param end		The end index of the instructions to search.
 */
void instructions_check_orphanded_jumps(int start, int end)
{
	int i;

	for (i = start; i < end; i++)
	{
		if (m_instructions[i].f == OC_JMP || m_instructions[i].f == OC_JPC)
		{
			if (m_instructions[i].l != 0)
			{
				error(11600);
			}
		}
	}	
}

/*
 * Reads an assembly instruction parameter.
 */
BOOL read_asm(int level, int *ptx, int *cx)
{
	identinfo_t *ident_p;
	get_symbol();

	/*
	 * % means get the level difference of the ident.
	 */
	if (m_symbol == SYM_PERCENT)
	{
		get_symbol();

		ident_p = ident_find(m_ident_name, ptx);

		if (ident_p)
		{
			*cx = level - ident_p->level;
		}
		else
		{
			*cx = 0;
			error(4200);
			
			return FALSE;
		}		
	}
	/*
	 * & means get the instruction index.
	 */
	else if (m_symbol == SYM_AMPERSAND)
	{
		get_symbol();

		if (m_symbol == SYM_LITERAL_NUMBER)
		{
			*cx = m_instructions_index + (int)m_number;
		}
		else
		{
			*cx = 0;
			error(4200);

			return FALSE;
		}
	}
	/*
	 * % Loads up the number.
	 */
	else if (m_symbol == SYM_LITERAL_NUMBER)
	{
		if (m_number_kind == KIND_FLOAT)
		{
			memcpy(cx, &m_number, sizeof(float));
		}
		else
		{
			*cx = (int)m_number;
		}
	}
	/*
	 * Load an identifier.
	 */
	else if (m_symbol == SYM_IDENT)
	{		
		ident_p = ident_find(m_ident_name, ptx);

		if (ident_p)
		{
			/*
			 * It's a constant.
			 */
			if (ident_p->constant)
			{
				if (ident_p->kind == KIND_INTEGER)
				{
					*cx = ident_p->data.int_val.default_val;
				}
				else
				{
					memcpy(cx, &ident_p->data.float_val.default_val, sizeof(float));					
				}				
			}
			else 
			{
				*cx = ident_p->address;
			}
		}
		else
		{
			*cx = 0;
			error(4200);

			return FALSE;
		}
	}
	else
	{
		error(6600);

		return FALSE;
	}

	return TRUE;
}

/*
 * Inline assembly/machine code statments.
 */
void statement_asm(int level, int *ptx)
{	
	int l, a, instr;
	identinfo_t *ident_p;

	if (m_symbol == SYM_BEGIN)
	{
		get_symbol();
	}
	else
	{
		error(6202);
	}

	do
	{		
		if (m_symbol <= 0)
		{
			error(15002);
		}
		
		/*
		 * The instruction code should be a number or a constant.
		 */

		if (m_symbol == SYM_IDENT)
		{
			ident_p = ident_find(m_ident_name, ptx);

			if (ident_p && ident_p->kind == KIND_INTEGER && ident_p->constant)
			{
				instr = ident_p->data.int_val.default_val;
			}
			else
			{
				error(6600);

				break;
			}
		}
		else if (m_symbol == SYM_LITERAL_NUMBER && m_number_kind != KIND_FLOAT )
		{
			instr = (int)m_number;
		}
		else
		{
			instr = 0;
			error(6600);

			break;
		}

		if (!read_asm(level, ptx, &l))
		{
			break;
		}

		if (!read_asm(level, ptx, &a))
		{
			break;
		};

		instructions_add(instr, l, a);

		get_symbol();
	}
	while (m_symbol != SYM_END && m_symbol != SYM_EOFSYM);

	if (m_symbol == SYM_END)
	{
		get_symbol();
	}
	else
	{
		error(6204);
	}
}

/*
 * Processes return statements.
 */
void statement_return(int level, int *ptx, int *pdx, identinfo_t *func_ident_p, BOOL exit)
{	
	identinfo_t *ident_p2;	
	exp_res_t exp_res;

	if (func_ident_p && func_ident_p->data.function_val_p->return_var.kind != KIND_VOID)
	{		
		get_symbol();

		/*
		 * Expression should load onto the stack the same type as functon return's type.
		 */

		SET_EXP_RES(exp_res, func_ident_p->data.function_val_p->return_var);
		
		exp_res = expression(level, ptx, exp_res, m_empty_tag);
		
		ident_p2 = ident_find("retval", ptx);

		if (ident_p2 && func_ident_p->data.function_val_p->return_var.kind == KIND_ARRAY)
		{	
			memory_release(ident_p2, level, ptx);
		}
		
		instructions_add(OC_SRV, 0, 1);

		if (exit)
		{
			instructions_add(OC_JMP, JMP_RETURN, 0);
		}
	}
	else
	{
		get_symbol();

		if (m_symbol == SYM_SEMICOLON)
		{	
			instructions_add(OC_JMP, JMP_RETURN, 0);
		}
		else
		{
			error(11030); /* trying to set a void return value */
			get_symbol(); /* get_symbol() after the error to get a more accurate location of the error */
		}
	}
}

/*
 * Processes which statements.
 */
void statement_which(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	tag_t tag;
	BOOL release = FALSE;
	exp_res_t exp_res;	
	int cx1 = -1, cx2 = -1, start, op, sx1, sx2;

	ZEROMEMORY(exp_res);
	ZEROMEMORY(tag);

	get_symbol();

	/*
	 * Evaluate the which expression.  The expression result is kept on the stack
	 * rather than a register because which statements can be embedded.
	 */
	
	exp_res = expression_raw(level, ptx, tag);

	if (exp_res.type == KIND_ARRAY)
	{
		release = TRUE;
	}

	if (m_symbol == SYM_BEGIN)
	{
		get_symbol();
	}
	else
	{
		error(6202);
	}
	
	if (m_symbol != SYM_CASE)
	{
		/*
		 * Get rid of the above expression's result
		 */
		
		if (release)
		{
			memory_release_by_handle_off_stack(level, ptx);
		}
		else
		{
			instructions_add(OC_DEC, 0, 1);
		}

		error(6216);
	}

	start = m_instructions_index;

	while (m_symbol == SYM_CASE)
	{
		get_symbol();

		if (m_symbol == SYM_ELSE)
		{
			break;
		}
		
		/*
		 * Load up a 0, as the first operand.  This will be used if you have a case like:
		 *
		 * case 10 || 20.
		 *
		 * The case=10 will be ORed with this 0, then the result of that is ORed with case=20.
		 */

		instructions_add(OC_LIT, 0, 0);
		
		/*
		 * First time round, do an OR with the 0 (eg. true if true).
		 */

		op = OPR_BOR;

		/*
		 * Since we handle our own && and || operators, the expression shouldn't do it.
		 */

		tag.no_logical_and_or = TRUE;

		/*
		 * Evaluated all the conditions for each case.
		 *
		 * @example case >= 0 or 1:
		 */
		for (;;)
		{
			cx1 = -1;

			/*
			 * Load up the which expression's value off the stack (two pos down).
			 */			
			instructions_add(OC_SLD, 0, 1);
			
			if (release)
			{
				instructions_add(OC_SLD, 0, 0);

				memory_addref_by_handle_off_stack(level, ptx);
			}		
			
			expression_condition_worker(level, ptx, exp_res, TRUE, NULL, tag);

			/*
			 * Do the operation with the previous condition's answer (which was a bool)
			 */
			
			instructions_add_itof(OC_OPR, 0, op, KIND_INTEGER);			

			/*
			 * After the condition, the top of the stack is 0 (FALSE) or 1 (TRUE).
			 */

			/*
			 * If there's another condition for the case (&& or ||).
			 */
			if (m_symbol == SYM_LOR)
			{
				/*
				 * Bitwise OR will do a logical OR with the first bit.
				 */

				op = OPR_BOR;
				get_symbol();
			}
			else if (m_symbol == SYM_LAND)
			{
				/*
				 * Bitwise AND will do a logical AND with the first bit.
				 */

				 op = OPR_AND;
				 get_symbol();
				 
				 /*
				  * This is for lazy evaluation.
				  */

				 /*
				  * Since we want the result to remain if the JPC fails, SLD
				  * the result first.
				  */
				 
				 instructions_add(OC_SLD, 0, 0);

				 cx1 = m_instructions_index;

				 instructions_add(OC_JPC, 0, 0);				 
			}
			else
			{
				break;
			}
		}			
		
		/*
		 * Jump to the next case statement if this case isn't true.
		 */

		cx2 = m_instructions_index;
		instructions_add(OC_JPC, 0, 0);

		if (m_symbol == SYM_COLON)
		{
			get_symbol();
		}
		else
		{
			error(6017);
		}

		/*
		 * Do the statments for this case.
		 */

		sx1 = m_instructions_index;

		/*
		 * Cases support more than one statement without the need for
		 * 'begin' and 'end' ...this is C style.
		 */
		process_statement_block(level, ptx, pdx, func_ident_p);

		if (m_symbol == SYM_CASE || m_symbol == SYM_END)
		{
		}
		else
		{
			error(6019);
		}

		sx2 = m_instructions_index;

		if (m_symbol == SYM_SEMICOLON)
		{
			get_symbol();
		}
		
		/*
		 * The case statements have been done.  Exit the which statement.
		 */

		instructions_add(OC_JMP, EXIT_WHICH, 1);	
		
		/*
		 * This is for lazy evaluation.
		 */

		if (cx1 != -1)
		{
			/*
			 * Fix the lazy evaluation jump point.
			 */
		
			m_instructions[cx1].a = m_instructions_index;			
			
			/*
			 * Have to get rid of the case result since it's not needed anymore.
			 */

			instructions_add(OC_DEC, 0, 1);
		}

		instructions_fix_jumps(sx1, sx2, CONT_WHICH, m_instructions_index);

		/*
		 * Fix the jump for the case to go to the next case if the case was false.
		 */
		m_instructions[cx2].a = m_instructions_index;
	}

	/*
	 * Default case (case else) is handled here.
	 */

	if (m_symbol == SYM_ELSE)
	{
		get_symbol();

		if (m_symbol == SYM_COLON)
		{
			get_symbol();

			statement(level, ptx, pdx, func_ident_p);

			if (m_symbol == SYM_SEMICOLON)
			{
				get_symbol();
			}
		}
		else
		{
			error(6017);
		}
	}		
		
	/*
	 * Fix all the [exit which] statements.
	 */

	instructions_fix_jumps(start, m_instructions_index, EXIT_WHICH, m_instructions_index);
	

	/*
	 * Make sure any exit statement that passes by this which deallocates stuff this which
	 * statement will never get a chance to.
	 *
	 * BUGBUG: There is a SERIOUS problem here with garbage collection, in that there is no 
	 * chance to garbage collect if an exit statement exits past this which, and the which
	 * is working on strings.
	 */
	instructions_fix_jump_decs(start, m_instructions_index, 1);

	/*
	 * If this is a memory comparison.
	 */

	if (release)
	{
		/*
		 * Release the memory/array.
		 */

		memory_release_by_handle_off_stack(level, ptx);
	}
	else
	{
		/*
		 * Just get rid of the expression.
		 */

		instructions_add(OC_DEC, 0, 1);
	}
		
	if (m_symbol == SYM_END)
	{
		get_symbol();
	}
	else
	{
		error(6204);
	}
}

/*
 * Processes statements that start with identifiers.
 */
void statement_ident(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	tag_t tag;
	exp_res_t exp_res;

	get_symbol();

	if (m_symbol == SYM_COLON)
	{			
		ident_add_goto(ptx, &m_instructions_index, level);			

		return;
	}
	else
	{
		ZEROMEMORY(tag);
		tag.dont_getsymbol = TRUE;
		tag.statement = TRUE;
		exp_res = expression_ident(level, ptx, tag);
	}
}

/*
 * Processes do statements.
 */
void statement_do(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	int start, end, cx1;
	exp_res_t exp_res;

	get_symbol();

	start = m_instructions_index;
	statement(level, ptx, pdx, func_ident_p);

	if (m_symbol == SYM_WHILE)
	{
		get_symbol();
	}
	else
	{
		error(6260);
	}

	cx1 = m_instructions_index;

	exp_res = expression(level, ptx, exp_integer, m_empty_tag);
	
	logical_not_top(level, ptx, exp_res);
	
	instructions_add(OC_JPC, 0, start);
	end = m_instructions_index;

	instructions_fix_jumps(start, end, EXIT_DO, m_instructions_index);
	instructions_fix_jumps(start, end, EXIT_ANY, m_instructions_index);
	instructions_fix_jumps(start, end, CONT_DO, cx1);
	instructions_fix_jumps(start, end, CONT_ANY, cx1);
}

/*
 * Processes repeat statements.
 */
void statement_repeat(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	int start, end, cx1;

	get_symbol();

	if (m_symbol == SYM_DO)
	{
		get_symbol();
	}
	else
	{
		/*
		 * It's ok not to have a do as long as they have a begin.
		 */
		if (m_symbol != SYM_BEGIN)
		{
			error(6202);
		}
	}

	cx1 = m_instructions_index;
	start = m_instructions_index;

	statement(level, ptx, pdx, func_ident_p);

	end = m_instructions_index;

	instructions_add(OC_JMP, 0, cx1);

	instructions_fix_jumps(start, end, EXIT_REPEAT, m_instructions_index);
	instructions_fix_jumps(start, end, EXIT_ANY, m_instructions_index);
	instructions_fix_jumps(start, end, CONT_REPEAT, cx1);
	instructions_fix_jumps(start, end, CONT_ANY, cx1);
}

/*
 * Processes while statements.
 */
void statement_while(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	int start, end, cx1, cx2;

	/*
	 * Save position of the condition statement we can fix the jmp address later.
	 */
	start = m_instructions_index;
	cx1 = m_instructions_index;
	get_symbol();

	expression(level, ptx, exp_integer, m_empty_tag);
	
	cx2 = m_instructions_index;
	instructions_add(OC_JPC, 0, 0);

	if (m_symbol == SYM_DO)
	{
		get_symbol();
	}
	else
	{
		/*
		 * It's ok not to have a do as long as they have a begin.
		 */
		if (m_symbol != SYM_BEGIN)
		{
			error(6202);
		}
	}

	statement(level, ptx, pdx, func_ident_p);

	/*
	 * Loop to the top of the while.
	 */
	instructions_add(OC_JMP, 0, cx1);
	
	end = m_instructions_index;

	/*
	 * Fix the jump address from the condition at the while.
	 */
	m_instructions[cx2].a = m_instructions_index;
	
	instructions_fix_jumps(start, end, EXIT_WHILE, m_instructions_index);
	instructions_fix_jumps(start, end, EXIT_ANY, m_instructions_index);
	instructions_fix_jumps(start, end, CONT_WHILE, cx1);
	instructions_fix_jumps(start, end, CONT_ANY, cx1);
}

/*
 * Processes if statements.
 */
void statement_if(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	int cx1, cx2;

	get_symbol();

	expression(level, ptx, exp_integer, m_empty_tag);

	if (m_symbol == SYM_THEN)
	{
		get_symbol();
	}
	else
	{
		/*
		 * It's ok not to have a then as long as they begin a new block.
		 */
		if (m_symbol != SYM_BEGIN)
		{
			error(6210);
		}
	}

	cx1 = m_instructions_index;

	instructions_add(OC_JPC, 0, 0);

	statement(level, ptx, pdx, func_ident_p);
	
	if (m_symbol == SYM_ELSE)
	{
		get_symbol();

		cx2 = m_instructions_index;
		
		instructions_add(OC_JMP, 0, 0);

		m_instructions[cx1].a = m_instructions_index;
		
		statement(level, ptx, pdx, func_ident_p);
	}
	else
	{
		cx2 = cx1;
	}

	m_instructions[cx2].a = m_instructions_index;
}

/*
 * Checks to see if an ident returns something.
 */
BOOL returns_something(identinfo_t *ident_p)
{
	return ident_p && ident_p->kind == KIND_FUNCTION && ident_p->data.function_val_p->return_var.kind != KIND_VOID;
}

BOOL valid_statement(int symbol)
{
	return ((symbol == SYM_SEMICOLON) || (symbol == SYM_BEGIN)
			|| (symbol == SYM_IF) || (symbol == SYM_WHILE)
			|| (symbol == SYM_CALL) 
			|| (symbol == SYM_FOR) || (symbol == SYM_RETURN)
			|| (symbol == SYM_COLON) 
			|| (symbol == SYM_WHICH)
			|| (symbol == SYM_IDENT));
}

/*
 * Process general in function statements.
 *
 * @param func_ident_p The function identifier.
 */
void statement(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	exp_res_t exp_res;
	identinfo_t *ident_p;
	int cx1, symbol, symbol2;		

	ZEROMEMORY(exp_res);	

	if (m_symbol == SYM_LPAREN)
	{
		get_symbol();

		statement(level, ptx, pdx, func_ident_p);

		if (m_symbol == SYM_RPAREN)
		{
			get_symbol();
		}
		else
		{
			error(6002);
		}		
	}
	else if (m_symbol == SYM_VAR)
	{
		block_var(level, ptx, pdx, TRUE);
	}
	else if (m_symbol == SYM_PLUS_PLUS || m_symbol == SYM_MINUS_MINUS)
	{
		exp_res = expression_unary_left(level, ptx, m_empty_tag);

		instructions_add(OC_DEC, 0, 1);
	}
	else if (m_symbol == SYM_PRAGMA)
	{
		process_pragma(level, ptx);
	}
	else if (m_symbol == SYM_IDENT)
	{
		statement_ident(level, ptx, ptx, func_ident_p);	
	}
	else if (m_symbol == SYM_CALL)
	{
		get_symbol();

		if (m_symbol != SYM_IDENT)
		{
			error(4000);
		}
		else
		{
			ident_p = ident_find(m_ident_name, ptx);

			if (!ident_p)
			{
				error(4006); /* expected procedure */
			}
			else
			{
				get_symbol();

				function_procedure_caller(level, ptx, ident_p, FALSE);

				if (returns_something(ident_p))
				{
					instructions_add(OC_DEC, 0, 1); /* return value isn't assigned to anything */				
				}
			}
		}
	}
	else if (m_symbol == SYM_EXIT || m_symbol == SYM_CONTINUE)
	{
		symbol = m_symbol;

		get_symbol();

		cx1 = 1;

		symbol2 = m_symbol;			

		if (m_symbol != SYM_FUNCTION && m_symbol != SYM_PROCEDURE && m_symbol != SYM_SEMICOLON)
		{
			if (m_symbol != SYM_LITERAL_NUMBER)
			{
				get_symbol();
			}			

			if (m_symbol == SYM_LITERAL_NUMBER)
			{
				cx1 = (int)m_number;
				get_symbol();

				if (cx1 < 1)
				{
					cx1 = 1;
					error(11610);
				}
			}
		}

		switch (symbol2)
		{
		case SYM_WHICH:

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, (symbol == SYM_EXIT) ? EXIT_WHICH : CONT_WHICH, cx1);

			break;

		case SYM_FOR:

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, (symbol == SYM_EXIT) ? EXIT_FOR : CONT_FOR, cx1);

			break;

		case SYM_REPEAT:

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, (symbol == SYM_EXIT) ? EXIT_REPEAT : CONT_REPEAT, cx1);
			
			break;

		case SYM_WHILE:

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, (symbol == SYM_EXIT) ? EXIT_WHILE : CONT_DO, cx1);
			
			break;

		case SYM_DO:

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, (symbol == SYM_EXIT) ? EXIT_DO : CONT_DO, cx1);
			
			break;

		case SYM_FUNCTION:
		case SYM_PROCEDURE:

			instructions_add(OC_JMP, JMP_RETURN, 0);
			
			break;

		default:
			
			if (symbol == SYM_CONTINUE)
			{
				instructions_add(OC_JMP, CONT_ANY, (int)cx1);
			}
			else if (symbol == SYM_EXIT)
			{
				instructions_add(OC_JMP, EXIT_ANY, (int)cx1);
			}
			else
			{
				error(1000);
			}

		}		
	}
	else if (m_symbol == SYM_RETURN)
	{
		statement_return(level, ptx, pdx, func_ident_p, TRUE);
	}
	else if (m_symbol == SYM_GOTO)
	{
		get_symbol();

		if (m_symbol == SYM_IDENT)
		{
			get_symbol();

			instructions_add(OC_DEC, 0, 0);
			instructions_add(OC_JMP, GOTO_LABEL, (LONGLONG)_strdup(m_ident_name));
		}
		else
		{
			error(4008);
		}
	}
	else if (m_symbol == SYM_FOR)
	{
		get_symbol();

		statement_for(level, ptx, pdx, func_ident_p);	
	}
	else if (m_symbol == SYM_ASM)
	{
		get_symbol();

		statement_asm(level, ptx);
	}	
	else if (m_symbol == SYM_WHICH)
	{
		statement_which(level, ptx, pdx, func_ident_p);
	}
	else if (m_symbol == SYM_IF)
	{
		statement_if(level, ptx, pdx, func_ident_p);
	}	
	else if (m_symbol == SYM_DO)
	{
		statement_do(level, ptx, pdx, func_ident_p);		
	}
	else if (m_symbol == SYM_REPEAT)
	{		
		statement_repeat(level, ptx, pdx, func_ident_p);
	}
	else if (m_symbol == SYM_WHILE)
	{
		
		statement_while(level, ptx, pdx, func_ident_p);
	}
	else if (m_symbol == SYM_BEGIN)
	{
		get_symbol();

		process_statement_block(level, ptx, pdx, func_ident_p);
		
		if (m_symbol == SYM_END)
		{
			get_symbol();		
		}
		else
		{
			error(6204);
		}
		
	}
}

void process_statement_block(int level, int *ptx, int *pdx, identinfo_t *func_ident_p)
{
	int cx1;

	cx1 = m_instructions_index;

	for(;;)
	{
		statement(level, ptx, pdx, func_ident_p);
		
		if (!valid_statement(m_symbol))
		{
			break;
		}

		if(m_symbol == SYM_SEMICOLON || m_symbol == SYM_COLON)
		{
			get_symbol();
		}
		else
		{
			error(6018);
		}
	}

	fix_gotos(cx1, m_instructions_index, level, ptx, FALSE);
}

/*
 * Processes a single functional parameter.
 */
int process_param(int level, int *ptx, int *pdx, int paramx, identinfo_t *ident_p)
{
	BOOL byref;
	identinfo_t *ident_p2;
		
	if (m_symbol == SYM_REF)
	{
		byref = TRUE;
		get_symbol();
	}
	else if (m_symbol == SYM_VAL)
	{
		byref = FALSE;
		get_symbol();
	}
	else
	{
		byref = FALSE;
	}
	
	if (m_symbol == SYM_IDENT)
	{		
		/*
		 * ident_p2 is the variable the function this param is for sees.
		 */
		ident_p2 = ident_process_var(level, ptx, pdx, byref, FALSE);

		ident_p2->refed++;

		/*
		 * References that have defaults need to point to a valid
		 * ident that holds the thing that actually holds the default.
		 */

		ident_p2->is_param = TRUE;
		ident_p->data.function_val_p->params[paramx] = ident_copy(*ident_p2);
		
		if (ident_p2)
		{
			if (ident_p2->kind == KIND_FUNCTION)
			{
				/*
				 * The function will see this as a function pointer so calls indirectly.
				 */
				ident_p2->data.function_val_p->byref = TRUE;
			}

			return ident_p2->kind;
		}		
		else
		{
			error(66000); /* couldn't enter a function or procedure's param */
		}
	}
	else if (m_symbol != SYM_RPAREN)
	{
		error(7002);
	}

	return KIND_UNKNOWN;
}

/*
 * Processes functional parameters.
 */
int process_params(int level, int *ptx, int *pdx, identinfo_t *ident_p)
{
	BOOL nd;
	int i, num = 0, kind;

	if (m_symbol != SYM_RPAREN)
	{		
		for (;;)
		{			
			kind = process_param(level, ptx, pdx, num, ident_p);
			
			ident_p->data.function_val_p->params[num].kind = kind;

			num++;

			if (m_symbol == SYM_COMMA)
			{
				get_symbol();
			}
			else
			{
				break;
			}
						
		}

		if (m_symbol != SYM_RPAREN)
		{
			error(7002);
		}
	}
	
	ident_p->data.function_val_p->params_num = num;

	nd = FALSE;

	for (i = 0; i < num; i++)
	{
		if (ident_p->data.function_val_p->params[i].has_default)
		{
			nd = TRUE;
		}
		else if (nd == TRUE)
		{
			error(6610);
		}		
	}

	return num;
}

/*
 * Processes a single functional argument.
 */
int process_arg(int level, int *ptx, int paramx, identinfo_t *ident_p)
{	
	exp_res_t exp_res;
	BOOL byref = FALSE;	

	ZEROMEMORY(exp_res);	
	
	if (m_symbol == SYM_AMPERSAND || m_symbol == SYM_REF)
	{
		get_symbol();

		if (!(ident_p->data.function_val_p->params[paramx].byref))
		{
			 /*
			  * error(11058);
			  */			 			
		}

		byref = TRUE;
	}
	else if (m_symbol == SYM_VAL)
	{
		get_symbol();

		byref = FALSE;
	}

	if (ident_p->data.function_val_p->params[paramx].byref)
	{	
		exp_res = expression_ref(level, ptx, m_empty_tag, TRUE);
	}
	else
	{
		if (byref)
		{
			exp_res = expression_ref(level, ptx, m_empty_tag, TRUE);
		}
		else
		{
			SET_EXP_RES(exp_res, ident_p->data.function_val_p->params[paramx]);

			expression(level, ptx, exp_res, m_empty_tag);
		}
	}

	return exp_res.type;
}

exp_res_t expression_ref(int level, int *ptx, tag_t tag, BOOL ref)
{
	exp_res_t exp_res;
	identinfo_t *ident_p;

	ZEROMEMORY(exp_res);

	if (m_symbol == SYM_NULL)
	{
		get_symbol();

		instructions_add(OC_LIT, 0, 0);
	}
	else if (m_symbol == SYM_IDENT)
	{
		ident_p = ident_find(m_ident_name, ptx);

		get_symbol();

		if (ident_p)
		{
			if (ident_p->byref && ref)
			{
				instructions_add(OC_LOD, level - ident_p->level, ident_p->address);
			}
			else
			{
				instructions_add(OC_LDA, level - ident_p->level, ident_p->address);
			}

			/*
			 * If they're passing a function or a function containing variable.
			 */
			if (ident_p->kind == KIND_FUNCTION)
			{
				/*
				 * If they're not passing a function variable, but a function itself.
				 */
				if (!ident_p->data.function_val_p->byref)
				{
					error(11050);
				}
				else
				{
					/*
					 * Passing variables that point to functions is ok.
					 */
				}
			}
			else if (ident_p->kind == KIND_ARRAY)
			{
				/*
				 * References to arrays are owned by the calling function.
				 * No need to addref.
				 */
			}
		}
		else
		{
			error(4000);
		}
	}
	else
	{
		error(11060);
	}

	return exp_res;
}

/*
 * Processes functional arguments (what is sent).
 */
int process_args(int level, int *ptx, identinfo_t *ident_p /* the ident for the function */, BOOL aggregate)
{
	BOOL no_args = FALSE;
	int i, kind, num = 0;
	
	if (m_symbol != SYM_RPAREN)
	{		
		if (ident_p->data.function_val_p->params_num <= 0)
		{
			no_args = TRUE;
		}

		for (;;)
		{	
			if (!no_args)
			{
				if (m_symbol == SYM_COMMA || m_symbol == SYM_RPAREN)
				{
					/*
					 * They're skipping this parameter (no argument).
					 */

					/*
					 * If it's optional load it up, otherwise load a 0 and give an error
					 */

					if (ident_p->data.function_val_p->params[num].has_default)
					{
						load_ident_default(&ident_p->data.function_val_p->params[num], level, ptx);
					}
					else
					{
						/*
						 * Load a zero for the param.
						 * In the future, this will need to be smarter for multi-word params.
						 */

						instructions_add(OC_LIT, 0, 0);

						error(10570);
					}
				}
				else
				{
					kind = process_arg(level, ptx, (no_args) ? 1 : num, ident_p);
				
					if (kind != ident_p->data.function_val_p->params[num].kind)
					{
						/*
						 * Expression should have casted, but obviously failed.
						 * Continue on to find other errors.
						 */
					}
				}
			
				num++;
			}
			else
			{
				get_symbol();
			}			
			
			if (m_symbol != SYM_COMMA)
			{
				break;
			}

			if (!no_args)
			{
				get_symbol();

				if (m_symbol == SYM_RPAREN)
				{
					/*					
					 * They have done something like this: myfunction(x, );
					 * It's can be ok, but give them a warning that they could be missing
					 * an argument.
					 */

					error(10580);
				}
			}			
		}
	}
		
	if (num != ident_p->data.function_val_p->params_num)
	{
		if (ident_p->data.function_val_p->params[num].has_default)
		{
			for (i = num; i < ident_p->data.function_val_p->params_num; i++)
			{
				load_ident_default(&ident_p->data.function_val_p->params[i], level, ptx);

				num++;
			}
		}
		else
		{
			/*
			 * Incorrect number of args for functional/procedural call.
			 */

			for (i = num; i < ident_p->data.function_val_p->params_num; i++)
			{
				/*
				 * Load up zeros for the missing arguments (tihs presumes each
				 * parameter is only 1 word).
				 */

				instructions_add(OC_LIT, 0, 0);

				num++;
			}

			error(11001);
		}
	}

	for (i = 0; i < num; i++)
	{
		/*
		 * If this is an aggregate call, then apart from the return value space,
		 * there's also some space occupied (on top of the stack) by the address
		 * of the aggregate function.  If this is the case, then only store the
		 * parameter 2 stack positions up, rather than 3. (The aggregate function
		 * pointer will be overwritten when it comes time to call it).
		 * Storing the argument 3 levels up anyway will cause the arguments to be
		 * stored too high when the function pointer is processed by OC_CAS.
		 *
		 * @see interpretor's OC_CAS routine and function_procedure_caller()
		 */

		if (aggregate)
		{
			generate_stp_aggregate(ident_p);
		}
		else
		{
			generate_stp(ident_p);
		}		
	}

	if (no_args)
	{
		error(11001); /* function doesn't need arguments! */
	}

	if (num != ident_p->data.function_val_p->params_num)
	{
		error(11001); /* Incorrect number of params for functional/procedural call */		
	}

	return num;
}

/*
 * Checks to see if a statement is a valid toplevel statement.
 */
BOOL valid_top_level_symbol(int symbol)
{
	return (symbol == SYM_CONST) || (symbol == SYM_VAR)
		|| (symbol == SYM_PROCEDURE) || (symbol == SYM_FUNCTION)
		|| (symbol == SYM_INCLUDE)
		|| (symbol == SYM_PRAGMA)
		|| (symbol == SYM_TYPE)
		|| (symbol == SYM_DECLARE);
}

/*
 * Checks to see if a file has already been parsed.
 */

BOOL file_already_parsed(const char *name)
{
	int i;

	for (i = 0; i < m_num_parsed_files; i++)
	{
		if (strcmp(m_parsed_files[i], name) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
};

/*
 * Processes pragma compiler directives.
 */
void process_pragma(int level, int *ptx)
{	
	int symbol, number;
	identinfo_t *ident_p;
	BOOL hasnumber = FALSE;		

	get_symbol();
			
	switch(m_symbol)
	{
	case SYM_OPTIMIZE:
	case SYM_BOUNDARY_CHECK:

		symbol = m_symbol;

		get_symbol();

		if (m_symbol == SYM_IDENT)
		{
			get_symbol();

			ident_p = ident_find(m_ident_name, ptx);

			if (ident_p)
			{
				if (ident_p->constant && ident_p->kind == KIND_INTEGER)
				{
					number = ident_p->data.int_val.default_val;
					hasnumber = TRUE;
				}			
			}
			else
			{
				error(4000);
			}
		}

		if (m_symbol == SYM_LITERAL_NUMBER)
		{
			get_symbol();

			number = (int)m_number;
			hasnumber = TRUE;
		}
		
		if (hasnumber)
		{
			switch (symbol)
			{
			case SYM_BOUNDARY_CHECK:
				
				m_options.array_check_boundaries = number;

				break;

			case SYM_OPTIMIZE:

				m_options.optimize = number;

				break;
			}

			
		}
		else
		{
			error(6500);
		}


		break;
	}
}

void block_var(int level, int *ptx, int *pdx, BOOL statement)
{
	identinfo_t *ident_p;

	do
	{
		get_symbol();

		ident_p = ident_process_var(level, ptx, pdx, FALSE, statement);
	}
	while (m_symbol == SYM_COMMA);
}

/*
 * Processes a block.
 */
void process_block(int level, int tx, identinfo_t *ident_p)
{	
	BOOL root = FALSE, predeclare = TRUE;
	int i, j, symbol, dx, otx, otx2, odx, cx, inx, vars = 0;
	identinfo_t *ident_p2 = NULL, *ident_p3 = NULL;
	identinfo_t ident;	

	dx = 3;
	otx = tx;

	m_num_level_gcidents = 0;

	inx = m_instructions_index;

	m_idents[tx].address = m_instructions_index;
	
	instructions_add(OC_JMP, 0, 0);

	if (level > MAX_LEVEL)
	{
		error(20000);
	}

	do
	{
		if (m_symbol == SYM_PRAGMA)
		{
			process_pragma(level, &tx);

			if (m_symbol == SYM_SEMICOLON)
			{
				get_symbol();
			}
		}

		if (m_symbol == SYM_TYPE)
		{
			get_symbol();

			if (m_symbol == SYM_IDENT)
			{
				get_symbol();

				ident_p2 = identinfo_new();
				strcpy(ident_p2->name, m_ident_name);
				ident_read_type(ident_p2, level, &tx);

				type_add(ident_p2);
			}			
			else
			{	
				get_symbol();

				error(4000);
			}

			if (m_symbol == SYM_SEMICOLON)
			{
				get_symbol();
			}
		}
		
		if (m_symbol == SYM_INCLUDE)
		{
			m_ignore_esc_chars = TRUE;
			get_symbol();
			m_ignore_esc_chars = FALSE;
			
			if (m_symbol == SYM_LITERAL_STRING)
			{
				if (!load_file(m_string))
				{					
					/*
					 * File has already been parsed, read away the string.
					 */

					if (m_symbol == SYM_LITERAL_STRING)
					{
						get_symbol();
					}					
				}
			}
			
			get_symbol();
		}
		
		if (m_symbol == SYM_CONST)
		{
			get_symbol();

			do
			{
				ident_add_const(level, &tx, &dx);

				while (m_symbol == SYM_COMMA)
				{
					get_symbol();

					/*
					 * Added checking to make sure arguments are valid names.
					 */
					if (m_symbol != SYM_IDENT)
					{
						error(4000);
					}

					ident_add_const(level, &tx, &dx);
				}
				
				if (m_symbol == SYM_SEMICOLON)
				{
					get_symbol();
				}
				else
				{	
					error(6018);
				}				
			}
			while (m_symbol == SYM_IDENT);
		}

		if (m_symbol == SYM_VAR)
		{
			odx = dx;

			do
			{
				block_var(level, &tx, &dx, FALSE);
			}
			while (m_symbol == SYM_IDENT);

			vars += (dx - odx);
			
			if (m_symbol == SYM_SEMICOLON)
			{
				get_symbol();
			}
			else
			{					
				error(6018);
			}
		}

		if (m_symbol == SYM_LPAREN)
		{
			if (!ident_p)
			{
				error(4000); /* adding parameters with no function, expected identifier */
			}
			else
			{
				get_symbol();

				process_params(level, &tx, &dx, ident_p);
				
				if (m_symbol == SYM_RPAREN)
				{
					get_symbol();
				}
				else
				{
					error(6002);
				}

				ident_read_type(&ident_p->data.function_val_p->return_var, level, &tx);

				if (ident_p->data.function_val_p->return_var.kind != KIND_VOID)
				{	
					/*
					 * Make a variable called this which represents the return value of
					 * the function.
					 */
					ident = ident_copy(ident_p->data.function_val_p->return_var);

					strcpy(ident.name, "retval");
					ident.level = level;
					ident.is_this = TRUE;
					ident.address = - 1;
					ident_add_any(&ident, &tx, &dx);

					/*
					 * retval isn't in this datasegment.
					 */

					dx--;
				}				
			}
		}
		
		while (m_symbol == SYM_PROCEDURE || m_symbol == SYM_FUNCTION || m_symbol == SYM_DECLARE)
		{			
			symbol = m_symbol;

			if (symbol == SYM_DECLARE)
			{
				predeclare = TRUE;

				get_symbol();

				symbol = m_symbol;
			}
			else
			{
				predeclare = FALSE;
			}
			
			get_symbol();

			if (m_symbol == SYM_IDENT)
			{
				ident_p2 = ident_add_function_ex(level, &tx, &dx, FALSE, (BOOL)((symbol == SYM_PROCEDURE) ? TRUE : FALSE));

				get_symbol();
			}
			else
			{
				error(4006);
			}		
						
			if (!ident_p2)
			{
				error(66000); /* Couldn't register function */
			}

			if (predeclare)
			{
				ident_p2->address = -100;
			}

			if (m_symbol == SYM_SEMICOLON)
			{
				error(7018);
				get_symbol();
			}
			
			if (predeclare)
			{				
				if (m_symbol == SYM_LPAREN)
				{
					get_symbol();
				}

				otx2 = tx;
				odx = dx;

				process_params(level, &tx, &dx, ident_p2);

				tx = otx2;
				dx = odx;

				if (m_symbol == SYM_RPAREN)
				{
					get_symbol();
				}

				ident_read_type(&ident_p2->data.function_val_p->return_var, level, &tx);				
			}
			else
			{
				process_block(level + 1, tx, ident_p2);
			}

			if (m_symbol == SYM_SEMICOLON)
			{
				get_symbol();
			}
			else
			{
				/*
				 * Expected semicolon.
				 */
				error(6018);
			}
		}
	}
	while(valid_top_level_symbol(m_symbol));

	if (!ident_p)
	{
		root = TRUE;
		ident_p = ident_p2;
	}

	if (!(m_symbol == SYM_BEGIN || m_symbol == SYM_SEMICOLON) && !root)
	{		
		error(6650);

		get_symbol();
	}

	if ((m_symbol == SYM_NOSYM || m_symbol == SYM_EOFSYM) && !root)
	{
		error(14002);
	}
	else
	{		
		m_instructions[m_idents[otx].address].a = m_instructions_index;
		m_idents[otx].address = m_instructions_index;
		
		/*
		 * Increment enough space for the data segment header and parameters.
		 */
		instructions_add(OC_INC, 0, dx - vars);

		/*
		 * Load up zeros for the non-parameter data (eg. vars).
		 */
		cx = m_instructions_index;	
		instructions_add(OC_LIS, 0, 0);
				
		j = 0;

		/*
		 * Allocate dynamic arrays and set defaults.
		 */
		set_defaults(level, &tx, otx + 1, tx);
		
		odx = dx;

		if (root)
		{
			ident_p2 = ident_find("main", &tx);

			if (ident_p2 && ident_p2->data.function_val_p->params_num == 0
				&& ident_p2->data.function_val_p->return_var.kind == KIND_INTEGER)
			{
				/*
				 * function main() : integer
				 */

				instructions_add(OC_LIT, 0, 0);
				instructions_add(OC_CAL, 0, ident_p2->address);
				instructions_add(OC_DEC, 0, 1);
			}
			else if (ident_p2 && ident_p2->data.function_val_p->params_num == 1
				&& can_be_string(&ident_p2->data.function_val_p->params[0])
				&& ident_p2->data.function_val_p->return_var.kind == KIND_INTEGER)
			{
				/*
				 * function main(args : string) : integer
				 */
				
				instructions_add(OC_SRG, ARRAY_HEADER_SIZE, REG_CX);
				instructions_add(OC_CLI, 0, 0);
				instructions_add(OC_SRG, ARRAY_LENGTH_OFFSET, REG_CX);
				instructions_add(OC_MSS, 0, 1);				
				instructions_add(OC_SLD, 0, 0);
				memory_addref_by_handle_off_stack(level, &tx);

				generate_stp(ident_p2);
				generate_call_func(ident_p2, ident_p2->level);
			}
			else
			{
				if (ident_p2)
				{
					error(15505);
				}
				else
				{
					error(15500);
				}
			}
		}
		else
		{
			statement(level, &tx, &dx, ident_p);			
		}	

		/*
		 * Fix the number of zeros to load up for the vars.
		 */
		if (dx - (odx - vars) > 0)
		{
			m_instructions[cx].l = dx - (odx - vars);
		}
	}
	
	j = m_instructions_index;

	fix_gotos(inx, m_instructions_index, level, &tx, TRUE);

	/*
	 * Optimize out the last instruction if it was a JMP to the next statement.
	 * Happens when you have a "return blah" on the last line of the function.
	 */
	if (m_instructions[m_instructions_index - 1].f == OC_JMP
		&& (m_instructions[m_instructions_index - 1].a == JMP_RETURN
		|| m_instructions[m_instructions_index - 1].a == m_instructions_index
		))
	{
		m_instructions_index--;
	}

	/*
	 * Fix all the return statements to jump to here
	 * right before the garbage collection of arrays.
	 *
	 * Fix all the call instructions that call predeclared functions.
	 *	
	 */ 	
	for (i = inx; i < m_instructions_index; i++)
	{
		if (m_instructions[i].f == OC_JMP)
		{
			if (m_instructions[i].l == JMP_RETURN)
			{
				m_instructions[i].l = 0;
				m_instructions[i].a = m_instructions_index;
			}
		}
		else if (m_instructions[i].f == OC_FN_CAL
			|| m_instructions[i].f == OC_FN_LOD
			|| m_instructions[i].f == OC_FN_LIT)
		{
			ident_p3 = (identinfo_t*)m_instructions[i].a;
			ident_p2 = ident_find(ident_p3->name, &tx);

			if (ident_p2 && ident_p2->address > 0)
			{
				m_instructions[i].a = ident_p2->address;

				m_instructions[i].f = -m_instructions[i].f;
			}
			else
			{
				/*				 
				 * Functions have to be declared on the same level as which they're predeclared.
				 */
				if (!ident_p2 || (ident_p2 && (ident_p3->level > ident_p2->level)))
				{					
					strcpy(m_message, " (");
					strcat(m_message, (char*)(((identinfo_t*)m_instructions[i].a)->name));
					strcat(m_message, ")");

					error_extra(15100, m_message, FALSE, FALSE);

					m_instructions[i].a = 0;
					m_instructions[i].l = 0;
					m_instructions[i].f = -m_instructions[i].f;
				}
			}
		}
	}		
		
	/*
	 * Give errors about goto/exit statements that were never resolved.
	 */
	instructions_check_orphanded_jumps(inx, m_instructions_index);
	
	/*
	 * Optimize tail calls.
	 */

	if (m_options.optimize)
	{	
		for (i = inx + 4; i <= m_instructions_index; i++)
		{
			if ((m_instructions[i].f == OC_JMP
				&& m_instructions[i].a == m_instructions_index)
				|| (i == m_instructions_index))
			{
				if (m_instructions[i - 1].f == OC_SRV
					|| m_instructions[i - 1].f == OC_DEC)
				{
					if (m_instructions[i - 2].f == OC_CAL)
					{
						if (m_instructions[i - 3].f == OC_LIT)
						{
							if (m_instructions[i - 2].l == 1
								&& (m_instructions[i - 2].a == ident_p->address))
							{
								int cx2, inscnt = 0;
								instruction_t instmp[30];
								
								for (j = i - 4; j >= inx + 4; j--)
								{
									if (m_instructions[j].f == OC_STP)
									{
										/*
										 * Don't worry about return space for the params.
										 */

										m_instructions[j].a -= 1;

										instmp[inscnt++] = m_instructions[j];

										m_instructions[j].f = OC_NOP;
										m_instructions[j].a = 0;
										m_instructions[j].l = 0;										
									}
									else
									{
										break;
									}
								}
								
								cx = m_instructions_index;
								instructions_add(OC_JMP, 0, 0);

								cx2 = m_instructions_index;

								do_garbage_collection(level, &tx, otx + 1, tx);

								for (j = inscnt - 1; j >= 0; j--)
								{
									m_instructions[m_instructions_index++] = instmp[j];
								}

								instructions_add(OC_JMP, 0, i - 2);

								m_instructions[cx].a = m_instructions_index;

								m_instructions[i - 3].f = OC_JMP;
								m_instructions[i - 3].l = 0;
								m_instructions[i - 3].a = cx2;
								
								m_instructions[i - 2].f = OC_TAC;
								m_instructions[i - 2].l = 1;
								m_instructions[i - 2].a = ident_p->address;								

								m_instructions[i - 1].f = OC_OPR;
								m_instructions[i - 1].l = 0;
								m_instructions[i - 1].a = 0;

								m_instructions[i].f = OC_NOP;
								m_instructions[i].l = 0;
								m_instructions[i].a = 0;
							}
						}
					}
				}
			}
		}
	}

	
	/*
	 * Can't do tail call when there needs to be GC (currently).
	 */
	do_garbage_collection(level, &tx, otx + 1, tx);

	
	/*
	 * End of data segment.
	 */
	instructions_add(OC_OPR, 0, 0);
	
	for (i = otx + 1; i <= tx; i++)
	{
		if (m_idents[i].level == level)
		{
			identinfo_delete(&m_idents[i]);
		}
	}

	/*
	 * Free up some other identifiers that aren't on the variable stack
	 * but were allocated on C's heap.
	 */
	for (i = 0; i < m_num_level_gcidents; i++)
	{
		identinfo_delete(m_level_gcidents[i]);
	}

	m_num_level_gcidents = 0;
}

void set_defaults(int level, int *ptx, int sx, int ex)
{
	int i;

	for (i = sx; i <= ex; i++)
	{
		if (m_idents[i].level == level && !m_idents[i].is_param
				&& !m_idents[i].constant
				&& !(strcmp(m_idents[i].name, "retval") == 0))
		{
			/*
			 * If the variable is an array and is actually of some sort of size (eg. not a reference)
			 */
			if (m_idents[i].kind == KIND_ARRAY)
			{
				if ((m_idents[i].data.array_val_p->contains.kind == KIND_INTEGER)
					&& m_idents[i].data.array_val_p->array_default)
				{
					load_new_array(m_idents[i].data.array_val_p->array_default, level, ptx);
					instructions_add(OC_STO, 0, m_idents[i].address);
				}
				else if (m_idents[i].data.array_val_p->size >= 0)
				{
					/*
					 * Allocate enough memory for the array handle's header.
					 */					
					instructions_add(OC_LIT, 0, ARRAY_HEADER_SIZE);
					instructions_add(OC_MAL, 0, 0);
					instructions_add(OC_STO, 0, m_idents[i].address);
					
					/* 
					 * Addref the array.
					 */
					memory_addref(&m_idents[i], level, ptx);
					
					/* 
					 * Reallocate the array to the default size (or the wanted size).
					 */

					if (m_idents[i].data.array_val_p->size > 0)
					{
						array_realloc(&m_idents[i], level, ptx, m_idents[i].data.array_val_p->size);
					}
				}
				else
				{
					/*
					 * No need to load up NULLs.  Default is 0.
					 */
				}
			}
			else if ((m_idents[i].kind == KIND_INTEGER) && !m_idents[i].constant)
			{
				/*
				 * No need to load up 0.  Default is 0.
				 */
				if (m_idents[i].data.int_val.default_val != 0)
				{
					instructions_add(OC_LIT, 0, m_idents[i].data.int_val.default_val);
					instructions_add(OC_STO, 0, m_idents[i].address);
				}
			}
			else if (m_idents[i].kind == KIND_FLOAT && !m_idents[i].constant)
			{
				/*
				 * No need to load up 0.  Default is 0.
				 */
				if (m_idents[i].data.float_val.default_val != 0)
				{
					instructions_add_f(OC_LIT, 0, m_idents[i].data.float_val.default_val);
					instructions_add(OC_STO, 0, m_idents[i].address);
				}
			}
			else if
				(
					m_idents[i].kind == KIND_FUNCTION
					&& (strcmp(m_idents[i].name, "static_init") == 0)
					&& m_idents[i].data.function_val_p->params_num == 0
					&& m_idents[i].data.function_val_p->return_var.kind == KIND_INTEGER
				)
			{
				instructions_add(OC_LIT, 0, 0);
				instructions_add(OC_CAL, level - m_idents[i].level, m_idents[i].address);
				instructions_add(OC_DEC, 0, 1);
			}
		}
	}
}

void fix_gotos(int start, int end, int level, int *ptx, BOOL warn)
{
	int i;

	identinfo_t *ident_p;

	/*
	 * Fix gotos.
	 */
	for (i = start; i < end; i++)
	{
		if (m_instructions[i].f == OC_JMP && m_instructions[i].l == GOTO_LABEL)
		{
			ident_p = ident_find((char*)m_instructions[i].a, ptx);
			
			if (ident_p)
			{
				m_instructions[i].l = 0;
				free((char*)m_instructions[i].a);
			
				m_instructions[i].a = ident_p->address;
			}
			else
			{
				if (warn)
				{
					error(8030);
				}
			}
		}
	}
}

#ifdef WIN32
	#define PATH_SEPERATOR ';'
#else
	#define PATH_SEPERATOR ':'
#endif

/*
 * Loads a new file and keeps track of it on the file stack.
 */
BOOL load_file(const char *file)
{	
	char file_path[MAX_PATH];

	FILE *file_p;

	file_path[0] = 0;
	_fullpath(file_path, file, MAX_PATH);

	file_p = fopen(file_path, "r");
	
	if (!file_p)
	{

		int len;
		char *pc, *path;
		char current_path[MAX_PATH];
		
		if (strlen(m_options.source_path) > 0)
		{
			path = &m_options.source_path[0];
		}
		else
		{
			path = getenv("P_SOURCE_PATH");

			if (path == 0)
			{
#if WIN32
				path = ".\\lib";
#else
				path = "./lib";
#endif
			}
		}
		
		if (path)
		{
			do
			{
				pc = strchr(path, PATH_SEPERATOR);

				if (pc)
				{
					len = pc - path;
				}
				else
				{
					len = strlen(path);
				}

				if (len <= 0)
				{
					break;
				}

				strncpy(current_path, path, len);
				current_path[len] = 0;
				
#if WIN32
				sprintf(file_path, "%s\\%s", current_path, file);
#else
				sprintf(file_path, "%s/%s", current_path, file);
#endif		
				file_p = fopen(file_path, "r");

				path += len + 1;
			}
			while (file_p == NULL && path);
		}
	}
		

	if (file_already_parsed(file_path))
	{
		if (file_p)
		{
			fclose(file_p);
		}

		return FALSE;
	}
	
	if (!file_p)
	{
		if (m_files_stack_index >= 0)
		{
			error(14000);
		}

		return FALSE;
	}
	
	if (m_options.list_source)
	{
		printf("FILE     :[%s]\n", file);
	}
	
	m_files_stack_index++;

	m_files_stack[m_files_stack_index].file = file_p;
	strcpy(m_files_stack[m_files_stack_index].file_name, file);

	m_files_stack[m_files_stack_index].line_count = 0;
	m_files_stack[m_files_stack_index].line_index = 0;
	m_files_stack[m_files_stack_index].line_length = 0;

	m_char = ' ';

	strncpy(m_parsed_files[m_num_parsed_files++], file_path, MAX_PATH);

	return TRUE;
}

/*
 * Compiles a P++ source file.
 */
compiler_results_t compile(const char *file)
{
	int i;
	char *pc;
	FILE* pf;
	char path[MAX_PATH];
	compiler_results_t compiler_results;
	
	ZEROMEMORY(compiler_results);
	
	init();
	
	_fullpath(path, file, MAX_PATH);

	pc = strstr(path, ".pins");

	compiler_results.instructions = NULL;

	if (pc && ((size_t)((path + strlen(path)) - pc == 5)))
	{
		if (!m_options.quiet && m_options.verbose)
		{
			printf("Opening binary (%s)...\n\n", path);
		}

		pf = fopen(path, "rb");

		if (pf)
		{
			i = 0;

			while (!feof(pf))
			{
				fread(&m_instructions[i].f, sizeof(int), 1, pf);
				fread(&m_instructions[i].l, sizeof(int), 1, pf);
				fread(&m_instructions[i].a, sizeof(int), 1, pf);

				i++;
			}

			fclose(pf);
		}

		compiler_results.instructions = m_instructions;
		compiler_results.lines_compiled = -1;
		compiler_results.opcodes_generated = i;
		compiler_results.number_of_errors = 0;
	}
	else
	{
		if (!m_options.quiet && m_options.verbose)
		{
			printf("Compiling...\n\n");
		}

		if (!load_file(path))
		{
			printf("\nFile (%s) not found.\n", path);

			return compiler_results;
		}

		get_symbol();

		if (m_symbol != SYM_NOSYM && m_symbol != SYM_EOFSYM)
		{
			process_block(0, 0, NULL);
			m_files_stack_index--;
		}

		if (m_options.write_binary)
		{
			pc = strrchr(path, '.');

			if (pc)
			{
				*pc = 0;
			}

			strcat(path, ".pins");

			pf = fopen(path, "w+b");

			if (pf)
			{
				for (i = 0; i < m_instructions_index; i++)
				{
					fwrite(&m_instructions[i].f, sizeof(int), 1, pf);
					fwrite(&m_instructions[i].l, sizeof(int), 1, pf);
					fwrite(&m_instructions[i].a, sizeof(int), 1, pf);
				}

				fclose(pf);	
			}
		}

		fclose(m_files_stack[0].file);

		compiler_results.instructions = m_instructions;
		compiler_results.lines_compiled = m_line_count;
		compiler_results.opcodes_generated = m_instructions_index;
		compiler_results.number_of_errors = m_number_of_errors;
	}
		
	return compiler_results;
}
