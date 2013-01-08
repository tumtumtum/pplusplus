#include "types.h"
#include "time.h"
#include "string.h"
#include "compiler.h"
#include "hashtable.h"
#include "interpretor.h"

#define VERSION_MAJOR		1
#define VERSION_MINOR		0
#define VERSION_REVISION	0

static char m_file_name[MAX_PATH];
static compiler_results_t m_compiler_results;

void print_help()
{
	printf("P++ improved PL/0 compiler version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
	printf("By Thong Nguyen (tng12) for COSC 230 - University of Canterbury\n\n");
	printf("Usage: ");
	printf("p++ <options> filename <command line arguments>\n\n");
	printf("Options:\n\n");
	printf("(-/+)v\t Verbose output\n");
	printf("(-/+)o\t Write output binary (.pins)\n");
	printf("(-/+)Z1\t Optimize\n");
	printf("(-/+)i\t Interpret after compiling\n");
	printf("(-/+)lc\t List opcodes after compiling\n");
	printf("(-/+)ts\t Trace the stack\n");
	printf("(-/+)ls\t List source code while compiling\n");	
	printf("(-/+)se\t Suppress all errors\n");	
	printf("(-/+)q\t Quiet compiling\n");
	printf("(-/+)p\t Pause after each step\n");	
	printf("(-/+)h\t Help\n");	
	printf("\n\nExample:\n\n");
	printf("p++ +v +i -lc -ls quicksort.p++\n");
	printf("\nWill compile quicksort.p++ and interpret with verbose mode on and no opcode or sourcecode listing\n\n");

};

int main(int argc, char* argv[])
{
	int i;
	clock_t c;
	
	if (argc <= 1)
	{
		print_help();

		return 0;
	}

	memset(&m_options, 0, sizeof(m_options));

	m_options.verbose = FALSE;
	m_options.interpret = TRUE;
	m_options.pause = FALSE;
	m_options.array_check_boundaries = TRUE;
	m_options.optimize = TRUE;
	m_options.quiet = TRUE;
	m_options.source_path[0] = 0;
	m_options.command_line[0] = 0;
	m_options.write_binary = FALSE;	
	
	for (i = 1; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "+v") == 0)
		{
			m_options.verbose = TRUE;
		}
		else if (strcmp(argv[i], "-v") == 0)
		{
			m_options.verbose = FALSE;
		}
		else if (strcmp(argv[i], "+i") == 0)
		{
			m_options.interpret = TRUE;
		}
		else if (strcmp(argv[i], "-i") == 0)
		{
			m_options.interpret = FALSE;
		}
		else if (strcmp(argv[i], "+o") == 0)
		{
			m_options.write_binary = TRUE;
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			m_options.write_binary = FALSE;
		}
		else if (strcmp(argv[i], "+Z1") == 0)
		{
			m_options.optimize = TRUE;
		}
		else if (strcmp(argv[i], "-Z1") == 0)
		{
			m_options.optimize = FALSE;
		}
		else if (strcmp(argv[i], "+lc") == 0)
		{
			m_options.list_opcodes = TRUE;
		}
		else if (strcmp(argv[i], "-lc") == 0)
		{
			m_options.list_opcodes = FALSE;
		}		
		else if (strcmp(argv[i], "+ts") == 0)
		{
			m_options.trace_stack = TRUE;
		}
		else if (strcmp(argv[i], "-ts") == 0)
		{
			m_options.trace_stack = FALSE;
		}
		else if (strcmp(argv[i], "+ls") == 0)
		{
			m_options.list_source = TRUE;
		}
		else if (strcmp(argv[i], "-ls") == 0)
		{
			m_options.list_source = FALSE;
		}
		else if (strcmp(argv[i], "+se") == 0)
		{
			m_options.supress_errors = TRUE;
		}
		else if (strcmp(argv[i], "-se") == 0)
		{
			m_options.supress_errors = FALSE;
		}
		else if (strcmp(argv[i], "+q") == 0)
		{
			m_options.quiet = TRUE;
		}
		else if (strcmp(argv[i], "-q") == 0)
		{
			m_options.quiet = FALSE;
		}
		else if (strcmp(argv[i], "+p") == 0)
		{
			m_options.pause = TRUE;
		}
		else if (strcmp(argv[i], "-p") == 0)
		{
			m_options.pause = FALSE;
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			print_help();
		}
		else if (strcmp(argv[i], "/?") == 0)
		{
			print_help();
		}
		else if (strcmp(argv[i], "-?") == 0)
		{
			print_help();
		}
		else if (strcmp(argv[i], "+sourcepath") == 0)
		{
			if (i < argc - 1)
			{
				strncpy(m_options.source_path, argv[i + 1], sizeof(m_options));

				i++;
			}
		}
		else if (strchr(argv[i], '-') == argv[i] || strchr(argv[i], '+') == argv[i])
		{
			printf("Unknown option [%s]\n", argv[i]);
		}
		else
		{
			break;
		}
	}

	if (i == argc)
	{
		printf("\nNo file specified.\n");

		return 0;
	}

	strcpy(m_file_name, argv[i++]);

	if (i < argc)
	{
		for (; i <= argc - 1; i++)
		{
			strcat(m_options.command_line, argv[i]);
			strcat(m_options.command_line, " ");
		}

		m_options.command_line[strlen(m_options.command_line) - 1] = 0;
	}

	c = clock();

	m_compiler_results = compile(m_file_name);	

	if (!m_compiler_results.instructions && !m_options.quiet)
	{
		printf("There were compilation errors.\n");

		return 0;
	}

	if (m_compiler_results.lines_compiled < 0)
	{
		m_options.interpret = TRUE;
	}

	if (!m_options.quiet && m_options.verbose)
	{
		printf("--------------------------------------------------------------------------------\n");

		if (m_compiler_results.lines_compiled >= 0)
		{
			printf("Compiled %d lines of P++ sourcecode in %.3f real second(s).\n", m_compiler_results.lines_compiled, (float)(clock() - c) / CLOCKS_PER_SEC);
			printf("%d P++ machine opcodes processed.\n\n", m_compiler_results.opcodes_generated);
		}
		else
		{
			/*
			 * Was a pins file.
			 */

			printf("%d P++ machine opcodes processed in %.3f real second(s).\n\n", m_compiler_results.opcodes_generated, (float)(clock() - c) / CLOCKS_PER_SEC);			
		}
		
		printf("--------------------------------------------------------------------------------\n");
	}

	if (m_compiler_results.number_of_errors && !m_options.quiet)
	{
		printf("Warning! %d compilation error(s) were encountered.\n", m_compiler_results.number_of_errors);
	}
	
	if (m_options.pause && m_options.list_opcodes)
	{
		printf("Press any key to list opcodes.\n");
		
		getchar();
	}

	if (m_options.list_opcodes)
	{
		print_out_code(m_compiler_results.instructions, m_compiler_results.opcodes_generated);
		printf("\n");	
	}

	if (m_options.interpret)
	{
		if (m_options.pause)
		{
			if (!m_options.quiet)
			{
				printf("Press any key to start the interpretor.\n");
			}

			getchar();

			if (!m_options.quiet)
			{
				printf("Interpretor started.\n");
			}
		}

		interpret(m_compiler_results.instructions);

		if (!m_options.quiet)
		{
			printf("Interpretor finished.\n");
		}
	}

	if (m_options.pause)
	{
		getchar();
	}

	return 0;
}