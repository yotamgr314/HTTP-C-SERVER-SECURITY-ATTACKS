#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <string.h>
#include <stdio.h>

// Server control
void serve_forever(const char *PORT);

// Globals set by analyze_http()
extern char *method, *uri, *qs, *prot;
extern char *payload;
extern int   payload_size;

// Helpers
char *request_header(const char *name);
void analyze_http(char *buf, int rcvd);

// Your router implementation
void route(void);

// Routing macros
#define ROUTE_START()       if (0) {
#define ROUTE(METHOD,URI)   } else if (strcmp(method, METHOD)==0 && strcmp(uri, URI)==0) {
#define ROUTE_GET(URI)      ROUTE("GET", URI)
#define ROUTE_POST(URI)     ROUTE("POST", URI)
#define ROUTE_END()         } else printf(\
                                "HTTP/1.1 500 Not Handled\r\n\r\n" \
                                "The server has no handler for the request.\r\n");

#endif // _HTTPD_H_
