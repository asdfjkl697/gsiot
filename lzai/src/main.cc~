/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.cc
 * Copyright (C) 2016 chen <chen@chen-X450CC>
 * 
 * lzai is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * lzai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILENAME   "curlposttest.log"  

void HtmlToText(char* inbuffer,char* outbuffer)
{
char* p = outbuffer;
bool bIsText = false;
while(*inbuffer)
{
if(*inbuffer == '<') 
{
bIsText = false;
}
else if(*inbuffer == '>')
{
bIsText = true;
inbuffer++;
continue;
};
if(bIsText)
{
*p = *inbuffer;
p++;
*p = '\0';
}
inbuffer++;
}

} 
  
size_t write_data(void* buffer,size_t size,size_t nmemb,void *stream)  
{  
    FILE *fptr = (FILE*)stream;  
    fwrite(buffer,size,nmemb,fptr);  
    return size*nmemb;  
}  

int main(void)
{
  CURL *curl;
  CURLcode res;
  char inBuffer[1024*10],outBuffer[1024*10];

  FILE *fp,*fptxt;
  if (!(fp = fopen ("info.html", "w+"))) {
        perror("fopen error:");
        exit (EXIT_FAILURE);
  } 
  if (!(fptxt = fopen ("info.txt", "w+"))) {
        perror("fopen error:");
        exit (EXIT_FAILURE);
  }
	

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl) {
	  curl_easy_setopt(curl,CURLOPT_URL, "http://www.baidu.com/s?wd=sister");
	  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data); //对返回的数据进行操作的函数地址  
      curl_easy_setopt(curl,CURLOPT_WRITEDATA,fp); //这是write_data的第四个参数值  
	  //curl_easy_setopt(curl,CURLOPT_HEADERDATA, fp); //将返回的html主体数据输出到fp指向的文件

#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
	  
	printf("get ok! \n");

	fread(inBuffer,1, sizeof(inBuffer), fp);	
	HtmlToText(inBuffer,outBuffer);
	char testbuffer[20] = {"good job!"};
	//fwrite(testbuffer,1,sizeof(outBuffer),fptxt);
	fwrite(outBuffer,1, sizeof(outBuffer), fptxt);
	printf("change ok! \n");
	printf("%s\n",testbuffer);
	fclose(fptxt);
	fclose(fp);
    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();

  return 0;
}

