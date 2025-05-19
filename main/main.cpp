#include "app.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(app::init());

    // FIXME: Remove this in the release
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_ERROR_CHECK(app::start());
}
