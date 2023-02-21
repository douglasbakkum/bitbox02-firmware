// Copyright 2023 Shift Crypto AG
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sensors.h"
#include "touch/gestures.h"
#include "button.h"
#include "image.h"
#include "ui_images.h"
#include <qtouch/qtouch.h>

#include <hardfault.h>
#include <screen.h>
#include <util.h>
#include "label.h"
#include <ui/fonts/arial_fonts.h>
#include <string.h>

#if !defined(TESTING)
#include <qtouch.h>
#else
#include <mock_qtouch.h>
#endif


/**
 * The sensor data.
 */
typedef struct {
    component_t* label_top;
    component_t* label_bottom;
    uint8_t bottom_position;
    uint8_t top_position;
    bool top_active;
    bool bottom_active;
    bool top_tap;
    bool bottom_tap;

} sensor_data_t;


static void _render(component_t* component)
{
    sensor_data_t* data = (sensor_data_t*)component->data;
    char text[500];
    uint16_t x,y;

    // Display "top" sensor readings
    if (screen_is_upside_down()) {
        snprintf(
            text,
            sizeof(text),
            "%17d\n%5d %6d %6d %6d\n%5d %5d %5d %5d",
            data->top_position,
            qtouch_get_sensor_node_signal(4)-qtouch_get_sensor_node_reference(4),
            qtouch_get_sensor_node_signal(5)-qtouch_get_sensor_node_reference(5),
            qtouch_get_sensor_node_signal(6)-qtouch_get_sensor_node_reference(6),
            qtouch_get_sensor_node_signal(7)-qtouch_get_sensor_node_reference(7),
            qtouch_get_sensor_node_reference(4),
            qtouch_get_sensor_node_reference(5),
            qtouch_get_sensor_node_reference(6),
            qtouch_get_sensor_node_reference(7)
            );
    } else {
        snprintf(
            text,
            sizeof(text),
            "%17d\n%5d %6d %6d %6d\n%5d %5d %5d %5d",
            data->top_position,
            qtouch_get_sensor_node_signal(0)-qtouch_get_sensor_node_reference(0),
            qtouch_get_sensor_node_signal(1)-qtouch_get_sensor_node_reference(1),
            qtouch_get_sensor_node_signal(2)-qtouch_get_sensor_node_reference(2),
            qtouch_get_sensor_node_signal(3)-qtouch_get_sensor_node_reference(3),
            qtouch_get_sensor_node_reference(0),
            qtouch_get_sensor_node_reference(1),
            qtouch_get_sensor_node_reference(2),
            qtouch_get_sensor_node_reference(3)
            );
    }
    label_update(data->label_top, text);

    // Display "bottom" sensor readings
    if (screen_is_upside_down()) {
        snprintf(
            text,
            sizeof(text),
            "%5d %5d %5d %5d\n%5d %6d %6d %6d\n%17d",
            qtouch_get_sensor_node_reference(3),
            qtouch_get_sensor_node_reference(2),
            qtouch_get_sensor_node_reference(1),
            qtouch_get_sensor_node_reference(0),
            qtouch_get_sensor_node_signal(3)-qtouch_get_sensor_node_reference(3),
            qtouch_get_sensor_node_signal(2)-qtouch_get_sensor_node_reference(2),
            qtouch_get_sensor_node_signal(1)-qtouch_get_sensor_node_reference(1),
            qtouch_get_sensor_node_signal(0)-qtouch_get_sensor_node_reference(0),
            data->bottom_position
            );
    } else {
        snprintf(
            text,
            sizeof(text),
            "%5d %5d %5d %5d\n%5d %6d %6d %6d\n%17d",
            qtouch_get_sensor_node_reference(7),
            qtouch_get_sensor_node_reference(6),
            qtouch_get_sensor_node_reference(5),
            qtouch_get_sensor_node_reference(4),
            qtouch_get_sensor_node_signal(7)-qtouch_get_sensor_node_reference(7),
            qtouch_get_sensor_node_signal(6)-qtouch_get_sensor_node_reference(6),
            qtouch_get_sensor_node_signal(5)-qtouch_get_sensor_node_reference(5),
            qtouch_get_sensor_node_signal(4)-qtouch_get_sensor_node_reference(4),
            data->bottom_position
            );
    }
    label_update(data->label_bottom, text);

    data->label_top->f->render(data->label_top);
    data->label_bottom->f->render(data->label_bottom);

    // Draw positions for touch events and a line for tap events
    if (data->top_active || data->top_tap) {
        x = data->top_position * SCREEN_WIDTH / MAX_SLIDER_POS;
        y = 1;
        UG_DrawLine(x - 2, y, x + 2, y, screen_front_color);
        if (data->top_tap) {
            data->top_tap = false;
            UG_DrawLine(x, y, x, y + 20, screen_front_color);
        }
    }
    if (data->bottom_active || data->bottom_tap) {
        x = data->bottom_position * SCREEN_WIDTH / MAX_SLIDER_POS;
        y = SCREEN_HEIGHT - 1;
        UG_DrawLine(x - 2, y, x + 2, y, screen_front_color);
        if (data->bottom_tap) {
            data->bottom_tap = false;
            UG_DrawLine(x, y, x, y - 20, screen_front_color);
        }
    }

    // Draw tick marks at typical button boundaries
    x = SLIDER_POSITION_ONE_THIRD * SCREEN_WIDTH / MAX_SLIDER_POS;
    UG_DrawLine(x, SCREEN_HEIGHT, x, SCREEN_HEIGHT - 3, screen_front_color);
    UG_DrawLine(x, 0, x, 2, screen_front_color);
    x = SLIDER_POSITION_TWO_THIRD * SCREEN_WIDTH / MAX_SLIDER_POS;
    UG_DrawLine(x, SCREEN_HEIGHT, x, SCREEN_HEIGHT - 3, screen_front_color);
    UG_DrawLine(x, 0, x, 2, screen_front_color);
}


static void _on_event(const event_t* event, component_t* component)
{
    sensor_data_t* data = (sensor_data_t*)component->data;
    gestures_slider_data_t* slider_data = (gestures_slider_data_t*)event->data;

    switch (event->id) {
        case EVENT_TOP_CONTINUOUS_TAP:
        case EVENT_TOP_SLIDE:
            data->top_position = slider_data->position;
            data->top_active = true;
            break;
        case EVENT_BOTTOM_CONTINUOUS_TAP:
        case EVENT_BOTTOM_SLIDE:
            data->bottom_position = slider_data->position;
            data->bottom_active = true;
            break;
        case EVENT_TOP_SHORT_TAP:
            data->top_tap = true;
            data->top_active = false;
            data->top_position = slider_data->position;
            break;
        case EVENT_BOTTOM_SHORT_TAP:
            data->bottom_tap = true;
            data->bottom_active = false;
            data->bottom_position = slider_data->position;
            break;
        /* FALLTHROUGH */
        default:
            data->top_active = false;
            data->bottom_active = false;
    }
}


/********************************** Component Functions **********************************/

/**
 * Collects all component functions.
 */
static component_functions_t _component_functions = {
    .cleanup = ui_util_component_cleanup,
    .render = _render,
    .on_event = _on_event,
};

/********************************** Create Instance **********************************/

component_t* sensors_create(void)
{
    component_t* sensors = malloc(sizeof(component_t));
    if (!sensors) {
        Abort("Error: malloc sensors");
    }
    sensor_data_t* data = malloc(sizeof(sensor_data_t));
    if (!data) {
        Abort("Error: malloc sensors data");
    }
    memset(data, 0, sizeof(sensor_data_t));
    memset(sensors, 0, sizeof(component_t));

    data->top_position = 0;
    data->bottom_position = 0;
    data->top_active = false;
    data->bottom_active = false;
    data->top_tap = false;
    data->bottom_tap = false;

    sensors->data = data;
    sensors->f = &_component_functions;
    sensors->dimension.width = SCREEN_WIDTH;
    sensors->dimension.height = SCREEN_HEIGHT;
    sensors->position.top = 0;
    sensors->position.left = 0;

    data->label_top = label_create("label", &font_font_a_9X9, LEFT_TOP, sensors);
    ui_util_add_sub_component(sensors, data->label_top);
    data->label_bottom = label_create("label", &font_font_a_9X9, LEFT_BOTTOM, sensors);
    ui_util_add_sub_component(sensors, data->label_bottom);
    return sensors;
}

