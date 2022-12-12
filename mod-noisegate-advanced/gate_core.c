/*
  ==============================================================================

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

  ==============================================================================
*/

#include "gate_core.h"
#include "circular_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/*******************************************************************************
								global functions
*******************************************************************************/

void Gate_Init(gate_t *gate)
{
    gate->_alpha = 1.0f;
    gate->_rmsValue = 0.0f;
    gate->_keyValue = 0.0f;
    gate->_upperThreshold = 0.0f;
    gate->_lowerThreshold = 0.0f;
    gate->_attackTime = 0;
    gate->_decayTime = 0;
    gate->_holdTime = 0;
    gate->_attackCounter = 0;
    gate->_decayCounter = 0;
    gate->_holdCounter = 0;
    gate->_currentState = IDLE;
    gate->_tau = 0;

    gate->_gainFactor = 0.0f;

    ringbuffer_clear(&gate->window1, MAX_BUFFER_SIZE);
    ringbuffer_clear(&gate->window2, MAX_BUFFER_SIZE);
}

void Gate_UpdateParameters(gate_t *gate, const uint32_t sampleRate, const uint32_t attack, const uint32_t hold,
                            const uint32_t decay, const uint32_t alpha, const float upperThreshold,
                            const float lowerThreshold)
{
    gate->_tau = sampleRate * 0.001f; //sample time in ms
    
    gate->_upperThreshold = powf(10.0f, (upperThreshold / 20.0f)); // dB to level
    gate->_lowerThreshold = powf(10.0f, (lowerThreshold / 20.0f));
    gate->_attackTime = attack * gate->_tau;
    gate->_decayTime = decay * gate->_tau;
    gate->_holdTime = hold * gate->_tau;
    gate->_alpha = alpha;
}

float Gate_RunGate(gate_t *gate, const float input)
{
    switch (gate->_currentState)
    {
        case IDLE:
            gate->_rmsValue = fabs(gate->_keyValue) * 0.707106781187;

            // add a bit of hysterisis in case the gate is in atack state and the RMS is close to the threshold, avoid rapid open/close states
            if ((gate->_rmsValue < gate->_upperThreshold) && (gate->_attackCounter != 0))
            {
                gate->_rmsValue += 0.1f;
            }

            if (gate->_rmsValue > gate->_upperThreshold)
            {
                if (gate->_attackCounter > gate->_attackTime)
                {
                    gate->_currentState = HOLD;
                    gate->_holdCounter = 0;
                    gate->_attackCounter = 0;
                    gate->_gainFactor = 1.0f;
                }
                else
                {
                    gate->_attackCounter++;
                    if (gate->_attackCounter != 0) 
                        gate->_gainFactor = (powf((float)gate->_attackCounter, 2.0f) / powf((float)gate->_attackTime, 2.0f));
                    else 
                        gate->_gainFactor = 0.0f;
                }
            }
            else
            {
                if (gate->_attackCounter != 0)
                {
                    if (gate->_attackCounter > gate->_holdTime)
                    {
                        gate->_currentState = HOLD;
                        gate->_holdCounter = 0;
                        gate->_attackCounter = 0;
                        gate->_gainFactor = 1.0f;
                    }
                    else
                    {
                        gate->_currentState = DECAY;
                        gate->_decayCounter = gate->_attackCounter;
                        gate->_holdCounter = 0;
                        gate->_attackCounter = 0;
                        gate->_gainFactor = powf((float)gate->_decayCounter, 2.0f) / powf((float)gate->_attackTime, 2.0f);
                    }
                }
                else
                    gate->_gainFactor = 0.0f;
            }
        break;

        case HOLD:
            gate->_rmsValue = fabs(gate->_keyValue) * 0.707106781187;
            if (gate->_rmsValue > gate->_lowerThreshold)
                gate->_holdCounter = 0;
            else if (gate->_holdCounter < gate->_holdTime)
                gate->_holdCounter++;
            else if (gate->_holdCounter >= gate->_holdTime)
            {
                gate->_currentState = DECAY;
                gate->_decayCounter = 0;
            }

            gate->_gainFactor = 1.0f;
        break;

        case DECAY:
            gate->_rmsValue = fabs(gate->_keyValue) * 0.707106781187;
            if (gate->_rmsValue > gate->_upperThreshold)
            {
                if (gate->_attackCounter > gate->_attackTime)
                {
                    gate->_currentState = HOLD;
                    gate->_holdCounter = 0;
                    gate->_attackCounter = 0;
                    gate->_gainFactor = 1.0f;
                }
                else
                {
                    gate->_attackCounter++;
                    gate->_decayCounter++;
                    gate->_gainFactor = powf((float)gate->_decayCounter - (float)gate->_decayTime, 2.0f) / powf((float)gate->_decayTime, 2.0f);
                }
            }
            else if (gate->_decayCounter > gate->_decayTime)
            {
                gate->_currentState = IDLE;
                gate->_gainFactor = 0.0f;
            }
            else
            {
                float dif = (float)gate->_decayCounter - (float)gate->_decayTime;
                if (gate->_decayCounter != 0) 
                {
                    gate->_decayCounter++;
                    gate->_gainFactor = powf(dif, 2.0f) / powf((float)gate->_decayTime, 2.0f);
                }
                else
                {
                    gate->_decayCounter++;
                    gate->_gainFactor = 1.0f;
                }
            }
        break;
    }  

    return input * gate->_gainFactor;
}

float Gate_ApplyGate(gate_t *gate, const float input)
{
    return input * gate->_gainFactor; 
}

void Gate_PushSamples(gate_t *gate, const float input1, const float input2)
{
    float key1 = ringbuffer_push_and_calculate_power(&gate->window1, input1);
    float key2 = ringbuffer_push_and_calculate_power(&gate->window2, input2);

    //get new keyValue
    gate->_keyValue = (key1>key2) ? key1 : key2;
}