/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************

	Module Name:
	ap_vow.c
*/
#ifdef VOW_SUPPORT

#include "rt_config.h"
#include "mcu/mt_cmd.h"
#define UMAC_DRR_TABLE_CTRL0            (0x00008388)

#define UMAC_DRR_TABLE_WDATA0           (0x00008340)
#define UMAC_DRR_TABLE_WDATA1           (0x00008344)
#define UMAC_DRR_TABLE_WDATA2           (0x00008348)
#define UMAC_DRR_TABLE_WDATA3           (0x0000834C)

/* Charge mode control control operation (0x8x : charge tx time & length) */

#define UMAC_CHARGE_BW_TOKEN_BIT_MASK                       BIT(0)
#define UMAC_CHARGE_BW_DRR_BIT_MASK                         BIT(1)
#define UMAC_CHARGE_AIRTIME_DRR_BIT_MASK                    BIT(2)
#define UMAC_CHARGE_ADD_MODE_BIT_MASK                       BIT(3)

#define UMAC_CHARGE_OP_BASE                                 0x80
#define UMAC_CHARGE_BW_TOKEN_OP_MASK                        (UMAC_CHARGE_OP_BASE | UMAC_CHARGE_BW_TOKEN_BIT_MASK)
#define UMAC_CHARGE_BW_DRR_OP_MASK                          (UMAC_CHARGE_OP_BASE | UMAC_CHARGE_BW_DRR_BIT_MASK)
#define UMAC_CHARGE_AIRTIME_DRR_OP_MASK                     (UMAC_CHARGE_OP_BASE | UMAC_CHARGE_AIRTIME_DRR_BIT_MASK)

#define UMAC_CHARGE_MODE_STA_ID_MASK                        BITS(0,7)
#define UMAC_CHARGE_MODE_STA_ID_OFFSET                      0
#define UMAC_CHARGE_MODE_QUEUE_ID_MASK                      BITS(8,11)
#define UMAC_CHARGE_MODE_QUEUE_ID_OFFSET                    8

#define UMAC_CHARGE_MODE_BSS_GROUP_MASK                     BITS(0,3)
#define UMAC_CHARGE_MODE_BSS_GROUP_OFFSET                   0


#define UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_MASK           BITS(0,15)
#define UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_OFFSET         0

#define UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_MASK             BITS(16,31)
#define UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_OFFSET           16


/* Change MODE Ctrl operation */
#define UMAC_DRR_TABLE_CTRL0_CHANGE_MODE_MASK               BIT(23)
#define UMAC_DRR_TABLE_CTRL0_CHANGE_MODE_OFFSET             23


/* 00000340 DRR_Table_WData DRR table Wdata register register   00000000 */

#define UMAC_DRR_TABLE_WDATA0_STA_MODE_MASK                  BITS(0, 15)
#define UMAC_DRR_TABLE_WDATA0_STA_MODE_OFFSET                0

/* 00000350 DRR_Table_Rdata DRR table control register read data    00000000 */
#define UMAC_DRR_TABLE_RDATA0_STA_MODE_MASK                  BITS(0, 15)
#define UMAC_DRR_TABLE_RDATA0_STA_MODE_OFFSET                0

/* 00000388 DRR_Table_ctrl0     DRR table control register register 0   00000000 */

#define UMAC_DRR_TABLE_CTRL0_EXEC_MASK                      BIT(31)
#define UMAC_DRR_TABLE_CTRL0_EXEC_OFFSET                    31
#define UMAC_DRR_TABLE_CTRL0_MODE_OP_OFFSET                 16


#define UMAC_BSS_GROUP_NUMBER               16
#define UMAC_WLAN_ID_MAX_VALUE          127

#ifndef _LINUX_BITOPS_H
#define BIT(n)                          ((UINT32) 1 << (n))
#endif /* BIT */
#define BITS(m,n)                       (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))




#define VOW_DEF_AVA_AIRTIME (1000000)  //us


/* global variables */
PRTMP_ADAPTER pvow_pad = NULL;
UINT32 vow_tx_time[MAX_LEN_OF_MAC_TABLE];
UINT32 vow_rx_time[MAX_LEN_OF_MAC_TABLE];
UINT32 vow_tx_ok[MAX_LEN_OF_MAC_TABLE];
UINT32 vow_tx_fail[MAX_LEN_OF_MAC_TABLE];
UINT16 vow_idx = 0;
UINT32 vow_tx_bss_byte[WMM_NUM_OF_AC];
UINT32 vow_rx_bss_byte[WMM_NUM_OF_AC];
UINT32 vow_tx_mbss_byte[VOW_MAX_GROUP_NUM];
UINT32 vow_rx_mbss_byte[VOW_MAX_GROUP_NUM];
UINT32 vow_ampdu_cnt = 0;
UINT32 vow_interval = 0;
UINT32 vow_last_free_cnt = 0;

/* VOW internal commands */
/***********************************************************/
/*      EXT_CMD_ID_DRR_CTRL = 0x36                         */
/***********************************************************/
/* for station DWRR configration */
INT32 vow_set_sta(PRTMP_ADAPTER pad, UINT8 sta_id, UINT32 subcmd)
{
    EXT_CMD_VOW_DRR_CTRL_T sta_ctrl;
    UINT16 Setting = 0;
    INT32 ret;

    NdisZeroMemory(&sta_ctrl, sizeof(sta_ctrl));

    sta_ctrl.u4CtrlFieldID = subcmd;
    sta_ctrl.ucStaID = sta_id;

    switch(subcmd)
    {
        case ENUM_VOW_DRR_CTRL_FIELD_STA_ALL:
            /* station configration */
            Setting |= pad->vow_sta_cfg[sta_id].group;
            Setting |= (pad->vow_sta_cfg[sta_id].ac_change_rule << 4);
            Setting |= (pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_BK] << 6);
            Setting |= (pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_BE] << 8);
            Setting |= (pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_VI] << 10);
            Setting |= (pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_VO] << 12);
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = Setting;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, Setting));
	        break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_BSS_GROUP:
            
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].group;
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_PRIORITY:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].ac_change_rule;
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_AC0_QUA_ID:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_BK];
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_AC1_QUA_ID:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_BE];
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_AC2_QUA_ID:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_VI];
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_STA_AC3_QUA_ID:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].dwrr_quantum[WMM_AC_VO];
            break;
            break;
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L0:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L1:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L2:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L3:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L4:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L5:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L6:
        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L7:
            sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_cfg.vow_sta_dwrr_quantum[subcmd - ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L0];

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", 
                __FUNCTION__, subcmd, pad->vow_cfg.vow_sta_dwrr_quantum[subcmd - ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L0]));
            break;

        case ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_ALL:
            {
                UINT32 i;

                /* station quantum configruation */
                for (i = 0; i < VOW_MAX_STA_DWRR_NUM; i++)
                {
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(vow_sta_dwrr_quantum[%d] = 0x%x)\n", __FUNCTION__, i, pad->vow_cfg.vow_sta_dwrr_quantum[i]));
                    sta_ctrl.rAirTimeCtrlValue.rAirTimeQuantumAllField.ucAirTimeQuantum[i] = pad->vow_cfg.vow_sta_dwrr_quantum[i];
                }

	            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, Setting));
            }
            break;
    
        case ENUM_VOW_DRR_CTRL_FIELD_STA_PAUSE_SETTING:
            {
                sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_sta_cfg[sta_id].paused;

	            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, pad->vow_sta_cfg[sta_id].paused));
            }
            break;
        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
            break;
    }

    ret = MtCmdSetVoWDRRCtrl(pad, &sta_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_VOW_DRR_CTRL_T)));

    return ret;
}

/* for DWRR max wait time configuration */
INT vow_set_sta_DWRR_max_time(PRTMP_ADAPTER pad)
{
    EXT_CMD_VOW_DRR_CTRL_T sta_ctrl;
    INT32 ret;

	NdisZeroMemory(&sta_ctrl, sizeof(sta_ctrl));

    sta_ctrl.u4CtrlFieldID = ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_DEFICIT_BOUND;
            
    sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_cfg.sta_max_wait_time;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(sta_max_wait_time = 0x%x)\n", __FUNCTION__, pad->vow_cfg.sta_max_wait_time));

    ret = MtCmdSetVoWDRRCtrl(pad, &sta_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_VOW_DRR_CTRL_T)));

    return ret;
}
/***********************************************************/
/*      EXT_CMD_ID_BSSGROUP_CTRL = 0x37                    */
/***********************************************************/
VOID vow_fill_group_all(PRTMP_ADAPTER pad, UINT8 group_id, EXT_CMD_BSS_CTRL_T *group_ctrl)
{
    /* DW0 */
    group_ctrl->arAllBssGroupMultiField[group_id].u2MinRateToken = pad->vow_bss_cfg[group_id].min_rate_token;
    group_ctrl->arAllBssGroupMultiField[group_id].u2MaxRateToken = pad->vow_bss_cfg[group_id].max_rate_token;
    /* DW1 */
    group_ctrl->arAllBssGroupMultiField[group_id].u4MinTokenBucketTimeSize = pad->vow_bss_cfg[group_id].min_airtimebucket_size;
    group_ctrl->arAllBssGroupMultiField[group_id].u4MinAirTimeToken = pad->vow_bss_cfg[group_id].min_airtime_token;
    group_ctrl->arAllBssGroupMultiField[group_id].u4MinTokenBucketLengSize = pad->vow_bss_cfg[group_id].min_ratebucket_size;
    /* DW2 */
    group_ctrl->arAllBssGroupMultiField[group_id].u4MaxTokenBucketTimeSize = pad->vow_bss_cfg[group_id].max_airtimebucket_size;
    group_ctrl->arAllBssGroupMultiField[group_id].u4MaxAirTimeToken = pad->vow_bss_cfg[group_id].max_airtime_token;
    group_ctrl->arAllBssGroupMultiField[group_id].u4MaxTokenBucketLengSize = pad->vow_bss_cfg[group_id].max_ratebucket_size;
    /* DW3 */
    group_ctrl->arAllBssGroupMultiField[group_id].u4MaxWaitTime = pad->vow_bss_cfg[group_id].max_wait_time;
    group_ctrl->arAllBssGroupMultiField[group_id].u4MaxBacklogSize = pad->vow_bss_cfg[group_id].max_backlog_size;
    

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(Group id = 0x%x, min_rate %d, max_rate %d, min_ratio %d, max_ratio %d)\n", 
                                                        __FUNCTION__, group_id,
                                                        pad->vow_bss_cfg[group_id].min_rate,
                                                        pad->vow_bss_cfg[group_id].max_rate,
                                                        pad->vow_bss_cfg[group_id].min_airtime_ratio,
                                                        pad->vow_bss_cfg[group_id].max_airtime_ratio));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(min rate token = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].min_rate_token));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max rate token = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_rate_token));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(min airtime token = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].min_airtime_token));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max airtime token = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_airtime_token));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(min rate bucket = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].min_ratebucket_size));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max rate bucket = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_ratebucket_size));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(min airtime bucket = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].min_airtimebucket_size));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max airtime bucket = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_airtimebucket_size));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max baclog size = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_backlog_size));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(max wait time = 0x%x)\n", __FUNCTION__, pad->vow_bss_cfg[group_id].max_wait_time));

}
/* for group configuration */
INT vow_set_group(PRTMP_ADAPTER pad, UINT8 group_id, UINT32 subcmd)
{
    EXT_CMD_BSS_CTRL_T group_ctrl;
    INT32 ret;

	NdisZeroMemory(&group_ctrl, sizeof(group_ctrl));

    group_ctrl.u4CtrlFieldID = subcmd;
    group_ctrl.ucBssGroupID = group_id;

    switch(subcmd)
    {
        /* group configuration */ 
        case ENUM_BSSGROUP_CTRL_ALL_ITEM_FOR_1_GROUP:

            vow_fill_group_all(pad, group_id, &group_ctrl);
            break;

        case ENUM_BSSGROUP_CTRL_MIN_RATE_TOKEN_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].min_rate_token;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_RATE_TOKEN_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_rate_token;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MIN_TOKEN_BUCKET_TIME_SIZE_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].min_airtimebucket_size;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MIN_AIRTIME_TOKEN_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].min_airtime_token;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MIN_TOKEN_BUCKET_LENG_SIZE_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].min_ratebucket_size;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_TOKEN_BUCKET_TIME_SIZE_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_airtimebucket_size;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_AIRTIME_TOKEN_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_airtime_token;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_TOKEN_BUCKET_LENG_SIZE_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_ratebucket_size;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_WAIT_TIME_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_wait_time;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_MAX_BACKLOG_SIZE_CFG_ITEM:
            group_ctrl.u4SingleFieldIDValue = pad->vow_bss_cfg[group_id].max_backlog_size;

	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(SubCmd %x, Value = 0x%x)\n", __FUNCTION__, subcmd, group_ctrl.u4SingleFieldIDValue));
            break;

        case ENUM_BSSGROUP_CTRL_ALL_ITEM_FOR_ALL_GROUP:
            {
                UINT32 i;

                for (i = 0; i < VOW_MAX_GROUP_NUM; i++)
                    vow_fill_group_all(pad, i, &group_ctrl);
            }
            break;

        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_00:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_01:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_02:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_03:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_04:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_05:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_06:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_07:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_08:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_09:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0A:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0B:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0C:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0D:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0E:
        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_0F:
            /* Group DWRR quantum */ 
            group_ctrl.ucBssGroupQuantumTime[group_id] = pad->vow_bss_cfg[group_id].dwrr_quantum;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(group %d DWRR quantum = 0x%x)\n", __FUNCTION__, group_id, pad->vow_bss_cfg[group_id].dwrr_quantum));
            break;

        case ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_ALL:
            {
                UINT32 i;

                for (i = 0; i < VOW_MAX_GROUP_NUM; i++)
                {
                    group_ctrl.ucBssGroupQuantumTime[i] = pad->vow_bss_cfg[i].dwrr_quantum;
                    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(group %d DWRR quantum = 0x%x)\n", __FUNCTION__, i, pad->vow_bss_cfg[i].dwrr_quantum));
                }
            }
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
            break;
    }


    ret = MtCmdSetVoWGroupCtrl(pad, &group_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_BSS_CTRL_T)));

    return ret;
}

/* for DWRR max wait time configuration */
INT vow_set_group_DWRR_max_time(PRTMP_ADAPTER pad)
{

    EXT_CMD_VOW_DRR_CTRL_T sta_ctrl;
    INT32 ret;

	NdisZeroMemory(&sta_ctrl, sizeof(sta_ctrl));

    sta_ctrl.u4CtrlFieldID = ENUM_VOW_DRR_CTRL_FIELD_BW_DEFICIT_BOUND;
            
    sta_ctrl.rAirTimeCtrlValue.u4ComValue = pad->vow_cfg.group_max_wait_time;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(group_max_wait_time = 0x%x)\n", __FUNCTION__, pad->vow_cfg.group_max_wait_time));

    ret = MtCmdSetVoWDRRCtrl(pad, &sta_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_VOW_DRR_CTRL_T)));
    return ret;
}

/***********************************************************/
/*      EXT_CMD_ID_VOW_FEATURE_CTRL = 0x38                 */
/***********************************************************/
/* for group configuration */
INT vow_set_feature_all(PRTMP_ADAPTER pad)
{
    EXT_CMD_VOW_FEATURE_CTRL_T feature_ctrl;
    INT32 ret, i;

	NdisZeroMemory(&feature_ctrl, sizeof(feature_ctrl));

    /* DW0 - flags */
    feature_ctrl.u2IfApplyBss_0_to_16_CtrlFlag = 0xFFFF; /* 16'b */
    feature_ctrl.u2IfApplyRefillPerildFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyDbdc1SearchRuleFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyDbdc0SearchRuleFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyEnTxopNoChangeBssFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyAirTimeFairnessFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyEnbwrefillFlag = TRUE; /* 1'b */
    feature_ctrl.u2IfApplyEnbwCtrlFlag = TRUE; /* 1'b */

    /* DW1 - flags */
    feature_ctrl.u2IfApplyBssCheckTimeToken_0_to_16_CtrlFlag = 0xFFFF;

    /* DW2 - flags */
    feature_ctrl.u2IfApplyBssCheckLengthToken_0_to_16_CtrlFlag = 0xFFFF;
    
    /* DW3 - ctrl values */
    feature_ctrl.u2Bss_0_to_16_CtrlValue = pad->vow_cfg.per_bss_enable; /* 16'b */
    feature_ctrl.u2RefillPerildValue = pad->vow_cfg.refill_period; /* 8'b */
    feature_ctrl.u2Dbdc1SearchRuleValue = pad->vow_cfg.dbdc1_search_rule; /* 1'b */
    feature_ctrl.u2Dbdc0SearchRuleValue = pad->vow_cfg.dbdc0_search_rule; /* 1'b */
    feature_ctrl.u2EnTxopNoChangeBssValue = pad->vow_cfg.en_txop_no_change_bss; /* 1'b */
    feature_ctrl.u2AirTimeFairnessValue = pad->vow_cfg.en_airtime_fairness; /* 1'b */
    feature_ctrl.u2EnbwrefillValue = pad->vow_cfg.en_bw_refill; /* 1'b */
    feature_ctrl.u2EnbwCtrlValue = pad->vow_cfg.en_bw_ctrl; /* 1'b */

    /* DW4 - ctrl values */
    for (i = 0; i < VOW_MAX_GROUP_NUM; i++)
        feature_ctrl.u2BssCheckTimeToken_0_to_16_CtrlValue |= (pad->vow_bss_cfg[i].at_on << i);
    
    /* DW5 - ctrl values */
    for (i = 0; i < VOW_MAX_GROUP_NUM; i++)
        feature_ctrl.u2BssCheckLengthToken_0_to_16_CtrlValue |= (pad->vow_bss_cfg[i].bw_on << i);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2Bss_0_to_16_CtrlValue  = 0x%x)\n", __FUNCTION__, pad->vow_cfg.per_bss_enable));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2RefillPerildValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.refill_period));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2Dbdc1SearchRuleValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.dbdc1_search_rule));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2Dbdc0SearchRuleValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.dbdc0_search_rule));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2EnTxopNoChangeBssValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.en_txop_no_change_bss));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2AirTimeFairnessValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.en_airtime_fairness));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2EnbwrefillValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.en_bw_refill));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2EnbwCtrlValue = 0x%x)\n", __FUNCTION__, pad->vow_cfg.en_bw_ctrl));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2BssCheckTimeToken_0_to_16_CtrlValue = 0x%x)\n", __FUNCTION__, feature_ctrl.u2BssCheckTimeToken_0_to_16_CtrlValue));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(u2BssCheckLengthToken_0_to_16_CtrlValue = 0x%x)\n", __FUNCTION__, feature_ctrl.u2BssCheckLengthToken_0_to_16_CtrlValue));

    ret = MtCmdSetVoWFeatureCtrl(pad, &feature_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_VOW_FEATURE_CTRL_T)));

    return ret;
}

/***********************************************************/
/*      EXT_CMD_ID_RX_AIRTIME_CTRL = 0x4a                  */
/***********************************************************/
/* for RX airtime configuration */
INT vow_set_rx_airtime(PRTMP_ADAPTER pad, UINT8 cmd, UINT32 subcmd)
{
    EXT_CMD_RX_AT_CTRL_T                rx_at_ctrl;
    INT32 ret;

    /* init structure to zero */
	NdisZeroMemory(&rx_at_ctrl, sizeof(rx_at_ctrl));

    /* assign cmd and subcmd */
    rx_at_ctrl.u4CtrlFieldID = cmd;
    rx_at_ctrl.u4CtrlSubFieldID = subcmd;
            
    switch(cmd)
    {
        /* RX airtime feature control */
        case ENUM_RX_AT_FEATURE_CTRL:

            switch(subcmd)
            {
                case ENUM_RX_AT_FEATURE_SUB_TYPE_AIRTIME_EN:
                    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtFeatureSubCtrl.fgRxAirTimeEn = pad->vow_rx_time_cfg.rx_time_en;
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, value = 0x%x)\n", 
                                                __FUNCTION__, cmd, subcmd, pad->vow_rx_time_cfg.rx_time_en));
                    break;
                case ENUM_RX_AT_FEATURE_SUB_TYPE_MIBTIME_EN:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(Not implemented yet = 0x%x)\n", __FUNCTION__, subcmd));
                    break;
                default:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such sub command = 0x%x)\n", __FUNCTION__, subcmd));
            }
            break;

        case ENUM_RX_AT_BITWISE_CTRL:
    
            switch(subcmd)
            {
                case ENUM_RX_AT_BITWISE_SUB_TYPE_AIRTIME_CLR: /* clear all RX airtime counters */
                    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.fgRxAirTimeClrEn = TRUE;

	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, value = 0x%x)\n", 
                                                __FUNCTION__, cmd, subcmd, rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.fgRxAirTimeClrEn));
                    break;
                case ENUM_RX_AT_BITWISE_SUB_TYPE_MIBTIME_CLR:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(Not implemented yet = 0x%x)\n", __FUNCTION__, subcmd));
                    break;
                default:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such sub command = 0x%x)\n", __FUNCTION__, subcmd));
            }
            break;

        case ENUM_RX_AT_TIMER_VALUE_CTRL:
      
            switch(subcmd)
            {
                case ENUM_RX_AT_TIME_VALUE_SUB_TYPE_ED_OFFSET_CTRL:
                    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.ucEdOffsetValue = pad->vow_rx_time_cfg.ed_offset;

	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd =  0x%x, value = 0x%x)\n", 
                                                __FUNCTION__, cmd, subcmd, pad->vow_rx_time_cfg.ed_offset));
                    break;
                default:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such sub command = 0x%x)\n", __FUNCTION__, subcmd));
            }
            break;

        case EMUM_RX_AT_REPORT_CTRL:
            
            switch(subcmd)
            {

                default:
	                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such sub command = 0x%x)\n", __FUNCTION__, subcmd));
            }
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
            break;
    }

    ret = MtCmdSetVoWRxAirtimeCtrl(pad, &rx_at_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_RX_AT_CTRL_T)));

    return ret;
}

/* select RX WMM backoff time for 4 OM */
INT vow_set_wmm_selection(PRTMP_ADAPTER pad, UINT8 om)
{
    EXT_CMD_RX_AT_CTRL_T                rx_at_ctrl;
    INT32 ret;

    /* init structure to zero */
	NdisZeroMemory(&rx_at_ctrl, sizeof(rx_at_ctrl));
            
    /* assign cmd and subcmd */
    rx_at_ctrl.u4CtrlFieldID = ENUM_RX_AT_BITWISE_CTRL;
    rx_at_ctrl.u4CtrlSubFieldID = ENUM_RX_AT_BITWISE_SUB_TYPE_STA_WMM_CTRL;

    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.ucOwnMacID= om;
    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.fgtoApplyWm00to03MibCfg= pad->vow_rx_time_cfg.wmm_backoff_sel[om];

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, OM = 0x%x, Map = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.ucOwnMacID, ENUM_RX_AT_BITWISE_SUB_TYPE_STA_WMM_CTRL, om, 
                pad->vow_rx_time_cfg.wmm_backoff_sel[om]));
    
    ret = MtCmdSetVoWRxAirtimeCtrl(pad, &rx_at_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_RX_AT_CTRL_T)));

    return ret;
}

/* set 16 MBSS  mapping to 4 RX backoff time configurations */
INT vow_set_mbss2wmm_map(PRTMP_ADAPTER pad, UINT8 bss_idx)
{
    EXT_CMD_RX_AT_CTRL_T                rx_at_ctrl;
    INT32 ret;

    /* init structure to zero */
	NdisZeroMemory(&rx_at_ctrl, sizeof(rx_at_ctrl));

    /* assign cmd and subcmd */
    rx_at_ctrl.u4CtrlFieldID = ENUM_RX_AT_BITWISE_CTRL;
    rx_at_ctrl.u4CtrlSubFieldID = ENUM_RX_AT_BITWISE_SUB_TYPE_MBSS_WMM_CTRL;

    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.ucMbssGroup= bss_idx;
    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtBitWiseSubCtrl.ucWmmGroup= pad->vow_rx_time_cfg.bssid2wmm_set[bss_idx];

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, bss_idx = 0x%x, Map = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, bss_idx, 
                pad->vow_rx_time_cfg.bssid2wmm_set[bss_idx]));

    ret = MtCmdSetVoWRxAirtimeCtrl(pad, &rx_at_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_RX_AT_CTRL_T)));

    return ret;
}

/* set backoff time for RX*/
INT vow_set_backoff_time(PRTMP_ADAPTER pad, UINT8 target)
{
    EXT_CMD_RX_AT_CTRL_T                rx_at_ctrl;
    INT32 ret;

    /* init structure to zero */
	NdisZeroMemory(&rx_at_ctrl, sizeof(rx_at_ctrl));

    /* assign cmd and subcmd */
    rx_at_ctrl.u4CtrlFieldID = ENUM_RX_AT_TIMER_VALUE_CTRL;
    rx_at_ctrl.u4CtrlSubFieldID = ENUM_RX_AT_TIME_VALUE_SUB_TYPE_BACKOFF_TIMER;
    rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackoffWmmGroupIdx= target;

    switch(target)
    {
        case ENUM_RX_AT_WMM_GROUP_0:
        case ENUM_RX_AT_WMM_GROUP_1:
        case ENUM_RX_AT_WMM_GROUP_2:
        case ENUM_RX_AT_WMM_GROUP_3:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC0Backoff = 
                                            pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_BK];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC1Backoff = 
                                            pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_BE];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC2Backoff = 
                                            pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_VI];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC3Backoff = 
                                            pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_VO];
            
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxAtBackoffAcQMask = 
                            (ENUM_RX_AT_AC_Q0_MASK_T | ENUM_RX_AT_AC_Q1_MASK_T | ENUM_RX_AT_AC_Q2_MASK_T | ENUM_RX_AT_AC_Q3_MASK_T);
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group = 0x%x, BK = 0x%x, BE = 0x%x, VI = 0x%x, VO = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target,
                pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_BK],
                pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_BE],
                pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_VI],
                pad->vow_rx_time_cfg.wmm_backoff[target][WMM_AC_VO]));
            break;

        case ENUM_RX_AT_WMM_GROUP_PEPEATER:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC0Backoff = 
                                            pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_BK];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC1Backoff = 
                                            pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_BE];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC2Backoff = 
                                            pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_VI];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC3Backoff = 
                                            pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_VO];
            
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxAtBackoffAcQMask = 
                            (ENUM_RX_AT_AC_Q0_MASK_T | ENUM_RX_AT_AC_Q1_MASK_T | ENUM_RX_AT_AC_Q2_MASK_T | ENUM_RX_AT_AC_Q3_MASK_T);
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group = 0x%x, BK = 0x%x, BE = 0x%x, VI = 0x%x, VO = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target,
                pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_BK],
                pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_BE],
                pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_VI],
                pad->vow_rx_time_cfg.repeater_wmm_backoff[WMM_AC_VO]));
            break;

        case ENUM_RX_AT_WMM_GROUP_STA:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC0Backoff = 
                                            pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_BK];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC1Backoff = 
                                            pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_BE];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC2Backoff = 
                                            pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_VI];
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC3Backoff = 
                                            pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_VO];
            
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxAtBackoffAcQMask = 
                            (ENUM_RX_AT_AC_Q0_MASK_T | ENUM_RX_AT_AC_Q1_MASK_T | ENUM_RX_AT_AC_Q2_MASK_T | ENUM_RX_AT_AC_Q3_MASK_T);
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group = 0x%x, BK = 0x%x, BE = 0x%x, VI = 0x%x, VO = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target,
                pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_BK],
                pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_BE],
                pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_VI],
                pad->vow_rx_time_cfg.om_wmm_backoff[WMM_AC_VO]));
            break;

        case ENUM_RX_AT_NON_QOS:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC0Backoff = 
                                                                pad->vow_rx_time_cfg.non_qos_backoff;
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group = 0x%x, backoff time = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target,
                pad->vow_rx_time_cfg.non_qos_backoff));
            break;
        case ENUM_RX_AT_OBSS:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtTimeValueSubCtrl.rRxATBackOffCfg.u2AC0Backoff = 
                                                                pad->vow_rx_time_cfg.obss_backoff;
            
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group = 0x%x, backoff time = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target,
                pad->vow_rx_time_cfg.obss_backoff));
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, target ));
            break;
    }


    ret = MtCmdSetVoWRxAirtimeCtrl(pad, &rx_at_ctrl);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_RX_AT_CTRL_T)));

    return ret;
}

/* set backoff time for RX*/
INT vow_get_rx_time_counter(PRTMP_ADAPTER pad, UINT8 target, UINT8 band_idx)
{
    EXT_CMD_RX_AT_CTRL_T                rx_at_ctrl;
    INT32 ret;

    /* init structure to zero */
	NdisZeroMemory(&rx_at_ctrl, sizeof(rx_at_ctrl));

    /* assign cmd and subcmd */
    rx_at_ctrl.u4CtrlFieldID = EMUM_RX_AT_REPORT_CTRL;
    rx_at_ctrl.u4CtrlSubFieldID = target;

    switch(target)
    {
        case ENUM_RX_AT_REPORT_SUB_TYPE_RX_NONWIFI_TIME:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtReportSubCtrl.ucRxNonWiFiBandIdx = band_idx;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, target = 0x%x, band_idx = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target, band_idx));
            break;
        case ENUM_RX_AT_REPORT_SUB_TYPE_RX_OBSS_TIME:
            rx_at_ctrl.rRxAtGeneralCtrl.rRxAtReportSubCtrl.ucRxObssBandIdx = band_idx;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, target = 0x%x, band_idx = 0x%x)\n", 
                __FUNCTION__, rx_at_ctrl.u4CtrlFieldID, rx_at_ctrl.u4CtrlSubFieldID, target, band_idx));
            break;
        case ENUM_RX_AT_REPORT_SUB_TYPE_MIB_OBSS_TIME:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(Not implemented yet = 0x%x)\n", __FUNCTION__, target));
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, target));
            
    }

    ret = MtCmdGetVoWRxAirtimeCtrl(pad, &rx_at_ctrl);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_RX_AT_CTRL_T)));

    if (target == ENUM_RX_AT_REPORT_SUB_TYPE_RX_NONWIFI_TIME)
        return rx_at_ctrl.rRxAtGeneralCtrl.rRxAtReportSubCtrl.u4RxNonWiFiBandTimer;
    else if (target == ENUM_RX_AT_REPORT_SUB_TYPE_RX_OBSS_TIME)
        return rx_at_ctrl.rRxAtGeneralCtrl.rRxAtReportSubCtrl.u4RxObssBandTimer;
    else if (target == ENUM_RX_AT_REPORT_SUB_TYPE_MIB_OBSS_TIME)
        return rx_at_ctrl.rRxAtGeneralCtrl.rRxAtReportSubCtrl.u4RxMibObssBandTimer;
    else
        return -1;
}
/***********************************************************/
/*      EXT_CMD_ID_AT_PROC_MODULE = 0x4b                   */
/***********************************************************/

/* for airtime estimator module */
INT vow_set_at_estimator(PRTMP_ADAPTER pad, UINT32 subcmd)
{
    EXT_CMD_AT_PROC_MODULE_CTRL_T   at_proc;
    INT32   ret;

    /* init structure to zero */
	NdisZeroMemory(&at_proc, sizeof(at_proc));

    /* assign cmd and subcmd */
    at_proc.u4CtrlFieldID = ENUM_AT_RPOCESS_ESTIMATE_MODULE_CTRL;
    at_proc.u4CtrlSubFieldID = subcmd;

    switch(subcmd)
    {
        case ENUM_AT_PROC_EST_FEATURE_CTRL:
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.fgAtEstimateOnOff = pad->vow_at_est.at_estimator_en;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_at_est.at_estimator_en));
            break;

        case ENUM_AT_PROC_EST_MONITOR_PERIOD_CTRL:
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.u2AtEstMonitorPeriod = pad->vow_at_est.at_monitor_period;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_at_est.at_monitor_period));
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
    }
        
    ret = MtCmdSetVoWModuleCtrl(pad, &at_proc);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_AT_PROC_MODULE_CTRL_T)));
    
    return ret;
}

INT vow_set_at_estimator_group(PRTMP_ADAPTER pad, UINT32 subcmd, UINT8 group_id)
{
    EXT_CMD_AT_PROC_MODULE_CTRL_T   at_proc;
    INT32   ret;

    /* init structure to zero */
	NdisZeroMemory(&at_proc, sizeof(at_proc));

    /* assign cmd and subcmd */
    at_proc.u4CtrlFieldID = ENUM_AT_RPOCESS_ESTIMATE_MODULE_CTRL;
    at_proc.u4CtrlSubFieldID = subcmd;

    switch(subcmd)
    {
        case ENUM_AT_PROC_EST_GROUP_RATIO_CTRL:
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.u4GroupRatioBitMask |= (1UL << group_id);
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.u2GroupMaxRatioValue[group_id] = pad->vow_bss_cfg[group_id].max_airtime_ratio;
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.u2GroupMinRatioValue[group_id] = pad->vow_bss_cfg[group_id].min_airtime_ratio;
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group %d, val = 0x%x/0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, group_id,
                pad->vow_bss_cfg[group_id].max_airtime_ratio,
                pad->vow_bss_cfg[group_id].min_airtime_ratio));
            break;

        case ENUM_AT_PROC_EST_GROUP_TO_BAND_MAPPING:
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.ucGrouptoSelectBand = group_id;
            at_proc.rAtProcGeneralCtrl.rAtEstimateSubCtrl.ucBandSelectedfromGroup = pad->vow_bss_cfg[group_id].band_idx;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, group %d, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, group_id, pad->vow_bss_cfg[group_id].band_idx));
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
    }
        
    ret = MtCmdSetVoWModuleCtrl(pad, &at_proc);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_AT_PROC_MODULE_CTRL_T)));

    return ret;
}

/* for bad node detector */
INT vow_set_bad_node(PRTMP_ADAPTER pad, UINT32 subcmd)
{
    EXT_CMD_AT_PROC_MODULE_CTRL_T   at_proc;
    INT32   ret;

    /* init structure to zero */
	NdisZeroMemory(&at_proc, sizeof(at_proc));

    /* assign cmd and subcmd */
    at_proc.u4CtrlFieldID = ENUM_AT_RPOCESS_BAD_NODE_MODULE_CTRL;
    at_proc.u4CtrlSubFieldID = subcmd;

    switch(subcmd)
    {
        case ENUM_AT_PROC_BAD_NODE_FEATURE_CTRL:
            at_proc.rAtProcGeneralCtrl.rAtBadNodeSubCtrl.fgAtBadNodeOnOff = pad->vow_badnode.bn_en;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_badnode.bn_en));
            break;

        case ENUM_AT_PROC_BAD_NODE_MONITOR_PERIOD_CTRL:
            at_proc.rAtProcGeneralCtrl.rAtBadNodeSubCtrl.u2AtBadNodeMonitorPeriod = pad->vow_badnode.bn_monitor_period;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_badnode.bn_monitor_period));
            break;

        case ENUM_AT_PROC_BAD_NODE_FALLBACK_THRESHOLD:
            at_proc.rAtProcGeneralCtrl.rAtBadNodeSubCtrl.ucFallbackThreshold = pad->vow_badnode.bn_fallback_threshold;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_badnode.bn_fallback_threshold));
            break;

        case ENUM_AT_PROC_BAD_NODE_PER_THRESHOLD:
            at_proc.rAtProcGeneralCtrl.rAtBadNodeSubCtrl.ucTxPERThreshold = pad->vow_badnode.bn_per_threshold;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(cmd = 0x%x, subcmd = 0x%x, val = 0x%x)\n", 
                __FUNCTION__, at_proc.u4CtrlFieldID, at_proc.u4CtrlSubFieldID, pad->vow_badnode.bn_per_threshold));
            break;

        default:
	        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(No such command = 0x%x)\n", __FUNCTION__, subcmd));
    }
        
    ret = MtCmdSetVoWModuleCtrl(pad, &at_proc);
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:(ret = %d), sizeof %d\n", __FUNCTION__, ret, sizeof(EXT_CMD_AT_PROC_MODULE_CTRL_T)));
    
    return ret;
}

void vow_dump_umac_CRs(PRTMP_ADAPTER pad)
{
    int i;
    
    for (i = 0x8340 ;i < 0x83c0; i+= 4)
    {
        UINT32 val;

        RTMP_IO_READ32(pad, i, &val);
        printk("0x%0x -> 0x%0x\n", i, val);
    }
}
/* ---------------------- end -------------------------------*/

VOID vow_init(PRTMP_ADAPTER pad)
{
    UINT8 i;
    BOOLEAN ret;

    /* for M2M test */
    pvow_pad = pad;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, 
            ("\x1b[31m%s: start ...\x1b[m\n", __FUNCTION__));

    //vow_dump_umac_CRs(pad);

    /* group DWRR quantum */
    ret = vow_set_group(pad, 0xff, ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_ALL);

    /* set group configuration */
    ret = vow_set_group(pad, 0xff, ENUM_BSSGROUP_CTRL_ALL_ITEM_FOR_ALL_GROUP);

    /* if ATF is disabled, the default max DWRR wait time is configured as 256us to force STA round-robin */
    if (pad->vow_cfg.en_airtime_fairness == FALSE)
       pad->vow_cfg.sta_max_wait_time = 1; //256us

    /* set max wait time for DWRR station */
    ret = vow_set_sta_DWRR_max_time(pad);

    /* station DWRR quantum */
    ret = vow_set_sta(pad, 0xff, ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_ALL);

    /* per station DWRR configuration */
    for (i = 0; i < 128; i++)
    {
        ret = vow_set_sta(pad, i, ENUM_VOW_DRR_CTRL_FIELD_STA_ALL);
        ret = vow_set_sta(pad, i, ENUM_VOW_DRR_CTRL_FIELD_STA_BSS_GROUP);

        /* set station pause status */
        ret = vow_set_sta(pad, i, ENUM_VOW_DRR_CTRL_FIELD_STA_PAUSE_SETTING);
    }

    /* set max BSS wait time and sta wait time */
    //RTMP_IO_WRITE32(pad, 0x8374, 0x00200020);
    vow_set_group_DWRR_max_time(pad);

    /* reset all RX counters */ 
    ret = vow_set_rx_airtime(pad, ENUM_RX_AT_BITWISE_CTRL, ENUM_RX_AT_BITWISE_SUB_TYPE_AIRTIME_CLR);
    
    /* RX airtime feature enable */
    ret = vow_set_rx_airtime(pad, ENUM_RX_AT_FEATURE_CTRL, ENUM_RX_AT_FEATURE_SUB_TYPE_AIRTIME_EN);
    
    /* set ED offset */
    ret = vow_set_rx_airtime(pad, ENUM_RX_AT_TIMER_VALUE_CTRL, ENUM_RX_AT_TIME_VALUE_SUB_TYPE_ED_OFFSET_CTRL);

    /* set OBSS backoff time */
    ret = vow_set_backoff_time(pad, ENUM_RX_AT_OBSS);

    /* set non QOS backoff time */
    ret = vow_set_backoff_time(pad, ENUM_RX_AT_NON_QOS);

    /* set repeater backoff time */
    ret = vow_set_backoff_time(pad, ENUM_RX_AT_WMM_GROUP_PEPEATER);

    /* set OM backoff time */
    ret = vow_set_backoff_time(pad, ENUM_RX_AT_WMM_GROUP_STA);

    /* set WMM AC backoff time */
    for( i = 0; i < VOW_MAX_WMM_SET_NUM; i++)
    {
        ret = vow_set_backoff_time(pad, i);
    }

    /* set BSS belogs to which WMM set */
    for( i = 0; i < VOW_MAX_GROUP_NUM; i++)
    {
        ret = vow_set_mbss2wmm_map(pad, i);
    }

    /* select RX WMM backoff time for 4 OM */
    for( i = 0; i < VOW_MAX_WMM_SET_NUM; i++)
    {
        ret = vow_set_wmm_selection(pad, i);
    }

    /* configure airtime estimator */
    for (i = 0; i < VOW_MAX_GROUP_NUM; i++)
    {
        vow_set_at_estimator_group(pad, ENUM_AT_PROC_EST_GROUP_RATIO_CTRL, i);
        vow_set_at_estimator_group(pad, ENUM_AT_PROC_EST_GROUP_TO_BAND_MAPPING, i);
    }
    vow_set_at_estimator(pad, ENUM_AT_PROC_EST_MONITOR_PERIOD_CTRL);

    /* configure badnode detector */

    /* feature control */
    ret = vow_set_feature_all(pad);

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, 
            ("\x1b[31m%s: end ...\x1b[m\n", __FUNCTION__));
}

static UINT32 vow_get_availabe_airtime(VOID)
{
    return VOW_DEF_AVA_AIRTIME;
}

/* get rate token */
UINT16 vow_convert_rate_token(PRTMP_ADAPTER pad, UINT8 type, UINT8 group_id)
{
    UINT16 period, rate, token = 0;

    period = (1 << pad->vow_cfg.refill_period);

    if (type == VOW_MAX)
    {
        rate = pad->vow_bss_cfg[group_id].max_rate;
        token = (period * rate);
    }
    else
    {
        rate = pad->vow_bss_cfg[group_id].min_rate;
        token = (period * rate);
    }

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("%s: period %dus, rate %u, token %u\n", __FUNCTION__, period, rate, token));

    return token;
}

/* get airtime token */
UINT16 vow_convert_airtime_token(PRTMP_ADAPTER pad, UINT8 type, UINT8 group_id)
{
    UINT16 period, ratio, token = 0;
    UINT32 atime = vow_get_availabe_airtime();
    UINT64 tmp;

    period = (1 << pad->vow_cfg.refill_period);

    if (type == VOW_MAX)
    {
        ratio = pad->vow_bss_cfg[group_id].max_airtime_ratio;
    }
    else
    {
        ratio = pad->vow_bss_cfg[group_id].min_airtime_ratio;
    }

    /* shift 3 --> because unit is 1/8 us, 
       10^8 --> ratio needs to convert from integer to %, preiod needs to convert from us to s
    */
    tmp = ((UINT64)period * atime * ratio) << 3;
    //printk("%s: tmp %llu\n", __FUNCTION__, tmp);
    token = div64_u64(tmp, 100000000); 

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: period %dus, ratio %u, available time %u, token %u\n", __FUNCTION__, period, ratio, atime, token));

    return token;
}


/* add client(station) */

VOID vow_set_client(PRTMP_ADAPTER pad, UINT8 group, UINT8 sta_id)
{
    BOOLEAN ret;

    /* set group for station */
    pad->vow_sta_cfg[sta_id].group = group;

    /* update station bitmap */
    ret = vow_set_sta(pad, sta_id, ENUM_VOW_DRR_CTRL_FIELD_STA_BSS_GROUP);
    ret = vow_set_sta(pad, sta_id, ENUM_VOW_DRR_CTRL_FIELD_STA_ALL);

    /* set station pause status */
    ret = vow_set_sta(pad, sta_id, ENUM_VOW_DRR_CTRL_FIELD_STA_PAUSE_SETTING);

}

INT set_vow_min_rate_token(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_rate_token = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_RATE_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_rate_token(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_rate_token = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_RATE_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_min_airtime_token(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_airtime_token = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_AIRTIME_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_airtime_token(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_airtime_token = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_AIRTIME_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_min_rate_bucket(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_ratebucket_size = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_TOKEN_BUCKET_LENG_SIZE_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_rate_bucket(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_ratebucket_size = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_TOKEN_BUCKET_LENG_SIZE_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_min_airtime_bucket(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_airtimebucket_size = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_TOKEN_BUCKET_TIME_SIZE_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
            
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_airtime_bucket(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_airtimebucket_size = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_TOKEN_BUCKET_TIME_SIZE_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_backlog_size(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_backlog_size = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_BACKLOG_SIZE_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_wait_time(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_wait_time = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_WAIT_TIME_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_group_dwrr_max_wait_time(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0)
        {
            INT ret;

            pad->vow_cfg.group_max_wait_time = val;
            
            ret = vow_set_group_DWRR_max_time(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_pause(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &val);
        if ((rv > 1) && (sta < 128))
        {
            pad->vow_sta_cfg[sta].paused = val;
            
            vow_set_sta(pad, sta, ENUM_VOW_DRR_CTRL_FIELD_STA_PAUSE_SETTING);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: sta %d set %u.\n", __FUNCTION__, sta, val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_sta_group(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &val);
        if ((rv > 1) && (sta < 128))
        {
            INT ret;

            pad->vow_sta_cfg[sta].group = val;
            
 	        ret = vow_set_sta(pad, sta, ENUM_VOW_DRR_CTRL_FIELD_STA_BSS_GROUP);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: sta %d group %u.\n", __FUNCTION__, sta, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bw_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.en_bw_ctrl = val;
            
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_refill_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.en_bw_refill = val;
            
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_airtime_fairness_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.en_airtime_fairness = val;
            
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_txop_switch_bss_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.en_txop_no_change_bss = val;
            
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_dbdc_search_rule(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, band;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &band, &val);
        if ((rv > 1))
        {
            INT ret;

            if (band == 0)
                pad->vow_cfg.dbdc0_search_rule = val;
            else
                pad->vow_cfg.dbdc1_search_rule = val;

            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
            
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_refill_period(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.refill_period = val;

            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bss_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, group;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < 16))
        {
            INT ret;

            pad->vow_cfg.per_bss_enable &= ~(1 << group);
            pad->vow_cfg.per_bss_enable |= (val << group);

            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_dwrr_quantum(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, id;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &id, &val);
        if ((rv > 1) && (id < 8))
        {
            INT ret;

            pad->vow_cfg.vow_sta_dwrr_quantum[id] = val;

            ret = vow_set_sta(pad, 0xff, ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_QUANTUM_L0 + id);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set quantum id %u, val %d.\n", __FUNCTION__, id, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_airtime_ctrl_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, group;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < 16))
        {
            INT ret;

            pad->vow_bss_cfg[group].at_on = val;
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bw_ctrl_en(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, group;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < 16))
        {
            INT ret;

            pad->vow_bss_cfg[group].bw_on = val;
            ret = vow_set_feature_all(pad);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_ac_priority(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, sta;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &val);
        if ((rv > 1) && (sta < 128) && (val < 4))
        {
            BOOLEAN ret;

            /* set AC change rule */
            pad->vow_sta_cfg[sta].ac_change_rule = val;
            ret = vow_set_sta(pad, sta, ENUM_VOW_DRR_CTRL_FIELD_STA_PRIORITY);
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("%s: sta %d W ENUM_VOW_DRR_PRIORITY_CFG_ITEM failed.\n", __FUNCTION__, sta));
                return FALSE;
            }
            else
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("%s: sta %d W AC change rule %d.\n", __FUNCTION__, sta, pad->vow_sta_cfg[sta].ac_change_rule));
            }
            
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_dwrr_quantum_id(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, sta, ac;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d", &sta, &ac, &val);
        if ((rv > 2) && (sta < 128) && (ac < 4) && (val < 8))
        {
            INT ret;

            pad->vow_sta_cfg[sta].dwrr_quantum[ac] = val;
            
            ret = vow_set_sta(pad, sta, ENUM_VOW_DRR_CTRL_FIELD_STA_AC0_QUA_ID + ac);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set sta %d, ac %d, quantum id %u.\n", __FUNCTION__, sta, ac, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
            
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bss_dwrr_quantum(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv, group;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < 16))
        {
            INT ret;

            pad->vow_bss_cfg[group].dwrr_quantum = val;
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_BW_GROUP_QUANTUM_L_00 + group);
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set group %d, quantum id %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
            
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_dwrr_max_wait_time(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pad->vow_cfg.sta_max_wait_time = val;
            //ret = vow_set_sta(pad, 0xFF, ENUM_VOW_DRR_CTRL_FIELD_AIRTIME_DEFICIT_BOUND);
            ret = vow_set_sta_DWRR_max_time(pad);
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));
            
            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_min_rate(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_rate = val;
            pad->vow_bss_cfg[group].min_rate_token = vow_convert_rate_token(pad, VOW_MIN, group);
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_RATE_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set rate %u\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_rate(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_rate = val;
            pad->vow_bss_cfg[group].max_rate_token = vow_convert_rate_token(pad, VOW_MAX, group);
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_RATE_TOKEN_CFG_ITEM);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_min_ratio(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].min_airtime_ratio = val;
            pad->vow_bss_cfg[group].min_airtime_token = vow_convert_airtime_token(pad, VOW_MIN, group);
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MIN_AIRTIME_TOKEN_CFG_ITEM);
            ret = vow_set_at_estimator_group(pad, ENUM_AT_PROC_EST_GROUP_RATIO_CTRL, group);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_max_ratio(
    IN  PRTMP_ADAPTER pad,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pad->vow_bss_cfg[group].max_airtime_ratio = val;
            pad->vow_bss_cfg[group].max_airtime_token = vow_convert_airtime_token(pad, VOW_MAX, group);
            
            ret = vow_set_group(pad, group, ENUM_BSSGROUP_CTRL_MAX_AIRTIME_TOKEN_CFG_ITEM);
            ret = vow_set_at_estimator_group(pad, ENUM_AT_PROC_EST_GROUP_RATIO_CTRL, group);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d set %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_counter_clr(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            ret = vow_set_rx_airtime(pAd, ENUM_RX_AT_BITWISE_CTRL, ENUM_RX_AT_BITWISE_SUB_TYPE_AIRTIME_CLR);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_airtime_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0))
        {
            INT ret;

            pAd->vow_rx_time_cfg.rx_time_en = val;
            ret = vow_set_rx_airtime(pAd, ENUM_RX_AT_FEATURE_CTRL, ENUM_RX_AT_FEATURE_SUB_TYPE_AIRTIME_EN);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_ed_offset(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0) && (val < 32))
        {
            INT ret;

            pAd->vow_rx_time_cfg.ed_offset = val;
            ret = vow_set_rx_airtime(pAd, ENUM_RX_AT_TIMER_VALUE_CTRL, ENUM_RX_AT_TIME_VALUE_SUB_TYPE_ED_OFFSET_CTRL);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }

        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_rx_obss_backoff(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0) && (val <= 0xFFFF))
        {
            INT ret;

            pAd->vow_rx_time_cfg.obss_backoff = val;
            ret = vow_set_backoff_time(pAd, ENUM_RX_AT_OBSS);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_rx_wmm_backoff(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 wmm, ac, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d", &wmm, &ac, &val);
        if ((rv > 2) && (wmm < VOW_MAX_WMM_SET_NUM) && (ac < WMM_NUM_OF_AC) && (val <= 0xFFFF))
        {
            INT ret;

            pAd->vow_rx_time_cfg.wmm_backoff[wmm][ac] = val;
            ret = vow_set_backoff_time(pAd, wmm);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: wmm %d ac %d set %u.\n", __FUNCTION__, wmm, ac, val));

            if (ret)
            {
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_non_qos_backoff(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0) && (val <= 0xFFFFF))
        {
            INT ret;

            pAd->vow_rx_time_cfg.non_qos_backoff = val;
            ret = vow_set_backoff_time(pAd, ENUM_RX_AT_NON_QOS);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_om_wmm_backoff(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 ac, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &ac, &val);
        if ((rv > 0) && (ac < WMM_NUM_OF_AC) && (val <= 0xFFFFF))
        {
            INT ret;

            pAd->vow_rx_time_cfg.om_wmm_backoff[ac] = val;
            ret = vow_set_backoff_time(pAd, ENUM_RX_AT_WMM_GROUP_STA);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set ac %d, val = %u.\n", __FUNCTION__, ac, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_repeater_wmm_backoff(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 ac, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &ac, &val);
        if ((rv > 0) && (ac < WMM_NUM_OF_AC) && (val <= 0xFFFFF))
        {
            INT ret;

            pAd->vow_rx_time_cfg.repeater_wmm_backoff[ac] = val;
            ret = vow_set_backoff_time(pAd, ENUM_RX_AT_WMM_GROUP_PEPEATER);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set ac %d, val = %u.\n", __FUNCTION__, ac, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_bss_wmmset(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 bss_idx, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &bss_idx, &val);
        if ((rv > 1) && (val < VOW_MAX_WMM_SET_NUM) && (bss_idx < VOW_MAX_GROUP_NUM))
        {
            INT ret;

            pAd->vow_rx_time_cfg.bssid2wmm_set[bss_idx] = val;
            ret = vow_set_mbss2wmm_map(pAd, bss_idx);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: bss_idx %d set %u.\n", __FUNCTION__, bss_idx, val));

            if (ret)
            {
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_rx_om_wmm_select(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 om_idx, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &om_idx, &val);
        if ((rv > 1) && (om_idx < 4)) //FIXME: enum --> 4
        {
            INT ret;

            pAd->vow_rx_time_cfg.wmm_backoff_sel[om_idx] = val;
            ret = vow_set_wmm_selection(pAd, om_idx);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: OM MAC index %d set %u.\n", __FUNCTION__, om_idx, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

BOOLEAN halUmacVoWChargeBwToken (
    IN UINT_8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT_16 u2ChargeLenValue,
    IN UINT_16 u2ChargeTimeValue
    )
{
    UINT32 reg;

    if ((ucBssGroup >= UMAC_BSS_GROUP_NUMBER ) ||
        ((fgChargeMode != TRUE) && (fgChargeMode != FALSE)))
    {
        return FALSE;
    }

    reg = (((u2ChargeLenValue << UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_OFFSET) & UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_MASK) |
                                         ((u2ChargeTimeValue << UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_OFFSET) & UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_MASK));

    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_WDATA0, reg);

    reg = ((UMAC_DRR_TABLE_CTRL0_EXEC_MASK) |
                                        ((UMAC_CHARGE_BW_TOKEN_OP_MASK | ((fgChargeMode << (ffs(UMAC_CHARGE_ADD_MODE_BIT_MASK) - 1)) & UMAC_CHARGE_ADD_MODE_BIT_MASK)) << UMAC_DRR_TABLE_CTRL0_MODE_OP_OFFSET) |
                                        ((ucBssGroup << UMAC_CHARGE_MODE_BSS_GROUP_OFFSET) & UMAC_CHARGE_MODE_BSS_GROUP_MASK));

    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_CTRL0, reg);

    return TRUE;
}

BOOLEAN halUmacVoWChargeBwTokenLength (
    IN UINT8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeLenValue
    )
{
    return halUmacVoWChargeBwToken(ucBssGroup, fgChargeMode, u2ChargeLenValue, 0);
}



BOOLEAN halUmacVoWChargeBwTokenTime (
    IN UINT8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeTimeValue
    )
{
    return halUmacVoWChargeBwToken(ucBssGroup, fgChargeMode, 0, u2ChargeTimeValue);
}

BOOLEAN halUmacVoWChargeBwDrr (
    IN UINT8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeLenValue,
    IN UINT16 u2ChargeTimeValue
    )
{
    UINT32 reg;

    if ((ucBssGroup >= UMAC_BSS_GROUP_NUMBER ) ||
        ((fgChargeMode != TRUE) && (fgChargeMode != FALSE)))
    {
        return FALSE;
    }

    reg = (((u2ChargeLenValue << UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_OFFSET) & UMAC_CHARGE_MODE_WDATA_CHARGE_LENGTH_MASK) |
                                         ((u2ChargeTimeValue << UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_OFFSET) & UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_MASK));

    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_WDATA0, reg);

    reg = ((UMAC_DRR_TABLE_CTRL0_EXEC_MASK) |
                                        ((UMAC_CHARGE_BW_DRR_OP_MASK | (fgChargeMode << UMAC_CHARGE_ADD_MODE_BIT_MASK)) << UMAC_DRR_TABLE_CTRL0_MODE_OP_OFFSET) |
                                        ((ucBssGroup << UMAC_CHARGE_MODE_BSS_GROUP_OFFSET) & UMAC_CHARGE_MODE_BSS_GROUP_MASK));

    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_CTRL0, reg);


    return TRUE;
}

BOOLEAN halUmacVoWChargeBwDrrLength (
    IN UINT8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeLenValue
    )
{
    return halUmacVoWChargeBwDrr(ucBssGroup, fgChargeMode, u2ChargeLenValue, 0);
}

BOOLEAN halUmacVoWChargeBwDrrTime (
    IN UINT8 ucBssGroup,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeTimeValue
    )
{
    return halUmacVoWChargeBwDrr(ucBssGroup, fgChargeMode, 0, u2ChargeTimeValue);
}


BOOLEAN halUmacVoWChargeAitTimeDRR (
    IN UINT8 ucStaID,
    IN UINT8 ucAcId,
    IN BOOLEAN fgChargeMode,
    IN UINT16 u2ChargeValue
    )
{
    UINT32 reg;

    if ((ucStaID > UMAC_WLAN_ID_MAX_VALUE) ||
        (ucAcId >= 4) ||
        ((fgChargeMode != TRUE) && (fgChargeMode != FALSE)))
    {
        return FALSE;
    }

    reg = ((u2ChargeValue << UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_OFFSET) & UMAC_CHARGE_MODE_WDATA_CHARGE_TIME_MASK);
    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_WDATA0, reg);

    reg = ((UMAC_DRR_TABLE_CTRL0_EXEC_MASK) |
                                        ((UMAC_CHARGE_AIRTIME_DRR_OP_MASK | ((fgChargeMode << (ffs(UMAC_CHARGE_ADD_MODE_BIT_MASK)-1) & UMAC_CHARGE_ADD_MODE_BIT_MASK))) << UMAC_DRR_TABLE_CTRL0_MODE_OP_OFFSET) |
                                        ((ucStaID << UMAC_CHARGE_MODE_STA_ID_OFFSET) & UMAC_CHARGE_MODE_STA_ID_MASK) |
                                        ((ucAcId << UMAC_CHARGE_MODE_QUEUE_ID_OFFSET) & UMAC_CHARGE_MODE_QUEUE_ID_MASK));

    RTMP_IO_WRITE32(pvow_pad, UMAC_DRR_TABLE_CTRL0, reg);


    return TRUE;
}



INT set_vow_charge_sta_dwrr(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, ac, mode, val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d-%d", &sta, &mode, &ac, &val);
        if ((rv > 3) && (sta < MAX_LEN_OF_MAC_TABLE) && (ac < WMM_NUM_OF_AC))
        {
            halUmacVoWChargeAitTimeDRR(sta, ac, mode, val);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: sta%d/ac%d %c charge--> %u.\n", __FUNCTION__, sta, ac, mode == 0 ? 'd' : 'a', val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_charge_bw_time(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, mode, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d", &group, &mode, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            halUmacVoWChargeBwTokenTime(group, mode, val);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group%d %c charge--> %u.\n", __FUNCTION__, group, mode == 0 ? 'd' : 'a', val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_charge_bw_len(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, mode, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d", &group, &mode, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            halUmacVoWChargeBwTokenLength(group, mode, val);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group%d %c charge--> %u.\n", __FUNCTION__, group, mode == 0 ? 'd' : 'a', val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_charge_bw_dwrr(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val, mode, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d-%d", &group, &mode, &val);
        if ((rv > 1) && (group < VOW_MAX_GROUP_NUM))
        {
            halUmacVoWChargeBwDrrTime(group, mode, val);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group%d %c charge--> %u.\n", __FUNCTION__, group, mode == 0 ? 'd' : 'a', val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_sta_psm(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, psm, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &psm);
        if ((rv > 1) && (sta < MAX_LEN_OF_MAC_TABLE))
        {
            UINT32 offset, reg;

            //clear PSM update mask bit31
            RTMP_IO_WRITE32(pAd, 0x23000, 0x80000000);

            //set PSM bit in WTBL DW3 bit 30
            offset = (sta << 8) | 0x3000c;

            RTMP_IO_READ32(pAd, offset, &reg);

            if (psm)
            {
                reg |= 0x40000000;
                RTMP_IO_WRITE32(pAd, offset, reg);
            }
            else
            {
                reg &= ~0x40000000;
                RTMP_IO_WRITE32(pAd, offset, reg);
            }

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: sta%d psm--> %u.\n", __FUNCTION__, sta, psm));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}



INT set_vow_monitor_sta(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &sta);
        if ((rv > 0) && (sta < 128))
        {
            pAd->vow_monitor_sta = sta;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: monitor sta%d.\n", __FUNCTION__, sta));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_monitor_bss(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 bss, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &bss);
        if ((rv > 0) && (bss <= 16))
        {
            pAd->vow_monitor_bss = bss;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: monitor bss%d.\n", __FUNCTION__, bss));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_monitor_mbss(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 bss, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &bss);
        if ((rv > 0) && (bss < 16))
        {
            pAd->vow_monitor_mbss = bss;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: monitor mbss%d.\n", __FUNCTION__, bss));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_avg_num(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 num, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &num);
        if ((rv > 0) && (num < 1000))
        {
            pAd->vow_avg_num = num;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: average numer %d.\n", __FUNCTION__, num));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_dvt_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 en, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &en);
        if (rv > 0)
        {
        
            pAd->vow_dvt_en = en;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: DVT enable %d.\n", __FUNCTION__, pAd->vow_dvt_en));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_help(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
        ("======== Group table =========\n"
         "vow_min_rate_token = <group>-<token>\n"
         "vow_max_rate_token = <group>-<token>\n"
         "vow_min_airtime_token = <group>-<token>\n"
         "vow_max_airtime_token = <group>-<token>\n"
         "vow_min_rate_bucket = <group>-<byte> 1K\n"
         "vow_max_rate_bucket = <group>-<byte> 1K\n"
         "vow_min_airtime_bucket = <group>-<time> 1.024\n"
         "vow_max_airtime_bucket = <group>-<time> 1.024\n"
         "vow_max_wait_time = <group>-<time> 1.024\n"
         "vow_max_backlog_size = <group>-<byte> 1K\n"
         "======== Control =============\n"
         "vow_bw_enable = <0/1> 0:disable, 1:enable\n"
         "vow_refill_en = <0/1> 0:disable, 1:enable\n"
         "vow_airtime_fairness_en = <0/1> 0:disable, 1:enable\n"
         "vow_txop_switch_bss_en = <0/1> 0:disable, 1:enable\n"
         "vow_dbdc_search_rule = <band>-<0/1> 0:WMM AC, 1:WMM set\n"
         "vow_refill_period = <n> 2^n\n"
         "vow_bss_enable = <group>-<0/1> 0:disable, 1:enable\n"
         "vow_airtime_control_en = <group>-<0/1> 0:disable, 1:enable\n"
         "vow_bw_control_en = <group>-<0/1> 0:disable, 1:enable\n"));

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
         ("======== Group others =============\n"
         "vow_bss_dwrr_quantum = <group>-<time> 256us\n"
         "vow_group_dwrr_max_wait_time = <time> 256us\n"
         "vow_group2band_map = <group>-<band>\n"
         "======== Station table =============\n"
         "vow_sta_dwrr_quantum = <Qid>-<val> 256us\n"
         "vow_sta_dwrr_quantum_id = <wlanidx>-<WMMA AC>-<Qid>\n"
         "vow_sta_ac_priority = <wlanidx>-<0/1/2> 0:disable, 1:BE, 2:BK\n"
         "vow_sta_pause = <wlanidx>-<0/1> 0: normal, 1: pause\n"
         "vow_sta_psm = <wlanidx>-<0/1> 0: normal, 1: power save\n"
         "vow_sta_group = <wlanidx>-<group>\n"
         "vow_dwrr_max_wait_time = <time> 256us\n"
         "======== User Config =============\n"
         "vow_min_rate = <group>-<Mbps>\n"
         "vow_max_rate = <group>-<Mbps>\n"
         "vow_min_ratio = <group>-<%%>\n"
         "vow_max_ratio = <group>-<%%>\n"));

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
         ("======== Rx Config =============\n"
         "vow_rx_counter_clr = <n>\n"
         "vow_rx_airtime_en = <0/1> 0:dieable, 1:enable\n"
         "vow_rx_ed_offset = <val> 1.024(5b)\n"
         "vow_rx_obss_backoff = <val> 1.024(16b)\n"
         "vow_rx_wmm_backoff = <WMM set>-<WMM AC>-<val>\n"
         "vow_om_wmm_backoff = <WMM AC>-<val>\n"
         "vow_repeater_wmm_backoff = <WMM AC>-<val>\n"
         "vow_rx_non_qos_backoff = <val>\n" 
         "vow_rx_bss_wmmset = <MBSS idx>-<0/1/2/3>\n"
         "vow_rx_om_wmm_sel = <OM idx>-<val> 0:RX WMM(1to1), 1:OM wmm\n"
         "======== Airtime estimator =============\n"
         "vow_at_est_en = <0/1> 0:dieable, 1:enable\n"
         "vow_at_mon_period = <period> ms\n"));

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
         ("======== Badnode detector =============\n"
         "vow_bn_en = <0/1> 0:dieable, 1:enable\n"
         "vow_bn_mon_period = <period> ms\n"
         "vow_bn_fallback_th = <count>\n"
         "vow_bn_per_th = <TX PER>\n"
         "======== Airtime counter test =============\n"
         "vow_counter_test = <0/1> 0:dieable, 1:enable\n"
         "vow_counter_test_period = <period> ms\n"
         "vow_counter_test_band = <band>\n"
         "vow_counter_test_avgcnt = <average num> sec\n"
         "vow_counter_test_target = <wlanidx>\n"
         "======== DVT =============\n"
         "vow_dvt_en = <0/1> 0:dieable, 1:enable\n"
         "vow_monitor_sta = <STA num>\n"
         "vow_show_sta = <STA num>\n"
         "vow_monitor_bss = <BSS num>\n"
         "vow_monitor_mbss = <MBSS num>\n"
         "vow_show_mbss = <MBSS num>\n"
         "vow_avg_num = <average num> sec\n"));
         
    return TRUE;
}

#if defined(MT7615_FPGA) || defined(MT7622_FPGA)
//M2M test

INT set_vow_cloned_wtbl(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 num, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &num);
        if ((rv > 0) && (num < 127))
        {
            UCHAR band = 0;
            UCHAR band_num = 0;

            /* DBDC */
            if (pAd->CommonCfg.dbdc_mode)
                band_num = 2;
            else
                band_num = 1;

            for (band = 0; band < band_num; band ++)
            {
                UINT32 i, from, start, end;
	            MAC_TABLE_ENTRY *pMacEntry = NULL;
                UCHAR wmm_set = 0;

                if (band == 0)
                {
                    /* DBDC */
                    if (pAd->CommonCfg.dbdc_mode)
                    {
                        pAd->vow_cloned_wtbl_num[band] = num + 2;
                        start = 3;
                    }
                    else
                    {
                        pAd->vow_cloned_wtbl_num[band] = num + 1;
                        start = 2;
                    }

                    end = pAd->vow_cloned_wtbl_num[band];
                    from  = WTBL_BASE_ADDR + WTBL_PER_ENTRY_SIZE;

		            pMacEntry = &pAd->MacTab.Content[1];
                    wmm_set = HcGetWmmIdx(pAd, pMacEntry->wdev);

                    pAd->vow_sta_wmm[1] = wmm_set;
                }
                else
                {
                    pAd->vow_cloned_wtbl_num[band] = pAd->vow_cloned_wtbl_num[0] + num;

                    start = pAd->vow_cloned_wtbl_num[0] + 1;
                    end = pAd->vow_cloned_wtbl_num[band];

                    from  = WTBL_BASE_ADDR + WTBL_PER_ENTRY_SIZE * 2;

		            pMacEntry = &pAd->MacTab.Content[2];
                    wmm_set = HcGetWmmIdx(pAd, pMacEntry->wdev);

                    pAd->vow_sta_wmm[2] = wmm_set;
                }

                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("%s: band%d, start/end = %d/%d, wmm set = %d wtbl num = %d.\n", 
                        __FUNCTION__, band, start, end, wmm_set, pAd->vow_cloned_wtbl_num[band]));


                //for(i = 3; i <= pAd->vow_cloned_wtbl_num; i++)
                for(i = start; i <= end; i++)
                {
                    UINT32 offset = WTBL_BASE_ADDR + i*WTBL_PER_ENTRY_SIZE;
                    UINT32 j;
                    UINT32 val;

                    //by WCID assigned wmm set number
                    pAd->vow_sta_wmm[i] = wmm_set;

                    /* copy a station WTBL */
                    for (j = 0; j < WTBL_PER_ENTRY_SIZE; j+=4)
                    {

                        RTMP_IO_READ32(pAd, from + j, &val);
                        RTMP_IO_WRITE32(pAd, offset +j , val);
                    }

                    /* modified peer addres */
                    RTMP_IO_READ32(pAd, offset, &val);
                    val &= ~0xffff;
                    val |= 0xaa;
                    val |= (i << 8);
                    RTMP_IO_WRITE32(pAd, offset, val);
                }

                /* DBDC */
                if (pAd->CommonCfg.dbdc_mode)
                    pAd->vow_cloned_wtbl_max = pAd->vow_cloned_wtbl_num[1];
                else
                    pAd->vow_cloned_wtbl_max = pAd->vow_cloned_wtbl_num[0];

                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("%s: clone wtbl numer %d.\n", __FUNCTION__, num));
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_pkt_ac(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, ac, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &ac);
        if ((rv > 1) && (sta < MAX_LEN_OF_MAC_TABLE))
        {
            pAd->force_pkt_sta = sta;
            pAd->force_pkt_ac = ac;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: sta%d ac--> %u.\n", __FUNCTION__, sta, ac));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_ack_all(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 ack, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &ack);
        if ((rv > 0) && (ack < 4))
        {
            UINT32 i, s_id;

            /* DBDC */
            if (pAd->CommonCfg.dbdc_mode)
                s_id = 3;
            else
                s_id = 2;

            for(i = s_id; i <= pAd->vow_cloned_wtbl_max; i++)
                pAd->vow_sta_ack[i] = ack;
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_ack(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, ack, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &ack);
        if ((rv > 0) && (sta < 127) && (ack < 4))
        {
        
            pAd->vow_sta_ack[sta] = ack;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: station %d, ack policy %d.\n", __FUNCTION__, sta, pAd->vow_sta_ack[sta]));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_wmm_all(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, wmm, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &wmm);
        if ((rv > 0) && (wmm < 4))
        {
            UINT32 i, s_id;
        
            /* DBDC */
            if (pAd->CommonCfg.dbdc_mode)
                s_id = 3;
            else
                s_id = 2;

            for(i = s_id; i <= pAd->vow_cloned_wtbl_max; i++)
                pAd->vow_sta_wmm[sta] = wmm;
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_wmm(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, wmm, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &wmm);
        if ((rv > 0) && (sta < 127) && (wmm < 4))
        {
        
            pAd->vow_sta_wmm[sta] = wmm;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: station %d, wmm set %d.\n", __FUNCTION__, sta, pAd->vow_sta_wmm[sta]));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_ac_all(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, ac, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &ac);
        if ((rv > 0) && (ac < 4))
        {
            UINT32 i, s_id;
        
            /* DBDC */
            if (pAd->CommonCfg.dbdc_mode)
                s_id = 3;
            else
                s_id = 2;

            for(i = s_id; i <= pAd->vow_cloned_wtbl_max; i++)
                pAd->vow_sta_ac[sta] = ac;
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_ac(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, ac, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &ac);
        if ((rv > 0) && (sta < 127) && (ac < 4))
        {
        
            pAd->vow_sta_ac[sta] = ac;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: station %d, ac set to %d.\n", __FUNCTION__, sta, pAd->vow_sta_ac[sta]));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_tx_en_all(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 en, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &en);
        if ((rv > 0) && (en < 4))
        {
            UINT32 i, s_id;
        
            /* DBDC */
            if (pAd->CommonCfg.dbdc_mode)
                s_id = 3;
            else
                s_id = 2;

            for(i = s_id; i <= pAd->vow_cloned_wtbl_max; i++)
                pAd->vow_tx_en[i] = en;
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_tx_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, en, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &en);
        if ((rv > 0) && (sta < 127) && (en < 4))
        {
        
            pAd->vow_tx_en[sta] = en;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: station %d, tx enable %d.\n", __FUNCTION__, sta, pAd->vow_tx_en[sta]));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_mbss(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, mbss, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &sta, &mbss);
        if ((rv > 0) && (sta < 127) && (mbss < 16))
        {
        
            pAd->vow_sta_mbss[sta] = mbss;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: station %d belong to MBSS%d.\n", __FUNCTION__, sta, mbss));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

/* for CR4 commands */
INT set_vow_sta_cnt(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 cnt, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &cnt);
        if ((rv > 0) && (cnt <= 127))
        {

            MtCmdSetStaCnt(pAd, HOST2CR4, cnt);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set CR4 station count to %d.\n", __FUNCTION__, cnt));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_q(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 q, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &q);
        if ((rv > 0) && (q <= 5000))
        {

            MtCmdSetStaQLen(pAd, HOST2CR4, q);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set CR4 station queue to %d.\n", __FUNCTION__, q));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta2_q(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 q, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &q);
        if ((rv > 0) && (q <= 5000))
        {

            MtCmdSetSta2QLen(pAd, HOST2CR4, q);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set CR4 station2 queue to %d.\n", __FUNCTION__, q));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_sta_th(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 th, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &th);
        if ((rv > 0) && (th <= 5000))
        {

            MtCmdSetEmptyThreshold(pAd, HOST2CR4, th);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set CR4 station empty threshold to %d.\n", __FUNCTION__, th));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}
/* for CR4 commands - end */

//show


INT set_vow_life_time(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if ((rv > 0) && (val >= 0))
        {
            pAd->vow_life_time = val;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: vow life time--> %d.\n", __FUNCTION__, val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bcmc_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0)
        {
            pAd->vow_bcmc_en = val;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: bcmc en --> %d.\n", __FUNCTION__, val));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

#endif /* defined(MT7615_FPGA) || defined(MT7622_FPGA) */

/* commands for show */
INT set_vow_show_sta(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &sta);
        if ((rv > 0) && (sta < 127))
        {
        
            pAd->vow_show_sta = sta;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: show station up to %d.\n", __FUNCTION__, pAd->vow_show_sta));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_show_mbss(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 mbss, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &mbss);
        if ((rv > 0) && (mbss <= 16))
        {
        
            pAd->vow_show_mbss = mbss;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: show MBSS up to %d.\n", __FUNCTION__, pAd->vow_show_mbss));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT show_vow_dump_vow(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    vow_dump_umac_CRs(pAd);

    return TRUE;
}

INT show_vow_dump_sta(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, val1, val2, val3, val4, val;

    for (sta = 0; sta < 16; sta++)
    {
        val = 0x80220000;
        val |= sta;
        RTMP_IO_WRITE32(pAd, 0x8388, val);
        RTMP_IO_READ32(pAd, 0x8350, &val1);
        RTMP_IO_READ32(pAd, 0x8354, &val2);
        RTMP_IO_READ32(pAd, 0x8358, &val3);
        RTMP_IO_READ32(pAd, 0x835c, &val4);

        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("STA%d~%d: 0x%08X, 0x%08X, 0x%08X, 0x%08X.\n", sta << 3, (sta << 3)+7, val1, val2, val3, val4));
        
    }

    return TRUE;
}

INT show_vow_dump_bss_bitmap(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val1, val2, val3, val4, val;

    for (group = 0; group < 16; group++)
    {
        val = 0x80440000;
        val |= group;
        RTMP_IO_WRITE32(pAd, 0x8388, val);
        RTMP_IO_READ32(pAd, 0x8350, &val1);
        RTMP_IO_READ32(pAd, 0x8354, &val2);
        RTMP_IO_READ32(pAd, 0x8358, &val3);
        RTMP_IO_READ32(pAd, 0x835c, &val4);

        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("Group%d BitMap: 0x%08X, 0x%08X, 0x%08X, 0x%08X.\n", group, val1, val2, val3, val4));
        
    }

    return TRUE;
}

INT show_vow_dump_bss(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val1, val2, val3, val4, val;

    for (group = 16; group < 32; group++)
    {
        val = 0x80440000;
        val |= group;
        RTMP_IO_WRITE32(pAd, 0x8388, val);
        RTMP_IO_READ32(pAd, 0x8350, &val1);
        RTMP_IO_READ32(pAd, 0x8354, &val2);
        RTMP_IO_READ32(pAd, 0x8358, &val3);
        RTMP_IO_READ32(pAd, 0x835c, &val4);

        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("Group%d config: 0x%08X, 0x%08X, 0x%08X, 0x%08X.\n", group - 16, val1, val2, val3, val4));
        
    }

    return TRUE;
}

INT vow_show_bss_atoken(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val1, val2, val3, val4, val;
    UINT32 rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &group);
        if ((rv > 0) && (group < 16))
        {
            val = 0x80440000;
            val |= group + 32;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8350, &val1);
            RTMP_IO_READ32(pAd, 0x8354, &val2);
            RTMP_IO_READ32(pAd, 0x8358, &val3);
            RTMP_IO_READ32(pAd, 0x835c, &val4);

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Group%d airtime token: max 0x%08X, min 0x%08X.\n", group, val2, val1));
                
        }
        else
            return FALSE;
    }
    else
        return FALSE;


    return TRUE;
}

INT vow_show_bss_ltoken(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val1, val2, val3, val4, val;
    UINT32 rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &group);
        if ((rv > 0) && (group < 16))
        {
            val = 0x80440000;

            val |= group + 32;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8350, &val1);
            RTMP_IO_READ32(pAd, 0x8354, &val2);
            RTMP_IO_READ32(pAd, 0x8358, &val3);
            RTMP_IO_READ32(pAd, 0x835c, &val4);

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Group%d length token: max 0x%08X, min 0x%08X\n", group, val4, val3));
                
        }
        else
            return FALSE;
    }
    else
        return FALSE;


    return TRUE;
}

INT vow_show_bss_dtoken(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 group, val1, val2, val3, val4, val;
    UINT32 rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &group);
        if ((rv > 0) && (group < 16))
        {
            val = 0x80460000;

            val |= group;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8350, &val1);
            RTMP_IO_READ32(pAd, 0x8354, &val2);
            RTMP_IO_READ32(pAd, 0x8358, &val3);
            RTMP_IO_READ32(pAd, 0x835c, &val4);

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Group%d airtime token: max 0x%08X, min 0x%08X\n", group, val3, val4));
                
        }
        else
            return FALSE;
    }
    else
        return FALSE;


    return TRUE;
}

INT vow_show_sta_dtoken(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, val1, val2, val3, val4, val;
    UINT32 rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &sta);
        if ((rv > 0) && (sta < 128))
        {
            val = 0x80000000;
            val |= (sta << 2);

            //BK
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8358, &val1);

            //BE
            val |= 1;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8358, &val2);

            //VI
            val |= 2;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8358, &val3);

            //VO
            val |= 3;
            RTMP_IO_WRITE32(pAd, 0x8388, val);
            RTMP_IO_READ32(pAd, 0x8358, &val4);

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Sta%d deficit token: ac0 0x%08X, ac1 0x%08X, ac2 0x%08X, ac3 0x%08X\n", sta, val1, val2, val3, val4));
                
        }
        else
            return FALSE;
    }
    else
        return FALSE;


    return TRUE;
}


INT show_vow_rx_time(
    IN  RTMP_ADAPTER *pAd, 
    IN  RTMP_STRING *arg)
{
    UINT32 counter[4];

    counter[0] = vow_get_rx_time_counter(pAd, ENUM_RX_AT_REPORT_SUB_TYPE_RX_NONWIFI_TIME, 0);
    counter[1] = vow_get_rx_time_counter(pAd, ENUM_RX_AT_REPORT_SUB_TYPE_RX_NONWIFI_TIME, 1);
    counter[2] = vow_get_rx_time_counter(pAd, ENUM_RX_AT_REPORT_SUB_TYPE_RX_OBSS_TIME, 0);
    counter[3] = vow_get_rx_time_counter(pAd, ENUM_RX_AT_REPORT_SUB_TYPE_RX_OBSS_TIME, 1);

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
        ("%s: nonwifi %u/%u, obss %u/%u.\n", __FUNCTION__, counter[0], counter[2], counter[1], counter[3]));
    
    return TRUE;
}

INT show_vow_sta_conf(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 sta, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &sta);
        if ((rv > 0) && (sta < MAX_LEN_OF_MAC_TABLE))
        {
            UINT32 pri, q;
            CHAR* pri_str[] = {"No change.", "To BE.", "To BK."};

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ************ sta%d ***********\n", __FUNCTION__, sta));
            /* group */
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Group --> %u\n", pAd->vow_sta_cfg[sta].group));

            /* priority */
            pri = pAd->vow_sta_cfg[sta].ac_change_rule;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Priority --> %s(%u)\n", pri_str[pri], pri));

            /* airtime quantum for AC */
            q = pAd->vow_sta_cfg[sta].dwrr_quantum[WMM_AC_BK];
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("Ac0 --> %uus(%u)\n", (pAd->vow_cfg.vow_sta_dwrr_quantum[q] << 8), q));

            q = pAd->vow_sta_cfg[sta].dwrr_quantum[WMM_AC_BE];
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("Ac1 --> %uus(%u)\n", (pAd->vow_cfg.vow_sta_dwrr_quantum[q] << 8), q));

            q = pAd->vow_sta_cfg[sta].dwrr_quantum[WMM_AC_VI];
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("Ac2 --> %uus(%u)\n", (pAd->vow_cfg.vow_sta_dwrr_quantum[q] << 8), q));

            q = pAd->vow_sta_cfg[sta].dwrr_quantum[WMM_AC_VO];
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                    ("Ac3 --> %uus(%u)\n", (pAd->vow_cfg.vow_sta_dwrr_quantum[q] << 8), q));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT show_vow_all_sta_conf(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 i;
    
    for(i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
    {
        CHAR str[4];
        
        sprintf(str, "%d", i);
        show_vow_sta_conf(pAd, str);
    }

    return TRUE;
}

INT show_vow_bss_conf(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 bss, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &bss);
        if ((rv > 0) && (bss < VOW_MAX_GROUP_NUM))
        {
            UINT32 val;

            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: ************** Group%d **********\n", __FUNCTION__, bss));

            /* per BSS bw control */
            val = pAd->vow_cfg.per_bss_enable & (1UL << bss);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("BW control --> %s(%d)\n", val == 0 ? "Disable" : "Enable", val));

            /* per BSS airtime control */
            val = pAd->vow_bss_cfg[bss].at_on;
            //val = halUmacVoWGetDisablePerBssCheckTimeTokenFeature(bss);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Airtime control --> %s(%d)\n", val == 0 ? "Disable" : "Enable", val));

            /* per BSS TP control */
            //val = halUmacVoWGetDisablePerBssCheckLengthTokenFeature(bss);
            val = pAd->vow_bss_cfg[bss].bw_on;
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                        ("Rate control --> %s(%d)\n", val == 0 ? "Disable" : "Enable", val));

            /* Rate Setting */
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Rate --> %u/%uMbps\n", pAd->vow_bss_cfg[bss].max_rate, pAd->vow_bss_cfg[bss].min_rate));

            /* Airtime Setting */
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Airtime ratio --> %u/%u %% \n", pAd->vow_bss_cfg[bss].max_airtime_ratio, pAd->vow_bss_cfg[bss].min_airtime_ratio));

            /* Rate token */
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Rate token --> %u Byte(%u)/%u Byte(%u)\n", 
                    (pAd->vow_bss_cfg[bss].max_rate_token >> 3), 
                    pAd->vow_bss_cfg[bss].max_rate_token, 
                    (pAd->vow_bss_cfg[bss].min_rate_token >> 3),
                    pAd->vow_bss_cfg[bss].min_rate_token));
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Rate bucket --> %u Byte/%u Byte\n", 
                    pAd->vow_bss_cfg[bss].max_ratebucket_size << 10, 
                    pAd->vow_bss_cfg[bss].min_ratebucket_size << 10)); 

            /* Airtime token */
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Airtime token --> %u us(%u)/%u us(%u)\n", 
                    (pAd->vow_bss_cfg[bss].max_airtime_token >> 3), 
                    pAd->vow_bss_cfg[bss].max_airtime_token, 
                    (pAd->vow_bss_cfg[bss].min_airtime_token >> 3),
                    pAd->vow_bss_cfg[bss].min_airtime_token));
            
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
            ("Airtime bucket --> %u us/%u us\n", 
                    pAd->vow_bss_cfg[bss].max_airtimebucket_size << 10, 
                    pAd->vow_bss_cfg[bss].min_airtimebucket_size << 10)); 

        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT show_vow_all_bss_conf(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 i;
    
    for(i = 0; i < VOW_MAX_GROUP_NUM; i++)
    {
        CHAR str[4];
        
        sprintf(str, "%d", i);
        show_vow_bss_conf(pAd, str);
    }

    return TRUE;
}

INT show_vow_help(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
    ("vow_rx_time (Non-wifi/OBSS)\n"
     "vow_sta_conf = <wlanidx>\n"
     "vow_sta_conf\n"
     "vow_bss_conf = <group>\n"
     "vow_all_bss_conf\n"
     "vow_dump_sta (raw)\n"
     "vow_dump_bss_bitmap (raw)\n"
     "vow_dump_bss (raw)\n"
     "vow_dump_vow (raw)\n"
     "vow_show_sta_dtoken = <wlanidx> DWRR\n"
     "vow_show_bss_dtoken = <group> DWRR\n"
     "vow_show_bss_atoken = <group> airtime\n"
     "vow_show_bss_ltoken = <group> length\n"));

    return TRUE;
}
/* airtime estimator */
INT set_vow_at_est_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_at_est.at_estimator_en = val;
            ret = vow_set_at_estimator(pAd, ENUM_AT_RPOCESS_ESTIMATE_MODULE_CTRL);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: value %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_at_mon_period(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_at_est.at_monitor_period = val;
            ret = vow_set_at_estimator(pAd, ENUM_AT_PROC_EST_MONITOR_PERIOD_CTRL);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: period %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_group2band_map(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, group, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d-%d", &group, &val);
        if ((rv > 1) && (group < 16)) //
        {
            INT ret;

            pAd->vow_bss_cfg[group].band_idx = val;
            ret = vow_set_at_estimator_group(pAd, ENUM_AT_PROC_EST_GROUP_TO_BAND_MAPPING, group);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: group %d, band %u.\n", __FUNCTION__, group, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}
/* bad node detetor */
INT set_vow_bn_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_badnode.bn_en = val;
            ret = vow_set_bad_node(pAd, ENUM_AT_PROC_BAD_NODE_FEATURE_CTRL);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: value %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bn_mon_period(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_badnode.bn_monitor_period = val;
            ret = vow_set_bad_node(pAd, ENUM_AT_PROC_BAD_NODE_MONITOR_PERIOD_CTRL);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: period %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bn_fallback_th(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_badnode.bn_fallback_threshold = val;
            ret = vow_set_bad_node(pAd, ENUM_AT_PROC_BAD_NODE_FALLBACK_THRESHOLD);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: period %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_bn_per_th(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 val, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &val);
        if (rv > 0) //
        {
            INT ret;

            pAd->vow_badnode.bn_per_threshold = val;
            ret = vow_set_bad_node(pAd, ENUM_AT_PROC_BAD_NODE_PER_THRESHOLD);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: per %u.\n", __FUNCTION__, val));

            if (ret)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: set command failed.\n", __FUNCTION__));
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


INT set_vow_counter_test_en(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 on, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &on);
        if (rv > 0)
        {
            MtCmdSetVoWCounterCtrl(pAd, 1, on);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: on = %d\n", __FUNCTION__, on));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_counter_test_period(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 period, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &period);
        if (rv > 0)
        {
            MtCmdSetVoWCounterCtrl(pAd, 2, period);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: period = %d\n", __FUNCTION__, period));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_counter_test_band(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 band, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &band);
        if (rv > 0)
        {
            MtCmdSetVoWCounterCtrl(pAd, 3, band);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: band = %d\n", __FUNCTION__, band));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_counter_test_avgcnt(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 avgcnt, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &avgcnt);
        if (rv > 0)
        {
            MtCmdSetVoWCounterCtrl(pAd, 4, avgcnt);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: avgcnt = %d\n", __FUNCTION__, avgcnt));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

INT set_vow_counter_test_target(
    IN  PRTMP_ADAPTER pAd,
    IN  RTMP_STRING *arg)
{
    UINT32 target, rv;

    if (arg)
    {
        rv = sscanf(arg, "%d", &target);
        if (rv > 0)
        {
            MtCmdSetVoWCounterCtrl(pAd, 5, target);
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, 
                ("%s: target = %d\n", __FUNCTION__, target));
        }
        else
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


VOID vow_display_info_periodic(
    IN  PRTMP_ADAPTER pAd)
{


    if (pAd->vow_dvt_en)
    {
        CHAR i, ac, bw;
        UINT32 wtbl_offset;
        UINT32 tx_sum, tx, rx_sum, rx, tx_ok[2], tx_fail[2], tx_cnt, tx_ok_sum, tx_fail_sum;
        UINT32 cnt, free_cnt;

        vow_idx++;


        /* airtime */
        for (i = 0; i <= pAd->vow_monitor_sta; i++)
        {
            UINT32 clr_wtbl = 0x1000;

            wtbl_offset = (i << 8) | 0x3004C;
            tx_sum = rx_sum = 0;

            for (ac = 0; ac < 4; ac++)
            {
                RTMP_IO_READ32(pAd, wtbl_offset + (ac << 3), &tx);
                tx_sum += tx;
                RTMP_IO_READ32(pAd, wtbl_offset + (ac << 3) + 4, &rx);
                rx_sum += rx;
            }

            /* clear WTBL airtime statistic */
            RTMP_IO_WRITE32(pAd, 0x23430, clr_wtbl | i);

            if (i <= pAd->vow_show_sta)
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("sta%d: tx -> %u, rx -> %u, vow_idx %d\n", i, tx_sum, rx_sum, vow_idx));

            vow_tx_time[i] += tx_sum;
            vow_rx_time[i] += rx_sum;

            if (vow_idx == pAd->vow_avg_num)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
                    ("AVG sta%d: tx -> %u(%u), rx -> %u(%u)\n", i, 
                        vow_tx_time[i]/pAd->vow_avg_num, 
                        vow_tx_time[i], 
                        vow_rx_time[i]/pAd->vow_avg_num, 
                        vow_rx_time[i]));

                vow_tx_time[i] = 0;
                vow_rx_time[i] = 0;

            }
        }

        /* tx counter */
        for (i = 1; i <= pAd->vow_monitor_sta; i++)
        {
            UINT32 clr_wtbl = 0x4000;

            wtbl_offset = (i << 8) | 0x30040;
            tx_ok_sum = tx_fail_sum = 0;

            for (bw = 0; bw < 2; bw++)
            {
                RTMP_IO_READ32(pAd, wtbl_offset + (bw << 2), &tx_cnt);
                tx_ok_sum += (tx_cnt & 0xffff);
                tx_ok[bw] = (tx_cnt & 0xffff);
                tx_fail_sum += ((tx_cnt >> 16) & 0xffff);
                tx_fail[bw] = ((tx_cnt >> 16) & 0xffff);
            }

            /* clear WTBL airtime statistic */
            RTMP_IO_WRITE32(pAd, 0x23430, clr_wtbl | i);

            if (i <= pAd->vow_show_sta)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("sta%d: tx cnt -> %u/%u, tx fail -> %u/%u, vow_idx %d\n",
                                    i, tx_ok[0], tx_ok[1], tx_fail[0], tx_fail[1], vow_idx));
            }

            vow_tx_ok[i] += tx_ok_sum;
            vow_tx_fail[i] += tx_fail_sum;

            if (vow_idx == pAd->vow_avg_num)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
                    ("AVG sta%d: tx cnt -> %u(%u), tx fail -> %u(%u)\n", i, 
                        vow_tx_ok[i]/pAd->vow_avg_num, 
                        vow_tx_ok[i], 
                        vow_tx_fail[i]/pAd->vow_avg_num, 
                        vow_tx_fail[i]));

                vow_tx_ok[i] = 0;
                vow_tx_fail[i] = 0;

            }
        }

        /* throughput */
        for (i = 0; i <= pAd->vow_monitor_bss; i++)
        {
            UINT32 txb, rxb;

            RTMP_IO_READ32(pAd, 0x23110 + (i << 2), &txb);
            RTMP_IO_READ32(pAd, 0x23130 + (i << 2), &rxb);

            MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("BSS%d: tx byte -> %u, rx byte -> %u\n", i, txb, rxb));

            vow_tx_bss_byte[i] += txb;
            vow_rx_bss_byte[i] += rxb;


            if (vow_idx == pAd->vow_avg_num)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
                    ("AVG bss%d: tx -> %u(%u), rx -> %u(%u)\n", i, 
                        vow_tx_bss_byte[i]/pAd->vow_avg_num, 
                        vow_tx_bss_byte[i], 
                        vow_rx_bss_byte[i]/pAd->vow_avg_num, 
                        vow_rx_bss_byte[i]));

                vow_tx_bss_byte[i] = 0;
                vow_rx_bss_byte[i] = 0;
            }

        }

        for (i = 0; i <= pAd->vow_monitor_mbss; i++)
        {
            UINT32 txb, rxb;

            RTMP_IO_READ32(pAd, 0x23240 + (i << 2), &txb);
            RTMP_IO_READ32(pAd, 0x232C0 + (i << 2), &rxb);

            if (i < pAd->vow_show_mbss)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("MBSS%d: tx byte -> %u, rx byte -> %u\n", i, txb, rxb));
            }

            vow_tx_mbss_byte[i] += txb;
            vow_rx_mbss_byte[i] += rxb;

            if (vow_idx == pAd->vow_avg_num)
            {
                MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,
                    ("AVG mbss%d: tx -> %u(%u), rx -> %u(%u)\n", i, 
                        vow_tx_mbss_byte[i]/pAd->vow_avg_num, 
                        vow_tx_mbss_byte[i], 
                        vow_rx_mbss_byte[i]/pAd->vow_avg_num, 
                        vow_rx_mbss_byte[i]));

                vow_tx_mbss_byte[i] = 0;
                vow_rx_mbss_byte[i] = 0;
            }
        }

        //read LPON free run counter
        RTMP_IO_READ32(pAd, 0x2427c, &free_cnt);

        if (vow_last_free_cnt)
        {
            vow_interval += (free_cnt - vow_last_free_cnt);
            MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("free count %d\n", free_cnt - vow_last_free_cnt));
            vow_last_free_cnt = free_cnt;
        }

        vow_last_free_cnt = free_cnt;

        //read AMPDU count
        RTMP_IO_READ32(pAd, 0x24838, &cnt);
        vow_ampdu_cnt += cnt;

        MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("AMPU count %d\n", cnt));

        if (vow_idx == pAd->vow_avg_num)
        {
            MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("total ampdu cnt -> %u, avg ampdu cnt --> %d\n",
                                vow_ampdu_cnt, vow_ampdu_cnt/pAd->vow_avg_num));

            MTWF_LOG(DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_OFF,("total interval -> %u, avg interval --> %d\n",
                                vow_interval, vow_interval/pAd->vow_avg_num));

            vow_interval = 0;
            vow_idx = 0;
            vow_ampdu_cnt = 0;
        }


    }
}

#endif /* VOW_SUPPORT */
