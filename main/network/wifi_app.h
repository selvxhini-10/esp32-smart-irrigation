#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_err.h"

#define MAXIMUM_RETRY  5

esp_err_t wifi_init_sta(void);

#endif // WIFI_APP_H