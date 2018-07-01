#include "../libs/cJSON.h"
#include "argparse.h"
#include <curl/curl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *token;
int userid;
const char *url = "https://api.telegram.org/bot%s/%s";

void fill_url(const char *method, char *buffer) { sprintf(buffer, url, token, method); }

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "Error allocating space for string\n.");
        return;
    }
    s->ptr[s->len] = '\0';
}

void cleanup_string(struct string *s) {
    s->len = 0;
    free(s->ptr);
}

size_t write_to_string(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_length = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_length + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "Error reallocating for string.\n");
        return -1;
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->len = new_length;
    s->ptr[s->len] = '\0';

    return size * nmemb;
}

cJSON *call_telegram_api(const char *token, const char *method, cJSON *data) {
    char buffer[1024];
    fill_url(method, buffer);

    CURL *handle = curl_easy_init();
    if (handle == NULL) {
        fprintf(stderr, "Error initialising curl easy handle\n");
        return NULL;
    }

    char *jsondata = NULL;
    struct curl_slist *headers = NULL;

    // curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(handle, CURLOPT_URL, buffer);
    if (data != NULL) {
        curl_easy_setopt(handle, CURLOPT_POST, 1);

        jsondata = cJSON_Print(data);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, jsondata);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, -1L);

        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }

    struct string out;
    init_string(&out);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &out);

    int ret = curl_easy_perform(handle);
    if (ret != 0) {
        fprintf(stderr, "Error curling: %d\n", ret);
    }

    if (data != NULL) {
        free(jsondata);
        curl_slist_free_all(headers);
    }

    curl_easy_cleanup(handle);

    // printf("Reponse: %s\n", out.ptr);
    return cJSON_Parse(out.ptr);
}

bool validate_token(const char *token) {
    cJSON *response = call_telegram_api(token, "getMe", NULL);

    cJSON *ok = cJSON_GetObjectItemCaseSensitive(response, "ok");
    bool valid = false;
    if (cJSON_IsBool(ok) && cJSON_IsTrue(ok)) {
        valid = true;
    }
    cJSON_Delete(response);

    return valid;
}

bool send_message(const char *token, const int userid, const char *message) {
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "chat_id", userid);
    cJSON_AddStringToObject(data, "text", message);
    cJSON *response = call_telegram_api(token, "sendMessage", data);
    cJSON_Delete(data);

    bool ret = false;
    cJSON *ok = cJSON_GetObjectItemCaseSensitive(response, "ok");
    if (cJSON_IsTrue(ok)) {
        ret = true;
    }
    cJSON_Delete(response);

    return ret;
}

void _read_user_id(char *value) {
    userid = atoi(value);
    printf("Set userid: %d\n", userid);
}

void _read_token(char *value) {
    token = malloc(strlen(value));
    memset(token, '\0', strlen(value));
    strcpy(token, value);
    printf("Set token: %s\n", token);
}

int read_properties() {
    argparse_register_argument("user_id", &_read_user_id);
    argparse_register_argument("token", &_read_token);
    argparse_read_properties("/etc/hearthbeat/conf");

    if (userid != 0 && token != NULL) {
        return 0;
    } else {
        return -1;
    }
}

int main() {
    if (read_properties() != 0) {
        fprintf(stderr, "Cannot read properties\n");
        exit(-1);
    }

    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        fprintf(stderr, "Error initialising libcurl\n");
        exit(-1);
    }

    const char *message = "message";
    if (validate_token(token)) {
        send_message(token, userid, message);
    }

    curl_global_cleanup();
}