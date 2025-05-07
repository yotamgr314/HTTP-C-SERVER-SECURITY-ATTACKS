#include "httpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

// ----------------------------------------------------------------------------
// Extracts "username" and "password" from the POST body.
// ----------------------------------------------------------------------------
static void parse_form(char *payload, char **out_user, char **out_pass) {
    *out_user = NULL; *out_pass = NULL;
    char *p = strstr(payload, "password=");
    if (p) {
        p += strlen("password=");
        *out_pass = p;
        char *end = strpbrk(p, "&\r\n");
        if (end) *end = '\0';
    }
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
        char *user = NULL, *pass = NULL;
        parse_form(payload, &user, &pass);
        if (!user || !pass) {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Error: malformed form data.\n");
            return;
        }
        FILE *f = fopen("users.db", "a");
        if (!f) {
            printf("HTTP/1.1 500 Internal Server Error\r\n\r\n");
            printf("Error writing users.db.\n");
            return;
        }
        fprintf(f, "%s:%s\n", user, pass);
        fclose(f);
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Registered user '%s'.\n", user);
    }

    // ------------------ Vulnerable Login ------------------
    ROUTE_POST("/login") {
        // We put buffer[64] and immediately after it an int flag
        struct {
            char buffer[64];
            int  authorized;
        } v = {{0}, 0};

        // UNSAFE: strcpy will overflow into v.authorized if payload >64 bytes
        strcpy(v.buffer, payload);

        // debug: show first 64 chars and the flag
        fprintf(stderr,
                "[VULN] buffer='%.64s'  authorized=%d\n",
                v.buffer, v.authorized);

        // only do the real check if overflow didn't set authorized
        if (!v.authorized) {
            char *user = NULL, *pass = NULL;
            parse_form(v.buffer, &user, &pass);
            if (user && pass) {
                FILE *f = fopen("users.db", "r");
                if (f) {
                    char line[BUF_SIZE];
                    while (fgets(line, BUF_SIZE, f)) {
                        line[strcspn(line, "\r\n")] = '\0';
                        char *colon = strchr(line, ':');
                        if (!colon) continue;
                        *colon = '\0';
                        if (strcmp(user, line) == 0 &&
                            strcmp(pass, colon + 1) == 0) {
                            v.authorized = 1;
                            break;
                        }
                    }
                    fclose(f);
                }
            }
        }

        if (v.authorized) {
            printf("HTTP/1.1 200 OK\r\n\r\n");
            printf("Login successful (authorized=%d).\n", v.authorized);
        } else {
            printf("HTTP/1.1 401 Unauthorized\r\n\r\n");
            printf("Login failed (authorized=%d).\n", v.authorized);
        }
    }

    // --------------- Other existing handlers ---------------
    ROUTE_GET("/") {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        int fd = open("page1.html", O_RDONLY);
        if (fd < 0) {
            printf("Error: cannot open page1.html\n");
            return;
        }
        char buf[BUF_SIZE];
        ssize_t n;
        while ((n = read(fd, buf, BUF_SIZE)) > 0)
            write(STDOUT_FILENO, buf, n);
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
