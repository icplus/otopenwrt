/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ***************************************************************************
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "ralink_gdma.h"
#include <linux/time.h>
#include <linux/kthread.h>

#define PATTERN_LEN      65000
#define PATTERN_BOUNDARY 65600 //for unalignment memcpy test
#define SHOW_INTERVAL	 500

/*
 * Notes: 
 * If you want to test GDMA, please use "DEBUG mode in channel allocation mode"
 * to use different channel for each run. 
 */

/* 
 * CONCURRENT: All channels run at the same time (forever) 
 * CHAIN: * use channel 0~MAX_GDMA_CHANNEL->....->0~MAX_GDMA_CHANNEL periodically
 *	    please modify GdmaMem2Mem()
 *	    {....  
 *		Entry.ChMask=1;
 *		...
 *	    } 
 *
 *	    Entry.NextUnMaskCh= (Entry.ChNum+1) % MAX_GDMA_CHANNEL;
 *
 * POLL: GDMA polling mode, start another gdma transaction when current gdma done.
 * FULL_TEST: Execute all test combination (48 pairs) using polling mode
 *
 */
//#define CONCURRENT	    1
//#define CHAIN		    1
#define POLL		    1
//#define GDMA_FULL_TEST    1


unsigned char *Src[MAX_GDMA_CHANNEL];
unsigned char *Dst[MAX_GDMA_CHANNEL];

dma_addr_t Src_phy[MAX_GDMA_CHANNEL];
dma_addr_t Dst_phy[MAX_GDMA_CHANNEL];

unsigned char DoneBit[MAX_GDMA_CHANNEL];
unsigned int Success=0;
unsigned int Fail=0;
static unsigned long last_timestamp;
static unsigned long fail_timestamp;
void check_result(unsigned char *src, unsigned char *dst, uint32_t len)
{
	printk("Src=%p Dst=%p Len=%x\n", src, dst, len);

	if(memcmp(src, dst, len)==0){
		printk("check .. ok\n");
		Success++;
	}else{
		printk("check .. fail\n");
		Fail++;
	}
}
void Done(uint32_t Ch)
{
	int i=0;
	static int count[MAX_GDMA_CHANNEL];
	
	if((++count[Ch] % SHOW_INTERVAL)==0) {
	    printk("Ch=%d is still alive\n", Ch);
	}

	for(i=0;i<PATTERN_LEN;i++) {
		if(Dst[Ch][i]!=Src[Ch][i]) {
			printk("***********<<WARNNING!!!>>*********\n");
			printk("Ch=%d Check fail (Dst=%x Src=%x)\n",Ch, Dst[Ch][i], Src[Ch][i]);
			printk("***********************************\n");
			return;
		}
	}
	
	DoneBit[Ch]=1;

	//clear content of destination address
	memset(Dst[Ch], 0, PATTERN_LEN);
	
#if defined (CHAIN)
	//set up related fields for next transaction
	GdmaMem2Mem(Src_phy[Ch], Dst_phy[Ch], PATTERN_LEN, Done);

	/*If channel is MAX_GDMA_CHANNEL-1, 
         *it means channel 0 - channal (MAX_GDMA_CHANNEL-1) is ready 
	 *just kill off channel 0 to start next chain-based transaction.
         */
	if(Ch == (MAX_GDMA_CHANNEL-1)) {
		GdmaUnMaskChannel(0);
	}
#elif defined (CONCURRENT)
	//set up related fields for next transaction
	GdmaMem2Mem(Src_phy[Ch], Dst_phy[Ch], PATTERN_LEN, Done);
#endif


}

int DonePoll(uint32_t Ch,int len)
{
	int i=0;
	static int count[MAX_GDMA_CHANNEL];
	
	struct timeval now; 
  //suseconds_t diff; 

	if((++count[Ch] % SHOW_INTERVAL)==0) {
	   printk("Ch=%d is still alive\n", Ch);
	   do_gettimeofday(&now); 
  	 printk("Current UTC: %lu (%lu)\n", now.tv_sec, now.tv_usec); 
	}

	for(i=0;i<len;i++) {
		if(Dst[Ch][i]!=Src[Ch][i]) {
			printk("***********<<WARNNING!!!>>*********\n");
			printk("Ch=%d len=%d Check fail (Dst=%x Src=%x)\n",Ch, len, Dst[Ch][i], Src[Ch][i]);
			printk("***********************************\n");
			fail_timestamp=jiffies;
			printk("GDMA spend =%lu jiffies\n",fail_timestamp - last_timestamp);
			printk("Src[%d]=%p, Dst[%d]=%p\n",Ch , &Src[Ch][0],Ch, &Dst[Ch][0] );
			return 1;
		}
	}
	//clear content of destination address
	memset(Dst[Ch], 0, PATTERN_BOUNDARY);
	return 0;
}
int poll_mode(void *unused) 
{
  int result;
  int i;
  int noInterrupt=0;
  last_timestamp=jiffies;
	while(1) {
		for (i=1;i<65535;i++){
			//printk("len=%d\n", i);
			noInterrupt = GdmaMem2Mem(Src_phy[0], Dst_phy[0], i, NULL);
			result=DonePoll(0, i);
			noInterrupt = GdmaMem2Mem(Src_phy[1], Dst_phy[1], i, NULL);
			result=DonePoll(1, i);
			noInterrupt = GdmaMem2Mem(Src_phy[2], Dst_phy[2], i, NULL);

			result=DonePoll(2, i);
			noInterrupt = GdmaMem2Mem(Src_phy[3], Dst_phy[3], i, NULL);
			result=DonePoll(3, i);
			noInterrupt = GdmaMem2Mem(Src_phy[4], Dst_phy[4], i, NULL);
			result=DonePoll(4, i);
			noInterrupt = GdmaMem2Mem(Src_phy[5], Dst_phy[5], i, NULL);
			result=DonePoll(5, i);
			noInterrupt = GdmaMem2Mem(Src_phy[6], Dst_phy[6], i, NULL);
			result=DonePoll(6, i);
			noInterrupt = GdmaMem2Mem(Src_phy[7], Dst_phy[7], i, NULL);
			result=DonePoll(7, i);
#if defined(CONFIG_RALINK_RT3883)  || defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_RT6855) || defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7620) || defined (CONFIG_RALINK_MT7621)
			noInterrupt = GdmaMem2Mem(Src_phy[8], Dst_phy[8], i, NULL);
			result=DonePoll(8, i);
			noInterrupt = GdmaMem2Mem(Src_phy[9], Dst_phy[9], i, NULL);
			result=DonePoll(9, i);
			noInterrupt = GdmaMem2Mem(Src_phy[10], Dst_phy[10], i, NULL);
			result=DonePoll(10, i);
			noInterrupt = GdmaMem2Mem(Src_phy[11], Dst_phy[11], i, NULL);
			result=DonePoll(11, i);
			noInterrupt = GdmaMem2Mem(Src_phy[12], Dst_phy[12], i, NULL);
			result=DonePoll(12, i);
			noInterrupt = GdmaMem2Mem(Src_phy[13], Dst_phy[13], i, NULL);
			result=DonePoll(13, i);
			noInterrupt = GdmaMem2Mem(Src_phy[14], Dst_phy[14], i, NULL);
			result=DonePoll(14, i);
			noInterrupt = GdmaMem2Mem(Src_phy[15], Dst_phy[15], i, NULL);
			result=DonePoll(15, i);
			if (noInterrupt < 0 ){
				printk("**************************************************\n");
				printk("GDMA status is wrong \n");
				printk("**************************************************\n");
				return -1;
			}
			msleep(10);
#endif
		}	
		if (result==1) return 1;
	}

	return 1;
}

int concurrent_mode(void *unused) 
{
	GdmaMem2Mem(Src_phy[0], Dst_phy[0], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[1], Dst_phy[1], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[2], Dst_phy[2], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[3], Dst_phy[3], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[4], Dst_phy[4], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[5], Dst_phy[5], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[6], Dst_phy[6], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[7], Dst_phy[7], PATTERN_LEN, Done);
#if defined(CONFIG_RALINK_RT3883) || defined (CONFIG_RALINK_RT3352) || defined (CONFIG_RALINK_RT5350) || defined (CONFIG_RALINK_RT6855) || defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7620) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_RALINK_MT7628)
	GdmaMem2Mem(Src_phy[8], Dst_phy[8], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[9], Dst_phy[9], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[10], Dst_phy[10], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[11], Dst_phy[11], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[12], Dst_phy[12], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[13], Dst_phy[13], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[14], Dst_phy[14], PATTERN_LEN, Done);
	GdmaMem2Mem(Src_phy[15], Dst_phy[15], PATTERN_LEN, Done);
#endif	

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	while(1) {
		mdelay(1000);
	}
#endif	
	return 1;
}

int full_test_mode(void *unused) 
{
	int i,j;

	//unaligned memory test
	for(i=0;i<4;i++) {
	    for(j=0;j<4;j++) {

		printk("\n======src base=%d to dst base=%d=====\n",i,j);
		GdmaMem2Mem(Src_phy[0]+i, Dst_phy[0]+j, PATTERN_LEN, NULL);
		check_result(&Src[0][i], &Dst[0][j], PATTERN_LEN);

		GdmaMem2Mem(Src_phy[0]+i, Dst_phy[0]+j, PATTERN_LEN+1, NULL);
		check_result(&Src[0][i], &Dst[0][j], PATTERN_LEN+1);

		GdmaMem2Mem(Src_phy[0]+i, Dst_phy[0]+j, PATTERN_LEN+2, NULL);
		check_result(&Src[0][i], &Dst[0][j], PATTERN_LEN+2);

		GdmaMem2Mem(Src_phy[0]+i, Dst_phy[0]+j, PATTERN_LEN+3, NULL);
		check_result(&Src[0][i], &Dst[0][j], PATTERN_LEN+3);

	    }
	}

	printk("========<Result>===========\n");
	printk("Success: %d\n",Success);
	printk("Fail: %d\n",Fail);
	printk("===========================\n");
	return 1;
}

int chain_mode(void *unused) {

	int i=0;
	for(i=0;i< MAX_GDMA_CHANNEL; i++) {
		GdmaMem2Mem(Src_phy[i], Dst_phy[i], PATTERN_LEN, Done);
	}

	GdmaUnMaskChannel(0); //kick of channel 0

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
	while(1) {
		mdelay(1000);
	}
#endif
	return 1;

}

static int RalinkGdmaTestInit(void)
{
	int i=0,j=0;

	printk("Enable Ralink GDMA Test Module\n");		
	//alloc phy memory for src
	for(i=0;i< MAX_GDMA_CHANNEL; i++) {
	    Src[i] = pci_alloc_consistent(NULL, PATTERN_BOUNDARY, &Src_phy[i]);
	    if(Src[i]==NULL) {
		printk("pci_alloc_consistent fail: Src[%d]==NULL\n", i);
		return 1;
	    }
	}

	//fill out content
	for(i=0;i< MAX_GDMA_CHANNEL; i++) {
	    for(j=0;j< PATTERN_BOUNDARY;j++) {
		Src[i][j]=i+j;
	    }
	}
	//alloc phy memory for dst
	for(i=0;i< MAX_GDMA_CHANNEL; i++) {
	    Dst[i] = pci_alloc_consistent(NULL, PATTERN_BOUNDARY, &Dst_phy[i]);

	    if(Dst[i]==NULL) {
		printk("pci_alloc_consistent fail: Dst[%d]==NULL\n", i);
		return 1;
	    }
	}

#if defined (CONCURRENT)
	printk("All channels run at the same time \n");
	kernel_thread(concurrent_mode, NULL, CLONE_VM);
#elif defined (GDMA_FULL_TEST)
	printk("Execute all test combination (48 pairs) \n");
	kernel_thread(full_test_mode, NULL, CLONE_VM);
#elif defined (POLL)
	printk("GDMA polling mode\n");
	kernel_thread(poll_mode, NULL, CLONE_VM);
	//kthread_create(poll_mode, NULL, "poll_mode"); 
 
	//wake_up_process(kmain_task); 

#elif defined (CHAIN)
        printk("GDMA chain mode (Channel 0~MAX_CHANNEL...->0~MAX_CHANNEL periodically\n");
	kernel_thread(chain_mode, NULL, CLONE_VM);
#endif
	
	return 0;
}

static void __exit RalinkGdmaTestExit(void)
{

	int i=0;

	printk("Disable Ralink GDMA Test Module\n");

	for(i=0;i<MAX_GDMA_CHANNEL;i++) {
	    pci_free_consistent(NULL, PATTERN_BOUNDARY, Src[i], Src_phy[i]);
	    pci_free_consistent(NULL, PATTERN_BOUNDARY, Dst[i], Dst_phy[i]);
	}

}


module_init(RalinkGdmaTestInit);
module_exit(RalinkGdmaTestExit);

MODULE_DESCRIPTION("Ralink SoC DMA Test Module");
MODULE_AUTHOR("Steven Liu <steven_liu@ralinktech.com.tw>");
MODULE_LICENSE("GPL");
