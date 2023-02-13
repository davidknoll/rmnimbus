#define IO_PCS_BASE	0x400
#define IO_PCS0		(IO_PCS_BASE + (0 * 0x80))
#define IO_PCS1		(IO_PCS_BASE + (1 * 0x80))
#define IO_PCS2		(IO_PCS_BASE + (2 * 0x80))
#define IO_PCS3		(IO_PCS_BASE + (3 * 0x80))
#define IO_PCS4		(IO_PCS_BASE + (4 * 0x80))

#define IO_DCB		IO_PCS0
#define IO_FDC_CONTROL	(IO_DCB + 0x00)
#define IO_FDC_STATUS	(IO_DCB + 0x08)
#define IO_FDC_COMMAND	(IO_DCB + 0x08)
#define IO_FDC_TRACK	(IO_DCB + 0x0A)
#define IO_FDC_SECTOR	(IO_DCB + 0x0C)
#define IO_FDC_DATA	(IO_DCB + 0x0E)
#define IO_SASI_STATUS	(IO_DCB + 0x10)
#define IO_SASI_CONTROL	(IO_DCB + 0x10)
#define IO_SASI_COMMAND	(IO_DCB + 0x18)
#define IO_SASI_DATA	(IO_DCB + 0x18)

#define IO_TJ		IO_PCS4
#define IO_TJ_INT_ROM	(IO_TJ + 0x00)
#define IO_TJ_RESET	(IO_TJ + 0x00)
#define IO_TJ_AN	(IO_TJ + 0x08)
#define IO_TJ_PORT_A	(IO_TJ + 0x10)
#define IO_TJ_PORT_B	(IO_TJ + 0x12)
#define IO_TJ_PORT_C	(IO_TJ + 0x14)
#define IO_TJ_CONTROL	(IO_TJ + 0x16)
#define IO_TJ_STATUS	(IO_TJ + 0x18)
