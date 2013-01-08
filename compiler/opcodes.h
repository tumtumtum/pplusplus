/*
 * @file opcodes.h
 *
 * @description
 *
 * P++ machine opcode definitions.
 */

/*
 * Basic P++ machine opcodes.
 */

#define OC_NOP				0x0000
#define OC_LIT				0x1000
#define OC_OPR				0x1010
#define OC_FOP				0x1015
#define OC_LOD				0x1020
#define OC_STO				0x1030
#define OC_CAL				0x1040
#define OC_TAC				0x1045
#define OC_INC				0x1050
#define OC_JMP				0x1060
#define OC_JPC				0x1070
#define OC_STP				0x1090
#define OC_LID				0x10A0
#define OC_LDA				0x10B0
#define OC_SID				0x10C0
#define OC_DEC				0x10D0
#define OC_CAI				0x10E0
#define OC_CII				0x10F0
#define OC_CAS				0x1100
#define OC_SLD				0x1110
#define OC_SWS				0x1120
#define OC_LIS				0x1130
#define OC_TRM				0x1FFF
 
 /*
  * Inline operation opcodes.
  */
#define OC_ISL				0x2000
#define OC_ISR				0x2010
#define OC_IAD				0x2040
#define OC_ISU				0x2050
#define OC_IMU				0x2060
#define OC_IDI				0x2070
#define OC_IXO				0x2080
#define OC_IOR				0x2090
#define OC_IAN				0x20A0


/*
 * Inline indirect operaton opcodes.
 */
#define OC_IZL				0x3000
#define OC_IZR				0x3010
#define OC_IIA				0x3020
#define OC_IIS				0x3030
#define OC_IIM				0x3040
#define OC_IID				0x3050
#define OC_IIX				0x3060
#define OC_IIO				0x3070
#define OC_IIB				0x3080

/*
 * Function sepcific opcodes.
 */
#define OC_SRV				0x5000
#define OC_LRV				0x5010


/*
 * Memory opcodes.
 */
#define OC_MAL				0xA000
#define OC_FRE				0xA020
#define OC_RAL				0xA030
#define OC_MSZ				0xA040
#define OC_MLS				0xA100
#define OC_MSS				0xA110
#define OC_MLO				0xA120
#define OC_MST				0xA130
#define OC_MLI				0xA140
#define OC_MSI				0xA150
#define OC_MCP				0xA160

/*
 * Register opcodes
 */
#define OC_POP				0xB000
#define OC_PUS				0xB010
#define OC_MOV				0xB020
#define OC_RIO				0xB030
#define OC_PAS				0xB040
#define OC_SRG				0xB050

/*
 * Utility opcodes.
 */
#define OC_TME				0xC000
#define OC_WRT				0xC010
#define OC_WRF				0xC020
#define OC_WRB				0xC030
#define OC_DEB				0xC040
#define OC_CLK				0xC050
#define OC_CLI				0xC060
#define OC_REB				0xC070
#define OC_RCH				0xC080
#define OC_WCH				0xC090

/*
 * File/Stream opcodes.
 */
#define OC_FOF				0xD000
#define OC_FCF				0xD010
#define OC_EOF				0xD020

/*
 * OC_OPR integer operators.
 */
#define OPR_ADD				0x1010
#define OPR_SUB				0x1020
#define OPR_MUL				0x1030
#define OPR_DIV				0x1040
#define OPR_MOD				0x1050
#define OPR_EQL				0x1060
#define OPR_NEQ				0x1070
#define OPR_LES				0x1080
#define OPR_LEQ				0x1090
#define OPR_GRE				0x1100
#define OPR_GRQ				0x1110
#define OPR_SHL				0x1120
#define OPR_SHR				0x1130
#define OPR_BOR				0x1140
#define OPR_AND				0x1150
#define OPR_XOR				0x1160

/*
 * Unary operators.
 */
#define OPR_ODD				0x1170
#define OPR_NEG				0x1180
#define OPR_NOT				0x1190

/*
 * OC_FOP floating point operators.
 */
#define FOP_ADD				0x1000
#define FOP_SUB				0x1010
#define FOP_MUL				0x1020
#define FOP_DIV				0x1030
#define FOP_EQL				0x1040
#define FOP_NEQ				0x1050
#define FOP_LES				0x1060
#define FOP_LEQ				0x1070
#define FOP_GRE				0x1080
#define FOP_GRQ				0x1090

/*
 * Unary operators.
 */
#define FOP_INT				0x10A0
#define FOP_FLO				0x10B0
#define FOP_NEG				0x10C0

/*
 * File flags
 */
#define F_READ				0x1
#define F_WRITE				0x2
#define F_APPEND			0x4
#define F_CREATE			0x80
#define F_TEXT				0x100
#define F_BINARY			0x200
