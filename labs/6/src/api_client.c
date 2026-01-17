#include "api_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

static char base_url[256] = {0};
static CURL *curl = NULL;

// Buffer for HTTP response
typedef struct {
    char *data;
    size_t size;
} MemoryStruct;

// Callback for writing HTTP response
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Not enough memory\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

int api_client_init(const char *url) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return -1;
    }

    strncpy(base_url, url, sizeof(base_url) - 1);
    return 0;
}

void api_client_cleanup(void) {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    curl_global_cleanup();
}

// Helper function to make HTTP GET request
static char* http_get(const char *endpoint) {
    if (!curl) return NULL;

    char url[512];
    snprintf(url, sizeof(url), "%s%s", base_url, endpoint);

    MemoryStruct chunk;
    chunk.data = malloc(1);
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        return NULL;
    }

    return chunk.data;
}

int api_get_current_temp(double *temp, time_t *timestamp) {
    if (!temp || !timestamp) return -1;

    char *response = http_get("/api/current");
    if (!response) return -1;

    struct json_object *jobj = json_tokener_parse(response);
    free(response);

    if (!jobj) return -1;

    struct json_object *jtemp, *jtime;
    
    if (json_object_object_get_ex(jobj, "temperature", &jtemp) &&
        json_object_object_get_ex(jobj, "timestamp", &jtime)) {
        *temp = json_object_get_double(jtemp);
        *timestamp = json_object_get_int64(jtime);
        json_object_put(jobj);
        return 0;
    }

    json_object_put(jobj);
    return -1;
}

TempDataArray* api_get_hourly_data(void) {
    char *response = http_get("/api/hourly");
    if (!response) return NULL;

    struct json_object *jobj = json_tokener_parse(response);
    free(response);

    if (!jobj) return NULL;

    struct json_object *jdata;
    if (!json_object_object_get_ex(jobj, "data", &jdata)) {
        json_object_put(jobj);
        return NULL;
    }

    int count = json_object_array_length(jdata);
    
    TempDataArray *result = malloc(sizeof(TempDataArray));
    result->count = count;
    result->data = malloc(count * sizeof(TempData));

    for (int i = 0; i < count; i++) {
        struct json_object *item = json_object_array_get_idx(jdata, i);
        struct json_object *jtemp, *jtime;
        
        if (json_object_object_get_ex(item, "temperature", &jtemp) &&
            json_object_object_get_ex(item, "timestamp", &jtime)) {
            result->data[i].temperature = json_object_get_double(jtemp);
            result->data[i].timestamp = json_object_get_int64(jtime);
        }
    }

    json_object_put(jobj);
    return result;
}

TempDataArray* api_get_daily_data(void) {
    char *response = http_get("/api/daily");
    if (!response) return NULL;

    struct json_object *jobj = json_tokener_parse(response);
    free(response);

    if (!jobj) return NULL;

    struct json_object *jdata;
    if (!json_object_object_get_ex(jobj, "data", &jdata)) {
        json_object_put(jobj);
        return NULL;
    }

    int count = json_object_array_length(jdata);
    
    TempDataArray *result = malloc(sizeof(TempDataArray));
    result->count = count;
    result->data = malloc(count * sizeof(TempData));

    for (int i = 0; i < count; i++) {
        struct json_object *item = json_object_array_get_idx(jdata, i);
        struct json_object *jtemp, *jtime;
        
        if (json_object_object_get_ex(item, "temperature", &jtemp) &&
            json_object_object_get_ex(item, "timestamp", &jtime)) {
            result->data[i].temperature = json_object_get_double(jtemp);
            result->data[i].timestamp = json_object_get_int64(jtime);
        }
    }

    json_object_put(jobj);
    return result;
}

void api_free_data_array(TempDataArray *array) {
    if (array) {
        if (array->data) {
            free(array->data);
        }
        free(array);
    }
}
