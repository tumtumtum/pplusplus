#include "types.h"
#include "interpretor.h"
#include "symbols.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#ifdef _WIN32
#include "stdlib.h"
#include "conio.h"
#include "stdio.h"
#define GETCH _getch();
#else
#include <strings.h>
#include <curses.h>
#define GETCH getch();
#endif

/*
 Crashes if you try to read from a file handle that is no longer valid.
 try rewriting f_eof to always return 0 and keep reading past the end of file.
 */

/*
 * Program stack size.
 */
#define STACK_SIZE 0x2500

/*
 * How many dynamic memory handles are available.
 * Each p++ string/array requires one of these.
 */
#define NUMBER_OF_MEMORY_HANDLES 0x1000
#define NUMBER_OF_FILES 0x100

 /*
 * Error macros.
 */

/*
 * Generic error detected.
 */
#define INTERPRET_ERROR(s)	\
	printf("*ERROR* %s", s);	\
	return 0;

/*#ifdef _DEBUG*/
#define CHECK_MEMORY(x)	\
	if (!x || x <= 0 || x >= NUMBER_OF_MEMORY_HANDLES || !m_memory_handles[x])	\
	{					\
		printf("Null or invalid memory handle error (%d).\n", x);	\
		return 0;	\
	}

/*#else
#define CHECK_MEMORY(x)
#endif
*/

/*
 * Program, base, top, instruction and temp registers.
 */
static int p, b, t, m_temp, m_size, param_count = 0;

/*
 * Scratch pad registers (REG_AX - REG_EX).
 */
static int m_registers[6];
static int *m_memory_handles[NUMBER_OF_MEMORY_HANDLES];
static FILE *m_files[NUMBER_OF_FILES];
static int real_interpret(instruction_t* code);

/*
 * Stack.
 */
static int m_stack[STACK_SIZE];

int find_free_memory_handle()
{
	register int i;

	for (i = 1; i < NUMBER_OF_MEMORY_HANDLES; i++)
	{
		if (m_memory_handles[i] == 0)
		{
			return i;
		}
	}

	INTERPRET_ERROR("Out of memory handles.\n");
	
	return -1;
}


int find_free_file()
{
	register int i;

	for (i = 1; i < NUMBER_OF_FILES; i++)
	{
		if (m_files[i] == 0)
		{
			return i;
		}
	}

	INTERPRET_ERROR("Out of file storage locations.\n");
	
	return -1;
}


int all_handles_free()
{
	int i;

	for (i = 1; i < NUMBER_OF_MEMORY_HANDLES; i++)
	{
		if (m_memory_handles[i] != 0)
		{
			return 0;
		}
	}

	return 1;
}

int free_memory_handles()
{
	int i;

	for (i = 1; i < NUMBER_OF_MEMORY_HANDLES; i++)
	{
		if (m_memory_handles[i] != 0)
		{
			free(--m_memory_handles[i]);
		}
	}

	return 1;
}

int close_files()
{
	int i;

	for (i = 1; i < NUMBER_OF_MEMORY_HANDLES; i++)
	{
		if (m_files[i] != 0)
		{
			fclose(m_files[i]);
		}
	}

	return 1;
}


int base(int l, int b)
{  
	int b1; /* find base l levels down */

	b1 = b; 
	
	while (l > 0)
	{
		/*
		 * m_stack[b1] is the static link
		 */

		b1 = m_stack[b1];

		l--;
	}
	return b1;
}


int fop(int op, float *f1_p, float *f2_p, float *result);

int file_close(instruction_t *code)
{
	register int address;

	address = m_stack[t];

	if (address
		&& m_files[address])
	{
		fclose(m_files[address]);
		m_files[address] = NULL;
	}

	t--;
	
	return 1;
}

int file_open(instruction_t *code)
{
	int i, *i_p1, len, flags;
	register int address;
	char mode[128];
	char filename[MAX_PATH];
	
	mode[0] = 0;

	len = m_stack[t--];
	address = m_stack[t--];
	flags = m_stack[t];

	CHECK_MEMORY(address);
	
	i_p1 = ((int*)m_memory_handles[address]);
	i_p1 += m_registers[REG_CX];
	
	for (i = 0; i < len; i++)
	{
		filename[i] = i_p1[i];
	}

	filename[i] = 0;

	if (flags & F_APPEND)
	{
		strcat(mode, "a+");
	}
	else if (flags & F_READ)
	{
		if (flags & F_WRITE)
		{
			if (flags & F_CREATE)
			{
				strcat(mode, "w+");
			}
			else
			{
				strcat(mode, "r+");
			}
		}
		else
		{
			strcat(mode, "r");
		}
	}
	else if (flags & F_WRITE)
	{
		strcat(mode, "w");
	}

	if (flags & F_BINARY)
	{
		strcat(mode, "b");
	}
	else if (flags & F_TEXT)
	{
		strcat(mode, "t");
	}

	m_temp = find_free_file();
	m_files[m_temp] = fopen(filename, mode);

	if (m_files[m_temp])
	{
		m_stack[t] = m_temp;
	}
	else
	{
		m_stack[t] = 0;
	}

	return 1;
}

/*
 * Returns pointers to the two integers being worked on by inline operators
 * (++, --, *= etc).
 */
int inline_operands(instruction_t *code, int **i_pp1, int **i_pp2)
{	
	register int address;

	*i_pp2 = &m_stack[t];

	address = base(code[p].l, b) + code[p].a;

	if (m_registers[REG_DX] & REG_MEMORY_FLAG)
	{
		CHECK_MEMORY(m_stack[base(code[p].l, b) + code[p].a]);
		
		*i_pp1 = ((int*)m_memory_handles[m_stack[address]]);
		*i_pp1 += m_registers[REG_CX];
	}
	else
	{
		*i_pp1 = &m_stack[address];
	}

	return 1;
}

/*
 * Returns pointers to the two integers being worked on by inline operators
 * (++, --, *= etc).
 */
int inline_indirect_operands(instruction_t *code, int **i_pp1, int **i_pp2)
{	
	register int address;

	*i_pp2 = &m_stack[t];

	address = m_stack[base(code[p].l, b) + code[p].a];

	if (m_registers[REG_DX] & REG_MEMORY_FLAG)
	{
		CHECK_MEMORY(m_stack[address]);

		*i_pp1 = ((int*)m_memory_handles[m_stack[address]]);
		*i_pp1 += m_registers[REG_CX];
	}
	else
	{
		*i_pp1 = &m_stack[address];
	}

	return 1;
}

int interpret(instruction_t* code)
{
	if (code == NULL)
	{
		return FALSE;
	}

	real_interpret(code);
	free_memory_handles();

	return TRUE;
}

#ifdef WIN32
__forceinline 
#endif

int single_interpret(instruction_t* code)
{
	float *f_p1;	
	int *i_p1, *i_p2, j, len;
	int temp, address;

	switch (code[p].f)
	{
		case OC_NOP:
			
			p++;

			break;

		case OC_TRM:

			return 0;

			break;

		case OC_SWS:
			
			/*
			 * Swaps the top two elements on the stack around.
			 */

			temp = m_stack[t];
			m_stack[t] = m_stack[t - 1];
			m_stack[t - 1] = temp;

			p++;

			break;

		case OC_PAS:
		
			/*
			 * Pass stack into a working register (like pop but doesn't pop).
			 */

			if (code[p].a != 0)
			{
				m_registers[code[p].a] = m_stack[t];
			}

			p++;
			
			break;

		case OC_POP:
		
			/*
			 * Pop onto a working register.
			 */
			
			if (code[p].a == 0)
			{
				t--;
			}
			else
			{
				m_registers[code[p].a] = m_stack[t--];
			}

			p++;
			
			break;

		case OC_MOV:
			
			/*
			 * Move data from one register to another.
			 */

			m_registers[code[p].a] = m_registers[code[p].l];

			p++;

			break;

		case OC_RIO:
			
			/*
			 * Does a bit-wise OR on a register.
			 */

			m_registers[code[p].a] |= code[p].l;

			p++;

			break;

		case OC_SRG:

			/*
			 * Statically set a register.
			 */

			if (code[p].a != 0)
			{
				m_registers[code[p].a] = code[p].l;
			}

			p++;

			break;

		case OC_PUS:

			/*
			 * Push off a working register.
			 */
			
			if (code[p].a == 0)
			{
				t++;
			}
			else
			{
				m_stack[++t] = m_registers[code[p].a];
			}

			p++;

			break;

		case OC_SLD:
			
			/*
			 * Loads up a value from somewhere in the stack.
			 * The offset is given by code[p].a
			 */

			m_stack[t + 1] = m_stack[t - code[p].a];

			t++;

			p++;

			break;

		case OC_LIT:

			/*
			 * Load a literal number.
			 */

			m_stack[++t] = code[p].a;
			
			p++;
			
			break;

		case OC_LIS:
			
			/*
			 * Loads up 'l' number of 'a' integers.
			 */

			m_temp = t;

			while (t < m_temp + code[p].l)
			{
				t++;
				m_stack[t] = code[p].a;
			}				
			
			p++;

			break;

		case OC_SRV:
			
			/*
			 * Set the return value.
			 */
			
			m_stack[b - code[p].a] = m_stack[t];
			
			p++;
			t--;

			break;

		case OC_LRV:
			
			/*
			 * Load the return value.
			 */
			
			t++;
			m_stack[t] = m_stack[b - code[p].l];
			p++;

			break;

		case OC_OPR:

			switch (code[p].a)
			{
				
			case 0:
				
				/*
				 * End of program block.
				 */
					
				t = b - 1; /* stack pointer */

				b = m_stack[t + 2]; /* dynamic link */
				p = m_stack[t + 3]; /* return address */				

				break;						

			case OPR_NOT:

				/*
				 * Bitwise NOT and pop.
				 */
				
				m_stack[t] = ~m_stack[t];
				p++;

				break;

			case OPR_ADD:

				/*
				 * Add and pop.
				 */
				
				t--;

				m_stack[t] = m_stack[t] + m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_SUB:
				
				/*
				 * Subtract and pop.
				 */

				t--;
				
				m_stack[t] = m_stack[t] - m_stack[t + 1];

				p++;
				
				break;

			case OPR_MUL:
				
				/*
				 * Multiply and pop.
				 */

 				t--;
				
				m_stack[t] = m_stack[t] * m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_DIV:
				
				/*
				 * Divide and pop.
				 */

				t--;

				if (m_stack[t + 1] == 0)
				{
					INTERPRET_ERROR("Divide by zero error.\n");
				}

				m_stack[t] = m_stack[t] / m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_ODD:
				
				/*
				 * Odd.
				 */
				
				m_stack[t] = (m_stack[t] & 1);
				
				p++;

				break; 

			case OPR_EQL:
				
				/*
				 * Equals and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] == m_stack[t + 1]);
				
				p++;
				
				break;

			case OPR_NEQ:
				
				/*
				 * Not equls and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] != m_stack[t + 1]);
				
				p++;
				
				break;

			case OPR_LES:

				/*
				 * Less than and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] < m_stack[t + 1]);
				
				p++;
				
				break;

			case OPR_LEQ:
				
				/*
				 * Less than and equal to and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] <= m_stack[t + 1]);
				
				p++;
				
				break;

			case OPR_GRE:

				/*
				 * Greater than and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] > m_stack[t + 1]);
				
				p++;
				
				break;

			case OPR_GRQ:
				
				/*
				 * Greater than and equal to and pop.
				 */

				t--;
				
				m_stack[t] = (m_stack[t] >= m_stack[t + 1]);
				
				p++;
				
				break;
				
			case OPR_SHL:

				/*
				 * Shift left and pop.
				 */

				t--;
				
				m_stack[t] = m_stack[t] << m_stack[t + 1];
				
				p++;				
				
				break;

			case OPR_SHR:
				
				/*
				 * Shift right and pop.
				 */

				t--;
				
				m_stack[t] = m_stack[t] >> m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_BOR:
				
				/*
				 * Bitwise OR and pop.
				 */

				t--;
				
				m_stack[t] = m_stack[t] | m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_AND:

				/*
				 * Bitwise AND and pop.
				 */

				t--;

				m_stack[t] = m_stack[t] & m_stack[t + 1];

				p++;

				break;

			case OPR_XOR:
				
				t--;

				m_stack[t] = m_stack[t] ^ m_stack[t + 1];

				p++;

				break;

			case OPR_MOD:
				
				/*
				 * Integer MOD and pop.
				 */

				t--;
				
				m_stack[t] = m_stack[t] % m_stack[t + 1];
				
				p++;
				
				break;

			case OPR_NEG:

				/*
				 * Integer negative.
				 */

				m_stack[t] = -m_stack[t];

				p++;

				break;
			}
	
			break;

		case OC_LOD:

			/*
			 * Loads some stack or heap memory onto the stack.
			 */
			
			t++;
			
			/*
			 * If the REG_DX register is flagged.
			 */

			m_stack[t] = m_stack[base(code[p].l, b) + code[p].a];
			
			p++;
			
			break;

		case OC_STO:

			/*
			 * Store the stack onto some stack or heap memory.
			 */

			/*
			 * If the REG_DX register is flagged.
			 */

			m_stack[base(code[p]. l, b) + code[p].a] = m_stack[t];
			
			p++;
			t--;

			break;

		case OC_LID:
			
			/*
			 * Load indirectly.
			 */
			
			t++;

			m_stack[t] = m_stack[m_stack[base(code[p].l, b) + code[p].a]];
			
			p++;

			break;

		
		case OC_SID:

			/*
			 * Store indirectly.
			 */

			m_stack[m_stack[base(code[p].l, b) + code[p].a]] = m_stack[t];

			p++;
			t--;

			break;

		case OC_TAC:

			/*
			 * Tail call.
			 */						
			
			j = t;
			
			t = b - 1;
			
			/* Static link stays the same */
			/* Dynamic link stays the same */
			/* Return address stays the same */
			
			b = t + 1; 

			memcpy(&m_stack[t + 4], &m_stack[j + 4], param_count * sizeof(int));

			p = code[p].a;

			param_count = 0;
			
			break;

		case OC_CAL:
			
			/*
			 * Call directly.
			 */

			/*
			 * Work out static link.
			 */
			address = base(code[p].l, b);
			
			if (address == 0)
			{
				INTERPRET_ERROR("Null function pointer exception.\n");
			}
			else
			{
				m_stack[t + 1] = address; /* static link */
				m_stack[t + 2] = b; /* dynamic link */
				m_stack[t + 3] = p + 1; /* return address */

				b = t + 1; /* base address for this data segment */

				p = code[p].a;
			}
			
			param_count = 0;

			break;

		case OC_INC:
			
			/*
			 * Increments the m_stack.
			 */
			
			t += code[p].a;
			p++;

			break;

		case OC_DEC:
			
			/*				 
			 * Decrements (pops) the m_stack.
			 */

			t -= code[p].a;
			p++;

			break;			

		case OC_JMP:

			/*
			 * Jump.
			 */

			p = code[p].a;
			
			break;

		case OC_JPC:
			
			/*
			 * Conditional jump.
			 */

			if (m_stack[t] == 0)
			{
				p = code[p].a;
			}
			else
			{
				p++;
			}

			t--;
			
			break;

		case OC_STP:

			/*
			 * Store parameter.
			 */

			param_count++;

			m_stack[t + code[p].a] = m_stack[t];

			t--;
			p++;
			
			break;

		case OC_LDA:
			
			/*
			 * Load address.
			 */

			t++;
			m_stack[t] = base(code[p].l, b) + code[p].a;
			p++;
			
			break;

		case OC_CAI:

			param_count = 0;

			/*
			 * Call indirectly.
			 */
			
			address = base(code[p].l, b);

			if (m_stack[address + code[p].a] == 0)
			{
				INTERPRET_ERROR("Null function pointer exception.\n");
			}
			else
			{
				m_stack[t + 1] = base(code[p].l, b);
				m_stack[t + 2] = b;
				m_stack[t + 3] = p + 1;

				p = m_stack[address + code[p].a];;
				
				b = t + 1;
			}

			break;		

		case OC_MLO:

			t++;

			address = m_stack[base(code[p]. l, b) + code[p].a];

			CHECK_MEMORY(address);

			i_p1 = (int*)m_memory_handles[address];

			i_p1 += m_registers[REG_CX];
			
			m_stack[t] = *i_p1;

			p++;

			break;

		case OC_MLI:

			t++;

			address = m_stack[m_stack[base(code[p].l, b) + code[p].a]];

			CHECK_MEMORY(address);

			i_p1 = (int*)m_memory_handles[address];

			i_p1 += m_registers[REG_CX];

			m_stack[t] = *i_p1;

			p++;

			break;

		case OC_MST:

			address = base(code[p]. l, b) + code[p].a;

			CHECK_MEMORY(m_stack[base(code[p]. l, b) + code[p].a]);

			i_p1 = (int*)m_memory_handles[m_stack[address]];

			i_p1 += m_registers[REG_CX];

			*i_p1 = m_stack[t--];

			p++;
			
			break;

		case OC_MSI:

			address = m_stack[base(code[p]. l, b) + code[p].a];

			CHECK_MEMORY(m_stack[base(code[p]. l, b) + code[p].a]);

			i_p1 = (int*)m_memory_handles[m_stack[address]];

			i_p1 += m_registers[REG_CX];

			*i_p1 = m_stack[t--];

			p++;

			break;

		case OC_CAS:

			param_count = 0;

			temp = m_stack[t];

			t--;

			if (temp == 0)
			{
				INTERPRET_ERROR("Null function pointer exception.\n");
			}
			else
			{
				m_stack[t + 1] = base(code[p].l, b);
				m_stack[t + 2] = b;
				m_stack[t + 3] = p + 1;

				p = temp;
				
				b = t + 1;
			}

			break;

		case OC_CII:

			param_count = 0;

			/*
			 * Call two level indirectly.
			 */
			
			address = base(code[p].l, b);
			
			if (m_stack[m_stack[address + code[p].a]] == 0)
			{
				INTERPRET_ERROR("Null function pointer exception.\n");
			}
			else
			{
				m_stack[t + 1] = address;
				m_stack[t + 2] = b;
				m_stack[t + 3] = p + 1;

				p = m_stack[m_stack[address + code[p].a]];
				
				b = t + 1;
			}

			break;

		
		case OC_IAD:

			/*
			 * Inline adding.
			 */
			
			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			/*
			 * If the REG_DX register is flagged then this is a floating operation.
			 */

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_ADD, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				*i_p1 += *i_p2;
			}

			p++;
			t--;

			break;

		case OC_ISU:

			/*
			 * Inline substracting.
			 */
			
			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			/*
			 * If the REG_DX register is flagged then this is a floating operation.
			 */			

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_SUB, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{
				*i_p1 -= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_ISL:

			/*
			 * Inline shift left.
			 */

			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 <<= *i_p2;

			p++;
			t--;

			break;

		case OC_ISR:

			/*
			 * Inline shift right.
			 */

			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 >>= *i_p2;

			p++;
			t--;

			break;

		case OC_IZL:

			/*
			 * Inline indirect shift right.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 >>= *i_p2;

			p++;
			t--;

			break;

		case OC_IZR:

			/*
			 * Inline indirect shift left.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 <<= *i_p2;

			p++;
			t--;

			break;
		
		case OC_IXO:

			/*
			 * Inline XOR.
			 */
			
			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			/*
			 * If the REG_DX register is flagged then this is a floating operation.
			 */			

			*i_p1 ^= *i_p2;
			
			p++;
			t--;

			break;

		case OC_IOR:

			/*
			 * Inline OR.
			 */
			
			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			/*
			 * If the REG_DX register is flagged then this is a floating operation.
			 */			

			*i_p1 |= *i_p2;
			
			p++;
			t--;

			break;

		case OC_IAN:

			/*	
			 * Inline AND.
			 */
			
			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			/*
			 * If the REG_DX register is flagged then this is a floating operation.
			 */			

			*i_p1 &= *i_p2;
			
			p++;
			t--;

			break;

		case OC_IIX:

			/*
			 * Inline indirect XOR.
			 */
			
			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 ^= *i_p2;
							
			p++;
			t--;

			break;

		case OC_IIO:

			/*
			 * Inline indirect OR.
			 */
			
			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 |= *i_p2;
			
			p++;
			t--;

			break;

		case OC_IIB:

			/*	
			 * Inline indirect and.
			 */
			
			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			*i_p1 &= *i_p2;
			
			p++;
			t--;

			break;

		case OC_IMU:

			/*
			 * Inline multiplying.
			 */

			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_MUL, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				*i_p1 *= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_IDI:

			/*
			 * Inline dividing.
			 */

			if (!inline_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_DIV, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				if (*i_p2 == 0)
				{
					INTERPRET_ERROR("Divide by zero error.\n");
				}

				*i_p1 /= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_IIA:

			/*
			 * Inline indirect adding.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_ADD, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				*i_p1 += *i_p2;
			}

			p++;
			t--;

			break;

		case OC_IIS:

			/*
			 * Inline indirect substracting.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_SUB, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				*i_p1 -= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_IIM:

			/*
			 * Inline indirect multiplying.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_MUL, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{				
				*i_p1 *= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_IID:

			/*
			 * Inline indirect dividing.
			 */

			if (!inline_indirect_operands(code, &i_p1, & i_p2))
			{
				return 0;
			}

			if (m_registers[REG_DX] & REG_FLOAT_FLAG)
			{
				fop(FOP_DIV, (float*)i_p1, (float*)i_p2, NULL);
			}
			else
			{
				if (*i_p2 == 0)
				{
					INTERPRET_ERROR("Divide by zero error.\n");
				}

				*i_p1 /= *i_p2;
			}

			p++;
			t--;

			break;

		case OC_WRB:
		case OC_WCH:

			/*
			 * Write out a byte.
			 */

			if (m_stack[t - 1] == 0)
			{					
				putc(m_stack[t] & 0xff, stdout);
			}
			else
			{
				putc(m_stack[t], m_files[m_stack[t - 1]]);				
			}
			
			t -= 2;
			p++;

			break;
		
		case OC_REB:

			if (m_stack[t] == 0)
			{
				m_stack[t] = GETCH;
			}
			else
			{				
				m_stack[t] = getc(m_files[m_stack[t]]);
			}

			p++;

			break;

		case OC_RCH:
			
			if (m_stack[t] == 0)
			{
				m_stack[t] = getchar();
			}
			else
			{
				m_stack[t] = getc(m_files[m_stack[t]]);				
			}

			p++;

			break;

		case OC_MAL:

			m_size = (m_stack[t] + 1) * sizeof(int);

			i_p1 = calloc(m_size, 1);
			*i_p1 = m_stack[t];

			temp = find_free_memory_handle();
			m_memory_handles[temp] = (i_p1 + 1);
			
			m_stack[t] = temp;

			ASSERT(0, "\t*ASSERT* Allocating memory handle (%x)\n", temp);
			
			p++;

			break;

		case OC_RAL:
						
			address = m_stack[t - 1];

			CHECK_MEMORY(address);

			i_p1 = m_memory_handles[address];

			i_p1--;
			
			temp = (*i_p1 + 1) * sizeof(int);

			m_size = (m_stack[t] + 1) * sizeof(int);

			ASSERT1(0, "\t*ASSERT* Reallocating memory handle (%x) to %d integers\n", address, m_stack[t]);

			i_p2 = realloc(i_p1, m_size);
							
			*i_p2 = m_stack[t];
							
			if (m_size > temp)
			{	
				memset(((char*)i_p2) + temp, 0, m_size - temp);
			}
			
			m_memory_handles[address] = i_p2 + 1;

			t -= 2;
			p++;

			break;

		case OC_FRE:

			/*
			 * Frees some memory off the heap (takes a handle to the memory).
			 */

			address = m_stack[t];

			CHECK_MEMORY(address);

			i_p1 = (int*)m_memory_handles[address];

			i_p1--;

			ASSERT(0, "\t*ASSERT* Freeing memory handle (%x)\n", address);
			
			free(i_p1);

			m_memory_handles[address] = 0;
						
			p++;

			break;

		case OC_MSZ:
			
			address = m_stack[t];

			CHECK_MEMORY(address);

			i_p1 = m_memory_handles[address];

			i_p1--;

			m_stack[t] = (*i_p1);

			p++;

			break;
			
		case OC_MLS:
			
			/*
			 * Loads some memory onto the stack, the handle is given
			 * by the stack.
			 */
			
			t++;
			
			/*
			 * If the REG_DX register is flagged.
			 */

			temp = m_stack[t - 1 - code[p].a];
	
			/*
			 * Then this is a memory operation, the REG_CX register
			 * holds the offset.  The address and level provided is
			 * for the handle to the memory.
			 */					

			CHECK_MEMORY(temp);

			i_p1 = (int*)m_memory_handles[temp];

			i_p1 += m_registers[REG_CX];
			
			m_stack[t] = *i_p1;
						
			p++;
			
			break;
				
		case OC_MSS:

			/*
			 * Store the stack onto some heap memory.
			 */

			/*
			 * If the REG_DX register is flagged.
			 */

			CHECK_MEMORY(m_stack[t - code[p].a]);

			i_p1 = (int*)m_memory_handles[m_stack[t - code[p].a]];

			i_p1 += m_registers[REG_CX];

			*i_p1 = m_stack[t];

			p++;
			t--;

			break;

		case OC_MCP:

			/*
			 * Copy some memory garunteeing overlapped copies.
			 * [dest][dest offset][src][src offset][size]
			 */

			CHECK_MEMORY(m_stack[t - 4]);
			CHECK_MEMORY(m_stack[t - 2]);

			i_p1 = (int*)m_memory_handles[m_stack[t - 4]] + m_stack[t - 3];
			i_p2 = (int*)m_memory_handles[m_stack[t - 2]] + m_stack[t - 1];

			if (m_stack[t - 4] == m_stack[t - 2])
			{
				memmove(i_p1, i_p2, m_stack[t] * sizeof(int));
			}
			else
			{
				memcpy(i_p1, i_p2, m_stack[t] * sizeof(int));
			}

			t -= 5;
			p++;

			break;

		case OC_CLI:
				
			len = strlen(m_options.command_line);

			m_size = (len + m_registers[REG_CX] + 1) * sizeof(int);

			i_p1 = calloc(m_size, 1);
			*i_p1 = len + m_registers[REG_CX];
			i_p1++;
		
			temp = find_free_memory_handle();
			m_memory_handles[temp] = (i_p1);
			m_stack[++t] = temp;
			m_stack[++t] = len;

			ASSERT(0, "*ASSERT* Allocating memory handle for command line args (%x)\n", temp);
			
			i_p1 += m_registers[REG_CX];

			for (j = 0; j < len; j++)
			{
				i_p1[j] = m_options.command_line[j];
			}
			
			p++;

			break;
							
		case OC_FOP:

			/*
			 * If this is an operation that works on two values off the m_stack.
			 */

			if (code[p].a != FOP_INT && code[p].a != FOP_FLO && code[p].a != FOP_NEG)
			{
				/*
				 * Decrease the m_stack position.
				 */
				t--;
			}

			if (!fop(code[p].a, (float*)&m_stack[t], (float*)&m_stack[t + 1], (float*)&m_stack[t]))
			{
				return 0;
			}

			p++;
		
			break;
		
		case OC_WRT:

			/*
			 * Write a number.
			 */

			if (m_stack[t - 1] == 0)
			{
				printf("%d",m_stack[t]);
			}
			else
			{
				fprintf(m_files[m_stack[t - 1]], "%d", m_stack[t]);
			}
			
			t -= 2;
			p++;

			break;

		case OC_WRF:

			/*
			 * Write a number.
			 */

			f_p1 = (float*)&m_stack[t];

			if (m_stack[t - 1] == 0)
			{
				printf("%f", *f_p1);
			}
			else
			{
				fprintf(m_files[m_stack[t - 1]], "%f", *f_p1);
			}
			
			
			t -= 2;
			p++;

			break;

		case OC_TME:

			/*
			 * Pushes the time onto the m_stack.
			 */
			
			m_stack[++t] = (int)time(NULL);
			p++;
			
			break;

		case OC_CLK:

			m_stack[++t] = (int)clock();
			p++;

			break;

		case OC_DEB:
			
			/*
			 * Debugging.
			 */

			printf("[%d]\n", (int)code[p].a);

			p++;

			break;

		case OC_FOF:

			file_open(code);

			p++;

			break;

		case OC_FCF:

			file_close(&code[p]);

			p++;

			break;

		case OC_EOF:

			/*
			 * Top of the stack holds the stream handle.
			 * Overwrite it with whether the stream has ended.
			 */

			if (m_stack[t])
			{
				m_stack[t] = feof(m_files[m_stack[t]]) ? 1 : 0;
			}
			else
			{
				m_stack[t] = 0;
			}

			p++;

			break;

		default:

			INTERPRET_ERROR("Unknown instruction.\n");

			p++;
	}

	return 1;
}

void print_stack_line()
{
	int j;

	for (j = 1; j <= t; j++)
	{
		printf("%d ", m_stack[j]);
	}
}

/*
 * Interpret some code.
 */
int real_interpret(instruction_t* code)
{
	memset(m_registers, 0, sizeof(m_registers));
	memset(m_memory_handles, 0, sizeof(m_memory_handles));
	memset(m_files, 0, sizeof(m_files));

	t = 0;
	b = 1;
	p = 0;

	m_stack[1] = 0;
	m_stack[2] = 0;
	m_stack[3] = 0;

	if (m_options.trace_stack)
	{
		do
		{
			/*
			 * Conservative stack overflow error checking.
			 */

			if (t >= STACK_SIZE - 2)
			{
				INTERPRET_ERROR("\n\nStack overflow error.\n");
			}

			if (!single_interpret(code))
			{
				break;
			}

			print_stack_line();

			printf("\n");
		}
		while (p!=0);
	}
	else
	{
		do
		{
			/*
			 * Conservative stack overflow error checking.
			 */
			if (t >= STACK_SIZE - 2)
			{
				INTERPRET_ERROR("\n\nStack overflow error.\n");
			}

			if (!single_interpret(code))
			{
				break;
			}			
		}
		while (p!=0);
	}

	if(!all_handles_free())
	{
		printf("Possible memory leaks.");
	}
	
	return 1;
}

/*
 * Do floating point operation.
 */
int fop(int op, float *f1_p, float *f2_p, float *result_p)
{
	int *i1_p;

	switch (op)
	{
		case FOP_ADD:

			*f1_p = *f1_p + *f2_p;

			break;

		case FOP_SUB:

			*f1_p = *f1_p - *f2_p;

			break;

		case FOP_MUL:
			
			*f1_p = *f1_p * *f2_p;

			break;

		case FOP_DIV:

			if (*f2_p == 0)
			{
				INTERPRET_ERROR("Floating point divide by zero error.\n");
			}

			*f1_p = *f1_p / *f2_p;

			break;

		case FOP_EQL:
			
			*result_p = (float)(*f1_p == *f2_p);

			break;

		case FOP_NEQ:

			*result_p = (float)(*f1_p != *f2_p);

			break;

		case FOP_LES:

			*result_p = (float)(*f1_p < *f2_p);
			
			break;

		case FOP_LEQ:

			*result_p = (float)(*f1_p <= *f2_p);

			break;

		case FOP_GRE:

			*result_p = (float)(*f1_p > *f2_p);

			break;

		case FOP_GRQ:

			*result_p = (float)(*f1_p >= *f2_p);

			break;

		case FOP_INT:

			i1_p = ((int*)f1_p);

			*i1_p = ((int)*f1_p);

			break;

		case FOP_FLO:

			i1_p = ((int*)f1_p);

			*f1_p = (float)*i1_p;
			
			break;

		case FOP_NEG:

			*f1_p = -*f1_p;
	}	

	return 1;
}

void print_out_code(instruction_t* instructions, int len)
{
	int i;

	for (i = 0; i < len; i++)
	{
		printf("%3x: ", i);

		switch (instructions[i].f)
		{
		case OC_NOP:

			printf("NOP  ");

			break;

		case OC_LIT:
			
			printf("LIT  ");

			break;

		case OC_OPR:
			
			printf("OPR  ");
			
			break;

		case OC_LOD:
			
			printf("LOD  ");
			
			break;

		case OC_STO:
			
			printf("STO  ");
			
			break;

		case OC_CAL:
			
			printf("CAL  ");
			
			break;

		case OC_TAC:
			
			printf("TAC  ");
			
			break;

		case OC_INC:
			
			printf("INC  ");
			
			break;

		case OC_DEC:
			
			printf("DEC  ");
			
			break;
			
		case OC_JMP:
			
			printf("JMP  ");
			
			break;

		case OC_JPC:
			
			printf("JPC  ");

			break;

		case OC_WRT:
			
			printf("WRT  ");
			
			break;

		case OC_STP:
			
			printf("STP  ");
			
			break;

		case OC_LID:
			
			printf("LID  ");
			
			break;

		case OC_LDA:
			
			printf("LDA  ");
			
			break;

		case OC_SID:
			
			printf("SID  ");
			
			break;
			
		case OC_SRV:
			printf("SRV  ");
			
			break;

		case OC_LRV:
			
			printf("LRV  ");
			
			break;

		case OC_IAD:
			
			printf("IAD  ");
			
			break;

		case OC_ISU:
			
			printf("ISU  ");
			
			break;

		case OC_IMU:
			
			printf("IMU  ");
			
			break;

		case OC_IDI:
			
			printf("IDI  ");
			
			break;

		case OC_ISL:
			
			printf("ISL  ");
			
			break;

		case OC_ISR:
			
			printf("ISR  ");
			
			break;

		case OC_WRB:
			
			printf("PRB  ");
			
			break;

		case OC_POP:
			
			printf("POP  ");
			
			break;

		case OC_PUS:
			
			printf("PUS  ");
			
			break;

		case OC_PAS:
			
			printf("PAS  ");
			
			break;

		case OC_TME:
			
			printf("TME  ");
			
			break;

		case OC_CAI:
			
			printf("CAI  ");
			
			break;

		case OC_FOP:
			
			printf("FOP  ");
			
			break;

		case OC_SRG:
			
			printf("SRG  ");
			
			break;

		case OC_MAL:
			
			printf("MAL  ");
			
			break;

		case OC_RAL:
			
			printf("RAL  ");
			
			break;

		case OC_FRE:
			
			printf("FRE  ");
			
			break;

		case OC_SWS:
			
			printf("SWS  ");
			
			break;

		case OC_DEB:

			printf("DEB  ");

			break;

		case OC_MOV:

			printf("MOV  ");

			break;

		case OC_RIO:

			printf("RIO  ");

			break;
		
		case OC_SLD:

			printf("SLD  ");

			break;

		case OC_MLS:

			printf("MLS  ");

			break;

		case OC_WRF:

			printf("WRF  ");

			break;

		case OC_REB:

			printf("REB  ");

			break;

		case OC_MSZ:

			printf("MSZ  ");

			break;

		case OC_IIA:
			
			printf("IIA  ");

			break;

		case OC_IIS:

			printf("IIS  ");

			break;

		case OC_IIM:

			printf("IIM  ");

			break;

		case OC_IID:

			printf("IID  ");

			break;

		case OC_IXO:

			printf("IXO  ");

			break;

		case OC_IOR:

			printf("IOR  ");

			break;

		case OC_IAN:

			printf("IAN  ");

			break;

		case OC_IIX:

			printf("IIX  ");

			break;

		case OC_IIO:

			printf("IIO  ");

			break;

		case OC_IIB:

			printf("IIB  ");

			break;

		case OC_MSS:

			printf("JSS  ");

			break;

		case OC_MLO:

			printf("MLD  ");

			break;

		case OC_MLI:

			printf("MLI  ");

			break;

		case OC_MST:

			printf("MST  ");

			break;

		case OC_MSI:

			printf("MSI  ");

			break;

		case OC_TRM:

			printf("TRM  ");

			break;

		case OC_LIS:

			printf("LIS  ");

			break;

		default:

			printf("%d ", instructions[i].l);

			break;
		}

		printf("0x%x\t", instructions[i].l);
		printf("0x%x\n", (int)instructions[i].a);
	}
}
