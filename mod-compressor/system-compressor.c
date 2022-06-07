/*
 * VEJA Compressor
 * Copyright (C) 2021 Jan Janssen <jan@moddevices.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"

#include "compressor_core.h"

/**********************************************************************************************************************************************************/

#define PLUGIN_URI "http://moddevices.com/plugins/mod-devel/System-Compressor"

#define MAP(x, Imin, Imax, Omin, Omax)      ( x - Imin ) * (Omax -  Omin)  / (Imax - Imin) + Omin;

#define COMP_BUF_SIZE 256
#define SAMPLERATE 48000

#define DEZIPPER_CONSTANT  0.1

typedef enum {
    INPUT_L,
    INPUT_R,
    OUTPUT_L,
    OUTPUT_R,
    COMP_MODE,
    RELEASE,
    MASTER_VOL
}PortIndex;

/**********************************************************************************************************************************************************/

typedef struct{

    //ports
    float* input_left;
    float* input_right;
    float* output_left;
    float* output_right;

    float* release;
    float* mode;

    float* volume;

    float *bfr_l;
    float *bfr_r;

    float prev_release;
    float prev_mode;
    float prev_volume;

    const LV2_URID_Map* urid_map;

    sf_compressor_state_st compressor_state;

} Compressor;

/**********************************************************************************************************************************************************/
//                                                                 local functions                                                                        //
/**********************************************************************************************************************************************************/

/**********************************************************************************************************************************************************/
static LV2_Handle
instantiate(const LV2_Descriptor*   descriptor,
double                              samplerate,
const char*                         bundle_path,
const LV2_Feature* const* features)
{
    Compressor* self = (Compressor*)malloc(sizeof(Compressor));

    // query host features
    const LV2_Options_Option* options = NULL;
    for (int i=0; features[i] != NULL; ++i)
    {
        if (strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (strcmp(features[i]->URI, LV2_URID__map) == 0)
            self->urid_map = (const LV2_URID_Map*)features[i]->data;
    }

    // find max block length
    int maxBufSize = COMP_BUF_SIZE;
    for (int i=0; options[i].key != 0; ++i)
    {
        if (options[i].key == self->urid_map->map(self->urid_map->handle, LV2_BUF_SIZE__maxBlockLength))
        {
            if (options[i].type == self->urid_map->map(self->urid_map->handle, LV2_ATOM__Int))
            {
                maxBufSize = *(const int*)options[i].value;
                break;
            }
            break;
        }
    }

    self->bfr_l = (float*)malloc(maxBufSize*sizeof(float));
    self->bfr_r = (float*)malloc(maxBufSize*sizeof(float));

    compressor_init(&self->compressor_state, samplerate);

    // invalid initial values
    self->prev_release = self->prev_mode = -9999;
    self->prev_volume = 1.f;

    return (LV2_Handle)self;
}
/**********************************************************************************************************************************************************/
static void connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    Compressor* self = (Compressor*)instance;

    switch ((PortIndex)port)
    {
        case INPUT_L:
            self->input_left = (float*) data;
            break;
        case INPUT_R:
            self->input_right = (float*) data;
            break;

        case OUTPUT_L:
            self->output_left = (float*) data;
            break;
        case OUTPUT_R:
            self->output_right = (float*) data;
            break;

        case COMP_MODE:
            self->mode = (float*) data;
            break;
        case RELEASE:
            self->release = (float*) data;
            break;
        case MASTER_VOL:
            self->volume = (float*) data;
            break;
    }
}
/**********************************************************************************************************************************************************/
void activate(LV2_Handle instance)
{
    // TODO: include the activate function code here
}

/**********************************************************************************************************************************************************/
void run(LV2_Handle instance, uint32_t n_samples)
{
    Compressor* self = (Compressor*)instance;    

    if ((self->prev_release!= (float)*self->release) || (self->prev_mode != (float)*self->mode)) {
        switch((int)*self->mode)
        {
            //light compression
            case 1:
                compressor_set_params(&self->compressor_state, -12.f,
                                        12.f, 2.f, 0.0001f, ((float)*self->release/1000), -3.f);
            break;

            //medium compression
            case 2:
                compressor_set_params(&self->compressor_state, -12.f,
                                        12.f, 3.f, 0.0001f, ((float)*self->release/1000), -3.f);
            break;

            //heavy compression
            case 3:
                compressor_set_params(&self->compressor_state, -15.f,
                                        15.f, 4.f, 0.0001f, ((float)*self->release/1000), -3.f);
            break;

            //extreme compression
            case 4:
            default:
                compressor_set_params(&self->compressor_state, -25.f,
                                        15.f, 10.f, 0.0001f, ((float)*self->release/1000), -6.f);
            break;
        }

        self->prev_release = (float)*self->release;
        self->prev_mode = (float)*self->mode;
    }

    float linear_volume = cmop_db2lin((float)*self->volume);

    if ((int)*self->mode != 0)
    {
        compressor_process(&self->compressor_state, n_samples, self->input_left, self->input_right, self->bfr_l, self->bfr_r);
    
        for (uint32_t i = 0; i < n_samples; i++) {
            //moving average over volume, reduces zipper noise
            if (self->prev_volume != linear_volume)
                self->prev_volume = DEZIPPER_CONSTANT * linear_volume + (1.0 - DEZIPPER_CONSTANT) * self->prev_volume;

            self->output_left[i] = self->bfr_l[i] * self->prev_volume;
            self->output_right[i] = self->bfr_r[i] * self->prev_volume;
        }
    }
    else
    {
        for (uint32_t i = 0; i < n_samples; i++) {
            //moving average over volume, reduces zipper noise
            if (self->prev_volume != linear_volume)
                self->prev_volume = DEZIPPER_CONSTANT * linear_volume + (1.0 - DEZIPPER_CONSTANT) * self->prev_volume;

            self->output_left[i] = self->input_left[i] * self->prev_volume;
            self->output_right[i] = self->input_right[i] * self->prev_volume;
        }
    }
}

/**********************************************************************************************************************************************************/
void deactivate(LV2_Handle instance)
{
    // TODO: include the deactivate function code here
}
/**********************************************************************************************************************************************************/
void cleanup(LV2_Handle instance)
{
    Compressor* self = (Compressor*)instance;

    free(self->bfr_l);
    free(self->bfr_r);
    free(self);
}
/**********************************************************************************************************************************************************/
uint32_t set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    Compressor* self = (Compressor*)instance;

    for (int i=0; options[i].key != 0; ++i)
    {
        if (options[i].key != self->urid_map->map(self->urid_map->handle, LV2_BUF_SIZE__nominalBlockLength))
            continue;

        const int maxBufSize = *(const int*)options[i].value;

        free(self->bfr_l);
        free(self->bfr_r);
        self->bfr_l = (float*)malloc(maxBufSize*sizeof(float));
        self->bfr_r = (float*)malloc(maxBufSize*sizeof(float));
        break;
    }

    return 0;
}
/**********************************************************************************************************************************************************/
const void* extension_data(const char* uri)
{
    if (strcmp(uri, LV2_OPTIONS__interface) == 0)
    {
        static const LV2_Options_Interface options = { NULL, set_options };
        return &options;
    }
    return NULL;
}
/**********************************************************************************************************************************************************/
static const LV2_Descriptor Descriptor = {
    PLUGIN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};
/**********************************************************************************************************************************************************/
LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    if (index == 0) return &Descriptor;
    else return NULL;
}
/**********************************************************************************************************************************************************/
