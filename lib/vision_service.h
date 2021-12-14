/************************************************************************************
***
***	Copyright 2021 Dell(18588220928g@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-13 21:19:01
***
************************************************************************************/

#ifndef _ONNX_SERVICE_H
#define _ONNX_SERVICE_H

#define DEFINE_SERVICE_CODE(major, minor) ((((major) & 0xff) << 24) | ((minor) & 0xff) << 16)
#define SERVICE_CODE(msgcode) ((msgcode) & 0xffff0000)
#define SERVICE_ARGUMENT(msgcode) ((msgcode) & 0xffff)
#define DEFINE_SERVICE(service_code, service_arg) (((service_code) & 0xffff0000) | ((service_arg) & 0xffff))

/****************************************************************************************
***
***  DO NOT USE major service code 0xF for it define in NImage project for system service ...
*** 	For reference the following define ins nnmsg.h ...
***
*** 	#define HELLO_REQUEST_MESSAGE  0xFF010000
***     #define HELLO_RESPONSE_MESSAGE 0xFF020000
***     #define ERROR_TIMEOUT_MESSAGE 0xFF030000
***     #define OUTOF_SERVICE 0xFF040000
***     #define INVALID_SERVICE_MESSAGE 0xFF050000
***
****************************************************************************************/

#define IMAGE_CLEAN_SERVICE DEFINE_SERVICE_CODE(1,1)
#define IMAGE_CLEAN_SERVICE_WITH_GUIDED_FILTER DEFINE_SERVICE_CODE(1,2)
#define IMAGE_CLEAN_SERVICE_WITH_BM3D DEFINE_SERVICE_CODE(1,3)
#define IMAGE_CLEAN_SERVICE_WITH_DEHAZE DEFINE_SERVICE_CODE(1,4)
#define IMAGE_CLEAN_URL "tcp://127.0.0.1:9001"

#define IMAGE_COLOR_SERVICE DEFINE_SERVICE_CODE(2,1)
#define IMAGE_COLOR_URL "tcp://127.0.0.1:9002"

#define IMAGE_NIMA_SERVICE DEFINE_SERVICE_CODE(3,1)
#define IMAGE_NIMA_URL "tcp://127.0.0.1:9003"

#define IMAGE_PATCH_SERVICE DEFINE_SERVICE_CODE(4,1)
#define IMAGE_PATCH_URL "tcp://127.0.0.1:9004"

#define IMAGE_ZOOM_SERVICE DEFINE_SERVICE_CODE(5,1)
#define IMAGE_ZOOM_SERVICE_WITH_PAN DEFINE_SERVICE_CODE(5,2)
#define IMAGE_ZOOM_URL "tcp://127.0.0.1:9005"

#define IMAGE_LIGHT_SERVICE DEFINE_SERVICE_CODE(6,1)
#define IMAGE_LIGHT_SERVICE_WITH_CLAHE DEFINE_SERVICE_CODE(6,2)
#define IMAGE_LIGHT_URL "tcp://127.0.0.1:9006"

#define IMAGE_MATTING_SERVICE DEFINE_SERVICE_CODE(7,1)
#define IMAGE_MATTING_URL "tcp://127.0.0.1:9007"

#define IMAGE_FACEGAN_SERVICE DEFINE_SERVICE_CODE(8,1)
#define IMAGE_FACEGAN_URL "tcp://127.0.0.1:9008"

#define VIDEO_CLEAN_SERVICE DEFINE_SERVICE_CODE(9,1)
#define VIDEO_CLEAN_URL "tcp://127.0.0.1:9009"

#define VIDEO_COLOR_SERVICE DEFINE_SERVICE_CODE(10,1)
#define VIDEO_REFERENCE_SERVICE DEFINE_SERVICE_CODE(10,2)
#define VIDEO_COLOR_URL "tcp://127.0.0.1:9010"

#define VIDEO_SLOW_SERVICE DEFINE_SERVICE_CODE(11,1)
#define VIDEO_SLOW_URL "tcp://127.0.0.1:9011"

#define VIDEO_ZOOM_SERVICE DEFINE_SERVICE_CODE(12,1)
#define VIDEO_ZOOM_URL "tcp://127.0.0.1:9012"

#define VIDEO_RIFE_SERVICE DEFINE_SERVICE_CODE(13,1)
#define VIDEO_RIFE_URL "tcp://127.0.0.1:9013"

#define VIDEO_TOFLOW_CLEAN_SERVICE DEFINE_SERVICE_CODE(14,1)
#define VIDEO_TOFLOW_ZOOM_SERVICE DEFINE_SERVICE_CODE(14,2)
#define VIDEO_TOFLOW_SLOW_SERVICE DEFINE_SERVICE_CODE(14,3)
#define VIDEO_TOFLOW_URL "tcp://127.0.0.1:9014"


#endif // _ONNX_SERVICE_H
