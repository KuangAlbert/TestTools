#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "tef701x.h"

/* #define DEBUG */
#define INIT_TRY_TIMES	50

extern const size_t PatchSize_v101;
extern const unsigned char *pPatchBytes_v101;
extern const size_t LutSize_v101;
extern const unsigned char *pLutBytes_v101;

extern const size_t PatchSize_v102;
extern const unsigned char *pPatchBytes_v102;
extern const size_t LutSize_v102;
extern const unsigned char *pLutBytes_v102;

extern int gl_fd;

int send_data_to_tmc(char *pbuf, int len)
{
	if (gl_fd < 0)
		return -1;
	return write(gl_fd, pbuf, len);
}

int read_date_from_tmc(char *pbuf, int len)
{
	if (gl_fd < 0)
		return -1;
	return read(gl_fd, pbuf, len);
}

/*
 * Sends a command to the part and returns the reply bytes
 */
int tef701x_command(int cmd_size, u8 *cmd, int reply_size, u8 *reply)
{
#ifdef DEBUG
	int i;
#endif
	int size;
	/* Write the command to the part */
	size = send_data_to_tmc(cmd, cmd_size);
	if (size != cmd_size) {
		perror("tef701x_command read");
		return FAILURE;
	}

#if 0
	printf("command: ");
	for (i=0; i<cmd_size; i++)
	{
		printf("0x%02x ", *(cmd+i));
	}
	printf("\n");
#endif

	/* If the calling function would like to have results then read them. */
	if (reply_size)
	{
		size = read_date_from_tmc(reply, reply_size);
		if (size != reply_size) {
			perror("tef701x_command write");
			return FAILURE;
		}
	}

#ifdef DEBUG
	if (reply_size)
	{
		printf("relay: ");
		for(i = 0; i < reply_size; i++)
			printf("0x%02x ", *(reply + i));
		printf("\n");
	}
#endif
	return SUCCESS;
}



int Get_Signal_Status(u16 *status)
{
    int ret;
    TEF701X_COMMAND command;
    u8 rsp[2];

    command.MODULE = 0x20;
    command.CMD = 0x85;
    command.INDEX = 0x01;
    ret = tef701x_command(3, (u8 *)&command, 2, rsp);
    *status = rsp[1] | ((u16)rsp[0] << 8);

    return ret;
}

int APPL_Get_Operation_Status(u16 *status)
{
	int ret;
	TEF701X_COMMAND command;
	u8 rsp[2];

	command.MODULE = MODULE_APPL;
	command.CMD = GET_OPERATION_STATUS;
	command.INDEX = 0x01;
	ret = tef701x_command(3, (u8 *)&command, 2, rsp);
	*status = rsp[1] | ((u16)rsp[0] << 8);

	return ret;
}

int APPL_Set_OperationMode(u16 mode)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_APPL;
	command.CMD = SET_OPERATION_MODE;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((mode >> 8) & 0xFF);
	command.ARG2 = (u8)(mode & 0xFF);
	return tef701x_command(5, (u8 *)&command, 0, NULL);
}

int APPL_Activate(u16 act)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_APPL;
	command.CMD = APPL_ACTIVATE;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((act >> 8) & 0xFF);
	command.ARG2 = (u8)(act & 0xFF);
	return tef701x_command(5, (u8 *)&command, 0, NULL);
}

int FM_Get_RDS_Status(u16 *status)
{
	int ret;
	TEF701X_COMMAND command;
	u8 rsp[2];

	command.MODULE = MODULE_FM;
	command.CMD = GET_RDS_STATUS;
	command.INDEX = 0x01;
	ret = tef701x_command(3, (u8 *)&command, 2, rsp);
	*status = rsp[1] | ((u16)rsp[0] << 8);

	return ret;
}

int FM_Get_RDS_Data(TEF701X_RDS_DATA *data)
{
	int ret;
	TEF701X_COMMAND command;
	u8 rsp[12];

	command.MODULE = MODULE_FM;
	command.CMD = GET_RDS_DATA;
	command.INDEX = 0x01;
	ret = tef701x_command(3, (u8 *)&command, 12, rsp);

	data->Status = rsp[1] | ((u16)rsp[0] << 8);
	data->BlockA = rsp[3] | ((u16)rsp[2] << 8);
	data->BlockB = rsp[5] | ((u16)rsp[4] << 8);
	data->BlockC = rsp[7] | ((u16)rsp[6] << 8);
	data->BlockD = rsp[9] | ((u16)rsp[8] << 8);
	data->DecError = rsp[11] | ((u16)rsp[10] << 8);

#if 0
	printf("RDS Data output:\n");
	printf("Status 0x%04x \n", data->Status);
	printf("BlockA 0x%04x \n", data->BlockA);
	printf("BlockB 0x%04x \n", data->BlockB);
	printf("BlockC 0x%04x \n", data->BlockC);
	printf("BlockD 0x%04x \n", data->BlockD);
	printf("DecError 0x%04x \n", data->DecError);
#endif

	return ret;
}

int FM_Set_RDS(u16 mode, u16 restart, u16 interface)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_FM;
	command.CMD = SET_RDS;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((mode >> 8) & 0xFF);
	command.ARG2 = (u8)(mode & 0xFF);
	command.ARG3 = (u8)((restart >> 8) & 0xFF);
	command.ARG4 = (u8)(restart & 0xFF);
	command.ARG5 = (u8)((interface >> 8) & 0xFF);
	command.ARG6 = (u8)(interface & 0xFF);
	return tef701x_command(9, (u8 *)&command, 0, NULL);
}

int APPL_Set_GPIO(u16 pin, u16 module, u16 feature)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_APPL;
	command.CMD = SET_GPIO;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((pin >> 8) & 0xFF);
	command.ARG2 = (u8)(pin & 0xFF);
	command.ARG3 = (u8)((module >> 8) & 0xFF);
	command.ARG4 = (u8)(module & 0xFF);
	command.ARG5 = (u8)((feature >> 8) & 0xFF);
	command.ARG6 = (u8)(feature & 0xFF);
	return tef701x_command(9, (u8 *)&command, 0, NULL);
}

int APPL_Identification(u16 *device, u16 *hw_version, u16 *sw_version)
{
	int ret;
	TEF701X_COMMAND command;
	u8 rsp[6];
	command.MODULE = MODULE_APPL;
	command.CMD = GET_IDENTIFICATION;
	command.INDEX = 0x01;

	ret = tef701x_command(3, (u8 *)&command, 6, rsp);

	*device = rsp[1] | ((u16)rsp[0] << 8);
	*hw_version = rsp[3] | ((u16)rsp[2] << 8);
	*sw_version = rsp[5] | ((u16)rsp[4] << 8);

	return ret;
}

int FM_Set_Quality_Status(u16 mode, u16 interface)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_FM;
	command.CMD = SET_QUALITY_STATUS;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((mode >> 8) & 0xFF);
	command.ARG2 = (u8)(mode & 0xFF);
	command.ARG3 = (u8)((interface >> 8) & 0xFF);
	command.ARG4 = (u8)(interface & 0xFF);
	return tef701x_command(7, (u8 *)&command, 0, NULL);
}

int FM_Get_Quality_Data(u16 *status, u16 *level, u16 *usn, u16 *wam, u16 *offset, u16 *bandwidth, u16 *modulation)
{
	int ret;
	TEF701X_COMMAND command;
	u8 rsp[14];
	command.MODULE = MODULE_FM;
	command.CMD = GET_QUALITY_DATA;
	command.INDEX = 0x01;

	ret = tef701x_command(3, (u8 *)&command, 14, rsp);

	*status = rsp[1] | ((u16)rsp[0] << 8);
	*level = rsp[3] | ((u16)rsp[2] << 8);
	*wam = rsp[5] | ((u16)rsp[4] << 8);
	*offset = rsp[7] | ((u16)rsp[6] << 8);
	*bandwidth = rsp[9] | ((u16)rsp[8] << 8);
	*modulation = rsp[11] | ((u16)rsp[10] << 8);

	return ret;
}

int FM_Tune_To(u16 mode, u16 frequency)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_FM;
	command.CMD = FM_TUNE_TO;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((mode >> 8) & 0xFF);
	command.ARG2 = (u8)(mode & 0xFF);
	command.ARG3 = (u8)((frequency >> 8) & 0xFF);
	command.ARG4 = (u8)(frequency & 0xFF);
	return tef701x_command(7, (u8 *)&command, 0, NULL);
}

int FM_Set_Specials(u16 specials)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_FM;
	command.CMD = SET_SPECIALS;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((specials >> 8) & 0xFF);
	command.ARG2 = (u8)(specials & 0xFF);
	return tef701x_command(5, (u8 *)&command, 0, NULL);
}

int AUDIO_Set_Input(u16 source)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_AUDIO;
	command.CMD = SET_INPUT;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((source >> 8) & 0xFF);
	command.ARG2 = (u8)(source & 0xFF);

	return tef701x_command(5, (u8 *)&command, 0, NULL);
}

int AUDIO_Set_Output_Source(u16 signal, u16 source)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_AUDIO;
	command.CMD = SET_OUTPUT_SOURCE;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((signal >> 8) & 0xFF);
	command.ARG2 = (u8)(signal & 0xFF);
	command.ARG3 = (u8)((source >> 8) & 0xFF);
	command.ARG4 = (u8)(source & 0xFF);
	return tef701x_command(7, (u8 *)&command, 0, NULL);
}


int AUDIO_Set_Ana_Out(u16 signal,u16 mode)
{

    TEF701X_COMMAND command;

    command.MODULE = MODULE_AUDIO;
    command.CMD = Set_Ana_Out;
    command.INDEX = 0x01;
    command.ARG1 = (u8)((signal >> 8) & 0xFF);
    command.ARG2 = (u8)(signal & 0xFF);
    command.ARG3 = (u8)((mode >> 8) & 0xFF);
    command.ARG4 = (u8)(mode & 0xFF);
    return tef701x_command(7, (u8 *)&command, 0, NULL);
}

int FM_Set_DigitalRadio(u16 signal)
{
    TEF701X_COMMAND command;

    command.MODULE = MODULE_FM;
    command.CMD = Set_DigitalRadio;
    command.INDEX = 0x01;
    command.ARG1 = (u8)((signal >> 8) & 0xFF);
    command.ARG2 = (u8)(signal & 0xFF);
    return tef701x_command(5, (u8 *)&command, 0, NULL);
}

/*
 * signal:     33 = I²S digital audio IIS_SD_1 (output)
 * mode:       0 = off (default) 2 = output (only available for signal = 33)
 * fomat:      32 = I²S 32 bits (fIIS_BCK = 64 * samplerate) (default)
 * operation:  0 = slave mode; IIS_BCK and IIS_WS input defined by source (default)
 * samplerate: 4410 = 44.1 kHz (default)
*/
int AUDIO_Set_Dig_IO(u16 signal, u16 mode, u16 format, u16 operation, u16 samplerate)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_AUDIO;
	command.CMD = SET_DIG_IO;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((signal >> 8) & 0xFF);
	command.ARG2 = (u8)(signal & 0xFF);
	command.ARG3 = (u8)((mode >> 8) & 0xFF);
	command.ARG4 = (u8)(mode & 0xFF);
	command.ARG5 = (u8)((format >> 8) & 0xFF);
	command.ARG6 = (u8)(format & 0xFF);
	command.ARG7 = (u8)((operation >> 8) & 0xFF);
	command.ARG8 = (u8)(operation & 0xFF);
	command.ARG9 = (u8)((samplerate >> 8) & 0xFF);
	command.ARG10 = (u8)(samplerate & 0xFF);
	return tef701x_command(13, (u8 *)&command, 0, NULL);
}

int AUDIO_Set_WaveGen(u16 mode, u16 offset, u16 amplitude1, u16 frequency1, u16 amplitude2, u16 frequency2)
{
	TEF701X_COMMAND command;

	command.MODULE = MODULE_AUDIO;
	command.CMD = SET_WAVEGEN;
	command.INDEX = 0x01;
	command.ARG1 = (u8)((mode >> 8) & 0xFF);
	command.ARG2 = (u8)(mode & 0xFF);
	command.ARG3 = (u8)((offset >> 8) & 0xFF);
	command.ARG4 = (u8)(offset & 0xFF);
	command.ARG5 = (u8)((amplitude1 >> 8) & 0xFF);
	command.ARG6 = (u8)(amplitude1 & 0xFF);
	command.ARG7 = (u8)((frequency1 >> 8) & 0xFF);
	command.ARG8 = (u8)(frequency1 & 0xFF);
	command.ARG9 = (u8)((amplitude2 >> 8) & 0xFF);
	command.ARG10 = (u8)(amplitude2 & 0xFF);
	command.ARG11 = (u8)((frequency2 >> 8) & 0xFF);
	command.ARG12 = (u8)(frequency2 & 0xFF);
	return tef701x_command(15, (u8 *)&command, 0, NULL);
}

int firmware_data_control(void)
{
	char cmd[3];

	cmd[0] = 0x1C;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	return tef701x_command(3, cmd, 0, NULL);
}

int firmware_data_control_1(void)
{
	char cmd[3];

	cmd[0] = 0x1C;
	cmd[1] = 0x00;
	cmd[2] = 0x74;
	return tef701x_command(3, cmd, 0, NULL);
}

int firmware_data_control_2(void)
{
	char cmd[3];

	cmd[0] = 0x1C;
	cmd[1] = 0x00;
	cmd[2] = 0x75;
	return tef701x_command(3, cmd, 0, NULL);
}

int start_firmware(void)
{
	char cmd[3];

	cmd[0] = 0x14;
	cmd[1] = 0x00;
	cmd[2] = 0x01;
	return tef701x_command(3, cmd, 0, NULL);
}

int load_initializtiaon_data(const unsigned char *pLoad_data, int data_size)
{
	unsigned char buf[25];
	int i, j, ret;
	int load_left=0;
	int load_count=0;

	buf[0] = 0x1B;
	load_count = data_size/24;
	load_left = data_size%24;

	for (i=0; i<load_count; i++)
	{
		for (j=1; j<25; j++)
		{
			buf[j] = *pLoad_data;
			pLoad_data++;
		}
		ret = tef701x_command(25, buf, 0, NULL);
		if (ret != SUCCESS)
			return FAILURE;
	}

	if (load_left != 0)
	{
		for (j=1; j<=load_left; j++)
		{
			buf[j] = *pLoad_data;
			pLoad_data++;
		}
		ret = tef701x_command(25, buf, 0, NULL);
		if (ret != SUCCESS)
			return FAILURE;
	}
	return SUCCESS;
}

int ReStart_RDS()
{
	FM_Set_RDS(0, 0, 0);
	usleep(100);
	FM_Set_RDS(1, 2, 2);
	return 0;
}

int Stop_RDS()
{
	FM_Set_RDS(0, 0, 0);
	return 0;
}

int sent_reset_command(void)
{
	TEF701X_COMMAND command;

	command.MODULE = 0x1e;
	command.CMD = 0x5a;
	command.INDEX = 0x01;
	command.ARG1 = 0x5a;
	command.ARG2 = 0x5a;
	return tef701x_command(5, (u8 *)&command, 0, NULL);
}

int load_firmware(u8 version)
{
	int i;
	u16 Status;

	/* The source for required initialization transmissions */
	firmware_data_control();
	firmware_data_control_1();
	/* load firmware */
	if (version == VERSION_V101)
		load_initializtiaon_data(pPatchBytes_v101, PatchSize_v101);
	if (version == VERSION_V102)
		load_initializtiaon_data(pPatchBytes_v102, PatchSize_v102);

	firmware_data_control();
	firmware_data_control_2();
	/* load lut */
	if (version == VERSION_V101)
		load_initializtiaon_data(pLutBytes_v101, LutSize_v101);
	if (version == VERSION_V102)
		load_initializtiaon_data(pLutBytes_v102, LutSize_v102);

	firmware_data_control();
	start_firmware();

#if 0
	/* Requires 50ms delay */
	usleep(50000);
#else
	for (i=0; i<INIT_TRY_TIMES; i++) {
		APPL_Get_Operation_Status(&Status);
		if (Status == 1)
			break;
		usleep(10000);
	}
	if (i==INIT_TRY_TIMES)
		return -1;
#endif
	return 0;
}
