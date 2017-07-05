/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************
 
    Module Name:
 
    Abstract:
*/


#include "rt_config.h"
#include "bgnd_scan.h"

//extern MT_SWITCH_CHANNEL_CFG CurrentSwChCfg[2];

static inline INT GetABandChOffset(
	IN INT Channel)
{
#ifdef A_BAND_SUPPORT
	if ((Channel == 36) || (Channel == 44) || (Channel == 52) || (Channel == 60) || (Channel == 100) || (Channel == 108) ||
	    (Channel == 116) || (Channel == 124) || (Channel == 132) || (Channel == 149) || (Channel == 157))
	{
		return 1;
	}
	else if ((Channel == 40) || (Channel == 48) || (Channel == 56) || (Channel == 64) || (Channel == 104) || (Channel == 112) ||
			(Channel == 120) || (Channel == 128) || (Channel == 136) || (Channel == 153) || (Channel == 161))
	{
		return -1;
	}
#endif /* A_BAND_SUPPORT */
	return 0;
}

UCHAR BgndSelectBestChannel(RTMP_ADAPTER *pAd)
{
	int i;
	UCHAR BestChannel=0, BestPercen=0xff, Percen=0;
	for (i=0; i< pAd->BgndScanCtrl.GroupChListNum; i++)
	{
			Percen = ((pAd->BgndScanCtrl.GroupChList[i].Max_PCCA_Time)*100)/(((pAd->BgndScanCtrl.ScanDuration)*1000)-(pAd->BgndScanCtrl.GroupChList[i].Band0_Tx_Time));
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ChIdx=%d control-Channle=%d cen-channel=%d\n", i,pAd->BgndScanCtrl.GroupChList[i].BestCtrlChannel, pAd->BgndScanCtrl.GroupChList[i].CenChannel));
			MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("		Max_PCCA=%x, Min_PCCA=%x, Band0_Tx_Time=%x, Percentage=%d \n",
				pAd->BgndScanCtrl.GroupChList[i].Max_PCCA_Time, pAd->BgndScanCtrl.GroupChList[i].Min_PCCA_Time,
				pAd->BgndScanCtrl.GroupChList[i].Band0_Tx_Time, Percen));
			if (Percen <= BestPercen)
			{
				BestPercen=Percen;
				BestChannel = pAd->BgndScanCtrl.GroupChList[i].BestCtrlChannel;
			}	
	}
	return BestChannel;
}
VOID NextBgndScanChannel(RTMP_ADAPTER *pAd, UCHAR channel)
{
	int i;
	//UCHAR next_channel = 0;
	pAd->BgndScanCtrl.ScanChannel = 0;
	for (i = 0; i < (pAd->BgndScanCtrl.ChannelListNum - 1); i++)
	{
		if (channel == pAd->BgndScanCtrl.BgndScanChList[i].Channel)
		{			
				/* Record this channel's idx in ChannelList array.*/
			pAd->BgndScanCtrl.ScanChannel = pAd->BgndScanCtrl.BgndScanChList[i+1].Channel;
			pAd->BgndScanCtrl.ScanCenChannel = pAd->BgndScanCtrl.BgndScanChList[i+1].CenChannel;
			pAd->BgndScanCtrl.ChannelIdx = i+1;
			break;
		}
	}
	
}

VOID FirstBgndScanChannel(RTMP_ADAPTER *pAd)
{
	pAd->BgndScanCtrl.ScanChannel=pAd->BgndScanCtrl.BgndScanChList[0].Channel;
	pAd->BgndScanCtrl.ScanCenChannel = pAd->BgndScanCtrl.BgndScanChList[0].CenChannel;
	pAd->BgndScanCtrl.FirstChannel = pAd->BgndScanCtrl.BgndScanChList[0].Channel;
	pAd->BgndScanCtrl.ChannelIdx = 0;
}

VOID BuildBgndScanChList(RTMP_ADAPTER *pAd)
{
	INT channel_idx, ChListNum=0;
	UCHAR ch;

	if (pAd->BgndScanCtrl.IsABand)
	{ 
		for (channel_idx = 0; channel_idx < pAd->ChannelListNum; channel_idx++)	
		{
			ch = pAd->ChannelList[channel_idx].Channel;
			
			if (pAd->CommonCfg.RegTransmitSetting.field.BW == BW_20) 
			{
				pAd->BgndScanCtrl.BgndScanChList[ChListNum].Channel = pAd->ChannelList[channel_idx].Channel;
				pAd->BgndScanCtrl.BgndScanChList[ChListNum].CenChannel= pAd->ChannelList[channel_idx].Channel;
				pAd->BgndScanCtrl.BgndScanChList[ChListNum].DfsReq = pAd->ChannelList[channel_idx].DfsReq;
				ChListNum++;
			}
#ifdef DOT11_N_SUPPORT				
			else if (((pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40) 
#ifdef DOT11_VHT_AC				
				&&(pAd->CommonCfg.vht_bw == VHT_BW_2040)
#endif /* DOT11_VHT_AC */
				)
				&& N_ChannelGroupCheck(pAd, ch))
			{	
				pAd->BgndScanCtrl.BgndScanChList[ChListNum].Channel = pAd->ChannelList[channel_idx].Channel;
				if (GetABandChOffset(ch) == 1)
					pAd->BgndScanCtrl.BgndScanChList[ChListNum].CenChannel= pAd->ChannelList[channel_idx].Channel + 2;
				else
					pAd->BgndScanCtrl.BgndScanChList[ChListNum].CenChannel= pAd->ChannelList[channel_idx].Channel - 2;
				
				pAd->BgndScanCtrl.BgndScanChList[ChListNum].DfsReq = pAd->ChannelList[channel_idx].DfsReq;
				ChListNum++;
			}
#ifdef DOT11_VHT_AC	
			else if (pAd->CommonCfg.vht_bw == VHT_BW_80) 
			{				
				if (vht80_channel_group(pAd, ch))
				{
					pAd->BgndScanCtrl.BgndScanChList[ChListNum].Channel = pAd->ChannelList[channel_idx].Channel;
					pAd->BgndScanCtrl.BgndScanChList[ChListNum].CenChannel = vht_cent_ch_freq (ch, VHT_BW_80);
					pAd->BgndScanCtrl.BgndScanChList[ChListNum].DfsReq = pAd->ChannelList[channel_idx].DfsReq;
					ChListNum++;
				}	
			}
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */			
		}	
	}
	else
	{	/* 2.4G only support BW20 background scan */
		for (channel_idx = 0; channel_idx < pAd->ChannelListNum; channel_idx++)	
		{
			pAd->BgndScanCtrl.BgndScanChList[ChListNum].Channel = pAd->BgndScanCtrl.BgndScanChList[ChListNum].CenChannel = pAd->ChannelList[channel_idx].Channel;
			ChListNum++;
		}	
	}
	pAd->BgndScanCtrl.ChannelListNum = ChListNum;
	for (channel_idx = 0; channel_idx < pAd->BgndScanCtrl.ChannelListNum; channel_idx++)	
	{
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Support channel: PrimCh=%d, CentCh=%d, DFS=%d\n",
			pAd->BgndScanCtrl.BgndScanChList[channel_idx].Channel, pAd->BgndScanCtrl.BgndScanChList[channel_idx].CenChannel,
			pAd->BgndScanCtrl.BgndScanChList[channel_idx].DfsReq));
	}	
}

UINT8 GroupChListSearch(PRTMP_ADAPTER pAd, UCHAR CenChannel)
{
	UCHAR i;
	PBGND_SCAN_CH_GROUP_LIST	GroupChList=pAd->BgndScanCtrl.GroupChList;
	
	for (i = 0; i < pAd->BgndScanCtrl.GroupChListNum; i++)		
	{
		if (GroupChList->CenChannel == CenChannel)
			return i;

		GroupChList++;
	}

	return 0xff;
}

VOID GroupChListInsert(PRTMP_ADAPTER pAd, PBGND_SCAN_SUPP_CH_LIST pSource)
{
	UCHAR i=pAd->BgndScanCtrl.GroupChListNum;
	PBGND_SCAN_CH_GROUP_LIST	GroupChList=&pAd->BgndScanCtrl.GroupChList[i];
	
	GroupChList->BestCtrlChannel = pSource->Channel;
	GroupChList->CenChannel = pSource->CenChannel;
	GroupChList->Max_PCCA_Time = GroupChList->Min_PCCA_Time =pSource->PccaTime;
	GroupChList->Band0_Tx_Time = pSource->Band0TxTime;
	pAd->BgndScanCtrl.GroupChListNum=i+1;

	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Insert new group channel list Number=%d CenChannel=%d BestCtrlChannel=%d Max_PCCA_TIEM=%x\n", 
	__FUNCTION__, pAd->BgndScanCtrl.GroupChListNum, GroupChList->CenChannel, GroupChList->BestCtrlChannel,	GroupChList->Max_PCCA_Time));
}

VOID GroupChListUpdate(PRTMP_ADAPTER pAd, UCHAR index, PBGND_SCAN_SUPP_CH_LIST pSource)
{
	//UCHAR i;

	PBGND_SCAN_CH_GROUP_LIST	GroupChList=&pAd->BgndScanCtrl.GroupChList[index];

	if (pSource->PccaTime > GroupChList->Max_PCCA_Time )
	{
		GroupChList->Max_PCCA_Time = pSource->PccaTime;
		GroupChList->Band0_Tx_Time = pSource->Band0TxTime;			
	}	

	if (pSource->PccaTime < GroupChList->Min_PCCA_Time )
	{
		GroupChList->Min_PCCA_Time = pSource->PccaTime;
		GroupChList->BestCtrlChannel = pSource->Channel;

	}
		
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Update group channel list index=%d CenChannel=%d BestCtrlChannel=%d PCCA_TIEM=%x\n", 
	__FUNCTION__, pAd->BgndScanCtrl.GroupChListNum, GroupChList->CenChannel, GroupChList->BestCtrlChannel,	GroupChList->Max_PCCA_Time));
}

VOID GenerateGroupChannelList(PRTMP_ADAPTER pAd)
{
	UCHAR i, ListIndex;
	//PBGND_SCAN_CH_GROUP_LIST	GroupChList = pAd->BgndScanCtrl.GroupChList;
	PBGND_SCAN_SUPP_CH_LIST		SuppChList = pAd->BgndScanCtrl.BgndScanChList;
	//PBACKGROUND_SCAN_CTRL		BgndScanCtrl = &pAd->BgndScanCtrl;
	//PBGND_SCAN_SUPP_CH_LIST		SuppChList = BgndScanCtrl->BgndScanChList;
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ChannelListNum=%d \n", 
	__FUNCTION__, pAd->BgndScanCtrl.ChannelListNum));

	for (i=0; i<pAd->BgndScanCtrl.ChannelListNum; i++)
	{

		ListIndex=GroupChListSearch(pAd, SuppChList->CenChannel);
		if (ListIndex == 0xff) /* Not Found */
		{
			
			GroupChListInsert(pAd, SuppChList);
		}
		else
		{
			GroupChListUpdate(pAd, ListIndex, SuppChList);
		}
		SuppChList++;
	}
}


VOID BackgroundScanStateMachineInit(
	IN RTMP_ADAPTER *pAd,
	IN STATE_MACHINE *Sm,
	OUT STATE_MACHINE_FUNC Trans[])
{
	StateMachineInit(Sm, (STATE_MACHINE_FUNC *)Trans, BGND_SCAN_MAX_STATE, BGND_SCAN_MAX_MSG, (STATE_MACHINE_FUNC)Drop, BGND_SCAN_IDLE, BGND_SCAN_MACHINE_BASE);

	StateMachineSetAction(Sm, BGND_SCAN_IDLE, BGND_SCAN_REQ, (STATE_MACHINE_FUNC)BackgroundScanStartAction);
	StateMachineSetAction(Sm, BGND_SCAN_LISTEN, BGND_SCAN_TIMEOUT, (STATE_MACHINE_FUNC)BackgroundScanTimeoutAction);
	StateMachineSetAction(Sm, BGND_SCAN_LISTEN, BGND_SCAN_CNCL, (STATE_MACHINE_FUNC)BackgroundScanCancelAction);

}

VOID BackgroundScanInit (
	IN PRTMP_ADAPTER pAd)
{
//	UCHAR channel_idx = 0;
//	UINT32 Value;
//	RTMP_REG_PAIR Reg[2];
	UCHAR PhyMode=0;
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ===============>\n", __FUNCTION__));
	/*
	ToDo: Based on current settings to decide support background scan or not.
	Don't support case: DBDC, 80+80
	*/
	//Scan BW

	PhyMode = HcGetRadioPhyMode(pAd);	
	pAd->BgndScanCtrl.IsABand = (WMODE_CAP_5G(PhyMode)) ? TRUE : FALSE;
	if (pAd->BgndScanCtrl.IsABand)
	{ 
#ifdef DOT11_VHT_AC	
		if (pAd->CommonCfg.vht_bw == VHT_BW_80) 		
			pAd->BgndScanCtrl.ScanBW = BW_80;
		else
#endif /* DOT11_VHT_AC */			
			pAd->BgndScanCtrl.ScanBW = pAd->CommonCfg.RegTransmitSetting.field.BW;		
	}
	else //2.4G
		pAd->BgndScanCtrl.ScanBW = BW_20;

	/* Decide RxPath&TxStream for background */
	pAd->BgndScanCtrl.RxPath = 0xc;
	pAd->BgndScanCtrl.TxStream = 0x2;
	pAd->BgndScanCtrl.ScanDuration = 500;
	
				if (pAd->CommonCfg.dbdc_mode == TRUE) {
                        pAd->BgndScanCtrl.BgndScanSupport = 0;
                }        
#ifdef DOT11_VHT_AC	
                else if (pAd->CommonCfg.vht_bw == VHT_BW_160 || pAd->CommonCfg.vht_bw == VHT_BW_8080 ) {
                        pAd->BgndScanCtrl.BgndScanSupport = 0;
                 }    
#endif /* DOT11_VHT_AC */	                
                else {
                        pAd->BgndScanCtrl.BgndScanSupport = 1;
                }
				
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IsABand=%d, ScanBW=%d \n", pAd->BgndScanCtrl.IsABand, pAd->BgndScanCtrl.ScanBW));
	if (pAd->BgndScanCtrl.BgndScanSupport)
	{
		BackgroundScanStateMachineInit(pAd, &pAd->BgndScanCtrl.BgndScanStatMachine, pAd->BgndScanCtrl.BgndScanFunc);
		//Copy channel list for background.
		BuildBgndScanChList(pAd);
		RTMPInitTimer(pAd, &pAd->BgndScanCtrl.BgndScanTimer, GET_TIMER_FUNCTION(BackgroundScanTimeout), pAd, FALSE);
		
		//ToDo: Related CR initialization.
		MAC_IO_WRITE32(pAd, MIB_M1SCR, 0x7ef3ffff); /* 0x820fd200 Enable Band1 PSCCA time */
		MAC_IO_WRITE32(pAd, MIB_M1SCR1, 0xffff); /* 0x820fd208  */
		/*
		Reg[0].Register=0x820fd200;
		Reg[1].Register=0x820fd208;
		MtCmdMultipleMacRegAccessRead(pAd, Reg, 2);
		*/
		/* Enabel PSCCA time count & Enable EDCCA time count */
		
		//MtCmdMultipleMacRegAccessWrite
	}
	else
	{
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Background scan doesn't support in current settings....\n"));
	}
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s <===============\n", __FUNCTION__));
}

VOID BackgroundScanDeInit(
	IN PRTMP_ADAPTER pAd)
{
    BOOLEAN Cancelled;
    RTMPReleaseTimer(&pAd->BgndScanCtrl.BgndScanTimer, &Cancelled);	   
}

VOID BackgroundScanStart(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN	BgndscanEnable)
{
//	UINT32	Value;
	//In-band commad to notify FW(RA) background scan will start.
	//Switch channel for Band0 (Include Star Tx/Rx ?)
	//Scan channel for Band1

	/* Reset Group channel list */
	os_zero_mem(pAd->BgndScanCtrl.GroupChList, sizeof(BGND_SCAN_CH_GROUP_LIST) * MAX_NUM_OF_CHANNELS);
	pAd->BgndScanCtrl.GroupChListNum = 0;
	
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Enable=%d ===============>\n", __FUNCTION__, BgndscanEnable));
	//Notify RA with in-band command the background scan will start.
	if (BgndscanEnable) 	
		MlmeEnqueue(pAd, BGND_SCAN_STATE_MACHINE, BGND_SCAN_REQ, 0, NULL, 0); 
	else
		MlmeEnqueue(pAd, BGND_SCAN_STATE_MACHINE, BGND_SCAN_CNCL, 0, NULL, 0);
	
	RTMP_MLME_HANDLER(pAd);

	
}

VOID BackgroundScanStartAction(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ===============>\n", __FUNCTION__));
	
	FirstBgndScanChannel(pAd);
	BackgroundScanNextChannel(pAd);
}

VOID BackgroundScanTimeout( 
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)FunctionContext;
	
	MlmeEnqueue(pAd, BGND_SCAN_STATE_MACHINE, BGND_SCAN_TIMEOUT, 0, NULL, 0);
	RTMP_MLME_HANDLER(pAd);
}


VOID BackgroundScanTimeoutAction(
	RTMP_ADAPTER *pAd,
	MLME_QUEUE_ELEM *Elem)
{
	//UINT32	PCCA_TIME;
	RTMP_REG_PAIR Reg[5];
	//UINT32	pccatime, sccatime, edtime, band0txtime, mdrdy;
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s channel index=%d===============>\n", __FUNCTION__, pAd->BgndScanCtrl.ChannelIdx));
	//Updaet channel info
	//RTMP_IO_READ32(pAd, 0x820fd248, &PCCA_TIME);
	Reg[0].Register = 0x820fd248/*MIB_M1SDR16*/; /* PCCA Time */
	Reg[1].Register = 0x820fd24c/*MIB_M1SDR17*/; /* SCCA Time */
	Reg[2].Register = 0x820fd250/*MIB_M1SDR18*/; /* ED Time */
	Reg[3].Register = 0x820fd094/*MIB_M0SDR35*/; /* Bnad0 TxTime */
	Reg[4].Register = 0x820fd258/*MIB_M1SDR20*/; /* Mdrdy */
	MtCmdMultipleMacRegAccessRead(pAd, Reg, 5);	
	pAd->BgndScanCtrl.BgndScanChList[pAd->BgndScanCtrl.ChannelIdx].PccaTime = Reg[0].Value;
	pAd->BgndScanCtrl.BgndScanChList[pAd->BgndScanCtrl.ChannelIdx].SccaTime = Reg[1].Value;
	pAd->BgndScanCtrl.BgndScanChList[pAd->BgndScanCtrl.ChannelIdx].EDCCATime = Reg[2].Value;
	pAd->BgndScanCtrl.BgndScanChList[pAd->BgndScanCtrl.ChannelIdx].Band0TxTime = Reg[3].Value;
	pAd->BgndScanCtrl.BgndScanChList[pAd->BgndScanCtrl.ChannelIdx].Mdrdy= Reg[4].Value;
	
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ChannelIdx [%d].PCCA_TIME=%x, SCCA_TIEM=%x, EDCCA_TIME=%x, Band0TxTime=%x Mdrdy=%x ===============>\n",pAd->BgndScanCtrl.ChannelIdx, Reg[0].Value, Reg[1].Value, Reg[2].Value, Reg[3].Value, Reg[4].Value));
	NextBgndScanChannel(pAd, pAd->BgndScanCtrl.ScanChannel);	
	BackgroundScanNextChannel (pAd);

}

VOID BackgroundScanCancelAction(
	RTMP_ADAPTER *pAd,
	MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN Cancelled;
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s ===============>\n", __FUNCTION__));
	pAd->BgndScanCtrl.BgndScanStatMachine.CurrState = BGND_SCAN_IDLE;//Scan Stop
	RTMPCancelTimer(&pAd->BgndScanCtrl.BgndScanTimer, &Cancelled);	
}
VOID BackgroundScanNextChannel(
	IN PRTMP_ADAPTER pAd)
{
	MT_BGND_SCAN_CFG BgndScanCfg;
	RTMP_REG_PAIR Reg[5];
	MT_BGND_SCAN_NOTIFY BgScNotify;
	//INT	i;
//	UINT32	pccatime, sccatime, edtime, band0txtime, mdrdy;
	UCHAR	BestChannel=0;
	MT_SWITCH_CHANNEL_CFG *CurrentSwChCfg;

	/* Restore switch channel configuration */
	CurrentSwChCfg = &(pAd->BgndScanCtrl.CurrentSwChCfg[0]);
	MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Scan Channel=%d===============>\n", __FUNCTION__, pAd->BgndScanCtrl.ScanChannel));
	if (pAd->BgndScanCtrl.ScanChannel == 0)
	{		
		pAd->BgndScanCtrl.BgndScanStatMachine.CurrState = BGND_SCAN_IDLE;//Scan Stop
		BgndScanCfg.BandIdx=0;
		BgndScanCfg.Bw = CurrentSwChCfg->Bw;
		BgndScanCfg.ControlChannel= CurrentSwChCfg->ControlChannel;
		BgndScanCfg.CentralChannel = CurrentSwChCfg->CentralChannel;
		BgndScanCfg.Reason = CH_SWITCH_BACKGROUND_SCAN_STOP;
		BgndScanCfg.RxPath =CurrentSwChCfg->RxStream; /* return to 4 Rx */
		BgndScanCfg.TxStream = CurrentSwChCfg->TxStream;
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Stop Scan Bandidx=%d, BW=%d, CtrlCh=%d, CenCh=%d, Reason=%d, RxPath=%d\n", 
			__FUNCTION__, BgndScanCfg.BandIdx, BgndScanCfg.Bw, BgndScanCfg.ControlChannel,
			BgndScanCfg.CentralChannel, BgndScanCfg.Reason, BgndScanCfg.RxPath));
		MtCmdBgndScan(pAd, BgndScanCfg);
		//Notify RA background scan stop
		BgScNotify.NotifyFunc =  (BgndScanCfg.TxStream << 5 | 0xf);
		BgScNotify.BgndScanStatus = 0;//stop
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Background scan Notify NotifyFunc=%x, Status=%d", BgScNotify.NotifyFunc, BgScNotify.BgndScanStatus));
		MtCmdBgndScanNotify(pAd, BgScNotify);

		/* Dump Channel Info */
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Dump Channel Info \n"));
		GenerateGroupChannelList(pAd);
		BestChannel = BgndSelectBestChannel(pAd);
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Best Channel=%d", BestChannel));
		
	}
	else if (pAd->BgndScanCtrl.ScanChannel == pAd->BgndScanCtrl.FirstChannel)
	{
		BgScNotify.NotifyFunc =  (0x2 << 5 | 0xf);
		BgScNotify.BgndScanStatus = 1;//start
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Background scan Notify NotifyFunc=%x, Status=%d", BgScNotify.NotifyFunc, BgScNotify.BgndScanStatus));
		MtCmdBgndScanNotify(pAd, BgScNotify);
		
		//Switch Band1 channel to pAd->BgndScanCtrl.ScanChannel
		pAd->BgndScanCtrl.BgndScanStatMachine.CurrState = BGND_SCAN_LISTEN;
		//Fill band1 BgndScanCfg
		BgndScanCfg.BandIdx=1;
		BgndScanCfg.Bw = pAd->BgndScanCtrl.ScanBW;
		BgndScanCfg.ControlChannel = pAd->BgndScanCtrl.ScanChannel;
		BgndScanCfg.CentralChannel = pAd->BgndScanCtrl.ScanCenChannel;
		BgndScanCfg.Reason = CH_SWITCH_BACKGROUND_SCAN_START;
		BgndScanCfg.RxPath =0x0C; /* Distribute 2 Rx for background scan */
		BgndScanCfg.TxStream = 2;
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Start Scan Bandidx=%d,	BW=%d, CtrlCh=%d, CenCh=%d, Reason=%d, RxPath=%d\n", 
			__FUNCTION__, BgndScanCfg.BandIdx, BgndScanCfg.Bw, BgndScanCfg.ControlChannel,
			BgndScanCfg.CentralChannel, BgndScanCfg.Reason, BgndScanCfg.RxPath));
		MtCmdBgndScan(pAd, BgndScanCfg);

		//Fill band0 BgndScanCfg
		BgndScanCfg.BandIdx=0;
		BgndScanCfg.Bw = CurrentSwChCfg->Bw /*pAd->BgndScanCtrl.ScanBW*/;
		BgndScanCfg.ControlChannel= CurrentSwChCfg->ControlChannel;
		BgndScanCfg.CentralChannel = CurrentSwChCfg->CentralChannel;
		BgndScanCfg.Reason = CH_SWITCH_BACKGROUND_SCAN_START;
		BgndScanCfg.RxPath =0x03; /* Keep 2 Rx for original service */
		BgndScanCfg.TxStream = 2;
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Start Scan Bandidx=%d,	BW=%d, CtrlCh=%d, CenCh=%d, Reason=%d, RxPath=%d\n", 
			__FUNCTION__, BgndScanCfg.BandIdx, BgndScanCfg.Bw, BgndScanCfg.ControlChannel,
			BgndScanCfg.CentralChannel, BgndScanCfg.Reason, BgndScanCfg.RxPath));
		MtCmdBgndScan(pAd, BgndScanCfg);
		/* Read clear below CR */
		Reg[0].Register = 0x820fd248/*MIB_M1SDR16*/; /* PCCA Time */
		Reg[1].Register = 0x820fd24c/*MIB_M1SDR17*/; /* SCCA Time */
		Reg[2].Register = 0x820fd250/*MIB_M1SDR18*/; /* ED Time */
		Reg[3].Register = 0x820fd094/*MIB_M0SDR35*/; /* Bnad0 TxTime */
		Reg[4].Register = 0x820fd258/*MIB_M1SDR20*/; /* Mdrdy */
		MtCmdMultipleMacRegAccessRead(pAd, Reg, 5);	
		RTMPSetTimer(&pAd->BgndScanCtrl.BgndScanTimer, pAd->BgndScanCtrl.ScanDuration); //200ms timer

	}	
	else
	{
		//RTMPSetTimer(&pAd->BgndScanCtrl.BgndScanTimer, 200); //200ms timer
		//Switch Band1 channel to pAd->BgndScanCtrl.ScanChannel
		pAd->BgndScanCtrl.BgndScanStatMachine.CurrState = BGND_SCAN_LISTEN;
		//Fill band1 BgndScanCfg
		BgndScanCfg.BandIdx=1;
		BgndScanCfg.Bw = pAd->BgndScanCtrl.ScanBW;
		BgndScanCfg.ControlChannel = pAd->BgndScanCtrl.ScanChannel;
		BgndScanCfg.CentralChannel = pAd->BgndScanCtrl.ScanCenChannel;
		BgndScanCfg.Reason = CH_SWITCH_BACKGROUND_SCAN_RUNNING;
		BgndScanCfg.RxPath =0x0C; /* Distribute 2 Rx for background scan */
		BgndScanCfg.TxStream = 2;
		MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s Running Scan Bandidx=%d,	BW=%d, CtrlCh=%d, CenCh=%d, Reason=%d, RxPath=%d\n", 
			__FUNCTION__, BgndScanCfg.BandIdx, BgndScanCfg.Bw, BgndScanCfg.ControlChannel,
			BgndScanCfg.CentralChannel, BgndScanCfg.Reason, BgndScanCfg.RxPath));
		MtCmdBgndScan(pAd, BgndScanCfg);
		/* Read clear below CR */
		Reg[0].Register = 0x820fd248/*MIB_M1SDR16*/; /* PCCA Time */
		Reg[1].Register = 0x820fd24c/*MIB_M1SDR17*/; /* SCCA Time */
		Reg[2].Register = 0x820fd250/*MIB_M1SDR18*/; /* ED Time */
		Reg[3].Register = 0x820fd094/*MIB_M0SDR35*/; /* Bnad0 TxTime */
		Reg[4].Register = 0x820fd258/*MIB_M1SDR20*/; /* Mdrdy */
		MtCmdMultipleMacRegAccessRead(pAd, Reg, 5);
		
		RTMPSetTimer(&pAd->BgndScanCtrl.BgndScanTimer, pAd->BgndScanCtrl.ScanDuration); //500ms timer
	}
}


VOID BackgroundScanTest(
	IN PRTMP_ADAPTER pAd,
	IN MT_BGND_SCAN_CFG BgndScanCfg)
{
	/* Send Commad to MCU */
	
	MtCmdBgndScan(pAd, BgndScanCfg);	
}
