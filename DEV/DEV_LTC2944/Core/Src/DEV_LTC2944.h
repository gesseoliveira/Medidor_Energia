#ifndef DEV_GASG_H_
#define DEV_GASG_H_


#include "Returncode.h"
#include "stdbool.h"

#ifndef DEV_GASG_MAX_ID
#define DEV_GASG_MAX_ID	4
#endif

#define DEV_GASG_VER_MAJOR   2019
#define DEV_GASG_VER_MINOR   8
#define DEV_GASG_VER_PATCH   1
#define DEV_GASG_VER_BRANCH_MASTER

typedef struct
{
	float	ACC_Charge;
	float	Voltage;
	float	Current;
	float	Temperature;
	float	Qlsb;
}GAS_Gauge_Data_t;

typedef enum
{
	Sleep_Mode		= 0,	// Put the device on sleep mode
	Manual_Mode		= 1,	// Manual Mode: performing single conversions of voltage, current and temperature then sleep
	Scan_Mode		= 2,	// Scan Mode: performing voltage, current and temperature conversion every 10s
	Automatic_Mode  = 3		// Automatic Mode: continuously performing voltage, current and temperature	conversions
}adc_mode_t;

/*
 * Coulomb counter prescaling factor M between 1 and 4096.
 */
typedef enum
{
	Factor_1		= 0,
	Factor_4		= 1,
	Factor_16		= 2,
	Factor_64		= 3,
	Factor_256		= 4,
	Factor_1024		= 5,
	Factor_4096		= 6
}prescaler_factor_t;

typedef enum
{
	ALCC_Disabled	= 0, // !ALCC pin disabled
	Charge_Complete	= 1, // Sets the !ALCC pin to input to receive the signal of "charge complete" and set Charge Register to FFFFh
	Alert_Mode		= 2, // Sets the !ALCC pin to output to send alarm signal to host device.
}alert_charge_config_t;

typedef struct
{
	adc_mode_t				ADC_Mode;
	prescaler_factor_t		Prescaler_Factor;
	alert_charge_config_t	Alert_Pin_Mode;
	bool					Shutdown_Status;
	float					RSense_Value;
}Dev_Gas_Gauge_Configuration_t;

/**
 * @brief	Gas Gauge device configuration routine.
 * @param	ID that should be allocated and configured.
 * @retval	Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note	Initialize the internal IIC hardware of microcontroller
 */
ReturnCode_t Dev_Gas_Gauge_IntHwInit( uint8_t ID );
/**
 * @brief  Gas Gauge configuration routine.
 * @param  ID that should be allocated and configured.
 * @param  Configuration_Parameter to setup the device operation
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
ReturnCode_t Dev_Gas_Gauge_ExtDevConfig( uint8_t ID, Dev_Gas_Gauge_Configuration_t Configuration_Parameter );
/**
 * @brief  Update microcontroller internal copy of registers.
 * @param  ID of device.
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
//ReturnCode_t Dev_Gas_Gauge_Update( uint8_t ID );

/**
 * @brief  Start a manual conversion on Gas Gauge.
 * @param  ID of device.
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
ReturnCode_t Dev_Gas_Gauge_Manual_Convert( uint8_t  ID );

/**
 * @brief  Returns a structure containing data from the battery attached
 *         to the device (Voltage, Current, Acumulated/Used Charge and Temperature).
 * @param  ID of device.
 * @param  Pointer to an GAS_Gauge_Data_t type.
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
ReturnCode_t Dev_Gas_Gauge_Get_Battery_Data( uint8_t  ID , GAS_Gauge_Data_t *Batt_Data);

/**
 * @brief  Place the device on shutdown mode.
 * @param  ID of device.
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
ReturnCode_t Dev_Gas_Gauge_ShutDown( uint8_t  ID , bool Shutdown_State);

/**
 * @brief  Program the Gas Gauge with the capacity of attached battery.
 * @param  ID of device.
 * @param  Battery capacity in Ah
 * @retval Result : Result of Operation.
 *         			This parameter can be one of the following values:
 *             		@arg ANSWERED_REQUEST: All ok, time is being counted
 *             		@arg Else: Some error happened.
 * @note
 */
ReturnCode_t Dev_Gas_Gauge_Set_Battery_Capacity( uint8_t  ID , float Battery_Capacity);

#endif /* DEV_GASG_DEV_LTC2944_H_ */
