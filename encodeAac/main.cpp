
#include "recorder.h"
#include "player.h"
#include "faadtest.h"

#include <faac.h>

typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef char            _TCHAR;

#define BUFFER_SIZE 1024*100
#include<unistd.h>

int main(int argc, char *argv[])
{
    ULONG nSampleRate = SAMPLERATE;  // 采样率
    UINT nChannels = CHANNELS;         // 声道数
    UINT nPCMBitSize = PCMBITSIZE;      // 单样本位数
    ULONG nInputSamples = 0;
    ULONG nMaxOutputBytes = 0;

    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration;

    int nBytesRead;
    int nPCMBufferSize;
    BYTE* pbPCMBuffer;
    BYTE* pbAACBuffer;


    FILE* fpOut; // AAC file for output
    fpOut = fopen("out.aac", "w+");
    FILE* fpPcm = fopen("out.pcm", "w+");


    // (1) Open FAAC engine
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if(hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }

    nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    pbPCMBuffer = new BYTE [nPCMBufferSize];
    pbAACBuffer = new BYTE [nMaxOutputBytes];

    // (2.1) Get current encoding configuration
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;

    // (2.2) Set encoding configuration
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);

    Recorder recorder;
    recorder.initRecoder();

    char *buffer = (char*)malloc(2048*2*2*2);

    int audio_samples_per_frame = 32;
    int audio_sample_rate = nSampleRate;
//    int audio_pts_increment = (90000 * audio_samples_per_frame) / audio_sample_rate; //一秒钟采集多少次
    int audio_pts_increment = audio_samples_per_frame * 2;
    audio_pts_increment /= 2; //单声道除以2

    char *aacplaybuf = (char*)malloc(1024*1024*4);
    int aacplaylen=0;
    char *pcmplaybuf = (char*)malloc(1024*1024*2);
    int pcmplaylen=0;

    int i =0;
    for(;i<15*audio_pts_increment;i++) //采集15秒
    {
        fprintf(stderr,"%d\n",audio_pts_increment);

        int size;
        for (size = 0; ;)
        {
            char *buf = (char*)(buffer+size);
            int readSize = recorder.recode(buf,32);

            size += readSize *2;

            //fprintf(stderr,"readSize=%d size=%d\n",readSize,size);
            if (size >= 2048) break;
        }

        nBytesRead = size;

        // 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
        nInputSamples = nBytesRead / (nPCMBitSize / 8);

        fprintf(stderr,"nInputSamples %d\n",nInputSamples);


        // (3) Encode
        nRet = faacEncEncode(
        hEncoder, (int*) buffer, nInputSamples, pbAACBuffer, nMaxOutputBytes);

        //fwrite(pesBuffer, 1, size, fpPes);
        fwrite(pbAACBuffer, 1, nRet, fpOut);
        memcpy(&aacplaybuf[aacplaylen],pbAACBuffer,nRet);
        aacplaylen+=nRet;

        fwrite(buffer, 1, size, fpPcm);

        fprintf(stderr,"%d: faacEncEncode returns %d\n", i, nRet);

        fprintf(stderr,"read size=%d\n",nBytesRead);
        usleep(10000);
    }

    Player playtest;
    playtest.initPlayer();

//    fseek(fpOut,0L,SEEK_SET);
//    accdecoder(fpOut,fpPcm);
//    fseek(fpPcm,0L,SEEK_END);
//    int length=ftell(fpPcm);
//    fseek(fpPcm,0L,SEEK_SET);
//    pcmplaylen=fread(pcmplaybuf,1,length,fpPcm);

    pcmplaylen=accdecoder2(aacplaybuf,aacplaylen,pcmplaybuf);
    playtest.play(pcmplaybuf,pcmplaylen);

    fclose(fpPcm);
    free(pcmplaybuf);
    free(aacplaybuf);
    // (4) Close FAAC engine
    nRet = faacEncClose(hEncoder);

    delete[] pbAACBuffer;
    fclose(fpOut);
    free(buffer);

    return 0;
}






