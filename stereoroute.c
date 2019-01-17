/* stereoroute -- LV2 stereo control
 *
 * Copyright (C) 2013,2015 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#define MNR_URI "http://gareus.org/oss/lv2/stereoroute"

typedef enum {
	MNR_ROUTE,
	MNR_INL,
	MNR_INR,
	MNR_OUTL,
	MNR_OUTR
} PortIndex;

typedef struct {
	/* control ports */
	float* route;
	float* input[2];
	float* output[2];
} StereoRoute;

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	StereoRoute* self = (StereoRoute*)instance;
	const int route = rintf (*self->route);

	// TODO x-fade on change ?
	
	switch (route) {
		case 0:
			// monoize -3 dB
			for (uint32_t s = 0; s < n_samples; ++s) {
				self->output[0][s] = self->output[1][s] = .7079f * (self->input[0][s] + self->input[1][s]);
			}
			break;
		case 1:
			// monoize -6 dB
			for (uint32_t s = 0; s < n_samples; ++s) {
				self->output[0][s] = self->output[1][s] = .5f * (self->input[0][s] + self->input[1][s]);
			}
			break;
		case 2:
			// monoize -0 dB
			for (uint32_t s = 0; s < n_samples; ++s) {
				self->output[0][s] = self->output[1][s] = (self->input[0][s] + self->input[1][s]);
			}
			break;
		case 3:
			// left channel mono
			if (self->input[0] != self->output[0]) {
				memcpy (self->output[0], self->input[0], n_samples * sizeof(float));
			}
			memcpy (self->output[1], self->input[0], n_samples * sizeof(float));
			break;
		case 4:
			// right channel mono
			memcpy (self->output[0], self->input[1], n_samples * sizeof(float));
			if (self->input[1] != self->output[1]) {
				memcpy (self->output[1], self->input[1], n_samples * sizeof(float));
			}
			break;
		case 5:
			// swap channels - can't use memcpy due to in-place
			for (uint32_t s = 0; s < n_samples; ++s) {
				const float tmpL = self->input[0][s];
				const float tmpR = self->input[1][s];
				self->output[0][s] = tmpR;
				self->output[1][s] = tmpL;
			}
			break;
		default:
		case 6:
			// straight
			if (self->input[0] != self->output[0]) {
				memcpy (self->output[0], self->input[0], n_samples * sizeof(float));
			}
			if (self->input[1] != self->output[1]) {
				memcpy (self->output[1], self->input[1], n_samples * sizeof(float));
			}
			break;
		case 7:
			// left/right -> mid/side
			for (uint32_t s = 0; s < n_samples; ++s) {
				const float tmpS = self->input[0][s] - self->input[1][s];
				self->output[0][s] = .5 * (self->input[0][s] + self->input[1][s]);
				self->output[1][s] = .5 * tmpS;
			}
			break;
		case 8:
			// mid/side <> left/right
			for (uint32_t s = 0; s < n_samples; ++s) {
				const float tmpS = self->input[0][s] - self->input[1][s];
				self->output[0][s] = (self->input[0][s] + self->input[1][s]);
				self->output[1][s] = tmpS;
			}
			break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	StereoRoute* self = (StereoRoute*)calloc(1, sizeof(StereoRoute));
	if (!self) return NULL;
	return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	StereoRoute* self = (StereoRoute*)instance;

	switch ((PortIndex)port) {
	case MNR_ROUTE:
		self->route = data;
		break;
	case MNR_INL:
		self->input[0] = data;
		break;
	case MNR_INR:
		self->input[1] = data;
		break;
	case MNR_OUTL:
		self->output[0] = data;
		break;
	case MNR_OUTR:
		self->output[1] = data;
		break;
	}
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	MNR_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
