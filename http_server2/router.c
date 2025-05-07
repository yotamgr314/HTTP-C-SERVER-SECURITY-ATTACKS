#include "httpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

// ----------------------------------------------------------------------------
// Extracts "username" and "password" from the POST body.
// We search for "password=" first (before we ever rewrite the buffer),
// then we search for "username=" and replace the '&' with '\0'.
// ----------------------------------------------------------------------------
static void parse_form(char *payload, char **out_user, char **out_pass) {
    *out_user = NULL;
    *out_pass = NULL;

    // 1) find password= and set out_pass
    char *p = strstr(payload, "password=");
    if (p) {
        p += strlen("password=");
        *out_pass = p;
        // null-terminate at next delimiter if any (e.g., CR, LF, '&')
        char *end = strpbrk(p, "&\r\n");
        if (end) *end = '\0';
    }

    // 2) find username= and set out_user (now safe to null the '&')
    char *u = strstr(payload, "username=");
    if (u) {
        u += strlen("username=");
        *out_user = u;
        char *amp = strchr(u, '&');
        if (amp) *amp = '\0';
    }
}

void route(void) {
    ROUTE_START()

    // ------------------ Register endpoint ------------------
    ROUTE_POST("/register") {
        // debug: show exactly what we're parsing
        fprintf(stderr,
                "[DBG] /register payload='%.*s'\n",
                payload_size, payload);

        char *user = NULL, *pass = NULL;
        parse_form(payload, &user, &pass);

        fprintf(stderr,
                "[DBG] parse_form → user='%s' pass='%s'\n",
                user ? user : "(null)",
                pass ? pass : "(null)");

        if (!user || !pass) {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Error: malformed form data.\n");
            return;
        }

        FILE *f = fopen("users.db", "a");
        if (!f) {
            printf("HTTP/1.1 500 Internal Server Error\r\n\r\n");
            printf("Error: could not open users.db for writing.\n");
            return;
        }
        fprintf(f, "%s:%s\n", user, pass);
        fclose(f);

        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Registered user '%s' successfully.\n", user);
    }

    // --------------- Other existing handlers ---------------
    ROUTE_GET("/") {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        int fd = open("page1.html", O_RDONLY);
        if (fd < 0) {
            printf("Error: cannot open page1.html\n");
            return;
        }
        char buffer[BUF_SIZE];
        ssize_t n;
        while ((n = read(fd, buffer, BUF_SIZE)) > 0) {
            if (write(STDOUT_FILENO, buffer, n) != n) {
                fprintf(stderr, "Write error\n");
                break;
            }
        }
        close(fd);
    }

    ROUTE_POST("/") {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Posted %d bytes: %.*s\n",
               payload_size, payload_size, payload);
    }

    ROUTE_POST("/pass") {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Posted %d bytes: %s\n", payload_size, payload);
    }

    ROUTE_END()
}
