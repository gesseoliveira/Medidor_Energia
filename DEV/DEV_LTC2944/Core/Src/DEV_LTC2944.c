
#include "Returncode.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stm32l4xx_hal.h"
#include "DEV_LTC2944.h"

#define DEV_GASG_I2C_ADDRESS					( (uint8_t) 0xC8)

#define	DEV_GASG_Status	 					 	( (uint8_t) 0x00)
#define	DEV_GASG_Control	  					( (uint8_t) 0x01)
#define	DEV_GASG_Accumulated_Charge_MSB	  		( (uint8_t) 0x02)
#define	DEV_GASG_Accumulated_Charge_LSB	  		( (uint8_t) 0x03)
#define	DEV_GASG_Charge_Threshold_High_MSB	  	( (uint8_t) 0x04)
#define	DEV_GASG_Charge_Threshold_High_LSB	  	( (uint8_t) 0x05)
#define	DEV_GASG_Charge_Threshold_Low_MSB	  	( (uint8_t) 0x06)
#define	DEV_GASG_Charge_Threshold_Low_LSB	  	( (uint8_t) 0x07)
#define	DEV_GASG_Voltage_MSB	  				( (uint8_t) 0x08)
#define	DEV_GASG_Voltage_LSB	  				( (uint8_t) 0x09)
#define	DEV_GASG_Voltage_Threshold_High_MSB	  	( (uint8_t) 0x0A)
#define	DEV_GASG_Voltage_Threshold_High_LSB	  	( (uint8_t) 0x0B)
#define	DEV_GASG_Voltage_Threshold_Low_MSB	  	( (uint8_t) 0x0C)
#define	DEV_GASG_Voltage_Threshold_Low_LSB	  	( (uint8_t) 0x0D)
#define	DEV_GASG_Current_MSB	  				( (uint8_t) 0x0E)
#define	DEV_GASG_Current_LSB	  				( (uint8_t) 0x0F)
#define	DEV_GASG_Current_Threshold_High_MSB	  	( (uint8_t) 0x10)
#define	DEV_GASG_Current_Threshold_High_LSB	  	( (uint8_t) 0x11)
#define	DEV_GASG_Current_Threshold_Low_MSB	  	( (uint8_t) 0x12)
#define	DEV_GASG_Current_Threshold_Low_LSB	  	( (uint8_t) 0x13)
#define	DEV_GASG_Temperature_MSB	    		( (uint8_t) 0x14)
#define	DEV_GASG_Temperature_LSB 				( (uint8_t) 0x15)
#define	DEV_GASG_Temperature_Threshold_High	  	( (uint8_t) 0x16)
#define	DEV_GASG_Temperature_Threshold_Low	  	( (uint8_t) 0x17)

#define DEV_GASG_MAX_MANUAL_CONVERTION_RETRY	3
#define DEV_GASG_MANUAL_CONVERTION_OSDELAY		50

#define DEV_GASG_Voltage_CTE 	( (float) 70.8)
#define DEV_GASG_VSense_CTE		( (float) 0.064)
#define DEV_GASG_TSense_CTE		( (float) 510.0)
#define DEV_GASG_QLSB_CTE		( (float) 0.000340)
#define DEV_GASG_Rsense_CTE		( (float) 0.05)

float Perscaler_Table[] = {1.0, 4.0, 16.0, 64.0, 256.0, 1024.0, 4096.0};

uint8_t DEV_GASG_DataBuffer[23];

GAS_Gauge_Data_t				GASG_Updated_Data[DEV_GASG_MAX_ID];
Dev_Gas_Gauge_Configuration_t	Configuration_Parameters_Array[DEV_GASG_MAX_ID];

I2C_HandleTypeDef 				DEV_GAS_GAUGE_IIC_HANDLER;

ReturnCode_t Dev_Gas_Gauge_IntHwInit( uint8_t ID )
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOF_CLK_ENABLE();
	/**I2C2 GPIO Configuration
	    PF0     ------> I2C2_SDA
	    PF1     ------> I2C2_SCL
	 */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	/* I2C2 clock enable */
	__HAL_RCC_I2C2_CLK_ENABLE();

	DEV_GAS_GAUGE_IIC_HANDLER.Instance = I2C2;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.Timing = 0x307075B1;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.OwnAddress1 = 0;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.OwnAddress2 = 0;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	DEV_GAS_GAUGE_IIC_HANDLER.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&DEV_GAS_GAUGE_IIC_HANDLER) != HAL_OK)
	{
		__asm("BKPT 255");
	}
	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&DEV_GAS_GAUGE_IIC_HANDLER, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		__asm("BKPT 255");
	}
	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&DEV_GAS_GAUGE_IIC_HANDLER, 0) != HAL_OK)
	{
		__asm("BKPT 255");
	}

	return ANSWERED_REQUEST;
}

ReturnCode_t Dev_Gas_Gauge_ExtDevConfig( uint8_t ID, Dev_Gas_Gauge_Configuration_t Config_Parameters )
{
	uint8_t	Control_Register_Value[2];
	ReturnCode_t ReturnValue;
	float	Prescaler_Value_M;

	ReturnValue = OPERATION_RUNNING;

	memcpy(&Configuration_Parameters_Array[ID], &Config_Parameters, sizeof(Dev_Gas_Gauge_Configuration_t));

	Control_Register_Value[0] = 0;

	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].ADC_Mode << 6;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Prescaler_Factor << 3;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Alert_Pin_Mode << 1;
	if(Configuration_Parameters_Array[ID].Shutdown_Status)
		Control_Register_Value[0] |= 1;
	else
		Control_Register_Value[0] &= ~1;

	if(HAL_I2C_Mem_Write(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Control, I2C_MEMADD_SIZE_8BIT, Control_Register_Value, 1, 50) == HAL_OK)
	{
		Prescaler_Value_M = Perscaler_Table[Configuration_Parameters_Array[ID].Prescaler_Factor];
		GASG_Updated_Data[ID].Qlsb = DEV_GASG_QLSB_CTE *
				(DEV_GASG_Rsense_CTE / Configuration_Parameters_Array[ID].RSense_Value) *
				(Prescaler_Value_M / 4096.0);

		ReturnValue = ANSWERED_REQUEST;
	}
	else
	{
		ReturnValue = ERR_DEVICE;
	}

	return ReturnValue;
}

//ReturnCode_t Dev_Gas_Gauge_Update( uint8_t  ID )
//{
//	ReturnCode_t ReturnValue;
//	uint16_t Temp;
//	ReturnValue = OPERATION_RUNNING;
//
//
//	if(HAL_I2C_Mem_Read(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Status, I2C_MEMADD_SIZE_8BIT, DEV_GASG_DataBuffer, 23, 250) == HAL_OK) //if(IIC_ReceiveData(DEV_GASG_IIC_ID_Array[ID], DEV_GASG_Status, DEV_GASG_DataBuffer, 23) == ANSWERED_REQUEST)
//	{
//		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Voltage_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Voltage_LSB];
//		GASG_Updated_Data[ID].Voltage = DEV_GASG_Voltage_CTE * (float)(Temp / 65535.0);
//
//		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Current_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Current_LSB];
//		GASG_Updated_Data[ID].Current = (float)(DEV_GASG_VSense_CTE / Configuration_Parameters_Array[ID].RSense_Value) * (float)((Temp - 32767.0)/ 32767.0);
//
//		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Temperature_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Temperature_LSB];
//		GASG_Updated_Data[ID].Temperature = (DEV_GASG_TSense_CTE * (float)(Temp / 65535.0)) - 273.15; // Return the temperature in celcius
//
//		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Accumulated_Charge_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Accumulated_Charge_LSB];
//		GASG_Updated_Data[ID].ACC_Charge = GASG_Updated_Data[ID].Qlsb * Temp;
//		ReturnValue = ANSWERED_REQUEST;
//	}
//	else
//	{
//		ReturnValue = ERR_DEVICE;
//	}
//
//	return ReturnValue;
//}

ReturnCode_t Dev_Gas_Gauge_Get_Battery_Data( uint8_t  ID , GAS_Gauge_Data_t *Batt_Data)
{
	ReturnCode_t ReturnValue;
	uint16_t Temp;

	if(HAL_I2C_Mem_Read(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Status, I2C_MEMADD_SIZE_8BIT, DEV_GASG_DataBuffer, 23, 250) == HAL_OK) //if(IIC_ReceiveData(DEV_GASG_IIC_ID_Array[ID], DEV_GASG_Status, DEV_GASG_DataBuffer, 23) == ANSWERED_REQUEST)
	{
		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Voltage_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Voltage_LSB];
		GASG_Updated_Data[ID].Voltage = DEV_GASG_Voltage_CTE * (float)(Temp / 65535.0);

		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Current_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Current_LSB];
		GASG_Updated_Data[ID].Current = (float)(DEV_GASG_VSense_CTE / Configuration_Parameters_Array[ID].RSense_Value) * (float)((Temp - 32767.0)/ 32767.0);

		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Temperature_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Temperature_LSB];
		GASG_Updated_Data[ID].Temperature = (DEV_GASG_TSense_CTE * (float)(Temp / 65535.0)) - 273.15; // Return the temperature in celcius

		Temp = (DEV_GASG_DataBuffer[DEV_GASG_Accumulated_Charge_MSB] << 8) | DEV_GASG_DataBuffer[DEV_GASG_Accumulated_Charge_LSB];
		GASG_Updated_Data[ID].ACC_Charge = GASG_Updated_Data[ID].Qlsb * Temp;


		if(Batt_Data != NULL)
			memcpy(Batt_Data, &GASG_Updated_Data[ID], sizeof(GAS_Gauge_Data_t));
		else
			goto label_return_error;

		goto label_return_answered_request;
	}
	else
	{
		goto label_return_error;
	}


	label_return_answered_request:
	return ANSWERED_REQUEST;
	label_return_error:
	return ERR_DEVICE;
}

ReturnCode_t Dev_Gas_Gauge_ShutDown( uint8_t  ID , bool Shutdown_State)
{
	uint8_t	Control_Register_Value[2];
	ReturnCode_t ReturnValue;


	Control_Register_Value[0]  = 0;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].ADC_Mode << 6;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Prescaler_Factor << 3;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Alert_Pin_Mode << 1;

	if(Shutdown_State == true)
		Control_Register_Value[0] |=  1;
	else
		Control_Register_Value[0] &= ~1;

	if(HAL_I2C_Mem_Write(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Control, I2C_MEMADD_SIZE_8BIT, Control_Register_Value, 1, 50) == HAL_OK)
	{
		ReturnValue = ANSWERED_REQUEST;
	}
	else
	{
		ReturnValue = ERR_DEVICE;
	}

	return ReturnValue;
}

ReturnCode_t Dev_Gas_Gauge_Manual_Convert( uint8_t  ID )
{
	uint8_t	Control_Register_Value[2];
	uint8_t Retry_Counter = 0;

	Control_Register_Value[0]  = 0;
	Control_Register_Value[0] |= 0x01 << 6;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Prescaler_Factor << 3;
	Control_Register_Value[0] |= Configuration_Parameters_Array[ID].Alert_Pin_Mode << 1;
	//Control_Register_Value[0] |= 1;

	if(HAL_I2C_Mem_Write(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Control, I2C_MEMADD_SIZE_8BIT, Control_Register_Value, 1, 50) == HAL_OK)
	{
		do
		{
			HAL_Delay(50); // vTaskDelay( DEV_GASG_MANUAL_CONVERTION_OSDELAY / portTICK_PERIOD_MS );
			if(HAL_I2C_Mem_Read(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Control, I2C_MEMADD_SIZE_8BIT, Control_Register_Value, 1, 50) == HAL_OK)
			{
				if(!(Control_Register_Value[0] & 0xC0)) // Conversao completa!
				{
					goto label_return_answered_request;
				}
			}
			else
			{
				goto label_return_error;
			}
			Retry_Counter ++;
		}while(Retry_Counter > DEV_GASG_MAX_MANUAL_CONVERTION_RETRY);

		if( Retry_Counter >= DEV_GASG_MAX_MANUAL_CONVERTION_RETRY )
			goto label_return_error;
	}
	else
	{
		goto label_return_error;
	}

	label_return_answered_request:
	return ANSWERED_REQUEST;
	label_return_error:
	return ERR_DEVICE;
}

ReturnCode_t Dev_Gas_Gauge_Set_Battery_Capacity( uint8_t  ID , float Battery_Capacity)
{
	uint16_t Capacity_Value_u16;
	uint8_t Capacity_Bytes_u8[2];
	float Capacity_Value_Float;

	if(GASG_Updated_Data[ID].Qlsb > 0)
	{
		Capacity_Value_Float = Battery_Capacity / GASG_Updated_Data[ID].Qlsb;
		if(Capacity_Value_Float < 65535.0)
		{
			Capacity_Value_u16 = (uint16_t)Capacity_Value_Float;
			Capacity_Bytes_u8[0] = Capacity_Value_u16 >> 8;
			Capacity_Bytes_u8[1] = Capacity_Value_u16 & 0xFF;
		}
		else
		{
			goto label_return_error;
		}

		if(HAL_I2C_Mem_Write(&DEV_GAS_GAUGE_IIC_HANDLER, DEV_GASG_I2C_ADDRESS, DEV_GASG_Accumulated_Charge_MSB, I2C_MEMADD_SIZE_8BIT, Capacity_Bytes_u8, 2, 50) == HAL_OK)
			goto label_return_answered_request;
		else
			goto label_return_error;
	}
	else
	{
		goto label_return_error;
	}

	label_return_answered_request:
	return ANSWERED_REQUEST;
	label_return_error:
	return ERR_DEVICE;
}
