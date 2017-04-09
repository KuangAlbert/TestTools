#include <stdio.h>
#include "fbutils.h"
#include "auto-testing_ui.h"

static int palette [] = {
	0x000000, 0x202020, 0x606060, 0x40A250, 0xffffff
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

int ui_open(void)
{
	unsigned int i, len;

	if (open_framebuffer()) {
		close_framebuffer();
		return -1;
	}

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	return 0;
}

void ui_close(void)
{
	close_framebuffer();
}

#define STEP_NUM   20
int ui_progress(int percent)
{
	int i;
	int width = 20*STEP_NUM + 2;
	int high = 30;
	int x1 = (800-width)/2;
	int y1 = 480/3;
	int step = 0;
	static int step_old = 0;

	if(percent == 0)
	{
		rect(x1, y1, x1 + width, y1 + high, 2);
		fillrect(x1 + 1, y1 +1, x1 + width - 1, y1 + high -1, 1);
		step_old = 0;
		return 0;
	}

	step = percent/(100/STEP_NUM);

	if (step_old < step)
	{
		for (i=step_old; i<step; i++)
		{
			fillrect(x1+i*20+3, y1+3, x1+i*20+18, y1+high-3, 3);
		}
		step_old = step;
	}
	return 0;
}

int ui_put_string_info(char *buf, int i)
{
	int y = 40*i;

	fillrect(230, 150+y, 700, 180+y, 0);
	put_string (230, 150+y, buf, 2);
	return 0;
}

int ui_put_string_info_center(char *buf)
{
	fillrect(310, 100, 700, 130, 0);
	put_string (310, 100, buf, 2);
	return 0;
}
