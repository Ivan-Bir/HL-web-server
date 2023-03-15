#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>
#include <ev.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LENGTH 1024
#define SMALL_BUFF_SIZE 100

#define CONF_PARAM_CPU "cpu_limit"
#define CONF_PARAM_PORT "port"
#define CONF_PARAM_ROOT_DIR "root_dir"

#define DEFAULT_CONFIG_NAME "serv.conf"
#define DEFAULT_SERVER_PORT 8080
#define DEFAULT_CPU_LIMIT 1
#define DEFAULT_ROOT_DIR "/var/www/html"

#define DELAY_SEND_AGAIN_MS 5

#define GET "GET"
#define POST "POST"
#define HEAD "HEAD"
#define HTTP1_1 "HTTP/1.1"
#define INDEX_FILE "index.html"

#define OK 0

static struct st_conf {
    int port;
    int cpu_limit;
    char root_dir[1024];
} cfg = {0};

static const struct dictionary_extension_ctype {
    const char *extension;
    const char *ctype;
} ctype_dict[] = {
    {"html", "text/html"},
    {"htm", "text/htm"},
    {"php", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"swf", "application/x-shockwave-flash"},
    {"pdf", "application/pdf"},
    {"ps", "application/postscript"},
    {"txt", "text/plain"},
    {"ico", "image/x-icon"},
    {NULL, NULL},
};


int init_config_default(struct st_conf *cfg) {
    cfg->port = DEFAULT_SERVER_PORT;
    cfg->cpu_limit = DEFAULT_CPU_LIMIT;
    strncpy(cfg->root_dir, DEFAULT_ROOT_DIR, MAX_LINE_LENGTH);
    return 0;
}

int parse_config_file(const char *filename, struct st_conf *cfg) {
    char line[MAX_LINE_LENGTH];
    char *param_name, *param_value;

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Unable to open config file %s\n", filename);
        return -1;
    }

    if (init_config_default(cfg) != 0) {
        fprintf(stderr, "ERROR: initialize config error");
        return -1;
    }

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        param_name = strtok(line, " ");
        param_value = strtok(NULL, " ");
        if (param_value == NULL) {
            fprintf(stderr, "ERROR: Invalid config file format\n");
            fclose(fp);
            return -1;
        }
        if (strcmp(param_name, CONF_PARAM_PORT) == 0) {
            cfg->port = atoi(param_value);
        } else if (strcmp(param_name, CONF_PARAM_ROOT_DIR) == 0) {
            char absolute_root_path[MAX_PATH_LENGTH];
            if (!realpath(param_value, absolute_root_path)) {
                fprintf(stderr, "ERROR: unresolved root path\n");
                return -1;
            }
            strncpy(cfg->root_dir, absolute_root_path, MAX_LINE_LENGTH);
        } else if (strcmp(param_name, CONF_PARAM_CPU) == 0) {
            cfg->cpu_limit = atoi(param_value);
        } else {
            fprintf(stderr, "ERROR: Unknown parameter %s\n", param_name);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}

const char *get_content_type(const char *filename) {
    size_t pos_dot = 0;
    for (size_t curr_pos = 0; filename[curr_pos] != '\0'; curr_pos++) {
        if (filename[curr_pos] == '.') {
            pos_dot = curr_pos;
        }
    }

    for (int i = 0; i < sizeof(ctype_dict) / sizeof(ctype_dict[0]) - 1; i++) {
        if (strcmp(filename + pos_dot + 1, ctype_dict[i].extension) == 0) {
            return ctype_dict[i].ctype;
        }
    }
    return "";
}

int hex_str_to_ascii(const char* input, char* output) {
    if (output == NULL) {
        return -1;
    }

    int i = 0, j = 0;
    while (input[i] != '\0') {
        if (input[i] == '%' && input[i+1] != '\0' && input[i+2] != '\0') {
            char hex[3];
            hex[0] = input[i+1];
            hex[1] = input[i+2];
            hex[2] = '\0';

            int ascii = strtol(hex, NULL, 16);
            output[j] = (char)ascii;

            i += 3;
        } else {
            output[j] = input[i];
            i++;
        }
        j++;
    }

    output[j] = '\0';
    return OK;
}

const char* get_message_from_code(int status_code) {
    switch(status_code) {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        default:
            return "Unknown HTTP Status Code";
    }
}

void set_headers_http_ok(int status_code, int content_legth, const char *filename, char *headers) {
    char template[] = "%s %d %s\r\nServer: server\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n";
    sprintf(headers, template, HTTP1_1, 200 , get_message_from_code(status_code), content_legth, get_content_type(filename));
}

void set_headers_http_err(int status_code, char *headers) {
    char template[] = "%s %d %s\r\nServer: server\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n";
    sprintf(headers, template, HTTP1_1, status_code , get_message_from_code(status_code), 0, "text/plain");
}

int send_err_response(int sock_fd, int status_code) {
    char error_msg[MAX_BUFFER_SIZE];
    set_headers_http_err(status_code, error_msg);
    int bytes_sent = send(sock_fd, error_msg, strlen(error_msg), 0);
    if (bytes_sent == -1) {
        if (errno == EAGAIN) {
            fprintf(stderr, "ERROR: socket does not ready to write\n");
            return -1;
        }
    }
    return OK;
}

int is_directory_path(const char *path) {
    return path[strlen(path) - 1] == '/';
}

ssize_t calc_content_length(const char *filename) {
    struct stat statistics;
    if (stat(filename, &statistics) == -1) {
        return -1;
    }
    return statistics.st_size;
}

int handle_request(int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = read(client_socket, buffer, MAX_BUFFER_SIZE);

    if (bytes_read < 0) {
        fprintf(stderr, "ERROR: read bytes\n");
        return -1;
    }
    buffer[bytes_read] = '\0';

    char request[MAX_PATH_LENGTH];
    char http_version[SMALL_BUFF_SIZE];
    char path[MAX_PATH_LENGTH];
    if (sscanf((char *)buffer, "%s %s %s", request, path, http_version) != 3) {
        fprintf(stderr, "ERROR: reading request: %s\n", buffer);
        return -1;
    }
    char ascii_path[MAX_PATH_LENGTH];
    if (hex_str_to_ascii(path, ascii_path) != OK) {
        return -1;
    }
    char tmp[MAX_PATH_LENGTH];

    strcpy(tmp, cfg.root_dir);
    strcat(tmp, ascii_path);
    char *true_path = tmp;

    char *query_param_pos = strstr(true_path, "?");
    if (query_param_pos != NULL) {
        *query_param_pos = '\0';
    }

    printf("PATH: %s\n", true_path);

    if (strcmp(request, POST) == 0) {
        send_err_response(client_socket, 405);
        return OK;
    }

    if (strcmp(request, GET) == 0 || strcmp(request, HEAD) == 0) {
        int is_dir = is_directory_path(true_path);
        char resolved_path[MAX_PATH_LENGTH];
        if (realpath(true_path, resolved_path) == NULL) {
            if (is_dir && errno != ENOTDIR) {
                send_err_response(client_socket, 403);
                return OK;
            }

            send_err_response(client_socket, 404);
            return OK;
        }

        if (strncmp(cfg.root_dir, resolved_path, strlen(cfg.root_dir)) != 0) {
            send_err_response(client_socket, 404);
            return OK;
        }

        if (is_dir) {
            strcat(true_path, INDEX_FILE);
        }

        FILE *file = fopen(true_path, "r");

        if (file == NULL) {
            if (is_dir) {
                send_err_response(client_socket, 403);
            } else {
                send_err_response(client_socket, 404);
            }

            return OK;
        }

        char ok_msg[MAX_BUFFER_SIZE];

        int content_length = calc_content_length(true_path);

        set_headers_http_ok(200, content_length, true_path, ok_msg);
        send(client_socket, ok_msg, strlen(ok_msg), 0);
        if (strcmp(request, HEAD) == 0) {
            fclose(file);
            return OK;
        }

        int file_fd = fileno(file);
        int bytes_sent = 0;
        int curr_sent = 0;
        while (bytes_sent < content_length) {
            curr_sent = sendfile(client_socket, file_fd, NULL, content_length);

            if (curr_sent == -1) {
                if (errno == EAGAIN) {
                    usleep(DELAY_SEND_AGAIN_MS);
                    continue;
                }
                break;
            }
            bytes_sent += curr_sent;
        }

        fclose(file);
    }

    close(client_socket);
    return OK;
}

static void read_cb(struct ev_loop *loop, ev_io *watcher, int revents) {
    handle_request(watcher->fd);

    close(watcher->fd);
    ev_io_stop(loop, watcher);
    free(watcher);
}

static void accept_cb(struct ev_loop *loop, ev_io *watcher, int revents) {
    int client_sd = accept(watcher->fd, NULL, NULL);

    if (client_sd > 0) {
        fcntl(client_sd, F_SETFL, fcntl(client_sd, F_GETFL, 0) | O_NONBLOCK);

        ev_io *client_watcher = calloc(1, sizeof(*client_watcher));
        if (client_watcher == NULL) {
            close(client_sd);
            return;
        }
        ev_io_init(client_watcher, read_cb, client_sd, EV_READ);
        ev_io_start(loop, client_watcher);
    }
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    if (loop == NULL) {
        return -1;
    }

    parse_config_file(DEFAULT_CONFIG_NAME, &cfg);

    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(cfg.port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0) | O_NONBLOCK) != 0) {
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, INT_MAX) != 0) {
        close(server_socket);
        return -1;
    }

    printf("Count workers %d...\n", cfg.cpu_limit);
    printf("Running on port %d...\n", cfg.port);
    for (int i = 0; i < cfg.cpu_limit; ++i) {
        int pid = fork();
        if (pid == -1) {
            return -1;
        }

        if (pid == 0) {
            ev_io watcher = {0};
            ev_io_init(&watcher, accept_cb, server_socket, EV_READ);
            ev_io_start(loop, &watcher);
            ev_run(loop, 0);
            ev_loop_destroy(loop);
            exit(EXIT_SUCCESS);
        }
    }

    while (1) {
        sleep(INT_MAX);
    }
    return EXIT_SUCCESS;
}
