/*
 * VeJa NoiseGate
 * Copyright (C) 2022 Jan Janssen <veja.plugins@gmail.com>
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

#include "gate_core.h"

/**********************************************************************************************************************************************************/

#define PLUGIN_URI "http://moddevices.com/plugins/mod-devel/System-NoiseGate"

typedef enum {
    PLUGIN_INPUT_1,
    PLUGIN_INPUT_2,
    PLUGIN_OUTPUT_1,
    PLUGIN_OUTPUT_2,
    PLUGIN_GATE_MODE,
    PLUGIN_THRESHOLD,
    PLUGIN_DECAY
}PortIndex;

/**********************************************************************************************************************************************************/

typedef struct{
    
    //ports
    float*            input_1;
    float*            input_2;
    float*           output_1;
    float*      	 output_2;
    float*          gate_mode;
    float*          threshold;
    float*              decay;

    uint32_t     sampleRate;

    gate_t noisegate;

} NoiseGate;

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
    NoiseGate* self = (NoiseGate*)malloc(sizeof(NoiseGate));

    self->sampleRate = (uint32_t)samplerate;

    Gate_Init(&self->noisegate);

    return (LV2_Handle)self;
}
/**********************************************************************************************************************************************************/
static void connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    NoiseGate* self = (NoiseGate*)instance;

    switch (port)
    {
        case PLUGIN_INPUT_1:
            self->input_1 = (float*) data;
            break;
        case PLUGIN_INPUT_2:
            self->input_2 = (float*) data;
            break;
        case PLUGIN_OUTPUT_1:
            self->output_1 = (float*) data;
            break;
        case PLUGIN_OUTPUT_2:
            self->output_2 = (float*) data;
            break;
        case PLUGIN_GATE_MODE:
            self->gate_mode = (float*) data;
            break;
        case PLUGIN_THRESHOLD:
            self->threshold = (float*) data;
            break;
        case PLUGIN_DECAY:
            self->decay = (float*) data;
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
    NoiseGate* self = (NoiseGate*)instance;    

    //update parameters
    //lower threshold is 20dB lower
    Gate_UpdateParameters(&self->noisegate, (uint32_t)self->sampleRate, 10, 1,
                         (uint32_t)*self->decay, 1, *self->threshold, *self->threshold - 20.0f);


    for (uint32_t i = 0; i < n_samples; ++i)
    {
    	switch((int)*self->gate_mode)
    	{
    		//off, copy 1 & 2
    		case 0:
    			self->output_1[i] = self->input_1[i];
    			self->output_2[i] = self->input_2[i];
    		break;

    		//inp 1 only, copy 2
    		case 1:
                Gate_PushSamples(&self->noisegate, self->input_1[i], 0.f);
    			self->output_1[i] = Gate_RunGate(&self->noisegate, self->input_1[i]);
    			self->output_2[i] = self->input_2[i];
    		break;

    		//inp 2 only, copy 1
    		case 2:
                Gate_PushSamples(&self->noisegate, 0.f, self->input_2[i]);
    			self->output_2[i] = Gate_RunGate(&self->noisegate, self->input_2[i]);
    			self->output_1[i] = self->input_1[i];
    		break;
    	
    		//we only run the gate once, for the other just multiply
            //stereo is handled when pushing the samples to get the highest key
    		case 3:
                Gate_PushSamples(&self->noisegate, self->input_1[i], self->input_2[i]);
    			self->output_1[i] = Gate_RunGate(&self->noisegate, self->input_1[i]);
    			self->output_2[i] = Gate_ApplyGate(&self->noisegate, self->input_2[i]);
    		break;
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
    free(instance);
}
/**********************************************************************************************************************************************************/
const void* extension_data(const char* uri)
{
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
