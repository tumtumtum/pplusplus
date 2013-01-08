#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "symbols.h"
#include "compiler.h"
#include "expressions.h"

/*
 * Processes expressions casting the expression to a specified type wanted.
 */
exp_res_t expression(int level, int *ptx, exp_res_t exp_wanted, tag_t tag)
{
	exp_res_t exp_res;

	if ((exp_wanted.type == KIND_INTEGER
		&& exp_wanted.ident.data.int_val.is_handle)
		|| exp_wanted.ident.clean == TRUE
		)
	{
		tag.lvalue_nogc = TRUE;
	}

	exp_res = expression_raw(level, ptx, tag);

	return do_static_cast(level, ptx, exp_res, exp_wanted);
}


/*
 * Processes expressions allowing anything to come up.
 */
exp_res_t expression_raw(int level, int *ptx, tag_t tag)
{
	exp_res_t exp_res;

	ZEROMEMORY(exp_res);

	check_expression_start();

	exp_res = expression_inlineif(level, ptx, tag);

	return exp_res;
}

/*
 * Processes logical operations.
 */
exp_res_t expression_and_or(int level, int *ptx, tag_t tag)
{	
	exp_res_t exp_res;
	BOOL no_logical_and_or;
	int cx1 = -1, cx2 = -1, op;

	/*
	 * This is an exceptional case for case statements, which handle && and || themselves.
	 */
	no_logical_and_or = tag.no_logical_and_or;
	tag.no_logical_and_or = FALSE;

	exp_res = expression_condition(level, ptx, tag);

	if (!no_logical_and_or)
	{
		if (m_symbol == SYM_LAND || m_symbol == SYM_LOR)
		{
			/*
			 * Cast current result into an integer (expression now is in integer mode).
			 */
			do_static_cast(level, ptx, exp_res, exp_integer);

			/*
			 *
			 */
			while (m_symbol == SYM_LAND || m_symbol == SYM_LOR)
			{			
				if (m_symbol == SYM_LAND)
				{
					op = OPR_AND;
				}
				else
				{
					op = OPR_BOR;
				}

				/*
				 * Turn the top into a boolean (first bit is true if it's true).
				 */
				boolate_top(level, ptx, exp_integer);

				/*
				 * Do a short circuit jump out if the operation is an AND and
				 * the current result is false.
				 */
				
				if (op == OPR_AND)
				{	
					/*
					 * Want to keep the value, which JPC pops, so do an SLD.
					 */	

					instructions_add(OC_SLD, 0, 0);

					cx1 = m_instructions_index;
					
					
					instructions_add(OC_JPC, 0, 0);
				}

				get_symbol();

				/*
				 * If they accidentally put in another operator.
				 */
				if (m_symbol >= SYM_LOGICAL_START && m_symbol <= SYM_LOGICAL_END)					
				{
					/*
					 * Report the error.
					 */
					error(4210);
				}

				/*
				 * Get the right hand expression's answer, it should return an integer.
				 * Don't do a expression_cast_op, cause we don't want the left 
				 * operand to be cast to a float if the right is a float.  We expect
				 * and integer no matter what!
				 */
				exp_res = expression(level, ptx, exp_integer, m_empty_tag);

				/*
 				 * Boolate the right hand expression too.
				 */
				boolate_top(level, ptx, exp_res);

				/*
				 * Do the bit-wise and or or (same as logical and or or since the
				 * first bit is the only significant bit).
				 */
				instructions_add(OC_OPR, 0, op);
			}
			
			/*
			 * Jump past the code for short circuiting.
			 */
			cx2 = m_instructions_index;
			instructions_add(OC_JMP, 0, 0);

			/*
			 * Short circuiting jumps here (if there was a short circuit).
			 */
			if (cx1 != -1)
			{				
				m_instructions[cx1].a = m_instructions_index;				
			}

			/*
			 * Fix the jump pas the short circuit code.
			 */
			m_instructions[cx2].a = m_instructions_index;
		}
	}

	return exp_res;
}

/*
 * Is responsible for handling the right hand expression of a two operand expression.
 * Is responsible for doing the static casting to keep precision etc.
 */
exp_res_t expression_cast_op(processing_fn fn, int level, int *ptx, int operation, exp_res_t left_res, tag_t tag)
{
	exp_res_t exp_res;

	exp_res = fn(level, ptx, tag);

	if (exp_res.type == KIND_NULL
		&& left_res.type == KIND_ARRAY)
	{
		/*
		 * Array reference and null, just work with the reference and null!
		 * But first release the array on the left.
		 */

		instructions_add(OC_SWS, 0, 0);
		instructions_add(OC_SLD, 0, 0);
		memory_release_by_handle_off_stack(level, ptx);
		instructions_add(OC_SWS, 0, 0);
	}
	else if (left_res.type == KIND_NULL
		&& exp_res.type == KIND_ARRAY)
	{
		/*
		 * Array reference and null, just work with the reference and null!
		 * But first release the array on the right.
		 */

		instructions_add(OC_SLD, 0, 0);
		memory_release_by_handle_off_stack(level, ptx);
	}
	else if (exp_res.type == KIND_INTEGER && left_res.type == KIND_FLOAT)
	{
		instructions_add(OC_FOP, 0, FOP_FLO);

		exp_res.type = KIND_FLOAT;
	}
	else if (exp_res.type == KIND_FLOAT && left_res.type == KIND_INTEGER)
	{
		/*
		 * Floating point on the right, integer on the left
		 * Cast the left operand to a float so as not to lose detail.
		 */

		instructions_add(OC_SWS, 0, 0);
		instructions_add(OC_FOP, 0, FOP_FLO);
		instructions_add(OC_SWS, 0, 0);
				
		exp_res.type = KIND_FLOAT;
	}	
	else if (can_be_big_number(&left_res.ident)
		&& (exp_res.type == KIND_INTEGER || left_res.type == KIND_FLOAT))
	{
		/*
		 * 214324L + 2
		 *
		 * Left is a big number, right is a normal number.
		 * Convert right to a big number.
		 */
				
		xtos(level, ptx, exp_res);

		exp_res = exp_bignumber;
	}
	else if (can_be_big_number(&exp_res.ident)
		&& can_be_string(&left_res.ident)
		&& !can_be_big_number(&left_res.ident))
	{
		/*
		 * String + BigNumber = String
		 */

		return exp_string;
	}
	else if (can_be_big_number(&left_res.ident)
		&& can_be_string(&exp_res.ident)
		&& !can_be_big_number(&exp_res.ident))
	{
		/*
		 * BigNumber + String = String
		 */
		return exp_string;
	}
	else if (can_be_big_number(&exp_res.ident)
		&& (left_res.type == KIND_INTEGER ||left_res.type == KIND_FLOAT))
	{
		/*
		 * Left is a normal number.  Right is a big number.
		 * Convert the left to a big number.
		 */

		instructions_add(OC_SWS, 0, 0);
			
		xtos(level, ptx, left_res);

		instructions_add(OC_SWS, 0, 0);

		exp_res = exp_bignumber;
	}
	else if (can_be_string(&exp_res.ident)
		&& (left_res.type == KIND_INTEGER || left_res.type == KIND_FLOAT))
	{
		if (operation == SYM_PLUS)
		{
			/*
			 * Right is a string, convert left to a string (if they do a +)
			 */

			instructions_add(OC_SWS, 0, 0);
			
			xtos(level, ptx, left_res);
			
			instructions_add(OC_SWS, 0, 0);

			exp_res =  exp_string;
		}
		else
		{
			/*
			 * Convert right to an integer.
			 */

			stoi(level, ptx);
		}
	}
	else if (can_be_string(&left_res.ident)
		&& (exp_res.type == KIND_INTEGER || exp_res.type == KIND_FLOAT))
	{
		if (operation == SYM_PLUS)
		{
			/*
			 * Left is a string, convert right to a string (if they do a +).
			 */

			xtos(level, ptx, exp_res);
			
			exp_res = exp_string;
		}
		else
		{
			/*
			 * Left is a string, convert left to an integer.
			 */

			instructions_add(OC_SWS, 0, 0);
			
			stoi(level, ptx);

			instructions_add(OC_SWS, 0, 0);

			exp_res =  exp_integer;
		}
	}
	else
	{
		return do_static_cast(level, ptx, left_res, exp_res);
	}

	return exp_res;
}

/*
 * Processes bitwise operational expressions (eg. & | ~ ^).
 */
exp_res_t expression_bitwise(int level, int *ptx, tag_t tag)
{	
	int symbol;
	exp_res_t exp_res;

	exp_res = expression_term(level, ptx, tag);

	while ((m_symbol >= SYM_BITWISE_START && m_symbol <= SYM_BITWISE_END)
		&& m_symbol != SYM_TILD) /* tild is a unary operation */
	{
		if (exp_res.type != KIND_INTEGER)
		{
			error(10360);
		}

		symbol = m_symbol;

		get_symbol();

		exp_res = expression_cast_op(expression_term, level, ptx, 0, exp_res, tag);

		switch (symbol)
		{

		case SYM_HAT:

			instructions_add(OC_OPR, 0, OPR_XOR);

			break;

		case SYM_PIPE:

			instructions_add(OC_OPR, 0, OPR_BOR);

			break;

		case SYM_AMPERSAND:

			instructions_add(OC_OPR, 0, OPR_AND);
			
			break;

		}
	}

	return exp_res;
}


/*
 * Processes array element expressions.
 */
exp_res_t expression_array_element(identinfo_t *ident_p, int level, int *ptx, BOOL aggregate, BOOL push)
{
	ident_p->data.array_val_p->assigning_ref = FALSE;

	array_load_offset(ident_p, level, ptx, aggregate);
	
	if (m_symbol == SYM_BECOMES)
	{
		return statement_becomes(ident_p, level, ptx, push, m_empty_tag);
	}
	else
	{	
		return expression_pure_array_element(ident_p, level, ptx, aggregate); 
	}
}


/*
 * Processes array element expressions.
 */
exp_res_t expression_pure_array_element(identinfo_t *ident_p, int level, int *ptx, BOOL aggregate)
{	
	int cx;
	BOOL func_ag = FALSE;
	exp_res_t exp_res, exp_res2;

	ZEROMEMORY(exp_res);		
	
	/*
	 * Put the array index into the CX register.
	 */	
	
	cx = m_instructions_index;

	instructions_add(OC_PAS, 0, REG_CX);

	if (m_symbol == SYM_LPAREN)
	{
		/*
		 * Load the value of the array onto the stack.
		 */		

		m_instructions[cx].f = OC_POP;
		
		if (aggregate)
		{	
			/*
			 * JSL's 'a' argument is how far down the stack to load from (or get an array
			 * handle from).
			 */

			instructions_add(OC_MLS, 0, 0);
		}
		else if (ident_p->byref)
		{
			instructions_add(OC_MLI, level - ident_p->level, ident_p->address);
		}
		else
		{
			instructions_add(OC_MLO, level - ident_p->level, ident_p->address);
		}

		if (ident_p->data.array_val_p->contains.kind == KIND_FUNCTION
			&& ident_p->data.array_val_p->contains.data.function_val_p->return_var.kind != KIND_VOID)
		{
			/*
			 * Calling a function directly after getting the reference from
			 * an array.
			 * eg. array[0](100);  (The array has to hold a function).
			 */
						
			exp_res = function_procedure_caller(level, ptx, &ident_p->data.array_val_p->contains, TRUE);			

			func_ag = TRUE;

			instructions_add(OC_SWS, 0, 0);
			pop_kind(KIND_INTEGER, level, ptx);
		}
		else
		{
			error(7001);
			error(12010);
		}
	}
	else
	{
		exp_res.type = ident_p->data.array_val_p->contains.kind;
		exp_res.ident = ident_p->data.array_val_p->contains;
	}

	if (!func_ag)
	{
		SET_EXP_RES(exp_res2, (*ident_p));

		if (!aggregate && (m_symbol >= SYM_UNARY_START && m_symbol <= SYM_UNARY_END)
			&& ((ident_p->data.array_val_p->contains.kind == KIND_INTEGER
				|| ident_p->data.array_val_p->contains.kind == KIND_FLOAT)))
		{
			/*
			 * Make the top be the array index previously SLDed.
			 * Underneath this is the value loaded from the array.
			 */

			if ((m_symbol == SYM_PLUS_PLUS || m_symbol == SYM_MINUS_MINUS))
			{
				if (aggregate)
				{	
					/*
					 * JSL's 'a' argument is how far down the stack to load from (or get an array
					 * handle from).
					 */

					instructions_add(OC_MLS, 0, 0);
				}
				else if (ident_p->byref)
				{
					instructions_add(OC_MLI, level - ident_p->level, ident_p->address);
				}
				else
				{
					instructions_add(OC_MLO, level - ident_p->level, ident_p->address);
				}

				instructions_add(OC_SWS, 0, 0);
			}

			exp_res = expression_ident_unary(level, ptx, exp_res2);
		}
		else
		{
			m_instructions[cx].f = OC_POP;

			if (aggregate)
			{	
				/*
				 * JSL's 'a' argument is how far down the stack to load from (or get an array
				 * handle from).
				 */

				instructions_add(OC_MLS, 0, 0);				
			}
			else if (ident_p->byref)
			{
				instructions_add(OC_MLI, level - ident_p->level, ident_p->address);
			}
			else
			{
				instructions_add(OC_MLO, level - ident_p->level, ident_p->address);
			}
		}
	}

	if (aggregate)
	{
		/*
		 * If this was an aggregate call get rid of the array handle underneath the last
		 * answer (array element).  The array also needs to be released since it's not passed
		 * on.
		 */
		
		/*
		 * Swap the top two on the stack.  The top will then be the array handle.
		 */
		instructions_add(OC_SWS, 0, 0);
		
		/*
		 * Release the array by the handle off the stack.  This also pops the handle off the stack.
		 */
		memory_release_by_handle_off_stack(level, ptx);
	}

	return exp_res;
}


/*
 * Processes conditions.
 */
exp_res_t expression_condition(int level, int *ptx, tag_t tag)
{	
	exp_res_t exp_res;

	exp_res = expression_string_condition(level, ptx, tag);

	while (m_symbol >= SYM_CMP_START && m_symbol <= SYM_CMP_END)
	{			
		exp_res = expression_condition_worker(level, ptx, exp_res, TRUE, expression_string_condition, tag);
	}

	return exp_res;
}


/*
 * Handles expressions that start with identifiers like function calls and assignments.
 */
exp_res_t expression_ident(int level, int *ptx, tag_t tag)
{	
	BOOL statement = FALSE;
	exp_res_t exp_res;
	identinfo_t *ident_p;

	ZEROMEMORY(exp_res);	

	ident_p = ident_find((char*)m_ident_name, ptx);

	if (ident_p)
	{
		if (!tag.dont_getsymbol)
		{
			get_symbol();
		}
		else
		{
			tag.dont_getsymbol = FALSE;
		}

		/*
		 * Means the result shouldn't be pushed on the stack.
		 */
		if (tag.statement)
		{
			statement = TRUE;
			
			/*
			 * Statements only applies to first level, any subsequent recursion
			 * should be an expression.
			 */
			tag.statement = FALSE;
		}

		/*
		 * Is this a procedure or function returning void?
		 */

		if (ident_p->kind == KIND_PROCEDURE || (ident_p->kind == KIND_FUNCTION && ident_p->data.function_val_p->return_var.kind == KIND_VOID))			
		{
			/*
			 * If there's no argument list.
			 */

			if (m_symbol != SYM_LPAREN)
			{
				/*
				 * Then the function or procedure can be passed as a pointer
				 */

				if (!statement)
				{
					pass_or_return_function(ident_p, level, ptx);
				};
			}
			else
			{
				/*
				 * Has no return value, and the function isn't passed  as a pointer,
				 * so this is invalid.
				 */
				
				error(4208);				
			}	
			
			exp_res.ident = *ident_p;

			return exp_res;
		}
		
		if (ident_p->kind == KIND_ARRAY)
		{
			if (ident_p->constant && !tag.lvalue_nogc)
			{
				load_new_array(ident_p->data.array_val_p->array_default, level, ptx);

				SET_EXP_RES(exp_res, m_literal_string);

				/*
				 * Can't assign to a constant string.
				 */

				if (m_symbol == SYM_BECOMES)
				{
					error(11060);
				}
			}
			else
			{
				if (m_symbol == SYM_LBRACKET && !tag.lvalue_nogc)
				{	
					exp_res = expression_array_element(ident_p, level, ptx, FALSE, !statement);
				}
				else
				{
					if ((m_symbol == SYM_BECOMES || (m_symbol == SYM_EQL && statement))
							&& !tag.lvalue_nogc)
					{
						ident_p->data.array_val_p->assigning_ref = TRUE;

						exp_res = statement_becomes(ident_p, level, ptx, !statement, tag);
					}
					else
					{
						/*
						 * If this is a statement, we just can't ignore the array by itself
						 * cause there might be a unary operator after it.
						 */
						if (!(m_symbol == SYM_SEMICOLON && statement))
						{
							exp_res.type = KIND_ARRAY;
							exp_res.ident = *ident_p;

							/*
							 * Don't addref when a handle is wanted cause handles
							 * aren't garbage collected (and are unsafe!).
							 * This is used for garbage collection routines in
							 * array.p++.
							 */
							if (!tag.lvalue_nogc)
							{
								memory_addref(ident_p, level, ptx);
							}
													
							load_array_memory_address(ident_p, level, ptx);						

							exp_res = expression_ident_array_unary(level, ptx, exp_res);

							if (statement)
							{
								memory_release_by_handle_off_stack(level, ptx);
							}
						}
					}				
				}
			}
		}		
		else if (m_symbol == SYM_BECOMES || (m_symbol == SYM_EQL && statement))
		{	
			/*
			 * Do a becomes even if they use = (when it's a statement)
			 * so becomes can give an error about using the wrong operator.
			 * When it's not a statement, = is valid as a comparison
			 * operator which will be evaluated later on in the expression
			 * stack.
			 */

			exp_res = statement_becomes(ident_p, level, ptx, !statement, tag);
						
			return exp_res;
		}		
		else if (m_symbol == SYM_LPAREN)
		{
			if (ident_p->kind == KIND_FUNCTION)
			{	
				exp_res = function_procedure_caller(level, ptx, ident_p, FALSE);

				if (statement)
				{
					pop_kind(exp_res.type, level, ptx);
				}
			}
			else
			{
				error(7001); /* non function with parens () */
			}
		}
		else if (ident_p->kind == KIND_FUNCTION)
		{
			/*
			 * Function without parentheses, must be passing a pointer to it.
			 */

			if (!statement)
			{
				pass_or_return_function(ident_p, level, ptx);
				
				SET_EXP_RES(exp_res, (*ident_p));
			}
		}
		else if (ident_p->kind == KIND_INTEGER || ident_p->kind == KIND_FLOAT)
		{
			SET_EXP_RES(exp_res, (*ident_p));

			if (m_symbol >= SYM_UNARY_START && m_symbol <= SYM_UNARY_END)
			{				
				if (m_symbol == SYM_PLUS_PLUS || m_symbol == SYM_MINUS_MINUS)
				{
					if (!statement)
					{
						load_ident(ident_p, level, ptx);
					}
				}

				expression_ident_unary(level, ptx, exp_res);
			}
			else
			{
				if (!statement)
				{
					load_ident(ident_p, level, ptx);
				}
			}			
		}
		else
		{				
			error(4200);
		}		
	}
	else
	{		
		strcpy(m_message, " (");
		strcat(m_message, m_ident_name);
		strcat(m_message, ")");
	
		error_extra(4001, m_message, TRUE, FALSE);

		get_symbol();
	}

	return exp_res;
}

/*
 * Handles unary operators like ++, --, += etc.
 */
exp_res_t expression_unary(identinfo_t *ident_p, int symbol, int level, int *ptx, exp_res_t current_res,  BOOL left)
{
	int op = 0;
	exp_res_t exp_res;

	if (ident_p->kind == KIND_ARRAY)
	{
		SET_EXP_RES(current_res, ident_p->data.array_val_p->contains);
	}
	else
	{
		SET_EXP_RES(current_res, (*ident_p));
	}

	if (symbol == SYM_PLUS_PLUS)
	{
		if (ident_p->kind == KIND_ARRAY)
		{			
			instructions_add(OC_POP, 0, REG_CX);
			instructions_add(OC_PUS, 0, REG_DX);
			instructions_add(OC_SRG, REG_MEMORY_FLAG, REG_DX);
		}

		if (current_res.type == KIND_FLOAT)
		{
			instructions_add_f(OC_LIT, 0, 1);
		}
		else
		{
			instructions_add(OC_LIT, 0, 1);
		}

		if (ident_p->byref)
		{
			instructions_add_itof(OC_IIA, level - ident_p->level, ident_p->address, current_res.type);
		}
		else
		{				
			instructions_add_itof(OC_IAD, level - ident_p->level, ident_p->address, current_res.type);
		}

		if (!left)
		{
			get_symbol();
		}

		if (ident_p->kind == KIND_ARRAY)
		{	
			instructions_add(OC_POP, 0, REG_DX);
		}
	
		return current_res;
	}
	else if (symbol == SYM_MINUS_MINUS)
	{
		if (ident_p->kind == KIND_ARRAY)
		{			
			instructions_add(OC_POP, 0, REG_CX);
			instructions_add(OC_PUS, 0, REG_DX);
			instructions_add(OC_SRG, REG_MEMORY_FLAG, REG_DX);
		}

		if (current_res.type == KIND_FLOAT)
		{
			instructions_add_f(OC_LIT, 0, 1);
		}
		else
		{
			instructions_add(OC_LIT, 0, 1);
		}

		if (ident_p->byref)
		{
			instructions_add_itof(OC_IIS, level - ident_p->level, ident_p->address, current_res.type);
		}
		else
		{				
			instructions_add_itof(OC_ISU, level - ident_p->level, ident_p->address, current_res.type);
		}

		if (!left)
		{
			get_symbol();
		}

		if (ident_p->kind == KIND_ARRAY)
		{	
			instructions_add(OC_POP, 0, REG_DX);
		}

		return current_res;
	}
	else if (left == TRUE)
	{
		if (ident_p->kind == KIND_ARRAY)
		{	
			instructions_add(OC_POP, 0, REG_DX);
		}

		return current_res; /* left handed -- or ++ are valid, but you can't do += -= etc on the left */
	}
	else if (symbol == SYM_HAT_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIX;
		}
		else
		{
			op = OC_IXO;
		}

		get_symbol();
	}
	else if (symbol == SYM_PIPE_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIO;
		}
		else
		{
			op = OC_IOR;
		}

		get_symbol();
	}
	else if (symbol == SYM_AMPERSAND_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIB;
		}
		else
		{
			op = OC_IAN;
		}

		get_symbol();
	}
	else if (symbol == SYM_MINUS_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIS;
		}
		else
		{
			op = OC_ISU;
		}

		get_symbol();
	}
	else if (symbol  == SYM_PLUS_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIA;
		}
		else
		{
			op = OC_IAD;
		}

		get_symbol();
	}
	else if (symbol == SYM_STAR_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IIM;
		}
		else
		{
			op = OC_IMU;
		}

		get_symbol();
	}
	else if (symbol == SYM_SLASH_EQL)
	{
		if (ident_p->byref)
		{
			op = OC_IID;
		}
		else
		{
			op = OC_IDI;
		}

		get_symbol();
	}
	else if (symbol == SYM_RCHEVRON_EQL)
	{
		if (ident_p->kind != KIND_INTEGER
			&& (!(ident_p->kind == KIND_ARRAY 
			&& ident_p->data.array_val_p->contains.kind == KIND_INTEGER)))
		{
			error(13000); /* shifting non integer */
		}

		if (ident_p->byref)
		{
			op = OC_IZR;
		}
		else
		{
			op = OC_ISR;
		}

		get_symbol();
	}
	else if (symbol == SYM_LCHEVRON_EQL)
	{
		if (ident_p->kind != KIND_INTEGER
			&& (!(ident_p->kind == KIND_ARRAY 
			&& ident_p->data.array_val_p->contains.kind == KIND_INTEGER)))
		{
			error(13000); /* shifting non integer */
		}

		if (ident_p->byref)
		{
			op = OC_IZL;
		}
		else
		{		
			op = OC_ISL;
		}

		get_symbol();
	}
	else
	{
		if (ident_p->kind == KIND_ARRAY)
		{	
			instructions_add(OC_POP, 0, REG_DX);
		}

		return m_empty_res;
	}
	
	if (symbol == SYM_LCHEVRON_EQL || symbol == SYM_RCHEVRON_EQL)
	{
		current_res.type = KIND_INTEGER;
	}
	
	if (ident_p->kind == KIND_ARRAY)
	{
		instructions_add(OC_POP, 0, REG_CX);
		instructions_add(OC_PUS, 0, REG_DX);
	}

	exp_res = expression_cast_op(expression_raw, level, ptx, 0, current_res, m_empty_tag);

	if (ident_p->kind == KIND_ARRAY)
	{		
		instructions_add(OC_SRG, REG_MEMORY_FLAG, REG_DX);		
	}
	
	instructions_add_itof(op, level - ident_p->level, ident_p->address, exp_res.type);

	if (ident_p->kind == KIND_ARRAY)
	{	
		instructions_add(OC_POP, 0, REG_DX);
	}

	return current_res;
}


exp_res_t expression_ident_array_unary(int level, int *ptx, exp_res_t current_res)
{
	exp_res_t exp_res;

	exp_res = current_res;

	if (m_symbol == SYM_PLUS_EQL)
	{
		get_symbol();

		expression_cast_op(expression_raw, level, ptx, 0, current_res, m_empty_tag);

		binary_operation_function_caller("integer_array_append", level, ptx);
	}

	return exp_res;
}

/*
 * Does inner expressions (inside parenthesis), not caring about the return type. 
 */
exp_res_t expression_inner(int level, int *ptx, tag_t tag)
{	
	int kind;
	BOOL cast = FALSE;
	identinfo_t *ident_p;
	exp_res_t exp_res, cast_exp_res;

	ZEROMEMORY(exp_res);
	ZEROMEMORY(cast_exp_res);

	get_symbol();

	if ((m_symbol > SYM_TYPESTART && m_symbol < SYM_TYPEEND)
		|| m_symbol == SYM_IDENT)
	{
		if (m_symbol == SYM_IDENT)
		{
			ident_p = type_find(m_ident_name);

			if (ident_p)
			{
				SET_EXP_RES(cast_exp_res, *ident_p);
				cast = TRUE;
			}			
		}
		
		if (!cast)
		{
			kind = get_kind();

			cast = TRUE;

			switch (kind)
			{
			case KIND_STRING:
				
				if (m_symbol == SYM_BIGNUMBER)
				{
					cast_exp_res = exp_bignumber;					
				}
				else
				{
					cast_exp_res = exp_string;
				}

				break;

			case KIND_FLOAT:

				cast_exp_res = exp_float;

				break;

			case KIND_INTEGER:

				cast_exp_res = exp_integer;
				
				if (m_symbol == SYM_CHARACTER)
				{
					cast_exp_res.ident.data.int_val.is_character = TRUE;
				}
				else if (m_symbol == SYM_BOOLEAN)
				{
					cast_exp_res.ident.data.int_val.is_boolean = TRUE;
				}
								
				break;

			default:

				cast = FALSE;
			}
		}		

		if (cast)
		{				
			get_symbol();

			if (m_symbol == SYM_RPAREN)
			{
				get_symbol();
			}
			else
			{
				error(6002);
			}
		}
	}
	
	
	

	if (cast)
	{
		/*
		 * When you cast, you can only cast one a single operand.  If you want to cast more, then use parenthesis.
		 */

		exp_res = expression_operand(level, ptx, tag);

		exp_res = do_static_cast(level, ptx, exp_res, cast_exp_res);
	}
	else
	{
		exp_res = expression_raw(level, ptx, tag);
	}

	if (!cast)
	{
		if (m_symbol == SYM_RPAREN)
		{
			get_symbol();
		}
		else
		{
			error(6002);
		}
	}
	
	return exp_res;
}

/*
 * Processes left hand unary operations.
 */
exp_res_t expression_unary_left(int level, int *ptx, tag_t tag)
{
	int symbol;
	identinfo_t *ident_p;
	exp_res_t exp_res;

	ZEROMEMORY(exp_res);

	symbol = m_symbol;
	get_symbol();

	if (m_symbol == SYM_IDENT)
	{
		ident_p = ident_find(m_ident_name, ptx);

		get_symbol();

		if (ident_p)
		{
			if (ident_p->kind == KIND_INTEGER || ident_p->kind == KIND_FLOAT)			
			{
				SET_EXP_RES(exp_res, (*ident_p));

				exp_res = expression_unary(ident_p, symbol, level, ptx, exp_res, TRUE);
				
				load_ident(ident_p, level, ptx);				
			}
			else if (ident_p->kind == KIND_ARRAY)
			{
				if (ident_p->data.array_val_p->contains.kind == KIND_INTEGER
					|| ident_p->data.array_val_p->contains.kind == KIND_FLOAT)
					{
					/*
					 * Load up the array offset.
					 */
					array_load_offset(ident_p, level, ptx, FALSE);
					
					/*
					 * Need to SLD cause this is an expression, and we need to use the
					 * index again to get the element out after the -- or ++.
					 */
					instructions_add(OC_SLD, 0, 0);

					/*
					 * Do the unary operation on the array element.
					 */
					exp_res = expression_unary(ident_p, symbol, level, ptx, exp_res, TRUE);

					/*
					 * Load up the new array element value.
					 */
					expression_pure_array_element(ident_p, level, ptx, FALSE);
				}
				else
				{
					error(10400);
				}
			}
			else
			{
				get_symbol();
				error(13010);
			}
		}
		else
		{			
			error(4000);
		}
	}
	else
	{
		error(4200);
	}

	return exp_res;
}



/*
 * Processes top level operands like numbers and identifiers.
 */
exp_res_t expression_operand(int level, int *ptx, tag_t tag)
{	
	exp_res_t exp_res;
	int *i_p;
	int kind, sign;
	int symbol;

	ZEROMEMORY(exp_res);
	
	for(;;)
	{
		if (m_symbol == SYM_BEGIN)
		{
			exp_res = expression_arraydef(level, ptx, tag);
		}
		else if (m_symbol == SYM_NULL)
		{
			get_symbol();

			instructions_add(OC_LIT, 0, 0);

			exp_res = m_exp_null;

			break;
		}
		else if (m_symbol == SYM_LITERAL_STRING
			|| m_symbol == SYM_LITERAL_BIGNUMBER)
		{
			symbol = m_symbol;

			get_symbol();

			i_p = chars2ints(m_string);

			load_new_array(i_p, level, ptx);

			free(i_p);

			
			if (symbol == SYM_LITERAL_BIGNUMBER)
			{	
				SET_EXP_RES(exp_res, exp_bignumber.ident);
			}
			else
			{
				SET_EXP_RES(exp_res, m_literal_string);				
			}
			
			if (m_symbol == SYM_LBRACKET)
			{
				exp_res = expression_array_element(&m_literal_string, level, ptx, TRUE, FALSE);
			}

			break;
		}
		else if (m_symbol == SYM_PLUS || m_symbol == SYM_MINUS)
		{
			/*
			 * This handles negative signs on the left of numbers, identifiers or
			 * parenthesis.  Done here rather than in expression_plus_minus in the
			 * original PL/0 compiler because that caused bugs when you used * /
			 * operators like x:= -3 * -3.
			 */

			/*
			 * If the last symbol inidicates that this is a minus not negative sign.
			 */
			if ((
				(m_last_symbol == SYM_LITERAL_NUMBER)
				|| (m_last_symbol == SYM_RPAREN)
				|| (m_last_symbol == SYM_IDENT)
				|| (m_last_symbol == SYM_STRING)
				|| (m_last_symbol == SYM_BIGNUMBER)
				|| (m_last_symbol == SYM_RBRACKET)
				))
			{
				/*
				 * Then this plus/minus sign must be actually plus and minus, not
				 * "make this number negative or positive."
				 * Break out and drop down to the plus_minus function.
				 */
				break;
			}

			sign = 0;

			while (m_symbol == SYM_PLUS || m_symbol == SYM_MINUS)
			{
				if (m_symbol == SYM_PLUS)
				{
					get_symbol();
				}
				else if (m_symbol == SYM_MINUS)
				{
					get_symbol();
					sign++;
				}
			}

			if (m_symbol == SYM_LITERAL_NUMBER)
			{
				exp_res = load_number(level, ptx);
			}
			else if (m_symbol == SYM_LITERAL_BIGNUMBER)
			{
				exp_res = expression_operand(level, ptx, tag);
			}
			else if (m_symbol == SYM_IDENT)
			{
				exp_res = expression_ident(level, ptx, tag);
			}
			else if (m_symbol == SYM_LPAREN)
			{
				exp_res = expression_inner(level, ptx, tag);
			}
			else
			{
				/*
				 * Negative sign before an expression which can't be turned negative.
				 */
				error(7530);

				break;
			}

			if (sign % 2)
			{
				if (can_be_big_number(&exp_res.ident))
				{
					unary_operation_function_caller("big_number_negate", level, ptx);
				}
				else
				{
					instructions_add_itof(OC_OPR, 0, OPR_NEG, exp_res.type);
				}
			}
		}
		else if (m_symbol == SYM_PLUS_PLUS || m_symbol == SYM_MINUS_MINUS)
		{
			exp_res = expression_unary_left(level, ptx, m_empty_tag);

			break;
		}
		else if (m_symbol == SYM_IDENT)
		{
			exp_res = expression_ident(level, ptx, tag);
			
			break;
		}
		else if (m_symbol == SYM_ODD)
		{
			get_symbol();

			exp_res.type = KIND_INTEGER;

			exp_res = expression_cast_op(expression_raw, level, ptx, 0, exp_res, tag);

			instructions_add(OC_OPR, 0, OPR_ODD);					
		}
		else if (m_symbol == SYM_ABS)
		{
			get_symbol();

			exp_res = expression_operand(level, ptx, tag);
			
			absolute_top(exp_res.type);
		}
		else if (m_symbol == SYM_EXCLAMATION)
		{
			get_symbol();

			exp_res = expression(level, ptx, exp_integer, tag);

			logical_not_top(level, ptx, exp_res);

			exp_res.type = KIND_INTEGER;
		}
		else if (m_symbol == SYM_TILD)
		{
			get_symbol();

			kind = KIND_UNKNOWN;
						
			exp_res = expression(level, ptx, exp_integer, tag);

			if (exp_res.type != KIND_INTEGER)
			{
				error(10360);

				/*
				 * Cast to an integer before doing the NOT.
				 */

				do_static_cast(level, ptx, exp_unknown, exp_integer);
			}

			instructions_add(OC_OPR, 0, OPR_NOT);
		}
		else if (m_symbol == SYM_LAMBDA)
		{			
			get_symbol();
			
			exp_res = expression_lambda(level, ptx);
						
			break;;
		}
		else if (m_symbol == SYM_LITERAL_NUMBER)
		{
			exp_res = load_number(level, ptx);

			break;;
		}
		else if (m_symbol == SYM_LPAREN)
		{
			exp_res = expression_inner(level, ptx, tag);

			break;
		}

		break;
	}

	return exp_res;
}


/*
 * Processes bitwise shift expressions.
 */
exp_res_t expression_shift(int level, int *ptx, tag_t tag)
{
	int op;
	exp_res_t exp_res;

	exp_res = expression_plus_minus(level, ptx, tag);

	/*
	 * Handle bit shifts.
	 */

	while ((m_symbol == SYM_LCHEVRON) || (m_symbol == SYM_RCHEVRON))
	{
		op = m_symbol;

		get_symbol();

		if (exp_res.type != KIND_INTEGER)
		{
			/*
			 * Shifting a non-integer.
			 */
			error(13000);
		}

		exp_res = expression_cast_op(expression_plus_minus, level, ptx, 0, exp_res, tag);

		if (op == SYM_LCHEVRON)
		{
			instructions_add(OC_OPR, 0, OPR_SHL);
		}
		else
		{
			instructions_add(OC_OPR, 0, OPR_SHR);
		}
	}

	return exp_res;
}

/*
 * Processes inline if statements.
 */
exp_res_t expression_inlineif(int level, int *ptx, tag_t tag)
{
	int cx1, cx2;
	exp_res_t exp_res, exp_res2;

	exp_res = expression_and_or(level, ptx, tag);

	if (m_symbol == SYM_QMARK)
	{
		get_symbol();
			
		cx1 = m_instructions_index;
		instructions_add(OC_JPC, 0, 0);
		
		exp_res = expression_raw(level, ptx, tag);
		
		cx2 = m_instructions_index;
		instructions_add(OC_JMP, 0, 0);
		
		if (m_symbol == SYM_COLON)
		{
			get_symbol();
		}
		else
		{
			error(6016);
		}

		m_instructions[cx1].a = m_instructions_index;
		
		exp_res2 = expression_raw(level, ptx, tag);
				
		m_instructions[cx2].a = m_instructions_index;

		if (exp_res.type != exp_res2.type)
		{
			error(13400);
		}
	}

	return exp_res;
}	

/*
 * Processes conditions on strings.
 */
exp_res_t expression_string_condition(int level, int *ptx, tag_t tag)
{
	int op, symbol;
	exp_res_t exp_res;

	exp_res = expression_shift(level, ptx, tag);

	/*
	 * Can only do equals once.  Can't do "hi" $= "hi" $= "hi".
	 */
	if (((m_symbol >= SYM_AOP_START && m_symbol <= SYM_AOP_END)
		|| (m_symbol >= SYM_CMP_START && m_symbol <= SYM_CMP_END))
		)
	{
		if (!(m_symbol == SYM_EQL && !can_be_big_number(&exp_res.ident)))
		{
			if (can_be_string(&exp_res.ident))
			{
				symbol = m_symbol;
					
				get_symbol();

				exp_res = expression_cast_op(expression_shift, level, ptx, 0, exp_res, tag);
					
				if (can_be_big_number(&exp_res.ident))
				{
					binary_operation_function_caller("big_number_compare", level, ptx);
				}
				else
				{
					binary_operation_function_caller("integer_array_compare", level, ptx);
				}
										
				if (symbol == SYM_STR_EQUALS)
				{
					instructions_add(OC_LIT, 0, 0);
					instructions_add(OC_OPR, 0, OPR_EQL);
				}
				else if (symbol == SYM_STR_NEQUALS)
				{
					instructions_add(OC_LIT, 0, 0);
					instructions_add(OC_OPR, 0, OPR_NEQ);
				}
				else
				{
					instructions_add(OC_LIT, 0, 0);

					op = condition_operator(symbol, exp_res.type, tag);

					instructions_add(OC_OPR, 0, op);
				}
				
				exp_res = exp_integer;
				exp_res.ident.data.int_val.is_boolean = TRUE;
			}
		}
	}

	return exp_res;
}


/*
 * Processes multiplication expressions.
 */
exp_res_t expression_term(int level, int *ptx, tag_t tag)
{
	int op;
	exp_res_t exp_res;

	/*
	 * Load up a number if there is one, if there is a parenthesis, handle
	 * everything inside it (including addition/subtraction).
	 * Otherwise, multiplication has priority.
	 */
	exp_res = expression_operand(level, ptx, tag);

	if (exp_res.type == KIND_FLOAT || exp_res.type == KIND_INTEGER
		|| can_be_big_number(&exp_res.ident))
	{
		if (can_be_big_number(&exp_res.ident))
		{
			int x = 10;
		}

		/*
		 * Handle all the multiplication.
		 */
		while ((m_symbol == SYM_STAR) || (m_symbol == SYM_SLASH) || (m_symbol == SYM_PERCENT))
		{
			op = m_symbol;

			get_symbol();

			exp_res = expression_cast_op(expression_operand, level, ptx, 0, exp_res, tag);

			if (can_be_big_number(&exp_res.ident))
			{
				if (op == SYM_STAR)
				{
					binary_operation_function_caller("big_number_mul", level, ptx);	
				}
				else if (op == SYM_SLASH)
				{
					binary_operation_function_caller("big_number_div", level, ptx);	
				}
				else if (op == SYM_PERCENT)
				{
					binary_operation_function_caller("big_number_mod", level, ptx);	
				}
				else
				{
					error(40000);
				}
			}
			else
			{
				/*
				 * Now the the operation.
				 */
				if (op == SYM_STAR)
				{
					instructions_add_itof(OC_OPR, 0, OPR_MUL, exp_res.type);
				}
				else if (op == SYM_SLASH)
				{
					instructions_add_itof(OC_OPR, 0, OPR_DIV, exp_res.type);
				}
				else if (op == SYM_PERCENT)
				{
					instructions_add_itof(OC_OPR, 0, OPR_MOD, exp_res.type);
				}
			}
		}
	}

	return exp_res;
}