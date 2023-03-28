/* I/O port addresses for RM Nimbus PC-186 */

#define F_CPU		8000000UL

/* GA2 - Graphics */
#define IO_GA2_NOP1	0x00
#define IO_GA2_LOADX	0x02
#define IO_GA2_INCY	0x04
#define IO_GA2_LXIY	0x06
#define IO_GA2_NOP2	0x08
#define IO_GA2_INCX	0x0A
#define IO_GA2_LOADY	0x0C
#define IO_GA2_LYIX	0x0E
#define IO_GA2_PLOT1	0x10
#define IO_GA2_LXPLOT	0x12
#define IO_GA2_IYPLOT	0x14
#define IO_GA2_LXIYPL	0x16
#define IO_GA2_PLOT2	0x18
#define IO_GA2_IXPLOT	0x1A
#define IO_GA2_LYPLOT	0x1C
#define IO_GA2_LYIXPL	0x1E
#define IO_GA2_SCROLL	0x20
#define IO_GA2_UPDMD	0x22
#define IO_GA2_INTENS	0x24
#define IO_GA2_DISPMD	0x26
#define IO_GA2_LUT0	0x28
#define IO_GA2_LUT1	0x2A
#define IO_GA2_LUT2	0x2C
#define IO_GA2_LUT3	0x2E
#define IO_GA2_TIMSTAT	0x28
#define IO_GA2_XCSTAT	0x2A
#define IO_GA2_YCSTAT	0x2C

/* GA1 - Memory */
#define IO_GA1_CTL	0x80

/* GA3 - Various I/O */
#define IO_GA3_INTEN	0x92
#define IO_GA3_JSDATA	0xA0
#define IO_GA3_JSSEL0	0xA0
#define IO_GA3_JSSEL1	0xA2
#define IO_GA3_MSDATA	0xA4
#define IO_GA3_MSCLR	0xA4
#define IO_GA3_VOICEN	0xB0
#define IO_GA3_VOICDIS	0xB2
#define IO_GA3_8051DAT	0xC0
#define IO_GA3_8051CMD	0xC2
#define IO_GA3_8051STA	0xC2
#define IO_GA3_ROM0	0xD0
#define IO_GA3_EEROM0	0xD2
#define IO_GA3_ROM1	0xD4
#define IO_GA3_EEROM1	0xD6
#define IO_GA3_AYADDR	0xE0
#define IO_GA3_AYDATR	0xE0
#define IO_GA3_AYDATW	0xE2
#define IO_GA3_AYINAC	0xE2

/* Z80 SIO - printer, keyboard, network */
#define IO_SIO_BASE	0xF0
#define IO_SIO_AD	(IO_SIO_BASE + 0x00)
#define IO_SIO_BD	(IO_SIO_BASE + 0x02)
#define IO_SIO_AC	(IO_SIO_BASE + 0x04)
#define IO_SIO_BC	(IO_SIO_BASE + 0x06)

/* 80186 peripheral chip selects - expansion bus */
#define IO_PCS_BASE	0x400
#define IO_PCS0		(IO_PCS_BASE + (0 * 0x80))
#define IO_PCS1		(IO_PCS_BASE + (1 * 0x80))
#define IO_PCS2		(IO_PCS_BASE + (2 * 0x80))
#define IO_PCS3		(IO_PCS_BASE + (3 * 0x80))
#define IO_PCS4		(IO_PCS_BASE + (4 * 0x80))

/* Disc Controller Board */
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

/* Parallel I/O Port */
#define IO_LPT		IO_PCS1
#define IO_LPT_ORB	(IO_LPT + 0x00)
#define IO_LPT_IRB	(IO_LPT + 0x00)
#define IO_LPT_ORA	(IO_LPT + 0x02)
#define IO_LPT_IRA	(IO_LPT + 0x02)
#define IO_LPT_DDRB	(IO_LPT + 0x04)
#define IO_LPT_DDRA	(IO_LPT + 0x06)
#define IO_LPT_T1CL	(IO_LPT + 0x08)
#define IO_LPT_T1CH	(IO_LPT + 0x0A)
#define IO_LPT_T1LL	(IO_LPT + 0x0C)
#define IO_LPT_T1LH	(IO_LPT + 0x0E)
#define IO_LPT_T2CL	(IO_LPT + 0x10)
#define IO_LPT_T2CH	(IO_LPT + 0x12)
#define IO_LPT_SR	(IO_LPT + 0x14)
#define IO_LPT_ACR	(IO_LPT + 0x16)
#define IO_LPT_PCR	(IO_LPT + 0x18)
#define IO_LPT_IFR	(IO_LPT + 0x1A)
#define IO_LPT_IER	(IO_LPT + 0x1C)
#define IO_LPT_ORAN	(IO_LPT + 0x1E)
#define IO_LPT_IRAN	(IO_LPT + 0x1E)

/* DS12C887A RTC (note data bit rotation) */
#define IO_RTC		IO_PCS2
#define IO_RTC_SEC	(IO_RTC + 0x00)
#define IO_RTC_SECAL	(IO_RTC + 0x02)
#define IO_RTC_MIN	(IO_RTC + 0x04)
#define IO_RTC_MINAL	(IO_RTC + 0x06)
#define IO_RTC_HR	(IO_RTC + 0x08)
#define IO_RTC_HRAL	(IO_RTC + 0x0A)
#define IO_RTC_DAY	(IO_RTC + 0x0C)
#define IO_RTC_DATE	(IO_RTC + 0x0E)
#define IO_RTC_MONTH	(IO_RTC + 0x10)
#define IO_RTC_YEAR	(IO_RTC + 0x12)
#define IO_RTC_CTLA	(IO_RTC + 0x14)
#define IO_RTC_CTLB	(IO_RTC + 0x16)
#define IO_RTC_CTLC	(IO_RTC + 0x18)
#define IO_RTC_CTLD	(IO_RTC + 0x1A)
#define IO_RTC_RAM	(IO_RTC + 0x1C)
#define IO_RTC_CENT	(IO_RTC + 0x64)

/* Data Communications Controller */
#define IO_DCC		IO_PCS3
#define IO_DCC_BC	(IO_DCC + 0x00)
#define IO_DCC_AC	(IO_DCC + 0x02)
#define IO_DCC_BD	(IO_DCC + 0x04)
#define IO_DCC_AD	(IO_DCC + 0x06)
#define IO_DCC_AUX	(IO_DCC + 0x08)

/* Test Jig */
#define IO_TJ		IO_PCS4
#define IO_TJ_INT_ROM	(IO_TJ + 0x00)
#define IO_TJ_RESET	(IO_TJ + 0x00)
#define IO_TJ_AN	(IO_TJ + 0x08)
#define IO_TJ_PORT_A	(IO_TJ + 0x10)
#define IO_TJ_PORT_B	(IO_TJ + 0x12)
#define IO_TJ_PORT_C	(IO_TJ + 0x14)
#define IO_TJ_CONTROL	(IO_TJ + 0x16)
#define IO_TJ_STATUS	(IO_TJ + 0x18)

/* 80186/88 internal peripherals */
#define IO_186_BASE	0xFF00
#define IO_186_IEOI	(IO_186_BASE + 0x22)
#define IO_186_IPOLL	(IO_186_BASE + 0x24)
#define IO_186_IPLST	(IO_186_BASE + 0x26)
#define IO_186_IIMSK	(IO_186_BASE + 0x28)
#define IO_186_IPMSK	(IO_186_BASE + 0x2A)
#define IO_186_IINSV	(IO_186_BASE + 0x2C)
#define IO_186_IIRQ	(IO_186_BASE + 0x2E)
#define IO_186_IIST	(IO_186_BASE + 0x30)
#define IO_186_ITC	(IO_186_BASE + 0x32)
#define IO_186_ID0C	(IO_186_BASE + 0x34)
#define IO_186_ID1C	(IO_186_BASE + 0x36)
#define IO_186_II0C	(IO_186_BASE + 0x38)
#define IO_186_II1C	(IO_186_BASE + 0x3A)
#define IO_186_II2C	(IO_186_BASE + 0x3C)
#define IO_186_II3C	(IO_186_BASE + 0x3E)
#define IO_186_T0CNT	(IO_186_BASE + 0x50)
#define IO_186_T0MCA	(IO_186_BASE + 0x52)
#define IO_186_T0MCB	(IO_186_BASE + 0x54)
#define IO_186_T0CTL	(IO_186_BASE + 0x56)
#define IO_186_T1CNT	(IO_186_BASE + 0x58)
#define IO_186_T1MCA	(IO_186_BASE + 0x5A)
#define IO_186_T1MCB	(IO_186_BASE + 0x5C)
#define IO_186_T1CTL	(IO_186_BASE + 0x5E)
#define IO_186_T2CNT	(IO_186_BASE + 0x60)
#define IO_186_T2MCA	(IO_186_BASE + 0x62)
#define IO_186_T2CTL	(IO_186_BASE + 0x66)
#define IO_186_UMCS	(IO_186_BASE + 0xA0)
#define IO_186_LMCS	(IO_186_BASE + 0xA2)
#define IO_186_PACS	(IO_186_BASE + 0xA4)
#define IO_186_MMCS	(IO_186_BASE + 0xA6)
#define IO_186_MPCS	(IO_186_BASE + 0xA8)
#define IO_186_D0SPL	(IO_186_BASE + 0xC0)
#define IO_186_D0SPH	(IO_186_BASE + 0xC2)
#define IO_186_D0DPL	(IO_186_BASE + 0xC4)
#define IO_186_D0DPH	(IO_186_BASE + 0xC6)
#define IO_186_D0CNT	(IO_186_BASE + 0xC8)
#define IO_186_D0CTL	(IO_186_BASE + 0xCA)
#define IO_186_D1SPL	(IO_186_BASE + 0xD0)
#define IO_186_D1SPH	(IO_186_BASE + 0xD2)
#define IO_186_D1DPL	(IO_186_BASE + 0xD4)
#define IO_186_D1DPH	(IO_186_BASE + 0xD6)
#define IO_186_D1CNT	(IO_186_BASE + 0xD8)
#define IO_186_D1CTL	(IO_186_BASE + 0xDA)
#define IO_186_RELOC	(IO_186_BASE + 0xFE)
