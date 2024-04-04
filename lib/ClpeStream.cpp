#include "ClpeStream.h"
#include <math.h>

int process_block_done(PortData* data);
void intecept_frame(int camera_id, unsigned int cur_frame_seq, unsigned char* p_buffer, unsigned int size, struct timeval *pt_camera_timeStamp);

ProgramMain				g_MainData;
T_UDP_BLOCK_CONTENT		g_recv[AVAILALE_PORT];
unsigned int g_stop_signal = 0;

// related to clpe_getFrameCb()
#define MAX_SEQ_OFFSET		110000 // 3600 * 30 = 108000
unsigned int			g_max_seq_offset = MAX_SEQ_OFFSET;
struct timeval frame_pre[AVAILALE_PORT] = {{0,},};
struct timeval prePcTime[AVAILALE_PORT] = {{0,},}; /* added by dwsun */
unsigned int	seq_base[AVAILALE_PORT] = {0, };
int		g_lock	= 0;
#define SIMPLE_LOCK		while(g_lock) { usleep(10); } g_lock = 1;
#define SIMPLE_UNLOCK	g_lock = 0;

typedef struct function_arg{
    T_CB_APP callback_func;
    int display_on;
    char use_cam[AVAILALE_PORT];		// master & slave
}t_function_arg;

typedef struct frame_info{
    int camera_id;
    unsigned char *p_buffer;
    unsigned int size;
    unsigned int last_frame_seq;
    unsigned int current_frame_seq;
    struct timeval pt_camera_timeStamp;
}t_frame_info;

unsigned int g_last_cam_id = 0;
unsigned int g_cur_cam_id = 0;
unsigned int g_last_frame_seq = 0;
unsigned int g_cur_frame_seq = 0;

// FOR JPEG
int format = 1;
typedef struct jpeg_frame_info{
	unsigned int frame_seq;
	struct timeval jpeg_timeStamp;
}t_jpeg_frame_info;


t_frame_info gt_frame_info[AVAILALE_PORT] = {0,};    // master & slave
t_jpeg_frame_info gt_jpeg_frame_info[AVAILALE_PORT] = {0, };

pthread_t g_pthread;

GstBus* appSinkBus;
GstElement*		gAppSink;

void increase_seq(PortData* data, xu32 now_seq)
{
	xu32 	idx;

	if(! now_seq) {
		now_seq = data->frame_now_seq;
	}
	for(idx = 0; idx < MAX_FRAME; idx ++) {
		if((data->frameX[idx].seq != 0) && (data->frameX[idx].seq <= now_seq)) { // Remove also NOW  
			memset(&data->frameX[idx], 0x00, sizeof(PortDataFrameX) - sizeof(T_UDP_BLOCK));           
		}
	}

	data->frame_now_seq = now_seq + 1;
	data->frame_max_seq = data->frame_now_seq + MAX_FRAME - 1;
}

int process_block_done(PortData* data)
{
	PortDataFrameX* frame = &data->frameX[data->frame_now_seq % MAX_FRAME];
	xu32	now_seq = frame->seq;
    xu32    ui32_udpFrameSize = 0;
    xu32    ui32_udpMaxBlockNum = 0;

    if(data->idx == UDP_PORT_IDX_IMX490) // in case of IMX490
    {
        ui32_udpFrameSize = UDP_FRAME_SIZE_IMX490;
        ui32_udpMaxBlockNum = UDP_MAX_BLOCK_NUM_IMX490;
    }
    else
    {
        ui32_udpFrameSize = UDP_FRAME_SIZE;
        ui32_udpMaxBlockNum = UDP_MAX_BLOCK_NUM;
    }

	//if(IS_BLOCK_DONE(frame))
	if(frame->block_set_num == ui32_udpMaxBlockNum + 1)
	{
		// For STAT, Call Callback 
		data->up_count++;
		data->parent->up_count++;
		if(data->idx != UDP_PORT_IDX_IMX490)
		{
			memcpy(frame->block.block[UDP_MAX_BLOCK_NUM], frame->block.last.block, UDP_LAST_BLOCK_SIZE);
		}
#ifdef CANLAB_LOGGING_ENABLE
		if(data->cb_app(data->idx, frame->seq, frame->block.block[0], ui32_udpFrameSize, &frame->tv_frame
					, data->dropped) < 0) {

			g_main_loop_quit (data->parent->loop);
			return -1;
		}
#else // #ifdef CANLAB_LOGGING_ENABLE
#if 0        
        if(data->cb_app(data->idx, frame->block.block[0], UDP_FRAME_SIZE, &frame->tv_frame) < 0) {

			g_main_loop_quit (data->parent->loop);
			return -1;
		}
#else
        if(g_stop_signal == 1)
        {
            //g_main_loop_quit(data->parent->loop);
            g_stop_signal = 0;
            return -1;
        }
        else
        {
            data->cb_app(data->idx, frame->block.block[0], ui32_udpFrameSize, &frame->tv_frame);
            intecept_frame(data->idx, frame->seq, frame->block.block[0], ui32_udpFrameSize, &frame->tv_frame);
            
        }
            
#endif
#endif // #ifdef CANLAB_LOGGING_ENABLE

		// Send To SRC
		if(data->play) {		
			GstBuffer* buffer;
			GstMapInfo map;

			buffer = gst_buffer_new_and_alloc (ui32_udpFrameSize);
			gst_buffer_map (buffer, &map, GST_MAP_WRITE);
			memcpy(map.data, &frame->block, ui32_udpFrameSize);
			gst_buffer_unmap (buffer, &map);

			struct timeval	tv_frame_pts;
			timersub(&frame->tv_frame, &data->frame_base_tv, &tv_frame_pts);

			GST_BUFFER_PTS(buffer) = GST_TIMEVAL_TO_TIME(tv_frame_pts);
			gst_app_src_push_buffer (GST_APP_SRC (data->up_appsrc), buffer);
			
		}
	} else {
		// Old Discard : Not Completed
		data->dropped ++;
		data->parent->dropped ++;
	}

	increase_seq(data, now_seq);

	return 0;
}

static GstFlowReturn
on_new_sample_from_sink (GstElement* elt, PortData* data)
{
 if(format == 0){
	gettimeofday(&gt_jpeg_frame_info[data->idx].jpeg_timeStamp, NULL);

	GstSample *sample;
	GstBuffer *app_buffer, *buffer;
	GstFlowReturn ret = GST_FLOW_OK;
	GstMapInfo map;
      
        // get the sample from appsink 
	sample = gst_app_sink_pull_sample (GST_APP_SINK (elt));
	buffer = gst_sample_get_buffer (sample);	  	

        // make a copy
	app_buffer = gst_buffer_copy_deep (buffer);
		  
	gst_buffer_map (app_buffer, &map, GST_MAP_WRITE);
	
	data->cb_app(data->idx, (unsigned char *)map.data, (unsigned int)map.size, &gt_jpeg_frame_info[data->idx].jpeg_timeStamp);	
	intecept_frame(data->idx, gt_jpeg_frame_info[data->idx].frame_seq, (unsigned char *)map.data, (unsigned int)map.size, &gt_jpeg_frame_info[data->idx].jpeg_timeStamp);
	
	//get source an push new buffer 
	if(data->play){	
		gst_app_src_push_buffer (GST_APP_SRC (data->up_appsrc), app_buffer);
	}else{
		//nothing to do
	}

	gst_sample_unref (sample);

	gst_buffer_unmap (buffer, &map);

	gt_jpeg_frame_info[data->idx].frame_seq++;

	return ret;
  }else{
  	GstSample *sample;
	GstBuffer *buffer;
	GstFlowReturn ret = GST_FLOW_OK;
	gsize			recv_len;
	PortDataFrameX*	frame;
    xu16    ui16_udpMaxBlockNum = 0;
    xu32    ui32_udpLastBlockSize = 0;
	xu32	fps = 0;

    if(data->idx == UDP_PORT_IDX_IMX490) // in case of IMX490
    {
        ui16_udpMaxBlockNum = UDP_MAX_BLOCK_NUM_IMX490;
        ui32_udpLastBlockSize = UDP_LAST_BLOCK_SIZE_IMX490;
		fps = 25;
    }
    else
    {
        ui16_udpMaxBlockNum = UDP_MAX_BLOCK_NUM;
        ui32_udpLastBlockSize = UDP_LAST_BLOCK_SIZE;
		fps = 30;
    }

	sample = gst_app_sink_pull_sample (GST_APP_SINK (elt));


	if(data->skip_count < ((ui16_udpMaxBlockNum+1) * fps * 1))		// skip 1 sec
	{
		data->skip_count++;
		gst_sample_unref (sample);
		return ret;
	} 

	buffer = gst_sample_get_buffer (sample);
	recv_len = gst_buffer_extract(buffer, 0, data->recv, sizeof(*data->recv));

	T_MSG_BLOCK_INFO*	info = (T_MSG_BLOCK_INFO*) &data->recv->info;
	
	DATA_LOCK(data);

	if(data->do_check) {
		if((
					info->seq > data->frame_max_seq + 1
					|| info->seq < data->frame_now_seq
		   )) {
			data->discard_low_seq++;

			DATA_UNLOCK(data);

			gst_sample_unref (sample);
			return ret;
		}

#if 0
		if((

					info->seq == data->frame_max_seq + 1
		   )) {
			DATA_UNLOCK(data);
			gst_sample_unref (sample);
			return ret;
		}
#endif
	}

	xu32 now = info->seq % MAX_FRAME;
	frame = &data->frameX[now];

	// Valid Check
	if((
		info->block_id > ui16_udpMaxBlockNum
		|| frame->block_set[info->block_id] // Already Exist ; Pre seq -> Drop, Future Seq -> ToDO
		|| recv_len < (ui32_udpLastBlockSize + sizeof(T_MSG_BLOCK_INFO))
		)) {
		DATA_UNLOCK(data);
		gst_sample_unref (sample);
		return ret;
	}

	// Block processing
	if(info->block_id == ui16_udpMaxBlockNum) { // Last
		 memcpy(frame->block.last.block,			data->recv->block, ui32_udpLastBlockSize);
	} else {
		 memcpy(frame->block.block[info->block_id],data->recv->block, UDP_BLOCK_SIZE);
	}

	frame->block_set[info->block_id] = TRUE;
	frame->block_set_num ++;

	if(frame->block_set_num == 1) { // First
		frame->seq = info->seq;
		frame->tv_frame = info->frame;
	}
	// moved above
	//else if(IS_BLOCK_DONE(frame))
	else if(frame->block_set_num == (xu32)(ui16_udpMaxBlockNum + 1))
	{
		if(! data->do_check ) {
			data->do_check = 1;

			// Totally Init
			data->frame_now_seq = info->seq; // Error ???? frame->seq
			data->frame_max_seq = data->frame_now_seq + MAX_FRAME - 1;
			
			// Totally 1st
			data->frame_base_tv = info->frame;

#ifndef FIX_NOW_FRAME /* root.2021.0515 */
			xu32 	idx;
			for(idx = 0; idx < MAX_FRAME; idx ++) {
				if(data->frameX[idx].seq < info->seq) {
					memset(&data->frameX[idx], 0x00, sizeof(PortDataFrameX) - sizeof(T_UDP_BLOCK));
				}
			}
#endif /* FIX_NOW_FRAME  root.2021.0515 */
		}
		else
		{
			data->frame_now_seq = info->seq;
			data->frame_max_seq = data->frame_now_seq + MAX_FRAME - 1;
		}

		process_block_done(data);
	}

	DATA_UNLOCK(data);
	gst_sample_unref (sample);

	return ret;
  }
}

gboolean on_sink_message_appsink (GstBus * bus, GstMessage * message, PortData* data)
{
	xc8		name_app[32];

	snprintf(name_app, sizeof(name_app), "app_%u", data->idx);

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_EOS:
			g_print ("The appsink[%s] Finished playback\n", name_app);
			g_main_loop_quit (data->parent->loop);
			break;
		case GST_MESSAGE_ERROR:
			g_print ("The appsink[%s] received error\n", name_app);
			g_main_loop_quit (data->parent->loop);
			break;
		default:
			break;
	}
	return TRUE;
}

gboolean on_source_message (GstBus * bus, GstMessage * message, PortData * data)
{
	GstElement *appsrc;
	xc8		name_app[32];

	snprintf(name_app, sizeof(name_app), "app_%u", data->idx);

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_EOS:
			g_print ("The appsrc[%s] got dry\n", name_app);
			appsrc = gst_bin_get_by_name (GST_BIN (data->appsrc), name_app);
			gst_app_src_end_of_stream (GST_APP_SRC (appsrc));
			gst_object_unref (appsrc);
			break;
		case GST_MESSAGE_ERROR:
			g_print ("The appsrc[%s] received error\n", name_app);
			g_main_loop_quit (data->parent->loop);
			break;
		default:
			break;
	}
	return TRUE;
}

int launch_port(PortData* data)
{
	xc8		buff[4096];
	xc8		name_app[32];
	xc8		name_appsrc[32];
	//xu16	port = START_PORT + data->idx;
	xu16 port = 0;

    xu32    image_width = 0;
    xu32    image_height = 0;
	xc8		ip_address[32];

    if(data->idx == UDP_PORT_IDX_IMX490) // in case of IMX490
    {
        image_width = IMG_WIDTH_IMX490;
        image_height = IMG_HEIGHT_IMX490;
    }
    else
    {
        image_width = IMG_WIDTH_IMX390;
        image_height = IMG_HEIGHT_IMX390;
    }

	if(data->idx < MAX_PORT)
	{
		snprintf(ip_address, sizeof(ip_address), "192.168.7.8");	
		port = START_PORT + data->idx;
	}
	else
	{
		snprintf(ip_address, sizeof(ip_address), "192.168.8.8");
		port = START_PORT + data->idx - MAX_PORT;
	}

	/* ========================================= SINK ========================================= */
	snprintf(name_app, sizeof(name_app), "app_%u", data->idx);
	if(format == 0){
	snprintf(buff, sizeof(buff),
			"udpsrc address=%s port=%u retrieve-sender-address=false buffer-size=%u "
			" ! application/x-rtp, encoding-name=JPEG, payload=26 "
			" ! queue max-size-time=5000000000 max-size-buffers=5000000000 max-size-bytes=5000000000 " 
			" ! rtpjpegdepay ! appsink name=%s ",
			ip_address, port,
			UDP_GST_MAX_BUFFER_SIZE,
			name_app
			);
	}else{
	snprintf(buff, sizeof(buff),
			"udpsrc address=%s port=%u retrieve-sender-address=false buffer-size=%u " 
			" ! queue max-size-time=2000000000 max-size-buffers=2000000000 max-size-bytes=2000000000 " 
			" ! appsink name=%s",
			ip_address, port,
			UDP_GST_MAX_BUFFER_SIZE,
			name_app
			);
	}

	data->appsink = gst_parse_launch (buff, NULL);
	if (data->appsink == NULL) {
		g_print ("Launch fail for port[%u]\n", port);
		return FALSE;
	}

	
	appSinkBus = gst_element_get_bus (data->appsink);
	data->appSinkBusWatchId[data->idx] = gst_bus_add_watch (appSinkBus, (GstBusFunc) on_sink_message_appsink, data);
	gst_object_unref (appSinkBus);

	//GstElement* appsink;
	gAppSink = gst_bin_get_by_name (GST_BIN(data->appsink), name_app);
	g_object_set (G_OBJECT (gAppSink), "emit-signals", TRUE, "sync", FALSE, NULL);
	g_signal_connect (gAppSink, "new-sample", G_CALLBACK (on_new_sample_from_sink), data);
	gst_object_unref (gAppSink);

	/* ========================================= SOURCE ========================================= */
	snprintf(name_appsrc, sizeof(name_appsrc), "appsrc_%u", data->idx);

	if(data->play) { // play = ON
		if(format == 0)
		{
			if(g_MainData.ui32_use_camera_cnt > 4)
			{
				snprintf(buff, sizeof(buff), "appsrc name=%s stream-type=\"stream\" is-live=1 do-timestamp=0 format=time "
						" ! queue max-size-time=2000000000 max-size-buffers=2000000000 max-size-bytes=2000000000 " 
						" ! jpegparse ! jpegdec ! videoscale method=0 n-threads=9 ! video/x-raw, width=480, height=270 "
						" ! xvimagesink sync=false async=false",
						name_appsrc
						);			
			}
			else
			{
				snprintf(buff, sizeof(buff), "appsrc name=%s stream-type=\"stream\" is-live=1 do-timestamp=0 format=time "
						" ! queue max-size-time=2000000000 max-size-buffers=2000000000 max-size-bytes=2000000000 " 
						" ! jpegparse ! jpegdec ! videoconvert ! xvimagesink sync=false async=false",
						name_appsrc
						);
			}
		}	
		else
		{
			if(g_MainData.ui32_use_camera_cnt > 4)
			{
				snprintf(buff, sizeof(buff), "appsrc name=%s stream-type=\"stream\" is-live=1 do-timestamp=0 format=time "
						" caps=\"video/x-raw,format=UYVY,width=%d,height=%d,pixel-aspect-ratio=1/1,interlace-mode=progressive,framerate=30/1\" "
						" ! queue max-size-time=2000000000 max-size-buffers=2000000000 max-size-bytes=2000000000 " 
						" ! videoscale method=0 n-threads=9 ! video/x-raw, width=480, height=270 "
						" ! videoconvert ! xvimagesink sync=false async=false",
						name_appsrc,
						image_width,
						image_height
						);			
			}
			else
			{
				snprintf(buff, sizeof(buff), "appsrc name=%s stream-type=\"stream\" is-live=1 do-timestamp=0 format=time "
						" caps=\"video/x-raw,format=UYVY,width=%d,height=%d,pixel-aspect-ratio=1/1,interlace-mode=progressive,framerate=30/1\" "
						" ! queue max-size-time=2000000000 max-size-buffers=2000000000 max-size-bytes=2000000000 " 
						" ! videoconvert ! xvimagesink sync=false async=false",
						name_appsrc,
						image_width,
						image_height
						);
			}
		}
	} 

	if(data->play) { // play = ON
		GstBus* appSrcBus;
		data->appsrc = gst_parse_launch (buff, NULL);
		if (data->appsrc == NULL) {
			g_print ("Launch fail for port[%u]\n", port);
			return FALSE;
		}

		appSrcBus = gst_element_get_bus (data->appsrc);
		data->appSrcBusWatchId[data->idx] = gst_bus_add_watch (appSrcBus, (GstBusFunc) on_source_message, data);
		gst_object_unref (appSrcBus);

		data->up_appsrc = gst_bin_get_by_name (GST_BIN(data->appsrc), name_appsrc);
		g_object_set (data->up_appsrc, "format", GST_FORMAT_TIME, NULL);
	}

	return TRUE;
}


void *clpe_runStream(void *pArg)
{
    t_function_arg *pt_func_arg;
    T_CB_APP cb_app;
    int play;
	GstMessage *msg = NULL;
    //int result = 0;

    pt_func_arg = (t_function_arg *)pArg;

    cb_app = pt_func_arg->callback_func;
    play = pt_func_arg->display_on;
    
	if(! cb_app) {
		fprintf(stderr, "T_CB_APP is NULL.\n");
        //result = -1;
        free(pt_func_arg);
		//return (void *)&result;
		return NULL;
	}

#if 0
	char renice_cmd[ 128 ]; 
	int pid = getpid(); 
	sprintf (renice_cmd, "renice -20 %d > /dev/null" , pid); 
	if(0 > system (renice_cmd)) {
        result = -1;
		return (void *)&result;
	}
#endif    

	gst_init (NULL,NULL);
	memset(&g_MainData, 0x00, sizeof(g_MainData));
	g_MainData.loop = g_main_loop_new (NULL, FALSE);

	g_MainData.ui32_use_camera_cnt = 0;
	for(xu16 idx = 0; idx < AVAILALE_PORT; idx++) {
		if(pt_func_arg->use_cam[idx] == 1)
		{
			g_MainData.ui32_use_camera_cnt++;
		}
	}

	for(xu16 idx = 0; idx < AVAILALE_PORT; idx++) {
        if(pt_func_arg->use_cam[idx] == 1)
        {

    		g_MainData.port[idx].parent = &g_MainData;
    		g_MainData.port[idx].cb_app = cb_app;
#if 0		
		if(idx < 4)
	    		g_MainData.port[idx].play = play;
		else
			g_MainData.port[idx].play = 0;
#else
		g_MainData.port[idx].play = play;
#endif		
    		g_MainData.port[idx].idx = idx;
    		g_MainData.port[idx].seq = 0;
    		g_MainData.port[idx].recv = &g_recv[idx];

    		g_MainData.port[idx].frame_now_seq = 0;
    		g_MainData.port[idx].frame_max_seq = MAX_FRAME - 1;
    		if(! launch_port(&g_MainData.port[idx]) ) {
    			g_main_loop_unref (g_MainData.loop);
    			//result = -2;
    			free(pt_func_arg);
        		return NULL;
    		}
        }
	}

	for(xu16 idx = 0; idx < AVAILALE_PORT; idx++) {
        if(pt_func_arg->use_cam[idx] == 1)
        {
    		PortData* data = &g_MainData.port[idx];

    		if(data->appsrc) {
    			gst_element_set_state (data->appsrc, GST_STATE_PLAYING);
    		}
    		gst_element_set_state (data->appsink, GST_STATE_PLAYING);
			//gLastAppsink = data->appsrc;
        }
	}

	//g_main_loop_run (g_MainData.loop);
	msg = gst_bus_timed_pop_filtered(appSinkBus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_EOS));
	if(msg != NULL)
	{
		gst_message_unref(msg);
	}

	for(xu16 idx = 0; idx < AVAILALE_PORT; idx++) {
        if(pt_func_arg->use_cam[idx] == 1)
        {
        	PortData* data = &g_MainData.port[idx];
			
    		gst_element_set_state (g_MainData.port[idx].appsink, GST_STATE_NULL);
			if(data->appsrc) {
	    		gst_element_set_state (g_MainData.port[idx].appsrc, GST_STATE_NULL);
			}
			g_source_remove(data->appSinkBusWatchId[idx]);
    		gst_object_unref (g_MainData.port[idx].appsink);
			if(data->appsrc) {
				g_source_remove(data->appSrcBusWatchId[idx]);	
	    		gst_object_unref (g_MainData.port[idx].appsrc);
			}
        }
	}

	g_main_loop_unref (g_MainData.loop);

    free(pt_func_arg);
	//return (void *)&result;
	return NULL;
}

void intecept_frame(int camera_id, unsigned int cur_frame_seq, unsigned char* p_buffer, unsigned int size, struct timeval *pt_camera_timeStamp)
{
    if(format == 0)
    {
    	
	    gt_frame_info[camera_id].camera_id = camera_id;
	    gt_frame_info[camera_id].p_buffer = p_buffer;
	    gt_frame_info[camera_id].size = size;
	    gt_frame_info[camera_id].pt_camera_timeStamp = *pt_camera_timeStamp;
	    gt_frame_info[camera_id].current_frame_seq = cur_frame_seq;
	    
	    g_cur_cam_id = camera_id;
	    g_cur_frame_seq = cur_frame_seq;
	    	  
	    
    }
    else
    {
	    gt_frame_info[camera_id].camera_id = camera_id;
	    gt_frame_info[camera_id].p_buffer = p_buffer;
	    gt_frame_info[camera_id].size = size;
	    gt_frame_info[camera_id].pt_camera_timeStamp = *pt_camera_timeStamp;
	    gt_frame_info[camera_id].current_frame_seq = cur_frame_seq;

	    g_cur_cam_id = camera_id;
	    g_cur_frame_seq = cur_frame_seq;
    }   

}

void clpe_setFormat(int getformat)
{
    if(getformat == 0) format = 0;
    else format = 1;
}

int clpe_startStream(T_CB_APP cb_app, char use_cam_0, char use_cam_1, char use_cam_2, char use_cam_3, char use_cam_4, char use_cam_5, char use_cam_6, char use_cam_7, int display_on)
{
//    pthread_t p_thread;
    int thread_id;
    t_function_arg *pt_func_arg;
    int result = ERROR_NONE;

    pt_func_arg = (t_function_arg *)malloc(sizeof(t_function_arg));
    pt_func_arg->callback_func = cb_app;
    pt_func_arg->display_on = display_on;
	pt_func_arg->use_cam[0] = use_cam_0;
	pt_func_arg->use_cam[1] = use_cam_1;
	pt_func_arg->use_cam[2] = use_cam_2;
	pt_func_arg->use_cam[3] = use_cam_3;
	pt_func_arg->use_cam[4] = use_cam_4;
	pt_func_arg->use_cam[5] = use_cam_5;
	pt_func_arg->use_cam[6] = use_cam_6;
	pt_func_arg->use_cam[7] = use_cam_7;

    thread_id = pthread_create(&g_pthread, NULL, clpe_runStream, (void *)(pt_func_arg));

    if (thread_id < 0)
    {
        printf("Fail to create thread !!! \n");
        result = ERROR_CREATE_TASK;
    }
    return result;
}

int clpe_stopStream(void)
{
    int result = ERROR_NONE;
    
    g_stop_signal = 1;

    //g_main_loop_quit(g_MainData.loop);
    gst_element_post_message(gAppSink, gst_message_new_eos(GST_OBJECT(gAppSink)));

    pthread_join(g_pthread, NULL);

	memset(gt_frame_info, 0x00, sizeof(gt_frame_info));
	g_cur_cam_id = 0;
    g_last_cam_id = 0;
	g_cur_frame_seq = 0;
	g_last_frame_seq = 0;
	
    return result;
}

int clpe_getFrameAnyCam(int *p_camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp)
{
    int timeout = 3000;
    
    do{
        if((g_cur_cam_id != g_last_cam_id) || (g_cur_frame_seq != g_last_frame_seq))
        {
            *p_camera_id = g_cur_cam_id;
            *p_buffer = gt_frame_info[g_cur_cam_id].p_buffer;
            *p_size = gt_frame_info[g_cur_cam_id].size;
            *pt_camera_timeStamp = gt_frame_info[g_cur_cam_id].pt_camera_timeStamp;

            g_last_cam_id = g_cur_cam_id;
            g_last_frame_seq = g_cur_frame_seq;
            break;
        }
        usleep(1000);
    }while(--timeout);
    if(timeout == 0)
    {
        return ERROR_NO_FRAME;
    }
    
    return ERROR_NONE;
}

int clpe_getFrameWithCamId(int camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp)
{
    int timeout = 3000;
    if(camera_id > AVAILALE_PORT)			// master & slave
    {
        return ERROR_NOT_AVALIABLE_CAM_ID;        
    }
    do{
        if(gt_frame_info[camera_id].last_frame_seq != gt_frame_info[camera_id].current_frame_seq)
        {
            *p_buffer = gt_frame_info[camera_id].p_buffer;
            *p_size = gt_frame_info[camera_id].size;
            *pt_camera_timeStamp = gt_frame_info[camera_id].pt_camera_timeStamp;
            gt_frame_info[camera_id].last_frame_seq = gt_frame_info[camera_id].current_frame_seq;
            break;
        }
        usleep(1000);
    }while(--timeout);
    if(timeout == 0)
    {
        return ERROR_NO_FRAME;
    }
    return ERROR_NONE;
}


#ifdef CANLAB_LOGGING_ENABLE
int clpe_getFrameCb(unsigned int inst, unsigned int seq, unsigned char* buffer, unsigned int size, struct timeval* frame_us, unsigned int dropped)
{
	struct timeval tv_sub;
	struct timeval cur_sub;
	struct tm  ltm_frame;
	struct tm  ltm_curPc;
	struct timeval curPcTime; // added by dwsun

	gettimeofday(&curPcTime, NULL); // added by dwsun
	if(! seq_base[inst]) seq_base[inst] = seq;

	if(frame_pre[inst].tv_sec) {
		// 1st skip :
		localtime_r(&frame_us->tv_sec, &ltm_frame);
		timersub(frame_us, &frame_pre[inst], &tv_sub);

		localtime_r(&curPcTime.tv_sec, &ltm_curPc);
		timersub(&curPcTime, &prePcTime[inst], &cur_sub);

		SIMPLE_LOCK;
		printf("%u | %4u | " FORM_LTM " | %6lu |",
				50000 + inst,
				// seq,
				seq - seq_base[inst],
				ARGS_LTM(ltm_frame, *frame_us),
				tv_sub.tv_usec
			  );

		/* added by dwsun */
		printf(" " FORM_LTM " | %6lu | %u\n",
				//50000 + inst,
				//seq,
				ARGS_LTM(ltm_curPc, curPcTime),
				cur_sub.tv_usec,
				dropped
			  );
		SIMPLE_UNLOCK;
	}

	prePcTime[inst] = curPcTime; // added by dwsun
	memcpy(&frame_pre[inst], frame_us, sizeof(frame_pre[inst]));

	// Must Do Return as fast as posible (Recommend in 5ms).
	if(seq - seq_base[inst] > g_max_seq_offset) {
		return -1; // -1 : STOP
	}
	return 0; // -1 : STOP

}
#endif //#ifdef CANLAB_LOGGING_ENABLE 
