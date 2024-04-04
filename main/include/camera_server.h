#ifndef __CAMERA_SERVER_H__
#define __CAMERA_SERVER_H__

#define CAMERA_START_BYTE_1 0xAE
#define CAMERA_START_BYTE_2 0x6D

bool cameraInit();
void cameraServerTask(void *pvParameters);

#endif // __CAMERA_SERVER_H__