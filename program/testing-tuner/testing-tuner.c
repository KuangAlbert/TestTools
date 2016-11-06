/**
 * Read TEF6686 ID
 * Created by Xie Qihuai 2015-09-16 for DS03
 * modified by Li Shi 2016-09-10 for DS03H
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "tef701x.h"

#define I2C_SLAVE_ADDR		0x64
#define INIT_TRY_TIMES		50


char *i2c_tuner = "/dev/i2c-2";
static int fd_power = -1;
int gl_fd = -1;
/*
static int tuner_power_en(int value)
{
	int ret;
	char buf[2];

	if (fd_power < 0)
		fd_power = open(tuner_power, O_WRONLY);

	if (fd_power < 0) {
		printf("ipod_ic_reset: can not open %s\n", tuner_power);
		return -1;
	}

	sprintf(buf, "%s\n", value? "1":"0");
	lseek(fd_power, 0, SEEK_SET);
	ret = write(fd_power, buf, 2);
	if (ret != 2) {
		printf("%s: write error ret= %d\n", __FUNCTION__, ret);
		perror("write");
		return -1;
	}

	return 0;
}
*/

static int i2c_obtain(void)
{
	int ret;

	if (gl_fd < 0)
		gl_fd = open(i2c_tuner, O_RDWR);

	if (gl_fd < 0) {
		printf("i2c_obtain: can not open %s\n", i2c_tuner);
		return -1;
	}

	ret = ioctl(gl_fd, I2C_SLAVE_FORCE, I2C_SLAVE_ADDR);
	if (ret < 0)
		printf("i2c_obtain: set address error 0x%02x\n", I2C_SLAVE_ADDR);
	return ret;
}

static int i2c_release(void)
{
	if (gl_fd) {
		close(gl_fd);
		gl_fd = -1;
	}
	return 0;
}

int Init_RDS_Hardware(void)
{
	int i;
	int ret;
	u16 Status;
	u16 device, hw_version, sw_version;
	u8 version;

	APPL_Get_Operation_Status(&Status);
	if (Status != 0)
		sent_reset_command();	/* Reset IC */

	APPL_Get_Operation_Status(&Status);
	printf("TEF701x Operation Status %d\n", Status);

	ret = load_firmware(VERSION_NONE);
	if (ret != 0)
		return -1;
	APPL_Identification(&device, &hw_version, &sw_version);
	printf("TEF701x device     0x%04X \n", device);
	printf("TEF701x hw_version 0x%04X \n", hw_version);
	printf("TEF701x sw_version 0x%04X \n", sw_version);
	if (sw_version == 0x0101)
		version = VERSION_V101;
	else if (sw_version == 0x0200)
		version = VERSION_V102;
	else
		return -1;

	/* Reset IC */
	sent_reset_command();

	ret = load_firmware(version);
	if (ret != 0) {
		printf("TEF701x load firmware fail\n");
		return -1;
	}

	APPL_Set_OperationMode(1);
	APPL_Get_Operation_Status(&Status);
	printf("TEF701x Operation Status %d\n", Status);	/* idle state = 1 */

	/* appl goto active mode */
	APPL_Activate(1);
#if 0
	/* Requires 100ms delay */
	usleep(100000);
#else
	for (i=0; i<INIT_TRY_TIMES; i++) {
		APPL_Get_Operation_Status(&Status);
		if (Status == 2)
			break;
		usleep(10000);
	}
	if (i==INIT_TRY_TIMES)
		return -1;
#endif
	APPL_Get_Operation_Status(&Status);
	printf("TEF701x Operation Status %d\n", Status);	/* APPL state = 2 */

	if (Status != 2)
		return -1;
	return 0;
}

int main(int argc, char** argv)
{
	int i;
	int ret;
	u16 freq;
	u16 Status;
	//tuner_power_en(1);
	//usleep(30000);
	ret = i2c_obtain();
	if (ret < 0)
		goto end;
	ret = Init_RDS_Hardware();
	if (ret < 0) {
		goto end;
	}
	printf("TEF701x initialize Done\n");

#if 1
	if (argc > 1) {
        printf("\n\n**********************************************\n\n");
        if( strcmp(argv[1],"0") == 0){
            AUDIO_Set_Ana_Out(128,1); /* enable analog output */
            printf("enble analog output!\nDAC output!\n");
            //AUDIO_Set_Dig_IO(33, 2, 16, 256, 4800);
			 AUDIO_Set_Output_Source(128, 4); /* I2S output && analog radio */
        }
        else if(strcmp(argv[1],"1") == 0){

			 AUDIO_Set_Output_Source(33, 4); /* I2S output && analog radio */
            if(strcmp(argv[3],"master") == 0)
            {
                AUDIO_Set_Dig_IO(33, 2, 32, 256, 4800);
                printf("Tuner I2S master mode 48KHZ fs=32\ni2s output!");
            }
            else if(strcmp(argv[3],"slave") == 0)
            {
                AUDIO_Set_Dig_IO(33, 2, 32, 0, 4800);
                printf("Tuner I2S slave mode 48KHZ fs=32\ni2s output!");
            }
        }
        else
        {
            printf("Para/meter error!!\n");
            goto end;
        }

		if (strcmp(argv[2], "wave") == 0) {
			/* Gen Wave */
			AUDIO_Set_WaveGen(5, 0, -100, 400, -200, 1000);		/* Set -10 dB, 400 Hz sine wave */
			AUDIO_Set_Input(240);/* sine wave generator */
			AUDIO_Set_Output_Source(128, 240);
			printf("TEF701x Gen Wave\n");
		}
        else
        {
            //FM_Set_DigitalRadio(1); /* Enable digital radio for FM/AM use */
            freq = strtoul(argv[2], NULL, 10);
			printf("Tune to %d\n", freq);				/* Maybe 9880 is ok */
			FM_Tune_To(1, freq);

			 APPL_Get_Operation_Status(&Status);
			 printf("TEF701x Operation Status %d\n", Status);	/* FM state = 3 */

			 FM_Set_Quality_Status(32968, 2);
			 AUDIO_Set_Input(0); /* input from radio */
		 }
        }
          
		i2c_release();
		return ret;
#endif
end:
    i2c_release();
//	tuner_power_en(0);
	return ret;
}
