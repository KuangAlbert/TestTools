#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "svapi.h"
#include "auto-testing_ui.h"
extern int drm_Setcrtc_Property(CMD_GFX_IF *cmd_gfx_if);
extern int drm_Setplane_Property(CMD_VID_IF *cmd_vid_if);

extern void Send_Frame(void);
extern unsigned char Para_Data[];


int pow_2(int num) {
	int j,s=1;
	for(j = 0; j<num; j++) {
		s *= 2;
	}
	return s;
}

int main()
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
	int result=0;

	ret = ui_open();
	if (ret < 0) {
		printf("auto-testing: open framebuffer error\n");
		return -1;
	}
	drm_Setcrtc_Property(&cmd_gfx_if);
	ui_put_string_info_center("system auto testing");

	while(1) {
		printf("testing:start\n");
		printf("FA5F.F21*2A99900001E\n");

		if ((dfd = opendir(path)) == NULL ) {
			printf("open dir failed! dir: %s", path);
			break;
		}

		i = 0;
		for (dp = readdir(dfd); NULL!=dp; dp = readdir(dfd)) {
			if (strstr(dp->d_name,"testing-")!=NULL) {
				ret = system(dp->d_name);
				if (ret == 0) {
 					printf("%s	---- pass\n", dp->d_name);
					result += pow_2(i);
					Para_Data[8] = '1';
					sprintf(str, "%s test    ---- pass", dp->d_name);
				} else {
					printf("%s	---- fail\n", dp->d_name);
					sprintf(str, "%s test    ---- fail", dp->d_name);
					Para_Data[8] = '0';
					fail++;
				}
				ui_put_string_info(str, i);
				
				Para_Data[3] = (i+48);
				Send_Frame();
				i++;
			}
		}
		closedir(dfd);

		if (fail == 0) {
			printf("testing:pass\n");
			printf("testing:end\n");
			sprintf(str, "testing Successful");
			ui_put_string_info(str, ++i);
			ret = 0;
		} else {
			printf("testing:fail\n");
			printf("testing:end\n");
			sprintf(str, "testing Failed ...");
			ui_put_string_info(str, ++i);
			ret = -1;
		}
		break;
	}
	printf("OS:TEST:END:%d,15\n",result);
	printf("FA5F.F21*2A99999991E\n");

	return ret;
}
