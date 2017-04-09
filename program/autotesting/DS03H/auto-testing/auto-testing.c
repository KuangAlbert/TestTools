#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "auto-testing.h"
#include "fbutils.h"

#define RED   0xff0000
#define BLUE  0x0000ff
#define GREEN 0x00ff00
#define BLACK 0x000000
#define WHITE 0xffffff

int pow_2(int num)
{
	int j, s = 1;
	for (j = 0; j < num; j++)
		s *= 2;
	return s;
}

static void draw_pixel(struct fb_info *fb_info, int x, int y, unsigned color)
{
	void *fbmem;

	fbmem = fb_info->ptr;
	if (fb_info->var.bits_per_pixel == 8) {
		unsigned char *p;

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = color;
	} else if (fb_info->var.bits_per_pixel == 16) {
		unsigned short c;
		unsigned r = (color >> 16) & 0xff;
		unsigned g = (color >> 8) & 0xff;
		unsigned b = (color >> 0) & 0xff;
		unsigned short *p;

		r = r * 32 / 256;
		g = g * 64 / 256;
		b = b * 32 / 256;

		c = (r << 11) | (g << 5) | (b << 0);

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = c;
	} else if (fb_info->var.bits_per_pixel == 24) {
		unsigned int *p;
		unsigned c;

		fbmem += fb_info->fix.line_length * y;
		fbmem += 3 * x;

		p = fbmem;

        c = *p;
        c = (c & 0xFF000000) | (color & 0x00FFFFFF);

		*p = c;
	} else {
		unsigned int *p;

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = color;
	}
}

void fill_screen_solid(struct fb_info *fb_info, unsigned int color)
{

	unsigned x, y;
	unsigned h = fb_info->var.yres;
	unsigned w = fb_info->var.xres;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
			draw_pixel(fb_info, x, y, color);
	}
}

int main(void)
{
	int ret, i;
	int fail = 0;
	char path[128] = "/usr/bin";
	char str[128];
	struct dirent *dp;
	DIR *dfd;
	CMD_GFX_IF cmd_gfx_if;
	cmd_gfx_if.prop_id = G5_DRM_ZORDER;
	cmd_gfx_if.prop_value = 0;
	int result = 0;

	int req_fb = 0;


	fb_open(req_fb, &fb_info);
	if (ret < 0) {
		printf("auto-testing: open framebuffer error\n");
		return -1;
	}
	/* drm_Setcrtc_Property(&cmd_gfx_if); */

	fill_screen_solid(&fb_info, BLACK);
	/* ui_put_string_info_center("system auto testing"); */

	put_string_center(&fb_info, 100, "DS03H system auto testing", GREEN);

	while (1) {
		printf("testing:start\n");
		printf("FA5F.F21*2A99900001E\n");

		dfd = opendir(path);
		if (dfd == NULL) {
			printf("open dir failed! dir: %s", path);
			break;
		}
		i = 0;
		for (dp = readdir(dfd); NULL != dp; dp = readdir(dfd)) {
			if (strstr(dp->d_name, "testing-") != NULL) {
				ret = system(dp->d_name);
				if (ret == 0) {
					printf("%s	---- pass\n",
						dp->d_name);
					result += pow_2(i);

					sprintf(str, "%s test    ---- pass",
						dp->d_name);
					put_string_info(&fb_info, str, i, GREEN);
				} else {
					printf("%s	---- fail\n",
						dp->d_name);
					sprintf(str, "%s test    ---- fail",
						dp->d_name);

					fail++;
					put_string_info(&fb_info, str, i, WHITE);
				}
				i++;
			}
		}
		closedir(dfd);
		if (fail == 0) {
			printf("testing:pass\n");
			printf("testing:end\n");
			sprintf(str, "testing Successful");
			put_string_info(&fb_info, str, ++i, GREEN);
			ret = 0;
		} else {
			printf("testing:fail\n");
			printf("testing:end\n");
			sprintf(str, "testing Failed ...");
			put_string_info(&fb_info, str, ++i, WHITE);
			ret = -1;
		}
		break;
	}
	printf("OS:TEST:END:%d,15\n", result);
	printf("FA5F.F21*2A99999991E\n");

	return ret;
}
