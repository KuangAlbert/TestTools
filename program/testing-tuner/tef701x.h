/**
* @file tef701x.h
* @brief G4 platform TMC
* @copyright Huizhou Desay SV Automotive Co., Ltd.
* @par Revision History:
* @code
* Rev1.0 20130906 uida3983 Draft version
* Modified by Xie Qihuai 2015-09-16
* @endcode
*/

#ifndef _TEF701X_H_
#define _TEF701X_H_

#define SUCCESS		0
#define FAILURE		-1
typedef unsigned char u8;
typedef unsigned short u16;
/*
 * MODULE NAME
 */
#define MODULE_FM	0x20
#define MODULE_AM	0x21
#define MODULE_AUDIO	0x30
#define MODULE_APPL	0x40

/*
 * COMNAND
 */
#define FM_TUNE_TO		0x01	/* FM */
#define SET_RDS			0x51	/* FM */
#define SET_QUALITY_STATUS	0x52	/* FM */
#define SET_SPECIALS		0x55	/* FM */
#define GET_QUALITY_DATA	0x81	/* FM */
#define GET_RDS_STATUS		0x82	/* FM */
#define GET_RDS_DATA		0x83	/* FM */
#define SET_OPERATION_MODE	0x01	/* APPL */
#define SET_GPIO		0x03	/* APPL */
#define APPL_ACTIVATE		0x05	/* APPL */
#define GET_OPERATION_STATUS	0x80	/* APPL */
#define GET_IDENTIFICATION	0x82	/* APPL */
#define SET_INPUT		0x0C	/* AUDIO */
#define SET_OUTPUT_SOURCE	0x0D	/* AUDIO */
#define Set_Ana_Out  0x15
#define Set_DigitalRadio 0x1E
#define SET_DIG_IO		0x16	/* AUDIO */
#define SET_WAVEGEN		0x18	/* AUDIO */

#define VERSION_NONE	0x00
#define VERSION_V101	0x01	/* TEF7016 V101 */
#define VERSION_V102	0x02	/* TEF7016 V102 */
#define VERSION_EMPTY	0xFF

typedef struct {
	u8 MODULE;
	u8 CMD;
	u8 INDEX;
	u8 ARG1;
	u8 ARG2;
	u8 ARG3;
	u8 ARG4;
	u8 ARG5;
	u8 ARG6;
	u8 ARG7;
	u8 ARG8;
	u8 ARG9;
	u8 ARG10;
	u8 ARG11;
	u8 ARG12;
} TEF701X_COMMAND;

typedef struct {
	u16 Status;
	u16 BlockA;
	u16 BlockB;
	u16 BlockC;
	u16 BlockD;
	u16 DecError;
} TEF701X_RDS_DATA;
int Get_Signal_Status(u16 *status);
int APPL_Get_Operation_Status(u16 *status);
int APPL_Set_OperationMode(u16 mode);
int APPL_Activate(u16 act);
int APPL_Set_GPIO(u16 pin, u16 module, u16 feature);
int APPL_Identification(u16 *device, u16 *hw_version, u16 *sw_version);
int FM_Get_RDS_Status(u16 *status);
int FM_Get_RDS_Data(TEF701X_RDS_DATA *data);
int FM_Set_RDS(u16 mode, u16 restart, u16 interface);
int FM_Set_Quality_Status(u16 mode, u16 interface);
int FM_Get_Quality_Data(u16 *status, u16 *level, u16 *usn, u16 *wam, u16 *offset, u16 *bandwidth, u16 *modulation);
int FM_Tune_To(u16 mode, u16 frequency);
int FM_Set_Specials(u16 specials);
int AUDIO_Set_Input(u16 source);
int AUDIO_Set_Output_Source(u16 signal, u16 source);
int AUDIO_Set_Ana_Out(u16 signal,u16 mode);
int AUDIO_Set_Dig_IO(u16 signal, u16 mode, u16 format, u16 operation, u16 samplerate);
int AUDIO_Set_WaveGen(u16 mode, u16 offset, u16 amplitude1, u16 frequency1, u16 amplitude2, u16 frequency2);
int FM_Set_DigitalRadio(u16 signal);

#endif
