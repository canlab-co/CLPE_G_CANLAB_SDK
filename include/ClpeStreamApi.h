#ifndef ClpeStreamApi_header
#define ClpeStreamApi_header

#include <sys/time.h>
#include <fcntl.h>

//#define CANLAB_LOGGING_ENABLE // you have to comment out it.

#ifndef CANLAB_LOGGING_ENABLE
typedef int (*T_CB_APP) (unsigned int inst, unsigned char* buffer, unsigned int size, struct timeval* frame_us);
#endif // #ifndef CANLAB_LOGGING_ENABLE

/* error define */
#define ERROR_NONE                   0
#define ERROR_CREATE_TASK           -1
#define ERROR_NO_FRAME              -2
#define ERROR_NOT_AVALIABLE_CAM_ID  -3

#ifndef CANLAB_LOGGING_ENABLE
int clpe_startStream(T_CB_APP cb_app, char use_cam_0, char use_cam_1, char use_cam_2, char use_cam_3,
    char use_cam_4, char use_cam_5, char use_cam_6, char use_cam_7, int display_on);
int clpe_stopStream(void);
int clpe_getFrameAnyCam(int *p_camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp);
int clpe_getFrameWithCamId(int camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp);
void clpe_setFormat(int format);
#endif // #ifndef CANLAB_LOGGING_ENABLE

#endif /* ClpeStreamApi_header */
