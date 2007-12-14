/***************************************************************************
 *   Copyright (C) 2005 to 2007 by Jonathan Duddington                     *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write see:                           *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

#include "StdAfx.h"

#include <stdio.h>
#include <string.h>
#include <wctype.h>

#include "speak_lib.h"
#include "speech.h"
#include "phoneme.h"
#include "synthesize.h"
#include "voice.h"
#include "translate.h"


/* Note this module is mostly old code that needs to be rewritten to
   provide a more flexible intonation system.
*/

typedef struct {
	char stress;
	char env;
	char flags;   //bit 0=pitch rising, bit1=emnphasized
	short pitch1;
	short pitch2;
} SYLLABLE;

SYLLABLE syllable_tab[N_PHONEME_LIST];


static int tone_pitch_env;    /* used to return pitch envelope */



/* Pitch data for tone types */
/*****************************/


#define    PITCHfall   0
#define    PITCHrise   1
#define    PITCHfrise  2   // and 3 must be for the varient preceded by 'r'
#define    PITCHfrise2 4   // and 5 must be the 'r' variant
#define    PITCHdrop   6
#define    PITCHdrop2  8
//#define    PITCHsemi_r 10
//#define    PITCH1fall  11
//#define    PITCH1fall2 12

/*  0  fall */
unsigned char env_fall[128] = {
 0xff, 0xfd, 0xfa, 0xf8, 0xf6, 0xf4, 0xf2, 0xf0, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe2, 0xe0,
 0xde, 0xdc, 0xda, 0xd8, 0xd6, 0xd4, 0xd2, 0xd0, 0xce, 0xcc, 0xca, 0xc8, 0xc6, 0xc4, 0xc2, 0xc0,
 0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac, 0xaa, 0xa8, 0xa6, 0xa4, 0xa2, 0xa0,
 0x9e, 0x9c, 0x9a, 0x98, 0x96, 0x94, 0x92, 0x90, 0x8e, 0x8c, 0x8a, 0x88, 0x86, 0x84, 0x82, 0x80,
 0x7e, 0x7c, 0x7a, 0x78, 0x76, 0x74, 0x72, 0x70, 0x6e, 0x6c, 0x6a, 0x68, 0x66, 0x64, 0x62, 0x60,
 0x5e, 0x5c, 0x5a, 0x58, 0x56, 0x54, 0x52, 0x50, 0x4e, 0x4c, 0x4a, 0x48, 0x46, 0x44, 0x42, 0x40,
 0x3e, 0x3c, 0x3a, 0x38, 0x36, 0x34, 0x32, 0x30, 0x2e, 0x2c, 0x2a, 0x28, 0x26, 0x24, 0x22, 0x20,
 0x1e, 0x1c, 0x1a, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x00 };

/*  1  rise */
unsigned char env_rise[128] = {
 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfd, 0xff };

unsigned char env_frise[128] = {
 0xff, 0xf4, 0xea, 0xe0, 0xd6, 0xcc, 0xc3, 0xba, 0xb1, 0xa8, 0x9f, 0x97, 0x8f, 0x87, 0x7f, 0x78,
 0x71, 0x6a, 0x63, 0x5c, 0x56, 0x50, 0x4a, 0x44, 0x3f, 0x39, 0x34, 0x2f, 0x2b, 0x26, 0x22, 0x1e,
 0x1a, 0x17, 0x13, 0x10, 0x0d, 0x0b, 0x08, 0x06, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x13, 0x15, 0x17,
 0x1a, 0x1d, 0x1f, 0x22, 0x25, 0x28, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x39, 0x3b, 0x3d, 0x40,
 0x42, 0x45, 0x47, 0x4a, 0x4c, 0x4f, 0x51, 0x54, 0x57, 0x5a, 0x5d, 0x5f, 0x62, 0x65, 0x68, 0x6b,
 0x6e, 0x71, 0x74, 0x78, 0x7b, 0x7e, 0x81, 0x85, 0x88, 0x8b, 0x8f, 0x92, 0x96, 0x99, 0x9d, 0xa0,
 0xa4, 0xa8, 0xac, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xe0 };

static unsigned char env_r_frise[128] = {
 0xcf, 0xcc, 0xc9, 0xc6, 0xc3, 0xc0, 0xbd, 0xb9, 0xb4, 0xb0, 0xab, 0xa7, 0xa2, 0x9c, 0x97, 0x92,
 0x8c, 0x86, 0x81, 0x7b, 0x75, 0x6f, 0x69, 0x63, 0x5d, 0x57, 0x50, 0x4a, 0x44, 0x3e, 0x38, 0x33,
 0x2d, 0x27, 0x22, 0x1c, 0x17, 0x12, 0x0d, 0x08, 0x04, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0a, 0x0c, 0x0d, 0x0f, 0x12, 0x14, 0x16,
 0x19, 0x1b, 0x1e, 0x21, 0x24, 0x27, 0x2a, 0x2d, 0x30, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3f, 0x41,
 0x43, 0x46, 0x48, 0x4b, 0x4d, 0x50, 0x52, 0x55, 0x58, 0x5a, 0x5d, 0x60, 0x63, 0x66, 0x69, 0x6c,
 0x6f, 0x72, 0x75, 0x78, 0x7b, 0x7e, 0x81, 0x85, 0x88, 0x8b, 0x8f, 0x92, 0x96, 0x99, 0x9d, 0xa0,
 0xa4, 0xa8, 0xac, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xe0 };

static unsigned char env_frise2[128] = {
 0xff, 0xf9, 0xf4, 0xee, 0xe9, 0xe4, 0xdf, 0xda, 0xd5, 0xd0, 0xcb, 0xc6, 0xc1, 0xbd, 0xb8, 0xb3,
 0xaf, 0xaa, 0xa6, 0xa1, 0x9d, 0x99, 0x95, 0x90, 0x8c, 0x88, 0x84, 0x80, 0x7d, 0x79, 0x75, 0x71,
 0x6e, 0x6a, 0x67, 0x63, 0x60, 0x5d, 0x59, 0x56, 0x53, 0x50, 0x4d, 0x4a, 0x47, 0x44, 0x41, 0x3e,
 0x3c, 0x39, 0x37, 0x34, 0x32, 0x2f, 0x2d, 0x2b, 0x28, 0x26, 0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a,
 0x19, 0x17, 0x15, 0x14, 0x12, 0x11, 0x0f, 0x0e, 0x0d, 0x0c, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05,
 0x05, 0x04, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08,
 0x09, 0x0a, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x12, 0x13, 0x15, 0x17, 0x18, 0x1a, 0x1c, 0x1e, 0x20 };

static unsigned char env_r_frise2[128] = {
 0xd0, 0xce, 0xcd, 0xcc, 0xca, 0xc8, 0xc7, 0xc5, 0xc3, 0xc1, 0xc0, 0xbd, 0xbb, 0xb8, 0xb5, 0xb3,
 0xb0, 0xad, 0xaa, 0xa7, 0xa3, 0xa0, 0x9d, 0x99, 0x96, 0x92, 0x8f, 0x8b, 0x87, 0x84, 0x80, 0x7c,
 0x78, 0x74, 0x70, 0x6d, 0x69, 0x65, 0x61, 0x5d, 0x59, 0x55, 0x51, 0x4d, 0x4a, 0x46, 0x42, 0x3e,
 0x3b, 0x37, 0x34, 0x31, 0x2f, 0x2d, 0x2a, 0x28, 0x26, 0x24, 0x22, 0x20, 0x1e, 0x1c, 0x1a, 0x19,
 0x17, 0x15, 0x14, 0x12, 0x11, 0x0f, 0x0e, 0x0d, 0x0c, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x05,
 0x04, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08,
 0x09, 0x0a, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x12, 0x13, 0x15, 0x17, 0x18, 0x1a, 0x1c, 0x1e, 0x20 };


/*
unsigned char env_drop[128] = {
 0xff, 0xf9, 0xf4, 0xee, 0xe9, 0xe4, 0xdf, 0xda, 0xd5, 0xd0, 0xcb, 0xc6, 0xc1, 0xbc, 0xb8, 0xb3,
 0xaf, 0xaa, 0xa6, 0xa2, 0x9d, 0x99, 0x95, 0x91, 0x8d, 0x89, 0x85, 0x81, 0x7d, 0x79, 0x76, 0x72,
 0x6f, 0x6b, 0x68, 0x64, 0x61, 0x5e, 0x5b, 0x58, 0x55, 0x52, 0x4f, 0x4c, 0x49, 0x46, 0x44, 0x41,
 0x3e, 0x3c, 0x39, 0x37, 0x35, 0x33, 0x30, 0x2e, 0x2c, 0x2a, 0x28, 0x27, 0x25, 0x23, 0x21, 0x20,
 0x1e, 0x1d, 0x1b, 0x1a, 0x19, 0x18, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x10, 0x0f, 0x0e,
 0x0e, 0x0d, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0b, 0x0b, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x12, 0x13, 0x14,
 0x15, 0x16, 0x17, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x20, 0x21, 0x23, 0x24, 0x26, 0x28, 0x2a, 0x2c };


unsigned char env_r_drop[128] = {
 0xcf, 0xce, 0xcd, 0xcc, 0xcb, 0xca, 0xc8, 0xc6, 0xc5, 0xc3, 0xc1, 0xbe, 0xbc, 0xb9, 0xb7, 0xb4,
 0xb1, 0xae, 0xab, 0xa8, 0xa5, 0xa2, 0x9e, 0x9b, 0x97, 0x94, 0x90, 0x8c, 0x89, 0x85, 0x81, 0x7d,
 0x79, 0x75, 0x71, 0x6d, 0x69, 0x65, 0x61, 0x5d, 0x59, 0x55, 0x51, 0x4e, 0x4a, 0x46, 0x42, 0x3e,
 0x3b, 0x38, 0x36, 0x34, 0x32, 0x30, 0x2e, 0x2c, 0x2b, 0x29, 0x27, 0x25, 0x24, 0x22, 0x21, 0x1f,
 0x1e, 0x1c, 0x1b, 0x1a, 0x19, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x11, 0x10, 0x0f, 0x0e,
 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0b, 0x0b, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
 0x0b, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0e, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x13,
 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x23, 0x24, 0x26, 0x28 };


unsigned char env_drop2[128] = {
 0xff, 0xfc, 0xf9, 0xf6, 0xf3, 0xf0, 0xed, 0xea, 0xe7, 0xe5, 0xe2, 0xdf, 0xdc, 0xda, 0xd7, 0xd5,
 0xd2, 0xcf, 0xcd, 0xca, 0xc8, 0xc5, 0xc3, 0xc0, 0xbe, 0xbc, 0xb9, 0xb7, 0xb4, 0xb2, 0xb0, 0xae,
 0xab, 0xa9, 0xa7, 0xa5, 0xa3, 0xa1, 0x9f, 0x9d, 0x9b, 0x99, 0x97, 0x95, 0x93, 0x91, 0x8f, 0x8d,
 0x8b, 0x89, 0x88, 0x86, 0x84, 0x83, 0x81, 0x7f, 0x7e, 0x7c, 0x7a, 0x79, 0x77, 0x76, 0x74, 0x73,
 0x71, 0x70, 0x6f, 0x6d, 0x6c, 0x6a, 0x69, 0x68, 0x67, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f,
 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x54, 0x53, 0x52, 0x51, 0x51,
 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b, 0x4a, 0x4a, 0x4a, 0x49, 0x49,
 0x49, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48 };

unsigned char env_r_drop2[128] = {
 0xcf, 0xce, 0xcd, 0xcd, 0xcc, 0xcb, 0xcb, 0xca, 0xc9, 0xc8, 0xc7, 0xc6, 0xc5, 0xc4, 0xc3, 0xc2,
 0xc1, 0xc0, 0xbf, 0xbd, 0xbc, 0xbb, 0xba, 0xb8, 0xb7, 0xb6, 0xb4, 0xb3, 0xb2, 0xb0, 0xaf, 0xad,
 0xac, 0xaa, 0xa9, 0xa7, 0xa6, 0xa4, 0xa3, 0xa1, 0x9f, 0x9e, 0x9c, 0x9a, 0x99, 0x97, 0x96, 0x94,
 0x92, 0x91, 0x8f, 0x8d, 0x8c, 0x8a, 0x88, 0x87, 0x85, 0x83, 0x82, 0x80, 0x7e, 0x7d, 0x7b, 0x7a,
 0x78, 0x76, 0x75, 0x73, 0x72, 0x70, 0x6f, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x68, 0x67, 0x66,
 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5b, 0x5a, 0x59, 0x58,
 0x58, 0x57, 0x56, 0x56, 0x55, 0x54, 0x54, 0x53, 0x52, 0x52, 0x51, 0x51, 0x50, 0x4f, 0x4f, 0x4e,
 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b, 0x4b, 0x4a, 0x4a, 0x49, 0x49, 0x49, 0x48, 0x48, 0x48 };


// 10  semi_r
unsigned char env_semi_r[128] = {
 0x00, 0x01, 0x02, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0b, 0x0c, 0x0e, 0x0f, 0x11, 0x12, 0x14, 0x15,
 0x17, 0x18, 0x1a, 0x1b, 0x1d, 0x1f, 0x20, 0x22, 0x23, 0x25, 0x27, 0x28, 0x2a, 0x2c, 0x2d, 0x2f,
 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d, 0x3f, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50,
 0x52, 0x54, 0x57, 0x59, 0x5b, 0x5d, 0x5f, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6c, 0x6e, 0x70, 0x72,
 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x87, 0x89, 0x8b, 0x8d, 0x8f, 0x90,
 0x92, 0x94, 0x96, 0x97, 0x99, 0x9b, 0x9c, 0x9e, 0x9f, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xad, 0xae, 0xaf, 0xb0, 0xb0, 0xb1, 0xb2, 0xb2, 0xb3, 0xb3,
 0xb4, 0xb4, 0xb4, 0xb5, 0xb5, 0xb6, 0xb6, 0xb6, 0xb6, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb8 };


// 11  drop3
unsigned char env_drop3[128] = {
 0xff, 0xf8, 0xf2, 0xec, 0xe7, 0xe1, 0xdb, 0xd5, 0xd0, 0xca, 0xc5, 0xc0, 0xbb, 0xb5, 0xb1, 0xac,
 0xa7, 0xa3, 0x9e, 0x9b, 0x97, 0x93, 0x90, 0x8c, 0x89, 0x86, 0x83, 0x80, 0x7d, 0x7b, 0x78, 0x76,
 0x73, 0x71, 0x6f, 0x6d, 0x6b, 0x68, 0x66, 0x64, 0x62, 0x5f, 0x5d, 0x5b, 0x59, 0x57, 0x56, 0x54,
 0x52, 0x50, 0x4e, 0x4d, 0x4b, 0x49, 0x48, 0x46, 0x44, 0x43, 0x41, 0x40, 0x3e, 0x3d, 0x3b, 0x3a,
 0x39, 0x37, 0x35, 0x34, 0x32, 0x31, 0x2f, 0x2d, 0x2c, 0x2a, 0x29, 0x28, 0x26, 0x25, 0x23, 0x22,
 0x21, 0x1f, 0x1e, 0x1d, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f,
 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x0a, 0x09, 0x08, 0x08, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04,
 0x04, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// 12  r_drop3
unsigned char env_r_drop3[128] = {
 0xcf, 0xce, 0xcd, 0xcc, 0xcb, 0xca, 0xc8, 0xc7, 0xc5, 0xc4, 0xc2, 0xc0, 0xbe, 0xbc, 0xba, 0xb8,
 0xb5, 0xb3, 0xb1, 0xae, 0xac, 0xa9, 0xa6, 0xa4, 0xa1, 0x9e, 0x9b, 0x98, 0x95, 0x92, 0x8f, 0x8c,
 0x89, 0x86, 0x83, 0x80, 0x7d, 0x7a, 0x76, 0x73, 0x70, 0x6d, 0x6a, 0x67, 0x63, 0x60, 0x5d, 0x5a,
 0x57, 0x54, 0x51, 0x4e, 0x4b, 0x48, 0x45, 0x43, 0x40, 0x3d, 0x3b, 0x39, 0x37, 0x36, 0x34, 0x33,
 0x32, 0x30, 0x2f, 0x2d, 0x2c, 0x2b, 0x2a, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20,
 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11,
 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0d, 0x0c, 0x0b, 0x0b, 0x0a, 0x09, 0x09, 0x08, 0x08, 0x07, 0x06,
 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 };
*/

   // long vowel fall
static unsigned char env_long_fall[128] = {
	254,249,250,251,252,253,254,254, 255,255,255,255,254,254,253,252,
	251,250,249,247,244,242,238,234, 230,225,221,217,213,209,206,203,
	199,195,191,187,183,179,175,172, 168,165,162,159,156,153,150,148,
	145,143,140,138,136,134,132,130, 128,126,123,120,117,114,111,107,
	104,100,96,91, 86,82,77,73, 70,66,63,60, 58,55,53,51,
	49,47,46,45, 43,42,40,38, 36,34,31,28, 26,24,22,20,
	18,16,14,12, 11,10,9,8, 8,8,8,8, 9,8,8,8,
	8,8,7,7, 6,6,6,5, 4,4,3,3, 2,1,1,0 };

 




unsigned char *envelope_data[16] = {
	env_fall,   env_rise,  env_frise,  env_r_frise,
	env_frise2, env_r_frise2,

	env_long_fall, env_long_fall, env_fall, env_fall,
	env_fall, env_fall, env_fall, env_fall,
	env_fall, env_fall
 };


/* all pitches given in Hz above pitch_base */

// pitch change during the main part of the clause
static int drops_0[8] = {0x400,0x400,0x700,0x700,0x700,0xa00,0x1800,0x0e00};
static int drops_1[8] = {0x400,0x400,0x600,0x600,0xc00,0xc00,0x0e00,0x0e00};
static int drops_2[8] = {0x400,0x400,0x600,0x600,-0x800,0xc00,0x0e00,0x0e00};

typedef struct {
   unsigned char pitch_env0;     /* pitch envelope, tonic syllable at end */
   unsigned char tonic_max0;
   unsigned char tonic_min0;

   unsigned char pitch_env1;     /*     followed by unstressed */
   unsigned char tonic_max1;
   unsigned char tonic_min1;

   unsigned char pre_start;
   unsigned char pre_end;

   unsigned char body_start;
   unsigned char body_end;

   int  *body_drops;
   unsigned char body_max_steps;
   unsigned char body_lower_u;

   unsigned char tail_start;
   unsigned char tail_end;
   unsigned char tail_shape;
} TONE_TABLE;

#define N_TONE_TABLE  15

static TONE_TABLE tone_table[N_TONE_TABLE] = {
   {PITCHfall, 30, 5,  PITCHfall, 30, 8,              // statement
   20, 25,   34, 22,  drops_0, 3, 3,   12, 7, 0},

   {PITCHfrise, 37,10, PITCHfrise2, 35,10,              // comma
   20, 25,   34, 20,  drops_0, 3, 3,   15, 24, 0},

   {PITCHfrise, 39,10, PITCHfrise2, 36,10,              // question
   20, 25,   34, 20,  drops_0, 3, 3,   15, 29, 0},

   {PITCHfall, 36, 0,  PITCHfall, 40, 30,              // statement, emphatic
   20, 25,   34, 22,  drops_0, 3, 3,   20, 4, 0},


#ifdef deleted
   {PITCHfall, 30, 5,  PITCHfall, 30, 7,              // statement
   20, 25,   34, 22,  drops_1, 3, 3,   12, 8, 0},

   {PITCHfrise, 38,10, PITCHfrise2, 36,10,              // comma, or question
   20, 25,   34, 20,  drops_1, 3, 3,   15, 25, 0},

   {PITCHfall, 30, 5,  PITCHfall, 30, 7,              // exclamation
   20, 25,   34, 22,  drops_1, 3, 3,   12, 8, 0},



   {PITCHfall, 30, 5,  PITCHfall, 30, 7,              // statement
   20, 25,   34, 22,  drops_2, 3, 3,   12, 8, 0},

   {PITCHfrise, 38,10, PITCHfrise2, 36,10,              // comma, or question
   20, 25,   34, 20,  drops_2, 3, 3,   15, 25, 0},

   {PITCHfall, 30, 5,  PITCHfall, 30, 7,              // exclamation
   20, 25,   34, 22,  drops_2, 3, 3,   12, 8, 0},



// alternatives
   {PITCHfall, 36, 6,  PITCHfall, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   12, 8, 0},
   
   {PITCHfrise, 38, 8, PITCHfrise2, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   20, 32, 0},

   {PITCHfall, 36, 6,  PITCHfall, 36, 8,
   30, 20,   18, 34,  drops_0, 3, 3,   12, 8, 0},
#endif 
};
  



/* indexed by stress */
static int min_drop[] =  {0x300,0x300,0x400,0x400,0x900,0x900,0x900,0xb00};




#define SECONDARY  3
#define PRIMARY    4
#define PRIMARY_STRESSED 6
#define PRIMARY_LAST 7


static int  number_pre;
static int  number_body;
static int  number_tail;
static int  last_primary;
static int  tone_posn;
static int  tone_posn2;
static int  no_tonic;


static void count_pitch_vowels(int start, int end, int clause_end)
/****************************************************************/
{
	int  ix;
	int  stress;
	int  max_stress = 0;
	int  max_stress_posn = 0;  // last syllable ot the highest stress
	int  max_stress_posn2 = 0;  // penuntimate syllable of the highest stress
	int  last2_primary;

	number_pre = -1;    /* number of vowels before 1st primary stress */
	number_body = 0;
	number_tail = 0;   /* number between tonic syllable and next primary */
	last_primary = 0;
	last2_primary = 0;
	
	for(ix=start; ix<end; ix++)
	{
		stress = syllable_tab[ix].stress;   /* marked stress level */

		if(stress >= max_stress)
		{
			if(stress > max_stress)
			{
				max_stress_posn2 = ix;
			}
			else
			{
				max_stress_posn2 = max_stress_posn;
			}
			max_stress_posn = ix;
			max_stress = stress;
		}
		if(stress >= PRIMARY)
		{
			if(number_pre < 0)
				number_pre = ix - start;

			last_primary = ix;
		}

	}

	if(number_pre < 0)
		number_pre = 0;

	number_tail = end - max_stress_posn - 1;
	tone_posn = max_stress_posn;
	tone_posn2 = max_stress_posn2;

	if(no_tonic)
	{
		tone_posn = tone_posn2 = end-1;
	}
	else
	if(end == clause_end)
	{
		syllable_tab[last_primary].stress = PRIMARY_LAST;
	}
}   /* end of count_pitch_vowels */




static int count_increments(int ix, int end_ix, int min_stress)
/*************************************************************/
/* Count number of primary stresses up to tonic syllable or body_reset */
{
	int  count = 0;
	int  stress;

	while(ix < end_ix)
	{
		stress = syllable_tab[ix++].stress;
//		if(stress >= PRIMARY_MARKED)
//			break;

		if(stress >= min_stress)
			count++;
	}
	return(count);
}  /* end of count_increments */



static void set_pitch(SYLLABLE *syl, int base, int drop)
/******************************************************/
// Set the pitch of a vowel in syllable_tab.  Base & drop are Hz * 256
{
	int  pitch1, pitch2;
	int  flags = 0;

	/* adjust experimentally */
	int  pitch_range2 = 148;
	int  pitch_base2 = 72;

	if(base < 0)  base = 0;

	pitch2 = ((base * pitch_range2 ) >> 15) + pitch_base2;

	if(drop < 0)
	{
		flags = 1;
		drop = -drop;
	}

	pitch1 = pitch2 + ((drop * pitch_range2) >> 15);

	if(pitch1 > 511) pitch1 = 511;
	if(pitch2 > 511) pitch2 = 511;

	syl->pitch1 = pitch1;
	syl->pitch2 = pitch2;
	syl->flags |= flags;
}   /* end of set_pitch */



static int calc_pitch_segment(int ix, int end_ix, TONE_TABLE *t, int min_stress, int continuing)
/**********************************************************************************************/
/* Calculate pitches until next RESET or tonic syllable, or end.
	Increment pitch if stress is >= min_stress.
	Used for tonic segment */
{
	int  stress;
	int  pitch=0;
	int  increment=0;
	int  n_primary=0;
	int  initial;
	int  overflow=0;
	int *drops;
	short *overflow_tab;
	SYLLABLE *syl;

	static short overflow_tab1[5] = {0, 20, 12, 4, 0};
	static short continue_tab[5] = {-14, 16, 10, 4, 0};

	drops = t->body_drops;

	if(continuing)
	{
		initial =0;
		overflow = 0;
		overflow_tab = continue_tab;
		increment = (t->body_end - t->body_start) << 8;
		increment = increment / (t->body_max_steps -1);
	}
	else
	{
		overflow_tab = overflow_tab1;
		initial = 1;
	}

	while(ix < end_ix)
	{
		syl = &syllable_tab[ix];
		stress = syl->stress;

//		if(stress == PRIMARY_MARKED)
//			initial = 1;    // reset the intonation pattern

		if(initial || (stress >= min_stress))
		{
			// a primary stress

			if(initial)
			{
				initial = 0;
				overflow = 0;
				n_primary = count_increments(ix,end_ix,min_stress);

				if(n_primary > t->body_max_steps)
					n_primary = t->body_max_steps;

				if(n_primary > 1)
				{
					increment = (t->body_end - t->body_start) << 8;
					increment = increment / (n_primary -1);
				}
				else
					increment = 0;

				pitch = t->body_start << 8;
			}
			else
			{
				if(n_primary > 0)
					pitch += increment;
				else
				{
					pitch = (t->body_end << 8) - (increment * overflow_tab[overflow++])/16;
					if(overflow > 4)
					{
						overflow = 0;
						overflow_tab = overflow_tab1;
					}
				}
			}

			n_primary--;
		}

		if(stress >= PRIMARY)
		{
			syl->stress = PRIMARY_STRESSED;
			set_pitch(syl,pitch,drops[stress]);
		}
		else
		if(stress >= SECONDARY)
		{
			set_pitch(syl,pitch,drops[stress]);
		}
		else
		{
			/* unstressed, drop pitch if preceded by PRIMARY */
			if((syllable_tab[ix-1].stress & 0x3f) >= SECONDARY)
				set_pitch(syl,pitch - (t->body_lower_u << 8), drops[stress]);
			else
				set_pitch(syl,pitch,drops[stress]);
		}

		ix++;
	}
	return(ix);
}   /* end of calc_pitch_segment */





static int calc_pitch_segment2(int ix, int end_ix, int start_p, int end_p, int min_stress)
/****************************************************************************************/
/* Linear pitch rise/fall, change pitch at min_stress or stronger
	Used for pre-head and tail */
{
	int  stress;
	int  pitch;
	int  increment;
	int  n_increments;
	int  drop;
	SYLLABLE *syl;

	if(ix >= end_ix)
		return(ix);
		
	n_increments = count_increments(ix,end_ix,min_stress);
	increment = (end_p - start_p) << 8;
	
	if(n_increments > 1)
	{
		increment = increment / n_increments;
	}

	
	pitch = start_p << 8;
	while(ix < end_ix)
	{
		syl = &syllable_tab[ix];
		stress = syl->stress;

		if(increment > 0)
		{
			set_pitch(syl,pitch,-increment);
			pitch += increment;
		}
		else
		{
			drop = -increment;
			if(drop < min_drop[stress])
				drop = min_drop[stress];
				
			pitch += increment;
			set_pitch(syl,pitch,drop);
		}
			
		ix++;
	}
	return(ix);
}   /* end of calc_pitch_segment2 */






static int calc_pitches(int start, int end,  int group_tone)
/**********************************************************/
/* Calculate pitch values for the vowels in this tone group */
{
	int  ix;
	TONE_TABLE *t;
	int  drop;
	int continuing = 0;

	if(start > 0)
		continuing = 1;

	t = &tone_table[group_tone];
	ix = start;

	/* vowels before the first primary stress */
	/******************************************/

	if(number_pre > 0)
	{
		ix = calc_pitch_segment2(ix, ix+number_pre, t->pre_start, t->pre_end, 0);
	}

	/* body of tonic segment */
	/*************************/

if(option_tone2 & 1)
{
static int count=0;
count++;
//if(count & 1)
tone_posn = tone_posn2;  // TEST  put tone on the penultimate stressed word 
}
	ix = calc_pitch_segment(ix,tone_posn, t, PRIMARY, continuing);
		
	if(no_tonic)
		return(0);

	/* tonic syllable */
	/******************/
	
	if(number_tail == 0)
	{
		tone_pitch_env = t->pitch_env0;
		drop = t->tonic_max0 - t->tonic_min0;
		set_pitch(&syllable_tab[ix++],t->tonic_min0 << 8,drop << 8);
	}
	else
	{
		tone_pitch_env = t->pitch_env1;
		drop = t->tonic_max1 - t->tonic_min1;
		set_pitch(&syllable_tab[ix++],t->tonic_min1 << 8,drop << 8);
	}

	syllable_tab[tone_posn].env = tone_pitch_env;
	if(syllable_tab[tone_posn].stress == PRIMARY)
		syllable_tab[tone_posn].stress = PRIMARY_STRESSED;

	/* tail, after the tonic syllable */
	/**********************************/
	
	calc_pitch_segment2(ix, end, t->tail_start, t->tail_end, 0);

	return(tone_pitch_env);
}   /* end of calc_pitches */






void Translator::CalcPitches_Tone(int clause_tone)
{//===============================================
//  clause_tone: 0=. 1=, 2=?, 3=! 4=none
	PHONEME_LIST *p;
	int  ix;
	int  count_stressed=0;
	int  final_stressed=0;

	int  tone_ph;
	int pause;
	int tone_promoted;
	PHONEME_TAB *tph;
	PHONEME_TAB *prev_tph;   // forget across word boundary
	PHONEME_TAB *prevw_tph;  // remember across word boundary
	PHONEME_TAB *prev2_tph;  // 2 tones previous
	PHONEME_LIST *prev_p;

	int  pitch_adjust = 10;     // pitch gradient through the clause - inital value
	int  pitch_decrement = 0;   //   decrease by this for each stressed syllable
	int  pitch_low = 10;         //   until it drops to this
	int  pitch_high = 10;       //   then reset to this

	p = &phoneme_list[0];

	// count number of stressed syllables
	p = &phoneme_list[0];
	for(ix=0; ix<n_phoneme_list; ix++, p++)
	{
		if((p->type == phVOWEL) && (p->tone >= 4))
		{
			final_stressed = ix;
			count_stressed++;
		}
	}

	// language specific, changes to tones
	if(translator_name == L('v','i'))
	{
		// LANG=vi
		p = &phoneme_list[final_stressed];
		p->tone = 7;
		if(p->tone_ph == 0)
			p->tone_ph = LookupPh("7");   // change tone 1 to falling tone at end of clause
	}


	pause = 1;
	tone_promoted = 0;

	prev_p = p = &phoneme_list[0];
	prev_tph = prevw_tph = phoneme_tab[phonPAUSE];

	// perform tone sandhi
	for(ix=0; ix<n_phoneme_list; ix++, p++)
	{
		if((p->type == phPAUSE) && (p->ph->std_length > 50))
		{
			pause = 1;  // there is a pause since the previous vowel
		}

		if(p->newword)
		{
			prev_tph = phoneme_tab[phonPAUSE];  // forget across word boundaries
		}

		if(p->type == phVOWEL)
		{
			tone_ph = p->tone_ph;
			tph = phoneme_tab[tone_ph];

			// Mandarin
			if(translator_name == L('z','h'))
			{
				if(tone_ph == 0)
				{
					if(pause || tone_promoted)
					{
						tone_ph = LookupPh("55");  // no previous vowel, use tone 1
						tone_promoted = 1;
					}
					else
					{
						tone_ph = LookupPh("11");  // default tone 5
					}

					p->tone_ph = tone_ph;
					tph = phoneme_tab[tone_ph];

				}
				else
				{
					tone_promoted = 0;
				}

				if(prev_tph->mnemonic == 0x343132)  // [214]
				{
					if(tph->mnemonic == 0x343132)   // [214]
						prev_p->tone_ph = LookupPh("35");
					else
						prev_p->tone_ph = LookupPh("21"); 
				}
				if((prev_tph->mnemonic == 0x3135)  && (tph->mnemonic == 0x3135))  //  [51] + [51]
				{
					prev_p->tone_ph = LookupPh("53");
				}

				if(tph->mnemonic == 0x3131)  // [11] Tone 5
				{
					// tone 5, change its level depending on the previous tone (across word boundaries)
					if(prevw_tph->mnemonic == 0x3535)
						p->tone_ph = LookupPh("22");
					if(prevw_tph->mnemonic == 0x3533)
						p->tone_ph = LookupPh("33");
					if(prevw_tph->mnemonic == 0x343132)
						p->tone_ph = LookupPh("44");

					// tone 5 is unstressed (shorter)
					p->tone = 1;   // diminished stress
				}
			}

			prev_p = p;
			prev2_tph = prevw_tph;
			prevw_tph = prev_tph = tph;
			pause = 0;
		}
	}

	// convert tone numbers to pitch
	p = &phoneme_list[0];
	for(ix=0; ix<n_phoneme_list; ix++, p++)
	{
		if(p->type == phVOWEL)
		{
			tone_ph = p->tone_ph;

			if(p->tone != 1)  // TEST, consider all syllables as stressed
			{
				if(ix == final_stressed)
				{
					// the last stressed syllable
					pitch_adjust = pitch_low;
				}
				else
				{
					pitch_adjust -= pitch_decrement;
					if(pitch_adjust <= pitch_low)
						pitch_adjust = pitch_high;
				}
			}

			if(tone_ph ==0)
			{
				tone_ph = phonDEFAULTTONE;  // no tone specified, use default tone 1
				p->tone_ph = tone_ph;
			}
			p->pitch1 = pitch_adjust + phoneme_tab[tone_ph]->start_type;
			p->pitch2 = pitch_adjust + phoneme_tab[tone_ph]->end_type;
		}
	}
}  // end of Translator::CalcPitches_Tone



void Translator::CalcPitches(int clause_type)
{//==========================================
//  clause_type: 0=. 1=, 2=?, 3=! 4=none
	PHONEME_LIST *p;
	SYLLABLE *syl;
	int  ix;
	int  x;
	int  st_ix;
	int n_st;
	int  option;
	int  group_tone;
	int  group_tone_emph;
	int ph_start=0;
	int st_start;
	int count;
	int n_primary;
	int count_primary;
	int ph_end=n_phoneme_list;

	if(langopts.intonation == 1)
	{
		CalcPitches_Tone(clause_type);
		return;
	}


	option = option_tone1 & 0xf;
	if(option > 4)
		option = 0;

	group_tone_emph = group_tone = punct_to_tone[option][clause_type]; 
	group_tone_emph = punct_to_tone[option][3];   // emphatic form of statement

	if(clause_type == 4)
		no_tonic = 1;       /* incomplete clause, used for abbreviations such as Mr. Dr. Mrs. */
	else
		no_tonic = 0;

	n_st = 0;
	n_primary = 0;
	for(ix=0; ix<n_phoneme_list; ix++)
	{
		p = &phoneme_list[ix];
		if(p->synthflags & SFLAG_SYLLABLE)
		{
			syllable_tab[n_st].flags = 0;
			syllable_tab[n_st++].stress = p->tone;  // stress level

			if(p->tone >= 4)
				n_primary++;
		}
	}
	syllable_tab[n_st].stress = 0;   // extra 0 entry at the end

	if(n_st == 0)
		return;  // nothing to do

	st_start = 0;
	count_primary=0;
	for(st_ix=0; st_ix<n_st; st_ix++)
	{
		syl = &syllable_tab[st_ix];

		if(syl->stress >= 4)
			count_primary++;

		if((syl->stress == 6) && (syllable_tab[st_ix+1].stress == 6))
		{
			syllable_tab[st_ix].flags = 2;
			syl->stress = 4;
		}
		if(syl->stress == 6)
		{
			// an emphasized syllable, end the tone group after the next primary stress
			syllable_tab[st_ix].flags = 2;

			count = 0;
			if((n_primary - count_primary) > 1)
				count =1;

			for(ix=st_ix+1; ix<n_st; ix++)
			{
				if(syllable_tab[ix].stress > 4)
					break;
				if(syllable_tab[ix].stress == 4)
				{
					count++;
					if(count > 1)
						break;
				}
			}

			count_pitch_vowels(st_start, ix, n_st);
			if((ix < n_st) || (clause_type == 0))
				calc_pitches(st_start, ix, group_tone_emph);   // split into > 1 tone groups, use emphatic tone
			else
				calc_pitches(st_start, ix, group_tone);

			st_start = ix;
		}
	}

	if(st_start < st_ix)
	{
		count_pitch_vowels(st_start, st_ix, n_st);
		calc_pitches(st_start, st_ix, group_tone);
	}

	
	// unpack pitch data
	st_ix=0;
	for(ix=ph_start; ix < ph_end; ix++)
	{
		p = &phoneme_list[ix];
		p->tone = syllable_tab[st_ix].stress;
		
		if(p->synthflags & SFLAG_SYLLABLE)
		{
			syl = &syllable_tab[st_ix];

			x = syl->pitch1 - 72;
			if(x < 0) x = 0;
			p->pitch1 = x;

			x = syl->pitch2 - 72;
			if(x < 0) x = 0;
			p->pitch2 = x;

			p->env = PITCHfall;
			if(syl->flags & 1)
			{
				p->env = PITCHrise;
			}
			if(p->pitch1 > p->pitch2)
			{
				// swap so that pitch2 is the higher
				x = p->pitch1;
				p->pitch1 = p->pitch2;
				p->pitch2 = x;
			}
			if(p->tone > 5)
				p->env = syl->env;

			if(syl->flags & 2)
			{
				p->tone |= 8;      // emphasized
			}
	
			st_ix++;
		}
	}

}  // end of Translator::CalcPitches

 
