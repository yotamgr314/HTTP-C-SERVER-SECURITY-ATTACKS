#include "httpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

// ----------------------------------------------------------------------------
// Extracts "username" and "password" from the POST body.
// Searches for password= first so we don't clobber the '&'.
// ----------------------------------------------------------------------------
static void parse_form(char *payload, char **out_user, char **out_pass) {
    *out_user = NULL;
    *out_pass = NULL;

    // find password=
    char *p = strstr(payload, "password=");
    if (p) {
        p += strlen("password=");
        *out_pass = p;
        char *end = strpbrk(p, "&\r\n");
        if (end) *end = '\0';
    }

    // find username=
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
        fprintf(stderr,
                "[DBG] /register payload='%.*s'\n",
                payload_size, payload);

        char *user = NULL, *pass = NULL;
        parse_form(payload, &user, &pass);
        fprintf(stderr,
                "[DBG] parse_form → user='%s' pass='%s'\n",
                user?user:"(null)", pass?pass:"(null)");

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

    // ------------------ Login endpoint ------------------
    ROUTE_POST("/login") {
        fprintf(stderr,
                "[DBG] /login payload='%.*s'\n",
                payload_size, payload);

        char *user = NULL, *pass = NULL;
        parse_form(payload, &user, &pass);
        fprintf(stderr,
                "[DBG] parse_form → user='%s' pass='%s'\n",
                user?user:"(null)", pass?pass:"(null)");

        if (!user || !pass) {
            printf("HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Error: malformed form data.\n");
            return;
        }

        FILE *f = fopen("users.db", "r");
        if (!f) {
            printf("HTTP/1.1 500 Internal Server Error\r\n\r\n");
            printf("Error: cannot open users.db\n");
            return;
        }

        char line[BUF_SIZE];
        int success = 0;
        while (fgets(line, sizeof(line), f)) {
            // remove newline
            line[strcspn(line, "\r\n")] = '\0';
            char *colon = strchr(line, ':');
            if (!colon) continue;
            *colon = '\0';
            char *fuser = line;
            char *fpass = colon + 1;
            if (strcmp(user, fuser) == 0 && strcmp(pass, fpass) == 0) {
                success = 1;
                break;
            }
        }
        fclose(f);

        if (success) {
            printf("HTTP/1.1 200 OK\r\n\r\n");
            printf("Login successful.\n");
        } else {
            printf("HTTP/1.1 401 Unauthorized\r\n\r\n");
            printf("Login failed.\n");
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
        char buffer[BUF_SIZE];
        ssize_t n;
        while ((n = read(fd, buffer, BUF_SIZE)) > 0) {
            write(STDOUT_FILENO, buffer, n);
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
