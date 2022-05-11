#ifndef ClpeStream_header
#define ClpeStream_header

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <pthread.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h> 
#include <gst/app/gstappsink.h>
#include "ClpeStreamApi.h"

#define	START_PORT	50000
#define	END_PORT	50003
#define	MAX_PORT	(END_PORT - START_PORT + 1)
//#define AVAILALE_PORT   (MAX_PORT * 2)          // master & slave
#define AVAILALE_PORT   (MAX_PORT)          // master & slave
#define	PORT_NUM(_idx)				(START_PORT + (_idx))

#define	DEST_IP_FROM		"192.168.7.8"

#define	FORM_LTM				"%02d/%02d %02d:%02d:%02d.%06ld "
#define	ARGS_LTM(_ltm, _tv)		(_ltm).tm_mon+1, (_ltm).tm_mday, (_ltm).tm_hour, (_ltm).tm_min, (_ltm).tm_sec, (_tv).tv_usec

#define IMG_WIDTH_IMX390    1920//2048
#define IMG_HEIGHT_IMX390   1080//1280
#define IMG_WIDTH_IMX490    2880
#define IMG_HEIGHT_IMX490   1860

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
// Send UDP
#define	UDP_GST_MAX_BUFFER_SIZE			1073741823

#define	UDP_FRAME_SIZE			4147200 // 1920 * 1080 * 2 //5242880	//  2048*1280*2
#define	UDP_BLOCK_SIZE			64000
#define	UDP_MAX_BLOCK_NUM		64 // 65 - 1(Last)    81 // 82 - 1(Last)
// #define	UDP_LAST_BLOCK_SIZE		58880
//#define	UDP_LAST_BLOCK_SIZE		(5242880 - (UDP_MAX_BLOCK_NUM * UDP_BLOCK_SIZE))
#define	UDP_LAST_BLOCK_SIZE		(UDP_FRAME_SIZE - (UDP_MAX_BLOCK_NUM * UDP_BLOCK_SIZE))
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
#define UDP_PORT_IDX_IMX490             10              // temporal value because IMX490 doesn't support in clpe_soc
#define	UDP_FRAME_SIZE_IMX490	        (IMG_WIDTH_IMX490*IMG_HEIGHT_IMX490*2)
#define	UDP_MAX_BLOCK_NUM_IMX490	    167 // 168 - 1(Last)
#define	UDP_LAST_BLOCK_SIZE_IMX490	    (UDP_FRAME_SIZE_IMX490-(UDP_MAX_BLOCK_NUM_IMX490*UDP_BLOCK_SIZE))
#define	UDP_LAST_BLOCK_INFO_SIZE_IMX490	(UDP_LAST_BLOCK_SIZE_IMX490 + UDP_LAST_INFO_SIZE)
/* ---------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------- */
typedef char				xc8;
typedef unsigned char		xu8;
typedef unsigned short		xu16;
typedef int					x32;
typedef unsigned int		xu32;
typedef unsigned long long	xu64;

/* ---------------------------------------------------------------------------- */
typedef struct {
	xu32			seq;
	xu16			block_id;
	struct timeval	frame;
} T_MSG_BLOCK_INFO;

typedef struct {
	T_MSG_BLOCK_INFO	info;
	xu8					block[UDP_BLOCK_SIZE];
} T_UDP_BLOCK_CONTENT;

/* ---------------------------------------------------------------------------- */
typedef struct {
	xu8					block[UDP_LAST_BLOCK_SIZE];
} T_UDP_BLOCK_LAST;

typedef struct {
	xu8					block[UDP_MAX_BLOCK_NUM_IMX490][UDP_BLOCK_SIZE];
	T_UDP_BLOCK_LAST	last;
} T_UDP_BLOCK;

/* ---------------------------------------------------------------------------- */
typedef struct
{
	xu32			seq;
//#define	IS_BLOCK_DONE(_frame)	((_frame)->block_set_num == (UDP_MAX_BLOCK_NUM+1))
	xu32			block_set_num;
	xu8				block_set[UDP_MAX_BLOCK_NUM_IMX490+1/*last*/];
	struct timeval	tv_frame;
	
	T_UDP_BLOCK		block;
} PortDataFrameX;

#if 0
#ifdef CANLAB_LOGGING_ENABLE
typedef int (*T_CB_APP) (unsigned int inst, unsigned int seq, unsigned char* buffer, unsigned int size, struct timeval* frame_us, unsigned int dropped);
#else
typedef int (*T_CB_APP) (unsigned int inst, unsigned char* buffer, unsigned int size, struct timeval* frame_us);
#endif
#endif

#ifdef CANLAB_LOGGING_ENABLE
typedef int (*T_CB_APP) (unsigned int inst, unsigned int seq, unsigned char* buffer, unsigned int size, struct timeval* frame_us, unsigned int dropped);
#endif // #ifdef CANLAB_LOGGING_ENABLE

struct _ProgramMain;
typedef struct
{
	xu16			idx;
	xu32			seq;

	struct timeval	frame_base_tv;
//#define	MAX_SKIP_COUNT		((UDP_MAX_BLOCK_NUM+1) * 30 * 3) /* 3 sec */
	xu16			skip_count;
#define	FRAME_INTERVAL		33327/* 33ms */
	xu32			do_check;
	xu32			dropped;
	xu32			up_count;
	xu64			discard_low_seq;
	// Lock
#define	DATA_LOCK(_data)	GST_OBJECT_LOCK(&(_data)->lock)
#define	DATA_UNLOCK(_data)	GST_OBJECT_UNLOCK(&(_data)->lock)
	GstObject		lock;

#define	MAX_FRAME		16
	PortDataFrameX	frameX[MAX_FRAME];
	xu32			frame_now_seq;
	xu32			frame_max_seq; // frame_now_seq + MAX_FRAME - 1

	T_CB_APP		cb_app;
	x32				play;

	GstElement*		appsink;
	GstElement*		appsrc;
	GstElement*		up_appsrc;
    x32 appSinkBusWatchId[AVAILALE_PORT];
    x32 appSrcBusWatchId[AVAILALE_PORT];
	T_UDP_BLOCK_CONTENT*	recv;
	struct _ProgramMain*	parent;
} PortData;

typedef struct _ProgramMain
{
	GMainLoop*		loop;

	PortData		port[AVAILALE_PORT];
	
	xu32			dropped;
	xu32			up_count;

    xu32            ui32_use_camera_cnt;

} ProgramMain;



/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#endif /* ClpeStream_header */
