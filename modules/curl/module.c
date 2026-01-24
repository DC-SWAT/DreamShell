/* DreamShell ##version##

   module.c - cURL module
   Copyright (C) 2025-2026 SWAT 
*/

#include <ds.h>
#include <curl/curl.h>
#include <string.h>

DEFAULT_MODULE_HEADER(curl);

/* Stubs for missing symbols */
char *basename(char *path) {
    char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

int __xpg_strerror_r(int errnum, char *buf, size_t buflen) {
    snprintf(buf, buflen, "DS_ERROR: CURL error %d\n", errnum);
    return 0;
}

struct prog_ctx {
    CURL *curl;
    uint64_t last_time;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    file_t fd = (file_t)(intptr_t)userdata;
    size_t real_size = size * nmemb;
    ssize_t written = fs_write(fd, ptr, real_size);

    if(written < 0) {
        return 0;
    }
    return (size_t)written;
}

static size_t read_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    file_t fd = (file_t)(intptr_t)userdata;
    size_t real_size = size * nmemb;
    ssize_t read_bytes = fs_read(fd, ptr, real_size);
    if(read_bytes < 0) return CURL_READFUNC_ABORT;
    return (size_t)read_bytes;
}

static int debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
    (void)handle;
    (void)userptr;

    if (type == CURLINFO_TEXT) {
        ds_printf("CURL: %.*s\n", (int)size, data);
    }
    else if (type == CURLINFO_HEADER_IN) {
        ds_printf("<= %.*s\n", (int)size, data);
    }
    else if (type == CURLINFO_HEADER_OUT) {
        ds_printf("=> %.*s\n", (int)size, data);
    }
    return 0;
}

static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, 
                           curl_off_t ultotal, curl_off_t ulnow) {
    struct prog_ctx *ctx = (struct prog_ctx *)clientp;
    uint64_t now = timer_ms_gettime64();
    (void)ultotal;
    (void)ulnow;

    if(dltotal > 0 && (now - ctx->last_time >= 2000)) {
        ctx->last_time = now;

        double speed = 0.0;
        curl_easy_getinfo(ctx->curl, CURLINFO_SPEED_DOWNLOAD, &speed);

        ds_printf("DS_PROGRESS: %llu / %llu bytes (%d%%) Speed: %.2f KB/s\r", 
                  (unsigned long long)dlnow, (unsigned long long)dltotal, 
                  (int)((dlnow * 100) / dltotal),
                  speed / 1024.0);
    }
    return 0;
}

static int sockopt_callback(void *clientp, curl_socket_t curlfd, curlsocktype purpose) {
    (void)clientp;
    (void)purpose;

    uint32_t new_buf_sz = (64 * 1024) - 512;
    setsockopt(curlfd, SOL_SOCKET, SO_SNDBUF, &new_buf_sz, sizeof(new_buf_sz));
    setsockopt(curlfd, SOL_SOCKET, SO_RCVBUF, &new_buf_sz, sizeof(new_buf_sz));
    
    return CURL_SOCKOPT_OK;
}

int builtin_curl_cmd(int argc, char *argv[]) {
    if(argc < 2) {
        ds_printf("Usage: %s [options] <url>\n"
                  "Options:\n"
                  " -o, --output <file>    -Output file path\n"
                  " -v, --verbose          -Verbose output\n"
                  " -I, --head             -Show headers only\n"
                  " -k, --insecure         -Allow insecure SSL connections\n"
                  " -d, --data <data>      -HTTP POST data\n"
                  " --data-raw <data>      -HTTP POST data (raw)\n"
                  " -H, --header <head>    -Custom header (can be used multiple times)\n"
                  " -X, --request <meth>   -Custom request method (GET, POST, etc)\n"
                  " -L, --location         -Follow redirects\n"
                  " -T, --upload-file <f>  -Upload file\n"
                  " --connect-timeout <sec>-Maximum time allowed for connection (default 10)\n"
                  " -m, --max-time <sec>   -Maximum time allowed for the transfer\n\n", argv[0]);
        return CMD_NO_ARG; 
    }

    char *url = NULL;
    char *output_file = NULL;
    char *post_data = NULL;
    char *upload_file = NULL;
    char *custom_method = NULL;
    char **headers_array = NULL;
    int verbose = 0, head = 0, insecure = 0, location = 0;
    int connect_timeout = 10, max_time = 0;

    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    file_t fd = FILEHND_INVALID;
    file_t upload_fd = FILEHND_INVALID;
    struct prog_ctx prog;

    struct cfg_option options[] = {
        {"output",          'o', NULL, CFG_STR,  (void *) &output_file, 0},
        {"verbose",         'v', NULL, CFG_BOOL, (void *) &verbose,     0},
        {"head",            'I', NULL, CFG_BOOL, (void *) &head,        0},
        {"insecure",        'k', NULL, CFG_BOOL, (void *) &insecure,    0},
        {"location",        'L', NULL, CFG_BOOL, (void *) &location,    0},
        {"data",            'd', NULL, CFG_STR,  (void *) &post_data,   0},
        {"data-raw",        '\0',NULL, CFG_STR,  (void *) &post_data,   0},
        {"header",          'H', NULL, CFG_STR | CFG_MULTI, (void *) &headers_array, 0},
        {"request",         'X', NULL, CFG_STR,  (void *) &custom_method, 0},
        {"upload-file",     'T', NULL, CFG_STR,  (void *) &upload_file, 0},
        {"connect-timeout", '\0',NULL, CFG_INT,  (void *) &connect_timeout, 0},
        {"max-time",        'm', NULL, CFG_INT,  (void *) &max_time, 0},
        {NULL,              '\0',NULL, CFG_STR | CFG_LEFTOVER_ARGS, (void *) &url, 0},
        CFG_END_OF_LIST
    };

    CMD_DEFAULT_ARGS_PARSER(options);

    if(url == NULL) {
        ds_printf("Error: URL is required.\n");
        if(headers_array) free(headers_array);
        return CMD_ERROR;
    }

    /* Convert headers array to curl_slist */
    if(headers_array) {
        for(int i = 0; headers_array[i] != NULL; i++) {
            headers = curl_slist_append(headers, headers_array[i]);
        }
        free(headers_array);
    }

    curl = curl_easy_init();

    if(!curl) {
        ds_printf("DS_ERROR: curl_easy_init() failed\n");
        if(headers) curl_slist_free_all(headers);
        return CMD_ERROR;
    }

    if(output_file) {
        fd = fs_open(output_file, O_WRONLY | O_TRUNC | O_CREAT);

        if(fd < 0) {
            ds_printf("Error: Cannot open output file %s\n", output_file);
            curl_easy_cleanup(curl);
            if(headers) curl_slist_free_all(headers);
            return CMD_ERROR;
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(intptr_t)fd);
    }

    if(upload_file) {
        upload_fd = fs_open(upload_file, O_RDONLY);

        if(upload_fd < 0) {
            ds_printf("Error: Cannot open upload file %s\n", upload_file);
            if(fd != FILEHND_INVALID) fs_close(fd);
            curl_easy_cleanup(curl);
            if(headers) curl_slist_free_all(headers);
            return CMD_ERROR;
        }
        size_t file_size = fs_total(upload_fd);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)(intptr_t)upload_fd);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_size);
    }

    /* Setup progress callback */
    prog.curl = curl;
    prog.last_time = 0;
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    if(location) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    if(verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
    }
    if(head) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    }
    if(insecure) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    if(custom_method) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, custom_method);
    }
    if(post_data) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    }
    if(headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    if(connect_timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)connect_timeout);
    }
    if(max_time > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)max_time);
    }
    if(upload_file) {
        ds_printf("DS_PROGRESS: Uploading %s to %s...\n\n", upload_file, url);
    }
    else {
        if(output_file) {
            ds_printf("DS_PROGRESS: Downloading %s to %s...\n\n", url, output_file);
        }
        else {
            ds_printf("DS_PROGRESS: Downloading %s...\n\n", url);
        }
    }

    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        ds_printf("DS_ERROR: Request failed: %s\n", curl_easy_strerror(res));
    }
    else {
        double speed = 0.0;
        curl_off_t dlnow = 0, dltotal = 0;
        
        curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed);
        curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dltotal);
        dlnow = dltotal;

        ds_printf("DS_PROGRESS: %llu / %llu bytes (100%%) Speed: %.2f KB/s\r", 
                  (unsigned long long)dlnow, (unsigned long long)dltotal, 
                  speed / 1024.0);
        ds_printf("DS_OK: Request completed.\n");
    }

    if(fd != FILEHND_INVALID) {
        fs_close(fd);
    }
    if(upload_fd != FILEHND_INVALID) {
        fs_close(upload_fd);
    }
    if(headers) {
        curl_slist_free_all(headers);
    }

    curl_easy_cleanup(curl);

    return CMD_OK;
}

int lib_open(klibrary_t *lib) {
    AddCmd(lib_get_name(), "cURL command line tool", (CmdHandler *) builtin_curl_cmd);
    return 0;
}

int lib_close(klibrary_t *lib) {
    RemoveCmd(GetCmdByName(lib_get_name()));
    return 0;
}
