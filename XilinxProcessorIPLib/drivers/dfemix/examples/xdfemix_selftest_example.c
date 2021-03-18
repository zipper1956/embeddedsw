/******************************************************************************
* Copyright (C) 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xdfemix_selftest_example.c
*
* This file contains a selftest example for using the Mixer hardware
* and Mixer driver.
* This example does some writes to the hardware to do some sanity checks.
*
* Note: MGT si570 oscillator is set to 152.25MHz by default. The DFE IP wrapper
*       requires MGT clock to be set to 122.88MHz (some IP use 61.44MHz).
*       Prerequisite is to set the MGT si570 oscillator to the required IP
*       before running the example code. This is for the ZCU208 prodaction
*       platform.
*
* <pre>
*
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- -----  -------- -----------------------------------------------------
* 1.0   dc     12/06/20 Initial version
*       dc     01/04/21 Set mgt si570 oscillator to 122.88MHz
*       dc     02/02/21 Remove hard coded device node name
*       dc     02/15/21 align driver to curent specification
*       dc     02/22/21 include HW in versioning
*
* </pre>
*
*****************************************************************************/

/***************************** Include Files ********************************/
#ifdef __BAREMETAL__
#include "xparameters.h"
#include <metal/device.h>
#endif
#include "xdfemix.h"
#include "xdfemix_hw.h"

/************************** Constant Definitions ****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#ifdef __BAREMETAL__
#define XDFEMIX_DEVICE_ID XPAR_XDFEMIX_0_DEVICE_ID
#define XDFEMIX_BASE_ADDR XPAR_XDFEMIX_0_S_AXI_BASEADDR
#else
#define XDFEMIX_DEVICE_ID 0
#endif

/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/
#ifdef __BAREMETAL__
#define printf xil_printf
#endif

#define XDFESI570_CURRENT_FREQUENCY 156.25
#define XDFESI570_NEW_FREQUENCY 122.88

/************************** Function Prototypes *****************************/
extern int XDfeSi570_SetMgtOscillator(double CurrentFrequency,
				      double NewFrequency);
static int XDfeMix_SelfTestExample(u16 DeviceId);
static int XDfeMix_AddCCTestExample(u16 DeviceId);

/************************** Variable Definitions ****************************/
#ifdef __BAREMETAL__
metal_phys_addr_t metal_phys[1] = {
	XDFEMIX_BASE_ADDR,
};
struct metal_device CustomDevice[1] = {
	{
		.name = XPAR_XDFEMIX_0_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = { {
			.virt = (void *)XDFEMIX_BASE_ADDR,
			.physmap = &metal_phys[0],
			.size = 0x10000,
			.page_shift = (u32)(-1),
			.page_mask = (u32)(-1),
			.mem_flags = 0x0,
			.ops = { NULL },
		} },
		.node = { NULL },
		.irq_num = 0,
		.irq_info = NULL,
	},
};
#define XDFEMIX_NODE_NAME XPAR_XDFEMIX_0_DEV_NAME
#else
#define XDFEMIX_NODE_NAME "xdfe_cc_mixer"
#endif

/****************************************************************************/
/**
*
* Main function that invokes the polled example in this file.
*
* @param	None.
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note		None.
*
*****************************************************************************/
int main(void)
{
	printf("DFE Mixer (MIX) Selftest Example Test\r\n");

#ifdef __BAREMETAL__
	if (XST_SUCCESS !=
	    XDfeSi570_SetMgtOscillator(XDFESI570_CURRENT_FREQUENCY,
				       XDFESI570_NEW_FREQUENCY)) {
		printf("Setting MGT oscillator failed\r\n");
		return XST_FAILURE;
	}
#endif

	/*
	 * Run the DFE Mixer init/close example, specify the Device
	 * ID that is generated in xparameters.h.
	 */
	if (XST_SUCCESS != XDfeMix_SelfTestExample(XDFEMIX_DEVICE_ID)) {
		printf("Selftest Example Test failed\r\n");
		return XST_FAILURE;
	}

	/*
	 * Run the DFE Mixer pass through example, specify the Device
	 * ID that is generated in xparameters.h.
	 */
	if (XST_SUCCESS != XDfeMix_AddCCTestExample(XDFEMIX_DEVICE_ID)) {
		printf("Pass through Example Test failed\r\n");
		return XST_FAILURE;
	}

	printf("Successfully run Selftest and Add CC Example Test\r\n");
	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function runs a test on the DFE Mixer device using the driver APIs.
* This function does the following tasks:
*	- Create and system initialize the device driver instance.
*	- Reset the device.
*	- Configure the device.
*	- Initialize the device.
*	- Activate the device.
*	- Write and read coefficient.
*	- DeActivate the device.
*
* @param	DeviceId is the instances device Id.
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note   	None
*
****************************************************************************/
static int XDfeMix_SelfTestExample(u16 DeviceId)
{
	struct metal_init_params init_param = METAL_INIT_DEFAULTS;
	XDfeMix_Cfg Cfg;
	XDfeMix *InstancePtr = NULL;

	/* Initialize libmetal */
	if (0 != metal_init(&init_param)) {
		(void)printf("ERROR: Failed to run metal initialization\n");
		return XST_FAILURE;
	}

	/* Initialize the instance of channel filter driver */
	InstancePtr = XDfeMix_InstanceInit(DeviceId, XDFEMIX_NODE_NAME);
	/* Go through initialization states of the state machine */
	XDfeMix_Reset(InstancePtr);
	XDfeMix_Configure(InstancePtr, &Cfg);
	XDfeMix_Initialize(InstancePtr);
	XDfeMix_Activate(InstancePtr, true);

	/* Write and read a dummy frequency configuration */
	XDfeMix_WriteReg(InstancePtr, XDFEMIX_FREQ_CONTROL_WORD, 0x12345678);
	if (0x12345678 !=
	    XDfeMix_ReadReg(InstancePtr, XDFEMIX_FREQ_CONTROL_WORD)) {
		return XST_FAILURE;
	}

	XDfeMix_Deactivate(InstancePtr);
	XDfeMix_InstanceClose(InstancePtr);
	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function runs a test on the DFE Mixer device using the
* driver APIs.
* This function does the following tasks:
*	- Create and system initialize the device driver instance.
*	- Reset the device.
*	- Configure the device.
*	- Initialize the device.
	- Set the triggers
*	- Activate the device.
	- Add Component Channel.
*	- DeActivate the device.
*
* @param	DeviceId is the instances device Id.
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note   	None
*
****************************************************************************/
static int XDfeMix_AddCCTestExample(u16 DeviceId)
{
	struct metal_init_params init_param = METAL_INIT_DEFAULTS;
	XDfeMix_Cfg Cfg;
	XDfeMix *InstancePtr = NULL;
	u32 CCID = 2;
	u32 NCO = 1;
	u32 Rate = 8;
	u32 FrequencyControlWord = 0x11;
	u32 FrequencySingleModCount = 0x12;
	u32 FrequencyDualModCount = 0x13;
	u32 FrequencyPhaseOffset = 0x14;
	u32 PhaseAcc = 0x15;
	u32 PhaseDualModCount = 0x16;
	u32 PhaseDualModSel = 0x17;
	u32 NCOGain = 1;
	XDfeMix_TriggerCfg TriggerCfg;
	XDfeMix_CarrierCfg CarrierCfg = { { NCO, Rate },
					  { { FrequencyControlWord,
					      FrequencySingleModCount,
					      FrequencyDualModCount,
					      { FrequencyPhaseOffset } },
					    { PhaseAcc, PhaseDualModCount,
					      PhaseDualModSel },
					    NCOGain } };

	/* Initialize libmetal */
	if (0 != metal_init(&init_param)) {
		(void)printf("ERROR: Failed to run metal initialization\n");
		return XST_FAILURE;
	}

	/* Initialize the instance of channel filter driver */
	InstancePtr = XDfeMix_InstanceInit(DeviceId, XDFEMIX_NODE_NAME);

	/* Go through initialization states of the state machine */
	XDfeMix_Reset(InstancePtr);
	XDfeMix_Configure(InstancePtr, &Cfg);
	XDfeMix_Initialize(InstancePtr);
	XDfeMix_SetTriggersCfg(InstancePtr, &TriggerCfg);
	XDfeMix_Activate(InstancePtr, false);

	/* Add channel */
	XDfeMix_AddCC(InstancePtr, CCID, &CarrierCfg);

	XDfeMix_Deactivate(InstancePtr);
	XDfeMix_InstanceClose(InstancePtr);
	return XST_SUCCESS;
}