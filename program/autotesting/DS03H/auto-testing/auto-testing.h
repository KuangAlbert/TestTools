/*
 * fbutils.h
 *
 * Headers for utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 */

#ifndef _AUTO_TESTING_H
#define _AUTO_TESTING_H

typedef struct {
	unsigned char prop_id;		/*GFX:G5_DRM_MODE*/
	unsigned int prop_value;	/*GFX:G5_DRM_MODE value*/
}CMD_GFX_IF;

typedef struct {
	unsigned char index;		/*videopine range:0-2*/
	unsigned char prop_id;		/*vid:G5_DRM_MODE*/
	unsigned int prop_value;	/*vid:G5_DRM_MODE value*/
}CMD_VID_IF;

typedef enum {
	G5_DRM_ROTATION = 6,
	G5_DRM_ZORDER = 7,
	G5_DRM_GLOBAL_ALPHA = 8,
	G5_DRM_PREMULT_ALPHA = 9,
	G5_DRM_TRANSKEY_MODE = 11,	/*only support GFX setup*/
	G5_DRM_TRANSKEY = 12,		/*only support GFX setup*/
	G5_DRM_BACKGROUND = 13,		/*only support GFX setup*/
	G5_DRM_ALPHA_BLENDER= 14,	/*only support GFX setup*/
} G5_DRM_MODE;

extern int drm_Setcrtc_Property(CMD_GFX_IF *cmd_gfx_if);
extern int drm_Setplane_Property(CMD_VID_IF *cmd_vid_if);
extern void Send_Frame(void);
extern unsigned char Para_Data[];

#endif /* _FBUTILS_H */
