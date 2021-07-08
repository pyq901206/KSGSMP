#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "jpeglib.h"
#include "turbojpeg.h"
#include "jpeg_yuv.h"
#include "png.h"
//#include "ai_log.h"

#if 0
int jpeg_get_data_info(unsigned char *jpg_data, int data_size, tjp_info_t *tinfo)
{
	tjhandle handle = NULL;
	int img_subsamp,img_colorspace;
	handle = tjInitDecompress();
	if (NULL == handle) {
	  return -1;
	}
	tinfo->jpg_size = data_size;
	/*获取jpg图片相关信息但并不解压缩*/
	int ret = tjDecompressHeader3(handle,jpg_data,tinfo->jpg_size,&(tinfo->outwidth),&(tinfo->outheight),&img_subsamp,&img_colorspace);

	tjDestroy(handle);

	if(tinfo->outwidth == 640 && tinfo->outheight == 352)
	{
					/*输出图片信息*/
		printf("jpeg width:%d\n",tinfo->outwidth);
		printf("jpeg height:%d\n",tinfo->outheight);
		printf("jpeg subsamp:%d\n",img_subsamp);
		printf("jpeg colorspace:%d\n",img_colorspace);
		return 0;
	}
	else
	{
		printf("jpeg Resolution must be is 640 * 352\n");
		printf("now jpeg width:%d\n",tinfo->outwidth);
		printf("now jpeg height:%d\n",tinfo->outheight);
		return -1;
	}
}
#endif

int clipNv21ToNv21(const unsigned char* pNv21Source,  const int nWidth, const int nHeight,
    unsigned char* pNv21Dest,  const int nClipLeft, const int nClipTop, const int nClipWidth, const int nClipHeight)
{
    const unsigned char* pNv21Source0 = pNv21Source;
    int i = 0;
 
    //鹿脴录眉露镁脰庐脪禄隆拢
    //脠隆脮没隆拢鹿脌录脝虏禄脥卢脝陆脤篓脪陋脟贸脠隆脮没路露脦搂脫脨虏卯脪矛隆拢
    //脮芒脩霉录脝脣茫碌脛陆谩鹿没拢卢脫脨驴脡脛脺虏卯脪禄赂枚脧帽脣脴隆拢驴铆赂脽脳卯潞脙露脭脫娄碌梅脮没隆拢
    int clipLeft = (int)(nClipLeft+1)  / 2 * 2;
    int clipTop  = (int)(nClipTop +1)  / 2 * 2;
 
    //脪脝露炉碌陆脰赂露篓脦禄脰脙
    pNv21Source0 += clipTop * nWidth + clipLeft;
 
    //赂麓脰脝Y
    for (i=0; i<nClipHeight; i++)
    {
        memcpy(pNv21Dest, pNv21Source0, nClipWidth);
        pNv21Source0 += nWidth;
        pNv21Dest    += nClipWidth;
    }
 
    //赂麓脰脝U/V
    pNv21Source0  = pNv21Source + nWidth*nHeight;
    pNv21Source0 += (clipTop * nWidth/2 + clipLeft);
    //鹿脴录眉露镁脰庐露镁拢潞
    for (i=0; i<nClipHeight/2; i++)
    {
        memcpy(pNv21Dest, pNv21Source0, nClipWidth);
        pNv21Source0 += nWidth;
        pNv21Dest    += nClipWidth;
    }
    return 0;
}


int yuv420p_to_yuv420spback(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height)
{
	if(yuv420p==NULL)
		return 0;
	int i=0,j=0;
	for(i=0;i<width*height;i++)
	{
		yuv420sp[i]=yuv420p[i];
	}

	int m=0,n=0;
	for(j=0;j<width*height/2;j++)
	{
		if(j%2==0)
		{
			yuv420sp[j+width*height]=yuv420p[width*height+m];
			m++;
		}
		else 
		{
			yuv420sp[j+width*height]=yuv420p[width*height*5/4+n];
			n++;
		} 
	}  
}


int yuv420p_to_yuv420sp(unsigned char * yuv420p,unsigned char* yuv420sp,int width,int height)
{
	if(yuv420p==NULL)
		return -1;
	int i=0,j=0;
	for(i=0;i<width*height;i++)
	{
		yuv420sp[i]=yuv420p[i];
	}

	int m=0,n=0;
#if 0	
	for(j=0;j<width*height/2;j++)
	{
		if(j%2==0)
		{
			yuv420sp[j+width*height]=yuv420p[width*height+m];
			m++;
		}
		else 
		{
			yuv420sp[j+width*height]=yuv420p[width*height*5/4+n];
			n++;
		} 
	} 
#else
	for(j=0;j<width*height/2;j++)
	{
		if(j%2==0)
		{
			

			yuv420sp[j+width*height]=yuv420p[width*height*5/4+n];
			n++;
		}
		else 
		{
			yuv420sp[j+width*height]=yuv420p[width*height+m];
			m++;
		} 
	} 
	#endif
}

void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height)  
{  
    int i, j;  
    int y_size = width * height;  
  
    unsigned char* y = yuv420sp;  
    unsigned char* uv = yuv420sp + y_size;  
  
    unsigned char* y_tmp = yuv420p;  
    unsigned char* u_tmp = yuv420p + y_size;  
    unsigned char* v_tmp = yuv420p + y_size * 5 / 4;  
  
    // y  
    memcpy(y_tmp, y, y_size);  
  
    // u  
    for (j = 0, i = 0; j < y_size/2; j+=2, i++)  
    {  
        v_tmp[i] = uv[j];  
        u_tmp[i] = uv[j+1];  
    }  

}


int tyuv2jpeg(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** jpeg_buffer, unsigned long* jpeg_size, int quality)
{
    tjhandle handle = NULL;
    int flags = 0;
    int padding = 1;
    int need_size = 0;
    int ret = 0;
 
    handle = tjInitCompress();
   
    flags |= 0;
    need_size = tjBufSizeYUV2(width, padding, height, subsample);
    if (need_size != yuv_size)
    {
        printf("we detect yuv size: %d, but you give: %d, check again.\n", need_size, yuv_size);
        return 0;
    }
 
    ret = tjCompressFromYUV(handle, yuv_buffer, width, padding, height, subsample, jpeg_buffer, jpeg_size, quality, flags);
    if (ret < 0)
    {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }
 
    tjDestroy(handle);
 
    return ret;

}


int YUV422To420(unsigned char yuv422[], unsigned char yuv420[], int width, int height)  
{          
  
       int ynum=width*height;  
       int i,j,k=0;  
    //得到Y分量  
       for(i=0;i<ynum;i++){  
           yuv420[i]=yuv422[i];  
       }  
    //得到U分量  
       for(i=0;i<height;i++){  
           if((i%2)!=0)continue;  
           for(j=0;j<(width/2);j++){  
               if((4*j+1)>(2*width))break;  
               yuv420[ynum+k*2*width/4+j]=yuv422[i*2*width+4*j+1];  
                       }  
            k++;  
       }  
       k=0;  
    //得到V分量  
       for(i=0;i<height;i++){  
           if((i%2)==0)continue;  
           for(j=0;j<(width/2);j++){  
               if((4*j+3)>(2*width))break;  
               yuv420[ynum+ynum/4+k*2*width/4+j]=yuv422[i*2*width+4*j+3];  
                
           }  
            k++;  
       }    
       return 1;  
} 

int yv16_to_i420(unsigned char* pImageBuf, int width, int height) 
{ 
    int i, j; 
    int y_size,uvw;
    unsigned char* p_u; 
    unsigned char* p_v; 
   if(NULL == pImageBuf)
        return -1;
    uvw = width>>1;
    y_size = width * height; 
    p_u  = pImageBuf + y_size;
    p_v  = p_u + (y_size>>1);
    i = uvw;
 
    for(j = 2;j<height;j+=2)
    {
       memcpy(p_u + i,p_u+j*uvw, uvw);
       i+= uvw;
    }
    p_u += (y_size>>2);
    i = 0;
    for (j = 0; j < height; j+=2) 
    {
       memcpy(p_u + i, p_v+j*uvw,uvw);
       i+= uvw;
    }
    return 0;
}



int tjpeg2yuv(unsigned char* jpeg_buffer, int jpeg_size, unsigned char** yuv_buffer, int* yuv_size, int* yuv_type)
{
    tjhandle handle = NULL;
    int width, height, subsample, colorspace;
    int flags = 0;
    int padding = 1;
    int ret = 0;
 
    handle = tjInitDecompress();
	if (NULL == handle) {
	  return -1;
	}
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);
 
   // printf("w: %d h: %d subsample: %d color: %d\n", width, height, subsample, colorspace);
    
    flags |= 0;
    
    *yuv_type = subsample;
    // 注：经测试，指定的yuv采样格式只对YUV缓冲区大小有影响，实际上还是按JPEG本身的YUV格式来转换的
    *yuv_size = tjBufSizeYUV2(width, padding, height, subsample);
	//printf("yuv420 sizeof = [%d]\n",*yuv_size);
    *yuv_buffer =(unsigned char *)malloc(*yuv_size);
    if (*yuv_buffer == NULL)
    {
        printf("malloc buffer for rgb failed.\n");
		tjDestroy(handle);
        return -1;
    }
	
    ret = tjDecompressToYUV2(handle, jpeg_buffer, jpeg_size, *yuv_buffer, width,
			padding, height, flags);
    if (ret < 0)
    {
        printf("compress to jpeg failed: %s\n", tjGetErrorStr());
    }
    tjDestroy(handle);
 
    return ret;
}

int trgb2yuv(unsigned char* rgb_buffer, int width, int height, unsigned char** yuv_buffer, int* yuv_size, int subsample)
{
    tjhandle handle = NULL;
    int flags = 0;
    int padding = 1; 
    int pixelfmt = TJPF_RGB;
    int ret = 0;
 
    handle = tjInitCompress();
   
    flags |= 0;
 
    *yuv_size = tjBufSizeYUV2(width, padding, height, subsample);
 
    *yuv_buffer =(unsigned char *)malloc(*yuv_size);
    if (*yuv_buffer == NULL)
    {
        printf("malloc buffer for rgb failed.\n");
        return -1;
    }
    ret = tjEncodeYUV3(handle, rgb_buffer, width, 0, height, pixelfmt, *yuv_buffer, padding, subsample, flags);
    if (ret < 0)
    {
        printf("encode to yuv failed: %s\n", tjGetErrorStr());
    }
 
    tjDestroy(handle);
 
    return ret;
}
 
int tyuv2rgb(unsigned char* yuv_buffer, int yuv_size, int width, int height, int subsample, unsigned char** rgb_buffer, int* rgb_size)
{
    tjhandle handle = NULL;
    int flags = 0;
    int padding = 1;
    int pixelfmt = TJPF_RGB;
    int need_size = 0;
    int ret = 0;
 
    handle = tjInitDecompress();
   
    flags |= 0;
 
    need_size = tjBufSizeYUV2(width, padding, height, subsample);
    if (need_size != yuv_size)
    {
        printf("we detect yuv size: %d, but you give: %d, check again.\n", need_size, yuv_size);
        return -1;
    }
 
    *rgb_size = width*height*tjPixelSize[pixelfmt];
 
    *rgb_buffer =(unsigned char *)malloc(*rgb_size);
    if (*rgb_buffer == NULL)
    {
        printf("malloc buffer for rgb failed.\n");
        return -1;
    }
    ret = tjDecodeYUV(handle, yuv_buffer, padding, subsample, *rgb_buffer, width, 0, height, pixelfmt, flags);
    if (ret < 0)
    {
        printf("decode to rgb failed: %s\n", tjGetErrorStr());
    }
 
    tjDestroy(handle);
 
    return ret;
}

int tjpeg2rgb(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* rgb_buffer, int* size)
{
    tjhandle handle = NULL;
    int width, height, subsample, colorspace;
	int outwidth,outheight;
    int flags = 0;
    int pixelfmt = TJPF_BGR;
	tjscalingfactor sf;

	sf.num = 1;
  	sf.denom =1;
	
 
    handle = tjInitDecompress();
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);

	outwidth = TJSCALED(width, sf);
	outheight = TJSCALED(height, sf);
	printf("imagew:%d,imageh:%d outw :%d outh:%d\n",width,height,outwidth,outheight);
    flags |= 0;
    tjDecompress2(handle, jpeg_buffer, jpeg_size, rgb_buffer, outwidth, 0,
            outheight, pixelfmt, flags);

    tjDestroy(handle);
 
    return 0;

}

void NV21_TO_RGB24(unsigned char *data, unsigned char *rgb, int width, int height) 
{
    int index = 0;

    unsigned char *ybase = data;
    unsigned char *ubase = &data[width * height];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u_char Y = ybase[x + y * width];
            u_char U = ubase[y / 2 * width + (x / 2) * 2 + 1];
            u_char V = ubase[y / 2 * width + (x / 2) * 2];

            rgb[index++] = Y + 1.402 * (V - 128); //R
            rgb[index++] = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128); //G
            rgb[index++] = Y + 1.772 * (U - 128); //B
        }
    }
}





int trgb2jpeg(unsigned char* rgb_buffer, int width, int height, int quality, unsigned char** jpeg_buffer, unsigned long* jpeg_size)
{
    tjhandle handle = NULL;
    //unsigned long size=0;
    int flags = 0;
    int subsamp = TJSAMP_422;
    int pixelfmt = TJPF_RGB;
 
    handle=tjInitCompress();
    //size=tjBufSize(width, height, subsamp);
    tjCompress2(handle, rgb_buffer, width, 0, height, pixelfmt, jpeg_buffer, jpeg_size, subsamp,
            quality, flags);
 
    tjDestroy(handle);
 
    return 0;
}

int YUVToJpeg(unsigned char* yuv420sp,int width, int height,unsigned char** jpeg_buffer,unsigned long* jpeg_size)
{
	if(NULL == yuv420sp)
	{
		printf("yuv420sp is NULL \n");
		return -1;
	}

#if 1	
	int nSize = width * height * 3 / 2;
	//printf("nSize = %d\n",nSize);
	if (nSize + 1 < 0)
	{
		return -1;
	}
	unsigned char *yuv420p = (unsigned char *)malloc(nSize + 1);
	if(NULL == yuv420p)
	{
		printf("yuv420p malloc error\n");
		return -1;
	}

	
	yuv420sp_to_yuv420p(yuv420sp, yuv420p, width, height);
	tyuv2jpeg(yuv420p, nSize, width, height, TJSAMP_420, jpeg_buffer, jpeg_size, 100);
	if(NULL != yuv420p)
		free(yuv420p);
#endif

	return 0;
}

#if 1

int jpegcrop(unsigned char *jpegBuf, unsigned long jpegSize,int cropx,int cropy,int cropw,int croph,unsigned char **dstBuf,unsigned long *dstSize)
{

	unsigned char *yuv;
	int yuvlen,yuv_type;
	unsigned char *yuvbuffer420;
	
	yuvbuffer420 = (unsigned char*)malloc(1920*1080*3/2);
    if(NULL == yuvbuffer420)
    {
        printf("pJpeg_file malloc fail!\n");
        return -1;
    }
	tjpeg2yuv(jpegBuf,jpegSize,&yuv,&yuvlen,&yuv_type);
	yuv420p_to_yuv420sp(yuv,yuvbuffer420,1920,1080);
	int size = cropw * croph * 3 /2;
	unsigned char yuvcrop[size];
	unsigned char *jpeg_buf = NULL;
	unsigned long jpeg_size = 0;
	clipNv21ToNv21(yuvbuffer420,1920, 1080, yuvcrop,cropx,cropy,cropw,croph);
	YUVToJpeg(yuvcrop,cropw,croph,dstBuf,dstSize);
	free(yuv);
	free(yuvbuffer420);
}

#else
int jpegcrop(unsigned char *jpegBuf, unsigned long jpegSize,int cropx,int cropy,int cropw,int croph,unsigned char *dstBuf,unsigned long *dstSize)
{
	if(NULL == jpegBuf || 0 > jpegSize)
	{
		printf("jpegcrop jpegBuf is NULL \n");
		return -1;
	}
	tjhandle tjInstance = NULL;
	tjtransform xform;
	memset(&xform, 0, sizeof(tjtransform));
	xform.r.x = (cropx /16 * 16) ;
	xform.r.y = (cropy /16 *16);
	xform.r.w = cropw;
	xform.r.h = croph;
    xform.options |=TJXOPT_CROP;
	int flags = 0;
	
	if ((tjInstance = tjInitTransform()) == NULL)
	{
		printf("initializing transformer");
	}
	xform.options |=TJXOPT_TRIM;
	if (tjTransform(tjInstance, jpegBuf, jpegSize, 1, &dstBuf, dstSize,&xform, TJFLAG_ACCURATEDCT) < 0)
	{
        printf("transforming input image = %s\n",tjGetErrorStr());
		tjDestroy(tjInstance);
		return -1;
    }

	tjDestroy(tjInstance);
}
#endif

int jpeg_yuv_test()
{
printf("_______________________________________________\n");
	int ret = -1;
	FILE *pFile = NULL;
	int filesize = 0;
	int yuvsize= 0 ;
	int yuvtype = 0;
	unsigned char *jpegBuf;
	int pInLen;
	unsigned char *yuvbuffer;
	unsigned char *yuvbuffer420;

	if ((pFile = fopen("1.jpg", "rb")) == NULL)
    {
        printf("fopen error\n");
        return -1;
    }

	fseek(pFile, 0, SEEK_END);
    filesize = ftell(pFile);
    rewind(pFile);
	jpegBuf = (unsigned char*)malloc(filesize);
	fread(jpegBuf, 1, filesize, pFile);
    pInLen = filesize;

	unsigned char *dstBuf = NULL;
    unsigned long dstSize = 0;
	tjhandle tjInstance = NULL;
	tjtransform xform;
	memset(&xform, 0, sizeof(tjtransform));

	xform.r.x = 0;
	xform.r.y = 0;
	xform.r.w = 400;
	xform.r.h = 400;
	//xform.op = TJXOP_NONE;
    xform.options |=TJXOPT_CROP;
	//xform.customFilter=NULL;
	

	int flags = 0;
	
	if ((tjInstance = tjInitTransform()) == NULL)
		printf("initializing transformer");
	if (tjTransform(tjInstance, jpegBuf, pInLen, 1, &dstBuf, &dstSize,
                       &xform, flags) < 0)
        printf("transforming input image = %s\n",tjGetErrorStr());
        tjFree(jpegBuf);

	FILE *YpFile1 = NULL;	
	if ((YpFile1 = fopen("crop.jpg", "wb")) == NULL)
	{
		printf("fopen error\n");
		return -1;
	}
	ret = fwrite(dstBuf,1,dstSize,YpFile1);
	fclose(YpFile1);
	free(dstBuf);
	//tjFree(jpegBuf);
	tjDestroy(tjInstance);

printf("_______________________________________________\n");	
# if 0
	yuvbuffer420 = (unsigned char*)malloc(5*1024*1024);
    if(NULL == yuvbuffer420)
    {
        printf("pJpeg_file malloc fail!\n");
        return NULL;
    }
	pFileData = (unsigned char*)malloc(5*1024*1024);
    if(NULL == pFileData)
    {
        printf("pJpeg_file malloc fail!\n");
        return NULL;
    }
	if ((pFile = fopen("test4.jpg", "rb")) == NULL)
    {
        printf("fopen error\n");
        return -1;
    }

	fseek(pFile, 0, SEEK_END);
    filesize = ftell(pFile);
    rewind(pFile);

	fread(pFileData, 1, filesize, pFile);
    pInLen = filesize;

    fclose(pFile);
    pFile = NULL;

	tjpeg2yuv(pFileData,filesize, &yuvbuffer, &yuvsize, &yuvtype);
	yv16_to_i420(yuvbuffer,1920,1080);
	yuv420p_to_yuv420sp(yuvbuffer,yuvbuffer420,1920,1080);
	FILE *YpFile1 = NULL;	
	if ((YpFile1 = fopen("test4.yuv", "wb")) == NULL)
	{
		printf("fopen error\n");
		return -1;
	}
	printf("yuv420sp size = [%d]\n",1920*1080*3/2);
	ret = fwrite(yuvbuffer420,1,1920*1080*3/2,YpFile1);
	fclose(YpFile1);
#endif
}

#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>

typedef unsigned char uchar;
typedef struct __tjp_info{
  int outwidth;
  int outheight;
  unsigned long jpg_size;
}_tjp_info_t;

/*获取当前ms数*/
static int get_timer_now ()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return(now.tv_sec * 1000 + now.tv_usec / 1000);
}

/*读取文件到内存*/
uchar *read_file2buffer(char *filepath, _tjp_info_t *tinfo)
{
  FILE *fd;
  struct stat fileinfo;
  stat(filepath,&fileinfo);
  tinfo->jpg_size = fileinfo.st_size;

  fd = fopen(filepath,"rb");
  if (NULL == fd) {
    printf("file not open\n");
    return NULL;
  }
  uchar *data = (uchar *)malloc(sizeof(uchar) * fileinfo.st_size);
  fread(data,1,fileinfo.st_size,fd);
  fclose(fd);
  return data;
}

/*写内存到文件*/
void write_buffer2file(char *filename, uchar *buffer, int size)
{
  FILE *fd = fopen(filename,"wb");
  if (NULL == fd) {
    return;
  }
  fwrite(buffer,1,size,fd);
  fclose(fd);
}

/*图片解压缩*/
uchar *tjpeg_decompress(uchar *jpg_buffer, _tjp_info_t *tinfo, tjscalingfactor factor)
{
  tjhandle handle = NULL;
  int img_width,img_height,img_subsamp,img_colorspace;
  int flags = 0, pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle = tjInitDecompress();
  if (NULL == handle)  {
    return NULL;
  }
  /*获取jpg图片相关信息但并不解压缩*/
  int ret = tjDecompressHeader3(handle,jpg_buffer,tinfo->jpg_size,&img_width,&img_height,&img_subsamp,&img_colorspace);
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  /*输出图片信息*/
 // printf("jpeg width:%d\n",img_width);
 // printf("jpeg height:%d\n",img_height);
 // printf("jpeg subsamp:%d\n",img_subsamp);
 // printf("jpeg colorspace:%d\n",img_colorspace);
  /*计算1/4缩放后的图像大小,若不缩放，那么直接将上面的尺寸赋值给输出尺寸*/
  
  tinfo->outwidth = TJSCALED(img_width, factor);
  tinfo->outheight = TJSCALED(img_height, factor);
  //printf("w:%d,h:%d\n",tinfo->outwidth,tinfo->outheight);
  /*解压缩时，tjDecompress2（）会自动根据设置的大小进行缩放，但是设置的大小要在它的支持范围，如1/2 1/4等*/
  flags |= 0;
  int size = tinfo->outwidth * tinfo->outheight * 3;
  uchar *rgb_buffer = (uchar *)malloc(sizeof(uchar) * size);
  ret = tjDecompress2(handle, jpg_buffer, tinfo->jpg_size, rgb_buffer, tinfo->outwidth, 0,tinfo->outheight, pixelfmt, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  tjDestroy(handle);
  return rgb_buffer;
}

/*压缩图片*/
int tjpeg_compress(uchar *rgb_buffer, _tjp_info_t *tinfo, int quality, uchar **outjpg_buffer, unsigned long *outjpg_size)
{
  tjhandle handle = NULL;
  int flags = 0;
  int subsamp = TJSAMP_422;
  int pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle=tjInitCompress();
  if (NULL == handle) {
    return -1;
  }
  /*将rgb图或灰度图等压缩成jpeg格式图片*/
  int ret = tjCompress2(handle, rgb_buffer,tinfo->outwidth,0,tinfo->outheight,pixelfmt,outjpg_buffer,outjpg_size,subsamp,quality, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return -1;
  }
  tjDestroy(handle);
  return 0;
}

int jpg_compress(unsigned char *jpeg_buf,unsigned long jpg_size, int quality, tjscalingfactor factor,
	unsigned char **outjpg_buffer, unsigned long *outjpg_size,int *outjpegw,int *outjpegh)
{
	if(NULL == jpeg_buf || 0 > jpg_size)
	{
		printf("jpg_compress is NULL\n");
		return -1;
	}
	_tjp_info_t tinfo;
	tinfo.jpg_size = jpg_size;
	uchar *rgb = tjpeg_decompress(jpeg_buf,&tinfo,factor);
    if (NULL == rgb) 
	{
        printf("error\n");
    	return -1;
    }
	tjpeg_compress(rgb,&tinfo,quality,outjpg_buffer,outjpg_size);
	*outjpegw = tinfo.outwidth;
	*outjpegh = tinfo.outheight;
	free(rgb);
}

typedef struct
{
	unsigned char *data;
	int size;
	int offset;
}ImageSource;

static void pngReadCallback(png_structp png_ptr,png_bytep data,png_size_t length)
{
	ImageSource *isource = (ImageSource *)png_get_io_ptr(png_ptr);
	if(isource->offset + length <= isource->size)
	{
		memcpy(data,isource->data+isource->offset,length);
		isource->offset += length;
	}
	else
		png_error(png_ptr,"pngReaderCallback failed");
}

 int yuv420p_to_jpeg(unsigned char* pdata, int image_width, int image_height, int quality,unsigned char *jpgbuf,unsigned long *jpglen)
{
	if (image_width == 0 || image_height == 0) {
	    printf("err param\n");
	     return 0;
	 }
	 struct jpeg_compress_struct cinfo;
	 struct jpeg_error_mgr jerr;
	 cinfo.err = jpeg_std_error(&jerr);
	 jpeg_create_compress(&cinfo);

	 unsigned char *outbuffer;
	 unsigned long size = 0;

	 jpeg_mem_dest(&cinfo, &outbuffer, &size);

	 cinfo.image_width = image_width; // image width and height, in pixels 
	 cinfo.image_height = image_height;
	 cinfo.input_components = 3; // # of color components per pixel 
	 cinfo.in_color_space = JCS_YCbCr; //colorspace of input image 
	 jpeg_set_defaults(&cinfo);
	 jpeg_set_quality(&cinfo, quality, TRUE);

	 ////////////////////////////// 
	 // cinfo.raw_data_in = TRUE; 
	 cinfo.jpeg_color_space = JCS_YCbCr;
	 cinfo.comp_info[0].h_samp_factor = 2;
	 cinfo.comp_info[0].v_samp_factor = 2;
	 ///////////////////////// 
	 jpeg_start_compress(&cinfo, TRUE);

	 JSAMPROW row_pointer[1];

#if 1
	 unsigned char *yuvbuf;
	 if ((yuvbuf = (unsigned char *)malloc(image_width * 3)) != NULL)
	     memset(yuvbuf, 0, image_width * 3);
#else
	unsigned char yuvbuf[image_width * 3];
	memset(yuvbuf, 0, image_width * 3);

#endif
	 unsigned char *ybase, *ubase, *vbase;
	 ybase = pdata;
	 ubase = pdata + image_width*image_height;
	 vbase = ubase + image_width*image_height / 4;
	 int j = 0;
	 while (cinfo.next_scanline < cinfo.image_height)
	 {
	     int idx = 0;
	     for (int i = 0; i < image_width; i++)
	     {
	         yuvbuf[idx++] = ybase[i + j * image_width];
	         yuvbuf[idx++] = ubase[j / 2 * image_width/2 + (i / 2)];
	         yuvbuf[idx++] = vbase[j / 2 * image_width/2 + (i / 2)];
	     }
	     row_pointer[0] = yuvbuf;
	     jpeg_write_scanlines(&cinfo, row_pointer, 1);
	     j++;
	 }
	 jpeg_finish_compress(&cinfo);
	 jpeg_destroy_compress(&cinfo);
	 
	 if (yuvbuf != NULL)
	 {
	     free(yuvbuf);
	 }
	
	 memcpy(jpgbuf,outbuffer,size);
	 *jpglen = size;
	 free(outbuffer);

}


int pngTojpg(unsigned char *pngdata,int pnglen,unsigned char *jpgdata,unsigned long* jpglen)
{
	png_structp png_ptr;
	png_infop info_ptr;
	int image_width, image_height;
	int color_type = 0; 

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("png_create_read_struct error\n");
		return -1;
	}
	
	info_ptr = png_create_info_struct(png_ptr);   
	if (info_ptr == NULL)
	{
		printf("png_create_info_struct error\n");
	   png_destroy_read_struct(&png_ptr, NULL, NULL);
	   return -1;
	}
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("setjmp error\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return -1;
	}

	ImageSource imageSource;
	imageSource.data    = (unsigned char*)pngdata;
	imageSource.size    = pnglen;
	imageSource.offset  = 0;
	png_set_read_fn(png_ptr, &imageSource, pngReadCallback);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_ALPHA, 0);
	
	if ((png_ptr != NULL) && (info_ptr != NULL))
	{
		/* 获取图像的宽高 */
		image_width = png_get_image_width(png_ptr, info_ptr);		
		image_height = png_get_image_height(png_ptr, info_ptr); 
		printf("width:%d,height:%d\n", image_width, image_height);

		/* 获取图像颜色类型 */
		color_type = png_get_color_type(png_ptr, info_ptr); 
		printf("color type:%d\n", color_type);

	}
	
	png_bytep *row_pointers; /* 指针数组，数组中的每一个指针都指向一行图像数据 */
	row_pointers = png_get_rows(png_ptr, info_ptr);
	unsigned char *p_image_buf;
	int image_size = 0;
	int i ,j,pos;
	if(PNG_COLOR_TYPE_RGB == color_type)
	{
	   /* 图像数据的大小 */
		image_size = image_width * image_height * 3;   
		p_image_buf = malloc(image_size);
		
		for(i = 0; i < image_height; i++)
		{
		   for(j = 0; j < (image_width * 3); j += 3)
		   {
			   *(p_image_buf + pos++) = row_pointers[i][j + 0];
			   *(p_image_buf + pos++) = row_pointers[i][j + 1];
			   *(p_image_buf + pos++) = row_pointers[i][j + 2]; 		  
		   }
		}
		unsigned char *yuv_buffer;
		int yuv_size;
		trgb2yuv(p_image_buf,image_width, image_height, &yuv_buffer, &yuv_size, TJSAMP_420);
		if(NULL !=p_image_buf)
		{
			printf("p_image_buf free \n");
			free(p_image_buf);
			p_image_buf = NULL;
		}
		yuv420p_to_jpeg(yuv_buffer,image_width,image_height,100,jpgdata,jpglen);
		if(NULL !=yuv_buffer)
		{
			free(yuv_buffer);
			yuv_buffer = NULL;
		}

	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	printf("pngTojpg end \n");
	return 0;
}
#if 0
/*测试程序*/
int tj_test()
{
	_tjp_info_t tinfo;
	char *filename = "./tjout.jpg";
	int start = get_timer_now();
	/*读图像*/
	uchar *jpeg_buffer = read_file2buffer(filename,&tinfo);
	int rend = get_timer_now();
	printf("loadfile make time:%d\n",rend-start);
	if (NULL == jpeg_buffer) {
	    printf("read file failed\n");
	    return 1;
	}
	unsigned char *outjpg_buffer;
	unsigned long outjpg_size;
	int outjpegw;
	int outjpegh;

	printf("w = %d h = %d\n",tinfo.outwidth,tinfo.outheight);
	tjscalingfactor factor = {0};
	factor.num = 1;
	factor.denom = 1;
	jpg_compress(jpeg_buffer,tinfo.jpg_size, 100, factor, &outjpg_buffer,&outjpg_size,&outjpegw,&outjpegh);
	char *outfile = "./123.jpg";
	write_buffer2file(outfile,outjpg_buffer,outjpg_size);
	printf("w = %d h = %d outjpg_size = %ld\n",outjpegw,outjpegh,outjpg_size);

	free(jpeg_buffer);
	return 0;
}
#endif

