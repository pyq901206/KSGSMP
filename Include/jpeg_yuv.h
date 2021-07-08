#ifndef JPEG_YUV_H
#define JPEG_YUV_H
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "turbojpeg.h"
#include <time.h>

typedef struct tjp_info {
  int outwidth;
  int outheight;
  unsigned long jpg_size;
}tjp_info_t;

//int jpeg_get_data_info(unsigned char *jpg_data, int data_size, tjp_info_t *tinfo);
int yuv420p_to_yuv420spback(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height);
int yuv420p_to_yuv420sp(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height);
int YUV422To420(unsigned char yuv422[], unsigned char yuv420[], int width, int height);
int yv16_to_i420(unsigned char* pImageBuf, int width, int height) ;
int tjpeg2yuv(unsigned char* jpeg_buffer, int jpeg_size, unsigned char** yuv_buffer, int* yuv_size, int* yuv_type);
//int tyuv2jpeg(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** jpeg_buffer, unsigned long* jpeg_size, int quality);
int trgb2yuv(unsigned char* rgb_buffer, int width, int height, unsigned char** yuv_buffer, int* yuv_size, int subsample);
int tyuv2rgb(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** rgb_buffer, int* rgb_size);
int tjpeg2rgb(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* rgb_buffer, int* size);
int trgb2jpeg(unsigned char* rgb_buffer, int width, int height, int quality, unsigned char** jpeg_buffer, unsigned long* jpeg_size);
void NV21_TO_RGB24(unsigned char *data, unsigned char *rgb, int width, int height);
int tyuv2jpeg(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** jpeg_buffer, unsigned long* jpeg_size, int quality);
void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height);
int clipNv21ToNv21(const unsigned char* pNv21Source,  const int nWidth, const int nHeight,
    unsigned char* pNv21Dest,  const int nClipLeft, const int nClipTop, const int nClipWidth, const int nClipHeight);
int jpegcrop(unsigned char *jpegBuf, unsigned long jpegSize,int cropx,int cropy,int cropw,int croph,unsigned char **dstBuf,unsigned long *dstSize);
int jpg_compress(unsigned char *jpeg_buf,unsigned long jpg_size, int quality, tjscalingfactor factor, unsigned char **outjpg_buffer, unsigned long *outjpg_size,int *outjpegw,int *outjpegh);
void write_buffer2file(char *filename, unsigned char *buffer, int size);
int pngTojpg(unsigned char *pngdata,int pnglen,unsigned char *jpgdata,unsigned long* jpglen);


#endif

