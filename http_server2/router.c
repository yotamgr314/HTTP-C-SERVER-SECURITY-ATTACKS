#include "httpd.h"
#include <unistd.h>
#include <stdio.h>
#include<stdio.h> 
#include<unistd.h> 
#include<fcntl.h> 
#include<stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define BUF_SIZE 1024

void route()
{
    ROUTE_START()
 fprintf(stderr,"1\n");
ROUTE_GET("/eran"){

}

 /*   ROUTE_GET("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Hello! You are using %s", request_header("User-Agent"));
    }*/

    ROUTE_GET("/")
    {
 printf("HTTP/1.1 200 OK\r\n\r\n");
        //printf("Hello! you got here %s , %s %s",method,uri,qs);
         int fd = open("page1.html", O_RDONLY); 
      

        int n;
	char buffer[BUF_SIZE];

	while ((n = read(fd, buffer, BUF_SIZE)) > 0)
		if (write(STDOUT_FILENO, buffer, n) != n)
			 fprintf(stderr,"Wow write error\n");
    }

    ROUTE_POST("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Wow, seems that you POSTed %d bytes. \r\n", payload_size);
        printf("Fetch the data using `payload` variable.");
    }
    ROUTE_POST("/pass")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Wow, seems that you POSTed %d bytes. %s\r\n", payload_size,payload);

    }
  
    ROUTE_END()
}
