#pragma once

#include "settings.h"
#include "htp1_client.h"

// Start the async web server on port 80
// Needs pointers to settings and state so routes can read/write them.
void webserver_begin(AppSettings *settings,
                     void (*onSettingsChanged)());

// Nothing to poll — ESPAsyncWebServer runs on its own task
