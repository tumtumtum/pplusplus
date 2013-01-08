#include "errors.h"
#include "opcodes.h"
#include "registers.h"

/*
 * Basic symbols.
 */
#define SYM_NOSYM			0x0
#define SYM_EOFSYM			0xffffffff
#define SYM_NULL			0x100
#define SYM_IDENT			0x110
#define SYM_LITERAL_NUMBER	0x120
#define SYM_LITERAL_BIGNUMBER	0x125
#define SYM_LITERAL_STRING	0x130
#define SYM_FALSE			0x500
#define SYM_TRUE			0x510

/*
 * Comparison symbols.
 */
#define SYM_CMP_START			0x1000
#define SYM_EQL				0x1010
#define SYM_NOT_EQL			0x1020
#define SYM_LESS			0x1030
#define SYM_GREATER			0x1040
#define SYM_LES_EQL			0x1050
#define SYM_GRE_EQL			0x1060
#define SYM_CMP_END  			0x1070

/*
 * Basic opertional symbols.
 */
#define SYM_BMOP_START			0x2000
#define SYM_PLUS			0x2010
#define SYM_MINUS			0x2020
#define SYM_STAR			0x2030
#define SYM_SLASH			0x2040
#define SYM_BMOP_END			0x2250

/*
 * Array operational symbols.
 */
#define SYM_AOP_START			0x3000
#define SYM_STR_EQUALS			0x3010
#define SYM_STR_NEQUALS			0x3020
#define SYM_AOP_END			0x3200

/*
 * Unary opertional symbols.
 */
#define SYM_UNARY_START		0x4000
#define SYM_PLUS_PLUS		0x4010
#define SYM_MINUS_MINUS		0x4020
#define SYM_PLUS_EQL		0x4030
#define SYM_MINUS_EQL		0x4050
#define SYM_STAR_EQL		0x4060
#define SYM_SLASH_EQL		0x4070
#define SYM_LCHEVRON_EQL	0x4080
#define SYM_RCHEVRON_EQL	0x4090
#define SYM_HAT_EQL		0x40A0
#define SYM_PIPE_EQL		0x40B0
#define SYM_AMPERSAND_EQL	0x40C0
#define SYM_UNARY_END		0x40D0

/*
 * Bitwise operational symbols.
 */
#define SYM_BITWISE_START	0x5000
#define SYM_TILD		0x5010
#define SYM_HAT			0x5020
#define SYM_PIPE		0x5030
#define SYM_AMPERSAND		0x5040
#define SYM_BITWISE_END		0x5050

/*
 * Logical operational symbols.
 */
#define SYM_LOGICAL_START	0x6000
#define SYM_LOR			0x6010
#define SYM_LAND		0x6020
#define SYM_LOGICAL_END		0x6030

/*
 * Misc prefix unary opertional symbols.
 */
#define SYM_AT				0x7000
#define SYM_EXCLAMATION		0x7010
#define SYM_ABS				0x7020

/*
 * Fundamental symbols.
 */
#define SYM_LPAREN		0x8000
#define SYM_RPAREN		0x8010
#define SYM_COMMAND		0x8020
#define SYM_PERIOD		0x8030
#define SYM_SEMICOLON		0x8040
#define SYM_COMMA		0x8050
#define SYM_BECOMES		0x8060
#define SYM_BEGIN		0x8070
#define SYM_CALL		0x8080
#define SYM_CONST		0x8090
#define SYM_DO			0x80A0
#define SYM_END			0x80B0
#define SYM_IF			0x80C0
#define SYM_THEN		0x80D0
#define SYM_ODD			0x80E0
#define SYM_PROCEDURE		0x80F0
#define SYM_VAR			0x8100
#define SYM_WHILE		0x8110
#define SYM_COLON		0x8120
#define SYM_ELSE		0x8230
#define SYM_LINECOMMENT		0x8240
#define SYM_LBLOCKCOMMENT	0x8250
#define SYM_RBLOCKCOMMENT	0x8260
#define SYM_RCHEVRON		0x8270
#define SYM_LCHEVRON		0x8280
#define SYM_FOR			0x8290
#define SYM_TO			0x82A0
#define SYM_STEP		0x82B0
#define SYM_WHICH		0x82C0
#define SYM_CASE		0x82D0
#define SYM_EXIT		0x82E0
#define SYM_FUNCTION		0x82F0
#define SYM_RETURN		0x8300
#define SYM_GOTO		0x8310
#define SYM_INCLUDE		0x8320
#define SYM_LAMBDA		0x8330
#define SYM_EQMORE		0x8340
#define SYM_LESSMINUS		0x8350
#define SYM_REPEAT		0x8360
#define SYM_REF			0x8370
#define SYM_VAL			0x8380
#define SYM_PERCENT		0x8390
#define SYM_APOS		0x83A0
#define SYM_ASM			0x83B0
#define SYM_QMARK		0x83C0
#define SYM_LBRACKET		0x83D0
#define SYM_RBRACKET		0x83E0
#define SYM_DOTDOT		0x8400
#define SYM_CONTINUE		0x8420
#define SYM_DECLARE			0x8430
#define SYM_TYPE		0x8440
#define SYM_CLEAN	0x8450

/*
 * Type symbols.
 */
#define SYM_TYPESTART		0xA000
#define SYM_VOID		0xA010
#define SYM_INTEGER		0xA020
#define SYM_FLOAT		0xA030
#define SYM_HANDLE		0xA040
#define SYM_STRING		0xA050
#define SYM_BIGNUMBER	0xA055
#define SYM_ARRAY		0xA060
#define SYM_CHARACTER	0xA080
#define SYM_BOOLEAN	0xA090
#define SYM_TYPEEND		0xAFFF

#define SYM_PRAGMA		0xB000
#define SYM_BOUNDARY_CHECK	0xB010
#define	SYM_OPTIMIZE	0xB020