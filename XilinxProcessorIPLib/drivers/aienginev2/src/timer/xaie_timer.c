/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaie_timer.c
* @{
*
* This file contains routines for AIE timers
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date        Changes
* ----- ------  --------    ---------------------------------------------------
* 1.0   Dishita 04/06/2020  Initial creation
* 1.1   Dishita 06/10/2020  Add XAie_WaitCycles API
* 1.2   Dishita 06/17/2020  Resolve compiler warning
* 1.3   Tejus   06/10/2020  Switch to new io backend apis.
* 1.4   Nishad  09/14/2020  Add check for invalid XAie_Reset value in
*			    XAie_SetTimerResetEvent() API.
* </pre>
*
******************************************************************************/
/***************************** Include Files *********************************/
#include <stdlib.h>

#include "xaie_events.h"
#include "xaie_helper.h"
#include "xaie_timer.h"
#include "xaiegbl.h"

/*****************************************************************************/
/***************************** Macro Definitions *****************************/
#define XAIE_TIMER_32BIT_SHIFT		32U
#define XAIE_WAIT_CYCLE_MAX_VAL		0xFFFFFFFFFFFF

/************************** Function Definitions *****************************/
/*****************************************************************************/
/**
*
* This API sets the timer trigger events value. Timer low event will be
* generated if the timer low reaches the specified low event value. Timer high
* event will be generated if the timer high reaches the specified high event
* value.
*
* @param	DevInst: Device Instance
* @param	Loc: Location of AIE tile
* @param	Module: Module of tile.
*			For AIE Tile - XAIE_MEM_MOD or XAIE_CORE_MOD,
*			For Pl or Shim tile - XAIE_PL_MOD,
*
* @param	LowEventValue: Value to set for the timer to trigger timer low
*                              event.
* @param	HighEventValue: Value to set for the timer to trigger timer
*                               high event.
*
* @return	XAIE_OK on success.
* 		XAIE_INVALID_ARGS if any argument is invalid
*		XAIE_INVALID_TILE if tile type from Loc is invalid
*
* @note
*
*******************************************************************************/
AieRC XAie_SetTimerTrigEventVal(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, u32 LowEventValue, u32 HighEventValue)
{
	u64 RegAddr;
	u8 TileType, RC;
	const XAie_TimerMod *TimerMod;

	if((DevInst == XAIE_NULL) ||
		(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAIE_ERROR("Invalid Device Instance\n");
		return XAIE_INVALID_ARGS;
	}

	TileType = _XAie_GetTileTypefromLoc(DevInst, Loc);
	if(TileType == XAIEGBL_TILE_TYPE_MAX) {
		XAIE_ERROR("Invalid Tile Type\n");
		return XAIE_INVALID_TILE;
	}

	/* check for module and tiletype combination */
	RC = _XAie_CheckModule(DevInst, Loc, Module);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Invalid Module\n");
		return XAIE_INVALID_ARGS;
	}

	if(Module == XAIE_PL_MOD) {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[0U];
	}

	else {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[Module];
	}

	/* Set up Timer low event value */
	RegAddr = _XAie_GetTileAddr(DevInst, Loc.Row ,Loc.Col) +
		TimerMod->TrigEventLowValOff;
	RC = XAie_Write32(DevInst, RegAddr, LowEventValue);
	if(RC != XAIE_OK) {
		return RC;
	}

	/* Set up Timer high event value */
	RegAddr = _XAie_GetTileAddr(DevInst, Loc.Row ,Loc.Col) +
		TimerMod->TrigEventHighValOff;
	return XAie_Write32(DevInst, RegAddr, HighEventValue);
}

/*****************************************************************************/
/**
*
* This API resets the timer
*
* @param	DevInst - Device Instance.
* @param	Loc - Location of tile.
* @param	Module - Module of the tile
*			 For AIE Tile - XAIE_MEM_MOD or XAIE_CORE_MOD,
*			 For Pl or Shim tile - XAIE_PL_MOD,
*
* @return	XAIE_OK on success.
*		XAIE_INVALID_ARGS if any argument is invalid
*		XAIE_INVALID_TILE if tile type from Loc is invalid

* @note
*
*******************************************************************************/
AieRC XAie_ResetTimer(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module)
{
	u32 RegVal, Mask;
	u64 RegAddr;
	u8 TileType, RC;
	const XAie_TimerMod *TimerMod;

	if((DevInst == XAIE_NULL) ||
		(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAIE_ERROR("Invalid Device Instance\n");
		return XAIE_INVALID_ARGS;
	}

	TileType = _XAie_GetTileTypefromLoc(DevInst, Loc);
	if(TileType == XAIEGBL_TILE_TYPE_MAX) {
		XAIE_ERROR("Invalid Tile Type\n");
		return XAIE_INVALID_TILE;
	}

	/* check for module and tiletype combination */
	RC = _XAie_CheckModule(DevInst, Loc, Module);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Invalid Module\n");
		return XAIE_INVALID_ARGS;
	}

	if(Module == XAIE_PL_MOD) {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[0U];
	}

	else {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[Module];
	}

	RegAddr = _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
		TimerMod->CtrlOff;
	Mask = TimerMod->CtrlReset.Mask;
	RegVal = XAie_SetField(XAIE_RESETENABLE, TimerMod->CtrlReset.Lsb, Mask);

	return XAie_MaskWrite32(DevInst, RegAddr, Mask, RegVal);
}

/*****************************************************************************/
/**
*
* This API sets the timer reset event. The timer will reset when the event
* is raised.
*
* @param	DevInst - Device Instance.
* @param	Loc - Location of tile.
* @param	Module - Module of the tile
*                         For AIE Tile - XAIE_MEM_MOD or XAIE_CORE_MOD,
*                         For Pl or Shim tile - XAIE_PL_MOD,
* @param	Event - Reset event.
* @param	Reset - Indicate if reset is also required in this call.
*                       (XAIE_RESETENABLE, XAIE_RESETDISABLE)
*
* @return	XAIE_OK on success.
*		XAIE_INVALID_ARGS if any argument is invalid
* 		XAIE_INVALID_TILE if tile type from Loc is invalid
*
* @note
*
*******************************************************************************/
AieRC XAie_SetTimerResetEvent(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, XAie_Events Event,
		XAie_Reset Reset)
{
	u32 RegVal;
	u64 RegAddr;
	u8 TileType, IntEvent, RC;
	const XAie_TimerMod *TimerMod;
	const XAie_EvntMod *EvntMod;

	if((DevInst == XAIE_NULL) ||
		(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAIE_ERROR("Invalid Device Instance\n");
		return XAIE_INVALID_ARGS;
	}

	if (Reset > XAIE_RESETENABLE) {
		XAIE_ERROR("Invalid reset value\n");
		return XAIE_INVALID_ARGS;
	}

	TileType = _XAie_GetTileTypefromLoc(DevInst, Loc);
	if(TileType == XAIEGBL_TILE_TYPE_MAX) {
		XAIE_ERROR("Invalid Tile Type\n");
		return XAIE_INVALID_TILE;
	}

	/* check for module and tiletype combination */
	RC = _XAie_CheckModule(DevInst, Loc, Module);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Invalid Module\n");
		return XAIE_INVALID_ARGS;
	}

	if(Module == XAIE_PL_MOD) {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[0U];
		EvntMod = &DevInst->DevProp.DevMod[TileType].EvntMod[0U];
	}

	else {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[Module];
		EvntMod = &DevInst->DevProp.DevMod[TileType].EvntMod[Module];
	}

	/* check if the event passed as input is corresponding to the module */
	if(Event < EvntMod->EventMin || Event > EvntMod->EventMax) {
		XAIE_ERROR("Invalid Event id\n");
		return XAIE_INVALID_ARGS;
	}

	/* Subtract the module offset from event number */
	Event -= EvntMod->EventMin;

	/* Getting the true event number from the enum to array mapping */
	IntEvent = EvntMod->XAie_EventNumber[Event];

	/*checking for valid true event number */
	if(IntEvent == XAIE_EVENT_INVALID) {
		XAIE_ERROR("Invalid Event id\n");
		return XAIE_INVALID_ARGS;
	}

	RegVal = XAie_SetField(IntEvent, TimerMod->CtrlResetEvent.Lsb,
			TimerMod->CtrlResetEvent.Mask);

	RegVal |= XAie_SetField(Reset, TimerMod->CtrlReset.Lsb,
			TimerMod->CtrlReset.Mask);

	RegAddr = _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
		TimerMod->CtrlOff;

	return XAie_Write32(DevInst, RegAddr, RegVal);
}

/*****************************************************************************/
/**
*
* This API returns the current value of the module's 64-bit timer.
*
* @param	DevInst - Device Instance.
* @param	Loc - Location of tile.
* @param	Module - Module of the tile
*                         For AIE Tile - XAIE_MEM_MOD or XAIE_CORE_MOD,
*                         For Pl or Shim tile - XAIE_PL_MOD,
* @param	TimerVal - Pointer to store Timer Value.
*
* @return	XAIE_OK on success
*		XAIE_INVALID_ARGS if any argument is invalid
*		XAIE_INVALID_TILE if tile type from Loc is invalid
*
* @note		None.
*
********************************************************************************/
AieRC XAie_ReadTimer(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, u64 *TimerVal)
{
	AieRC RC;
	u32 CurValHigh, CurValLow;
	u8 TileType;
	const XAie_TimerMod *TimerMod;

	if((DevInst == XAIE_NULL) || (TimerVal == XAIE_NULL) ||
		(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAIE_ERROR("Invalid Device Instance or TimerVal\n");
		return XAIE_INVALID_ARGS;
	}

	TileType = _XAie_GetTileTypefromLoc(DevInst, Loc);
	if(TileType == XAIEGBL_TILE_TYPE_MAX) {
		XAIE_ERROR("Invalid Tile Type\n");
		return XAIE_INVALID_TILE;
	}

	/* check for module and tiletype combination */
	RC = _XAie_CheckModule(DevInst, Loc, Module);
        if(RC != XAIE_OK) {
		XAIE_ERROR("Invalid Module\n");
                return XAIE_INVALID_ARGS;
	}

	if(Module == XAIE_PL_MOD) {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[0U];
	}

	else {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[Module];
	}

	/* Read the timer high and low values before wait */
	RC = XAie_Read32(DevInst, _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
			TimerMod->LowOff, &CurValLow);
	if(RC != XAIE_OK) {
		return RC;
	}

	RC = XAie_Read32(DevInst, _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
			TimerMod->HighOff, &CurValHigh);
	if(RC != XAIE_OK) {
		return RC;
	}

	*TimerVal = ((u64)CurValHigh << XAIE_TIMER_32BIT_SHIFT) | CurValLow;

	return XAIE_OK;
}

/*****************************************************************************/
/**
* This API implements a blocking wait function until the specified clock cyles
* are elapsed in the given module's 64-bit counter.
*
* @param        DevInst - Device Instance.
* @param        Loc - Location of tile.
* @param        Module - Module of the tile
*                        For AIE Tile - XAIE_MEM_MOD or XAIE_CORE_MOD,
*                        For Pl or Shim tile - XAIE_PL_MOD,
* @param        CycleCnt - No. of timer clock cycles to elapse.
*
* @return       XAIE_OK on success
*               XAIE_INVALID_ARGS if any argument is invalid
*               XAIE_INVALID_TILE if tile type from Loc is invalid
*
* @note         CycleCnt has an upper limit of 0xFFFFFFFFFFFF or 300 trillion
*		cycles to prevent overflow.
*
******************************************************************************/
AieRC XAie_WaitCycles(XAie_DevInst *DevInst, XAie_LocType Loc,
		XAie_ModuleType Module, u64 CycleCnt)
{

	u64 StartVal, EndVal, CurVal = 0U;
	u32 CurHigh, CurLow;
	u8 TileType;
	AieRC RC;
	const XAie_TimerMod *TimerMod;

	if((DevInst == XAIE_NULL) ||
		(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAIE_ERROR("Invalid Device Instance\n");
		return XAIE_INVALID_ARGS;
	}

	TileType = _XAie_GetTileTypefromLoc(DevInst, Loc);
	if(TileType == XAIEGBL_TILE_TYPE_MAX) {
		XAIE_ERROR("Invalid Tile Type\n");
		return XAIE_INVALID_TILE;
	}

	/* check for module and tiletype combination */
	RC = _XAie_CheckModule(DevInst, Loc, Module);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Invalid Module\n");
		return XAIE_INVALID_ARGS;
	}

	if(CycleCnt > XAIE_WAIT_CYCLE_MAX_VAL) {
		XAIE_ERROR("CycleCnt above max value\n");
		return XAIE_INVALID_ARGS;
	}

	if(Module == XAIE_PL_MOD) {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[0U];
	} else {
		TimerMod = &DevInst->DevProp.DevMod[TileType].TimerMod[Module];
	}

	/* Read the timer high and low values before wait */
	RC = XAie_Read32(DevInst, _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
			TimerMod->LowOff, &CurLow);
	if(RC != XAIE_OK) {
		return RC;
	}

	RC = XAie_Read32(DevInst, _XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
			TimerMod->HighOff, &CurHigh);
	if(RC != XAIE_OK) {
		return RC;
	}

	StartVal = ((u64)CurHigh << XAIE_TIMER_32BIT_SHIFT) | CurLow;
	EndVal = StartVal + CycleCnt;

	while(CurVal < EndVal) {
		/* Read the timer high and low values */
		RC = XAie_Read32(DevInst,
				_XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
				TimerMod->LowOff, &CurLow);
		if(RC != XAIE_OK) {
			return RC;
		}

		RC = XAie_Read32(DevInst,
				_XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) +
				TimerMod->HighOff, &CurHigh);
		if(RC != XAIE_OK) {
			return RC;
		}

		CurVal = ((u64)CurHigh << XAIE_TIMER_32BIT_SHIFT) | CurLow;
	}

	return XAIE_OK;
}

/*****************************************************************************/
/**
* This API returns the broadcast event enum given a resource id from the
* event map.
*
* @param        DevInst: Device Instance
* @param        Loc: Location of Tile
* @param        Mod: Module type
* @param        RscId: Specific resource to be requested
*
* @return       Event enum on success.
*
* @note         Internal only.
*
*******************************************************************************/
static XAie_Events _XAie_GetBroadcastEventfromRscId(XAie_DevInst *DevInst,
		XAie_LocType Loc, XAie_ModuleType Mod, u8 RscId)
{
	u8 TileType;
	const XAie_EvntMod *EvntMod;

	TileType =  _XAie_GetTileTypefromLoc(DevInst, Loc);

	if(Mod == XAIE_PL_MOD)
		EvntMod = &DevInst->DevProp.DevMod[TileType].EvntMod[0U];
	else
		EvntMod = &DevInst->DevProp.DevMod[TileType].EvntMod[Mod];

	return EvntMod->BroadcastEventMap->Event + RscId;
}

/*****************************************************************************/
/**
* This API clears broadcast configuration for shim tiles till the mentioned
* column.
*
*
* @param        DevInst: Device Instance
* @param        StartCol: Start column from where broadcast conf needs to be
* 			  cleared
* @param        EndCol: End column till where broadcast conf needs to be cleared
* @param        BcastChannelId: ID of broadcast channel
*
* @return       None
*
* @note         Internal only.
*
*******************************************************************************/
static void _XAie_ClearShimBroadcast(XAie_DevInst *DevInst, u8 StartCol,
		u8 EndCol, u32 BcastChannelId)
{
	for(u32 i = StartCol; i < EndCol; i++) {
		XAie_LocType Loc = XAie_TileLoc(i, 0);
		XAie_EventBroadcast(DevInst, Loc, XAIE_PL_MOD,
			BcastChannelId, XAIE_EVENT_NONE_PL);
	}
}

/*****************************************************************************/
/**
* This API clears timer configuration for all the locations in Rscs list from
* start till the index mentioned.
*
* @param        DevInst: Device Instance
* @param        Index: End index till where timer conf needs to be cleared
* @param        RscBC: list of resource
*
* @return       None
*
* @note         Internal only.
*
*******************************************************************************/
static void _XAie_ClearTimerConfig(XAie_DevInst *DevInst, u32 Index,
		XAie_UserRsc *RscsBC)
{
	const XAie_EvntMod *EvntMod;
	u8 TileType;

	for(u32 k = 0; k < Index; k++) {

		TileType = _XAie_GetTileTypefromLoc(DevInst, RscsBC[k].Loc);
		if(RscsBC[k].Mod == XAIE_PL_MOD)
			EvntMod = &DevInst->DevProp.DevMod[TileType].
				EvntMod[0U];
		else
			EvntMod = &DevInst->DevProp.DevMod[TileType].
				EvntMod[RscsBC[k].Mod];

		XAie_SetTimerResetEvent(DevInst, RscsBC[k].Loc,
			RscsBC[k].Mod, EvntMod->EventMin, XAIE_RESETDISABLE);
	}
}

/*****************************************************************************/
/**
* This API synchronizes timer for all tiles for all modules in the partition.
*
* @param	DevInst - Device Instance.
*
* @return       XAIE_OK on success
*               XAIE_INVALID_ARGS if any argument is invalid
*               XAIE_INVALID_TILE if tile type from Loc is invalid
*
* @note		None
*
******************************************************************************/
AieRC XAie_SyncTimer(XAie_DevInst *DevInst)
{
	AieRC RC;
	u32 UserRscNum = 0, BcastChannelId;
	XAie_Events BcastEvent, ShimBcastEvent;
	XAie_UserRsc *RscsBC;

	for(u8 i = 0; i < XAIEGBL_TILE_TYPE_MAX; i++) {
		if(i == XAIEGBL_TILE_TYPE_SHIMNOC)
			continue;
		UserRscNum += (DevInst->DevProp.DevMod[i].NumModules) *
			_XAie_GetNumRows(DevInst, i) * DevInst->NumCols;
	}

	RscsBC = (XAie_UserRsc *)malloc(UserRscNum * sizeof(XAie_UserRsc));
	if(RscsBC == NULL) {
		XAIE_ERROR("Unable to allocate memory for resource\n");
		return XAIE_ERR;
	}

	/* Reserve a free BC across partition */
	RC = XAie_RequestBroadcastChannel(DevInst, &UserRscNum, RscsBC, 1U);
	if(RC != XAIE_OK)
		return RC;

	BcastChannelId = RscsBC[0].RscId;
	/* Setup broadcast for all shim tiles */
	XAie_LocType Loc = XAie_TileLoc(0, 0);
	ShimBcastEvent = _XAie_GetBroadcastEventfromRscId(DevInst,
		Loc, XAIE_PL_MOD, BcastChannelId);

	for(u32 i = 0; i < DevInst->NumCols; i++) {
		XAie_LocType Loc = XAie_TileLoc(i, 0);

		RC = XAie_EventBroadcast(DevInst, Loc, XAIE_PL_MOD,
			BcastChannelId, ShimBcastEvent);
		if(RC != XAIE_OK) {
			_XAie_ClearShimBroadcast(DevInst, 0, i, BcastChannelId);
			XAIE_ERROR("Unable to configure broadcast event for timer sync\n");
			return RC;
		}
	}

	/* Configure the timer control with the trigger event */
	for(u32 j = 0; j < UserRscNum; j++) {
		BcastEvent = _XAie_GetBroadcastEventfromRscId(DevInst,
			RscsBC[j].Loc, RscsBC[j].Mod, BcastChannelId);

		RC = XAie_SetTimerResetEvent(DevInst, RscsBC[j].Loc,
			RscsBC[j].Mod, BcastEvent, XAIE_RESETDISABLE);
		if(RC != XAIE_OK) {
			_XAie_ClearTimerConfig(DevInst, j, RscsBC);
			_XAie_ClearShimBroadcast(DevInst, 0, DevInst->NumCols,
				BcastChannelId);
			XAIE_ERROR("Unable to set timer control\n");
			return RC;
		}
	}

	/* Trigger Event */
	Loc = XAie_TileLoc(0, 0);
	RC = XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, ShimBcastEvent);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Unable to trigger event\n");
		return RC;
	}

	/* Clear timer reset event register */
	_XAie_ClearTimerConfig(DevInst, UserRscNum, RscsBC);

	/* Clear shim broadcast configuration */
	_XAie_ClearShimBroadcast(DevInst, DevInst->NumCols, BcastChannelId,
		XAIE_EVENT_NONE_PL);

	/* Release broadcast channel across partition */
	RC = XAie_ReleaseBroadcastChannel(DevInst, UserRscNum, RscsBC);
	if(RC != XAIE_OK) {
		XAIE_ERROR("Unable to release resource\n");
		return RC;
	}

	free(RscsBC);

	return XAIE_OK;
}
