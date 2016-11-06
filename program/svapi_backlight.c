/*
 *  Copyright (C) 2016 by Desay SV automotive
 *  Code ported to DS03H -- 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* DS03H */
const char *ds03h_tft_pwr_en = "/sys/devices/1000b000.pinctrl/gpio/SOC2_TFT_EN/value";


int main(int argc, char** argv)
{
    int ret;
    static int fd_power = -1;
    int value=0;

	if (fd_power < 0)
        fd_power = open(ds03h_tft_pwr_en, O_WRONLY);

	if (fd_power < 0) {
        printf(" can not open %s\n", ds03h_tft_pwr_en);
		return -1;
	}

    if(argc != 2)
    {
        perror("num error\n");
        return -1;
    }
    if( strcmp(argv[1],"ON") == 0){
        lseek(fd_power, 0, SEEK_SET);
        ret = write(fd_power,"1", 1);
    }
   else if( strcmp(argv[1],"OFF") == 0)
   {
        lseek(fd_power, 0, SEEK_SET);
        ret = write(fd_power, "0", 1);
   }
   else{
        printf("num error!\n");
   }
    if (ret != 1) {
		printf("%s: write error ret= %d\n", __FUNCTION__, ret);
		perror("write");
		return -1;
	}

	return 0;

}
