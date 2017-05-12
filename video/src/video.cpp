/*
* camera_yuv_x264.cpp
*/

#include <stdint.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define CLEAR(x) (memset((&x),0,sizeof(x)))
#define IMAGE_WIDTH   320
#define IMAGE_HEIGHT  240
#define ENCODER_PRESET "veryfast"
#define widthStep 960
//widthStep为图像宽度的3倍
/*配置参数
* 使用默认参数，在这里使用了zerolatency的选项，使用这个选项之后，就不会有
* delayed_frames，如果你使用不是这个的话，还需要在编码完成之后得到缓存的
* 编码帧
*/
#define ENCODER_TUNE   "zerolatency"
#define ENCODER_PROFILE  "baseline"
#define ENCODER_COLORSPACE X264_CSP_I420

extern "C"
{

#include <x264.h>

    typedef struct my_x264_encoder{
        x264_param_t  * x264_parameter;
        char parameter_preset[20];
        char parameter_tune[20];
        char parameter_profile[20];
        x264_t  * x264_encoder;
        x264_picture_t * yuv420p_picture;
        long colorspace;
        unsigned char *yuv;
        x264_nal_t * nal;
    } my_x264_encoder;

}


//global variable
char *write_filename = "test.264";
int ii = 1000; //frames_total，这里设置你需要录制多少帧
IplImage * img;
CvCapture * cap;
unsigned char * RGB1;

//在RGB2YUV.cpp中
void Convert(unsigned char *RGB, unsigned char *YUV, unsigned int width, unsigned int height);

void getyuv(unsigned char *yuv)
{
    img = cvQueryFrame(cap);
    for (int i = 0; i < IMAGE_HEIGHT; i++)
    {
        for (int j = 0; j < IMAGE_WIDTH; j++)
        {
            RGB1[(i*IMAGE_WIDTH + j) * 3] = img->imageData[i * widthStep + j * 3 + 2];;
            RGB1[(i*IMAGE_WIDTH + j) * 3 + 1] = img->imageData[i * widthStep + j * 3 + 1];
            RGB1[(i*IMAGE_WIDTH + j) * 3 + 2] = img->imageData[i * widthStep + j * 3];
        }
    }
    Convert(RGB1, yuv, IMAGE_WIDTH, IMAGE_HEIGHT);
}


int main(int argc, char **argv)
{
    int ret;

    my_x264_encoder * encoder = (my_x264_encoder *)malloc(sizeof(my_x264_encoder));
    if (!encoder){
        printf("cannot malloc my_x264_encoder !\n");
        exit(EXIT_FAILURE);
    }
    CLEAR(*encoder);

    //摄像头初始化
    cap = cvCreateCameraCapture(1);
    cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, IMAGE_WIDTH);
    cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, IMAGE_HEIGHT);

    //分配RGB空间，用于提取IplImage中的image_data
    RGB1 = (unsigned char *)malloc(IMAGE_HEIGHT * IMAGE_WIDTH * 3);

    /****************************************************************************
    * encoder结构体初始化
    ****************************************************************************/
    strcpy(encoder->parameter_preset, ENCODER_PRESET);
    strcpy(encoder->parameter_tune, ENCODER_TUNE);

    encoder->x264_parameter = (x264_param_t *)malloc(sizeof(x264_param_t));
    if (!encoder->x264_parameter){
        printf("malloc x264_parameter error!\n");
        exit(EXIT_FAILURE);
    }

    /*初始化编码器*/
    CLEAR(*(encoder->x264_parameter));
    x264_param_default(encoder->x264_parameter);

    if ((ret = x264_param_default_preset(encoder->x264_parameter, encoder->parameter_preset, encoder->parameter_tune))<0){
        printf("x264_param_default_preset error!\n");
        exit(EXIT_FAILURE);
    }

    /*cpuFlags 去空缓冲区继续使用不死锁保证*/
    encoder->x264_parameter->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
    /*视频选项*/
    encoder->x264_parameter->i_width = IMAGE_WIDTH;//要编码的图像的宽度
    encoder->x264_parameter->i_height = IMAGE_HEIGHT;//要编码的图像的高度
    encoder->x264_parameter->i_frame_total = 0;//要编码的总帧数，不知道用0
    encoder->x264_parameter->i_keyint_max = 10;//一般为i_fps_num的一倍或两倍，视频文件总时间=总帧数/该值，time = ii / i_keyint_max;
    /*流参数*/
    encoder->x264_parameter->i_bframe = 5;
    encoder->x264_parameter->b_open_gop = 0;
    encoder->x264_parameter->i_bframe_pyramid = 0;
    encoder->x264_parameter->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

    /*log参数，不需要打印编码信息时直接注释掉*/
    encoder->x264_parameter->i_log_level = X264_LOG_DEBUG;

    encoder->x264_parameter->i_fps_num = 10;//码率分子
    encoder->x264_parameter->i_fps_den = 1;//码率分母

    encoder->x264_parameter->b_intra_refresh = 1;
    encoder->x264_parameter->b_annexb = 1;


    strcpy(encoder->parameter_profile, ENCODER_PROFILE);
    if ((ret = x264_param_apply_profile(encoder->x264_parameter, encoder->parameter_profile))<0){
        printf("x264_param_apply_profile error!\n");
        exit(EXIT_FAILURE);
    }
    /*打开编码器*/
    encoder->x264_encoder = x264_encoder_open(encoder->x264_parameter);
    encoder->colorspace = ENCODER_COLORSPACE;

    /*初始化pic*/
    encoder->yuv420p_picture = (x264_picture_t *)malloc(sizeof(x264_picture_t));
    if (!encoder->yuv420p_picture){
        printf("malloc encoder->yuv420p_picture error!\n");
        exit(EXIT_FAILURE);
    }
    if ((ret = x264_picture_alloc(encoder->yuv420p_picture, encoder->colorspace, IMAGE_WIDTH, IMAGE_HEIGHT))<0){
        printf("ret=%d\n", ret);
        printf("x264_picture_alloc error!\n");
        exit(EXIT_FAILURE);
    }

    encoder->yuv420p_picture->img.i_csp = encoder->colorspace;
    encoder->yuv420p_picture->img.i_plane = 3;
    encoder->yuv420p_picture->i_type = X264_TYPE_AUTO;

    /*申请YUV buffer*/
    encoder->yuv = (uint8_t *)malloc(IMAGE_WIDTH*IMAGE_HEIGHT * 3 / 2);
    if (!encoder->yuv){
        printf("malloc yuv error!\n");
        exit(EXIT_FAILURE);
    }
    CLEAR(*(encoder->yuv));
    encoder->yuv420p_picture->img.plane[0] = encoder->yuv;
    encoder->yuv420p_picture->img.plane[1] = encoder->yuv + IMAGE_WIDTH*IMAGE_HEIGHT;
    encoder->yuv420p_picture->img.plane[2] = encoder->yuv + IMAGE_WIDTH*IMAGE_HEIGHT + IMAGE_WIDTH*IMAGE_HEIGHT / 4;

    encoder->nal = (x264_nal_t *)calloc(2,sizeof(x264_nal_t));//一般一帧分为两个nal，不行的话就改3-8
    if (!encoder->nal){
        printf("malloc x264_nal_t error!\n");
        exit(EXIT_FAILURE);
    }
    CLEAR(*(encoder->nal));
    //////////////////////////////////////////////////////////////////////////////////
    /************************* init finished ****************************************/

    int fd_write;
    if ((fd_write = open(write_filename, O_WRONLY  | O_CREAT, 0777))<0){     //| O_APPEND
        printf("cannot open output file!\n");
        exit(EXIT_FAILURE);
    }
    int n_nal = 0;
    x264_picture_t pic_out;
    x264_nal_t *my_nal;

    /*Encode*/
    while (ii--)
    {
        getyuv(encoder->yuv);
        encoder->yuv420p_picture->i_pts++;
        if ((ret = x264_encoder_encode(encoder->x264_encoder, &encoder->nal, &n_nal, encoder->yuv420p_picture, &pic_out))<0){
            printf("x264_encoder_encode error!\n");
            exit(EXIT_FAILURE);
        }
        for (my_nal = encoder->nal; my_nal<encoder->nal + n_nal; ++my_nal){
            write(fd_write, my_nal->p_payload, my_nal->i_payload);
        }
    }

    free(encoder->yuv);
    free(encoder->yuv420p_picture);
    free(encoder->x264_parameter);
    x264_encoder_close(encoder->x264_encoder);
    free(encoder);
    //close(fd_read); //jyc20170510
    close(fd_write);

    return 0;
}
