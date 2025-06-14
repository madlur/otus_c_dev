#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void get_weather(const char *location) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    long http_code = 0;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        char *escaped_location = curl_easy_escape(curl, location, 0);
        if (!escaped_location) {
            fprintf(stderr, "Ошибка кодирования URL\n");
            goto cleanup;
        }
        
        char url[256];
        snprintf(url, sizeof(url), "https://wttr.in/%s?format=j1", escaped_location);
        curl_free(escaped_location);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.68.0");
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Ошибка при запросе к API: %s\n", curl_easy_strerror(res));
            goto cleanup;
        }
        
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code >= 400) {
            if (http_code == 400) {
                fprintf(stderr, "Ошибка 400: Неверный запрос\n");
            } else if (http_code >= 500) {
                fprintf(stderr, "Ошибка %ld: Проблема на сервере\n", http_code);
            } else {
                fprintf(stderr, "Ошибка HTTP: %ld\n", http_code);
            }
            goto cleanup;
        }

        cJSON *json = cJSON_Parse(chunk.memory);
        if (json == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                fprintf(stderr, "Ошибка разбора JSON: %s\n", error_ptr);
            }
        } else {
            cJSON *current_condition = cJSON_GetObjectItemCaseSensitive(json, "current_condition");
            if (current_condition != NULL && cJSON_IsArray(current_condition)) {
                cJSON *condition = cJSON_GetArrayItem(current_condition, 0);
                if (condition != NULL) {
                    cJSON *weather_desc = cJSON_GetObjectItemCaseSensitive(condition, "weatherDesc");
                    if (weather_desc != NULL && cJSON_IsArray(weather_desc)) {
                        cJSON *desc_item = cJSON_GetArrayItem(weather_desc, 0);
                        if (desc_item != NULL) {
                            cJSON *value = cJSON_GetObjectItemCaseSensitive(desc_item, "value");
                            if (cJSON_IsString(value)) {
                                printf("Погода: %s\n", value->valuestring);
                            }
                        }
                    }

                    cJSON *windspeed = cJSON_GetObjectItemCaseSensitive(condition, "windspeedKmph");
                    cJSON *winddir = cJSON_GetObjectItemCaseSensitive(condition, "winddir16Point");
                    if (cJSON_IsString(winddir) && cJSON_IsString(windspeed)) {
                        printf("Ветер: %s, %s км/ч\n", winddir->valuestring, windspeed->valuestring);
                    }

                    cJSON *temp_C = cJSON_GetObjectItemCaseSensitive(condition, "temp_C");
                    if (cJSON_IsString(temp_C)) {
                        printf("Температура: %s°C\n", temp_C->valuestring);
                    }
                }
            } else {
                fprintf(stderr, "Не удалось получить данные о погоде\n");
            }

            cJSON *weather = cJSON_GetObjectItemCaseSensitive(json, "weather");
            if (weather != NULL && cJSON_IsArray(weather)) {
                cJSON *today = cJSON_GetArrayItem(weather, 0);
                if (today != NULL) {
                    cJSON *maxtempC = cJSON_GetObjectItemCaseSensitive(today, "maxtempC");
                    cJSON *mintempC = cJSON_GetObjectItemCaseSensitive(today, "mintempC");
                    if (cJSON_IsString(maxtempC) && cJSON_IsString(mintempC)) {
                        printf("Диапазон температур: %s°C ... %s°C\n", mintempC->valuestring, maxtempC->valuestring);
                    }
                }
            }

            cJSON_Delete(json);
        }
    }

cleanup:
    if (curl) curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <город>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc > 2) {
        char location[256] = "";
        for (int i = 1; i < argc; i++) {
            strcat(location, argv[i]);
            if (i < argc - 1) strcat(location, " ");
        }
        get_weather(location);
    } else {
        get_weather(argv[1]);
    }
    
    return EXIT_SUCCESS;
}