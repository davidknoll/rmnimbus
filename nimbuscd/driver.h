/* Modified/expanded by David Knoll, based on the book	*/
/* "Writing MS-DOS Device Drivers", second edition:	*/
/*	driver.h					*/
/*	October 15, 1991				*/
/*	R.Lai						*/

/*	Defines for Request Header Status word		*/

#define	ERROR	0x8000
#define	BUSY	0x0200
#define	DONE	0x0100

/*	Defines for Error Codes				*/

#define	WRITEPROTECT			0
#define	UNKNOWNUNIT			1
#define	DRIVENOTREADY			2
#define	UNKNOWN				3
#define	CRCERROR			4
#define	BADRHLENGTH			5
#define	SEEKERROR			6
#define	UNKNOWNMEDIA			7
#define	SECTORNOTFOUND			8
#define	OUTOFPAPER			9
#define	WRITEFAULT			10
#define	READFAULT			11
#define	GENERALFAILURE			12
#define	INVALIDDISKCHANGE		15

/*	Device Header Structure				*/

typedef struct device_header_struct {
	struct device_header_struct far *nextdev;
					/*  ptr to next dev hdr	*/
	unsigned int	attribute;	/*  device attribute	*/
	void (*dev_strategy)(void);	/*  strategy address	*/
	void (*dev_interrupt)(void);	/*  interrupt address	*/
	unsigned char	dev_name[8];	/*  device name		*/

	/*	Fields specific to CD-ROM device headers		*/
	unsigned int	reserved;	/*  reserved			*/
	unsigned char	drive;		/*  drive letter		*/
	unsigned char	units;		/*  number of CD ROM units	*/
} deviceheader_t;

/*	BIOS Parameter Block Structure			*/

typedef struct bpb_struct_struct {
	unsigned int	ss;		/*  sector size in bytes	*/
	unsigned char	au;		/*  allocation unit size	*/
	unsigned int	rs;		/*  reserved (boot) sectors	*/
	unsigned char	nf;		/*  number of FATs		*/
	unsigned int	ds;		/*  directory size in files	*/
	unsigned int	ts;		/*  total sectors		*/
	unsigned char	md;		/*  media descriptor		*/
	unsigned int	fs;		/*  FAT sectors			*/
	unsigned int	st;		/*  sectors per track		*/
	unsigned int	nh;		/*  number of heads		*/
	unsigned long	hs;		/*  hidden sectors		*/
	unsigned long	ls;		/*  large sector count		*/
} bpb_t;

/*	Request Header Structures			*/

typedef struct rhfixed_struct {
	unsigned char	len;		/*  Request Header length	*/
	unsigned char	unit;		/*  unit code			*/
	unsigned char	cmd;		/*  device driver command	*/
	unsigned int	status;		/*  driver returned status	*/
	unsigned char	res[8];		/*  reserved			*/
} rh_t;

typedef struct rh_init_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	nunits;		/*  number of units		*/
	char far	*brkadr;	/*  break address		*/
	bpb_t far	**bpbtab;	/*  pointer to array of BPBs	*/
	char		drive;		/*  1st available drive number	*/
} rh0_t;

typedef struct rh_media_check_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	media;		/*  media descriptor		*/
	unsigned char	md_stat;	/*  media status		*/
	char far	*volid;		/*  address of volume ID	*/
} rh1_t;

typedef struct rh_get_bpb_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	media;		/*  media descriptor		*/
	char far	*buf;		/*  address of data transfer	*/
	bpb_t far	*bpb;		/*  address ptr to BPB table	*/
} rh2_t;

typedef struct rh_ioctl_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	media;		/*  media descriptor		*/
	char far	*buf;		/*  address of data transfer	*/
	unsigned int	count;		/*  count (bytes or sectors)	*/
	unsigned int	start;		/*  start sector number		*/
} rh3_t, rh12_t;

typedef struct rh_io_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	media;		/*  media descriptor		*/
	char far	*buf;		/*  address of data transfer	*/
	unsigned int	count;		/*  count (bytes or sectors)	*/
	unsigned int	start;		/*  start sector number		*/
	char far	*volid;		/*  address of volume ID	*/
} rh4_t, rh8_t, rh9_t;

typedef struct rh_ndinput_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	ch;		/*  character returned		*/
} rh5_t;

typedef struct rh_output_busy_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	media;		/*  media descriptor		*/
	char far	*buf;		/*  address of data transfer	*/
	unsigned int	count;		/*  count (bytes or sectors)	*/
} rh16_t;

typedef struct rh_generic_ioctl_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	major;		/*  major function		*/
	unsigned char	minor;		/*  minor function		*/
	unsigned int	si;		/*  SI register			*/
	unsigned int	di;		/*  DI register			*/
	char far	*pkt;		/*  address of ioctl packet	*/
} rh19_t, rh25_t;

typedef struct rh_device_struct {
	rh_t		rh;		/*  fixed portion	*/
	unsigned char	io;		/*  input/output	*/
	unsigned char	devcmd;		/*  command code	*/
	unsigned int	devstat;	/*  status		*/
	unsigned char	res[4];		/*  reserved		*/
} rh23_t, rh24_t;

/* Request headers specific to CD-ROM drivers */

typedef struct rh_read_long_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	adr_mode;	/*  0 for HSG CD ROM type	*/
	char far	*buf;		/*  address of data transfer	*/
	unsigned int	count;		/*  sector count		*/
	unsigned long	start;		/*  starting sector		*/
	unsigned char	rmode;		/*  read mode 0=cooked / 1=raw	*/
	unsigned char	isize;		/*  interleave size		*/
	unsigned char	iskip;		/*  interleave skip factor	*/
} rh128_t, rh130_t;

typedef struct rh_seek_struct {
	rh_t		rh;		/*  fixed portion		*/
	unsigned char	adr_mode;	/*  0 for HSG CD ROM type	*/
	char far	*buf;		/*  address of data transfer	*/
	unsigned int	count;		/*  sector count		*/
	unsigned long	start;		/*  starting sector		*/
} rh131_t;

/* IOCTL input/output structures */

typedef struct ii_raddr_struct {	/* Return Address of Device Header */
	unsigned char cmd;		/* Control block code */
	deviceheader_t far *devhdr;	/* Address of device header */
} ii0_t;

typedef struct ii_lochead_struct {	/* Location of Head */
	unsigned char cmd;		/* Control block code */
	unsigned char adr_mode; 	/* Addressing mode */
	unsigned long location;		/* Location of drive head */
} ii1_t;

typedef struct ii_audinfo_struct {	/* Audio Channel Info */
	unsigned char cmd;		/* Control block code */
	unsigned char input0;		/* Input  channel (0, 1, 2, or 3) for output channel 0 */
	unsigned char volume0;		/* Volume control (0 - 0xff) for output channel 0 */
	unsigned char input1;		/* Input  channel (0, 1, 2, or 3) for output channel 1 */
	unsigned char volume1;		/* Volume control (0 - 0xff) for output channel 1 */
	unsigned char input2;		/* Input  channel (0, 1, 2, or 3) for output channel 2 */
	unsigned char volume2;		/* Volume control (0 - 0xff) for output channel 2 */
	unsigned char input3;		/* Input  channel (0, 1, 2, or 3) for output channel 3 */
	unsigned char volume3;		/* Volume control (0 - 0xff) for output channel 3 */
} ii4_t, io3_t;

typedef struct ii_drvbytes_struct {	/* Read Drive Bytes */
	unsigned char cmd;		/* Control block code */
	unsigned char bytes;		/* Number bytes read */
	unsigned char buf[128];		/* Read buffer */
} ii5_t;

typedef struct ii_devstat_struct {	/* Device Status */
	unsigned char cmd;		/* Control block code */
	unsigned long params;		/* Device parameters */
} ii6_t;

typedef struct ii_sectsize_struct {	/* Return Sector Size */
	unsigned char cmd;		/* Control block code */
	unsigned char rmode;		/* Read mode */
	unsigned int secsz;		/* Sector size */
} ii7_t;

typedef struct ii_volsize_struct {	/* Return Volume Size */
	unsigned char cmd;		/* Control block code */
	unsigned long volsz;		/* Volume size */
} ii8_t;

typedef struct ii_medchng_struct {	/* Media Changed */
	unsigned char cmd;		/* Control block code */
	unsigned char media;		/* Media byte */
} ii9_t;

typedef struct ii_diskinfo_struct {	/* Audio Disk Info */
	unsigned char cmd;		/* Control block code */
	unsigned char lotrk;		/* Lowest track number */
	unsigned char hitrk;		/* Highest track number */
	unsigned long leadout;		/* Starting point of the lead-out track */
} ii10_t;

typedef struct ii_tnoinfo_struct {	/* Audio Track Info */
	unsigned char cmd;		/* Control block code */
	unsigned char track;		/* Track number */
	unsigned long start;		/* Starting point of the track */
	unsigned char trkctl;		/* Track control information */
} ii11_t;

typedef struct ii_qinfo_struct {	/* Audio Q-Channel Info */
	unsigned char cmd;		/* Control block code */
	unsigned char ctladr;		/* CONTROL and ADR byte */
	unsigned char track;		/* Track number (TNO) */
	unsigned char index;		/* (POINT) or Index (X) */
	/* Running time within a track */
	unsigned char tmin;		/* (MIN) */
	unsigned char tsec;		/* (SEC) */
	unsigned char tframe;		/* (FRAME) */
	unsigned char tzero;		/* (ZERO) */
	/* Running time on the disk */
	unsigned char dmin;		/* (AMIN) or (PMIN) */
	unsigned char dsec;		/* (ASEC) or (PSEC) */
	unsigned char dframe;		/* (AFRAME) or (PFRAME) */
} ii12_t;

typedef struct ii_subchaninfo_struct {	/* Audio Sub-Channel Info */
	unsigned char cmd;		/* Control block code */
	unsigned long start;		/* Starting frame address */
	char far *buf;			/* Transfer address */
	unsigned long count;		/* Number of sectors to read */
} ii13_t;

typedef struct ii_upccode_struct {	/* UPC Code */
	unsigned char cmd;		/* Control block code */
	unsigned char ctladr;		/* CONTROL and ADR byte */
	unsigned char upcean[7];	/* UPC/EAN code (last 4 bits are zero; the low-order nibble of byte 7) */
	unsigned char zero;		/* Zero */
	unsigned char aframe;		/* Aframe */
} ii14_t;

typedef struct ii_audstat_struct {	/* Audio Status Info */
	unsigned char cmd;		/* Control block code */
	unsigned int status;		/* Audio status bits (Bit 0 is Audio Paused bit, Bits 1-15 are reserved) */
	unsigned long start;		/* Starting location of last Play or for next Resume */
	unsigned long end;		/* Ending location for last Play or for next Resume */
} ii15_t;

typedef struct io_lockdoor_struct {	/* Lock/Unlock Door */
	unsigned char cmd;		/* Control block code */
	unsigned char lock;		/* Lock function */
} io1_t;

/*	externally defined variables			*/
extern rh_t far *rhptr;			/* far pointer to request header */
extern deviceheader_t deviceheader;

/* sasicmd.c */
extern int sasi_docommand(unsigned char target, unsigned char far *cbuf, unsigned char far *dbuf);
extern int sasi_request_sense_ascq(unsigned char target, unsigned char lun);
extern int sasi_read(unsigned char target, unsigned char lun, unsigned long sector, unsigned int count, unsigned char far *buf);
extern int sasi_write(unsigned char target, unsigned char lun, unsigned long sector, unsigned int count, unsigned char far *buf);
extern int sasi_read_capacity(unsigned char target, unsigned char lun, unsigned long far *volend, unsigned long far *secsz);
extern int sasi_inquiry(unsigned char target, unsigned char lun, unsigned char bufsz, unsigned char far *buf);

/* cdriver.c */
extern void init(void);
extern void ioctlinput(void);
extern void inputflush(void);
extern void ioctloutput(void);
extern void deviceopen(void);
extern void deviceclose(void);
extern void badcommand(void);

extern void readlong(void);
extern void readlongprefetch(void);
extern void seek(void);
extern void playaudio(void);
extern void stopaudio(void);
extern void resumeaudio(void);
