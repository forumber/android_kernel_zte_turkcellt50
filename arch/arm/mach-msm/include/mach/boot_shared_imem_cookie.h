#ifndef BOOT_SHARED_IMEM_COOKIE_H
#define BOOT_SHARED_IMEM_COOKIE_H

/*===========================================================================

                                Boot Shared Imem Cookie
                                Header File

GENERAL DESCRIPTION
  This header file contains declarations and definitions for Boot's shared 
  imem cookies. Those cookies are shared between boot and other subsystems 

Copyright  2013 by Qualcomm Technologies, Incorporated.  All Rights Reserved.
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/core/pkg/bootloaders/rel/2.1/boot_images/core/boot/secboot3/src/boot_shared_imem_cookie.h#4 $
  $DateTime: 2013/10/22 05:28:46 $ 
  $Author: coresvc $

when       who          what, where, why
--------   --------     ------------------------------------------------------
10/21/13   aus          Add ABNORMAL_RESET_ENABLED
03/19/13   dh           Add UEFI_RAW_RAM_DUMP_MAGIC_NUM,
                        Allocate abnormal_reset_occurred cookie for UEFI 
02/28/13   kedara       Added feature flag FEATURE_TPM_HASH_POPULATE
02/21/13   dh           Move SHARED_IMEM_TPM_HASH_REGION_OFFSET to boot_shared_imem_cookie.h
02/15/13   dh           Add tz_dload_dump_shared_imem_cookie_type
11/27/12   dh           Add rpm_sync_cookie
07/24/12   dh           Add ddr_training_cookie
06/07/11   dh           Initial creation
============================================================================*/

/*=============================================================================

                            INCLUDE FILES FOR MODULE

=============================================================================*/
#include <linux/types.h>
#include <linux/compiler-gcc.h>
//#include "comdef.h"


/*=============================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

=============================================================================*/

/* 
 * Merged from boot_images/core/boot/secboot3/common/boot_shared_imem_cookie.h
 * by ZTE_BOOT_JIA_20120827, jia.jia
 */
#define SCL_IMEM_BASE 0xFE800000
#define SHARED_IMEM_BASE (SCL_IMEM_BASE + 0x0005000)
#define SHARED_IMEM_SIZE 0x0001000
#define SHARED_IMEM_BOOT_BASE SHARED_IMEM_BASE

/* 
 * Following magic number indicates the boot shared imem region is initialized
 * and the content is valid
 */
#define BOOT_SHARED_IMEM_MAGIC_NUM        0xC1F8DB40

/* 
 * Magic number for UEFI crash dump
 */
#define UEFI_CRASH_DUMP_MAGIC_NUM         0x1

/* 
 * Abnormal reset occured 
 */
#define ABNORMAL_RESET_ENABLED         0x1

/* 
 * Magic number for raw ram dump
 */
#define BOOT_RAW_RAM_DUMP_MAGIC_NUM       0x2

/* Default value used to initialize shared imem region */
#define SHARED_IMEM_REGION_DEF_VAL  0xFFFFFFFF 

/* 
 * Magic number for RPM sync
 */
#define BOOT_RPM_SYNC_MAGIC_NUM  0x112C3B1C 

  /* Magic number which indicates tz dload dump shared imem has been initialized
     and the content is valid.*/
#define TZ_DLOAD_DUMP_SHARED_IMEM_MAGIC_NUM  0x49445a54

  /* TZ dload dump cookie's offset from shared imem base */
#define TZ_DLOAD_DUMP_SHARED_IMEM_OFFSET   0x748

#ifdef FEATURE_TPM_HASH_POPULATE
/* offset in shared imem to store image tp hash */
#define SHARED_IMEM_TPM_HASH_REGION_OFFSET 0x834
#define SHARED_IMEM_TPM_HASH_REGION_SIZE   256

#endif /* FEATURE_TPM_HASH_POPULATE */

/*zte_modify BEGIN*/
typedef struct {
  uint32_t magic_1;
  uint32_t magic_2;
  uint32_t opt;
  uint32_t status;
  }__packed ftm_fusion_info;
/*zte_modify END*/

/* 
 * Following structure defines all the cookies that have been placed 
 * in boot's shared imem space.
 * The size of this struct must NOT exceed SHARED_IMEM_BOOT_SIZE
 */
struct boot_shared_imem_cookie_type
{
  /* First 8 bytes are two dload magic numbers */
  uint32_t dload_magic_1;
  uint32_t dload_magic_2;
  
  /* Magic number which indicates boot shared imem has been initialized
     and the content is valid.*/ 
  uint32_t shared_imem_magic;
  
  /* Magic number for UEFI ram dump, if this cookie is set along with dload magic numbers,
     we don't enter dload mode but continue to boot. This cookie should only be set by UEFI*/
  uint32_t uefi_ram_dump_magic;
  
  /* Pointer that points to etb ram dump buffer, should only be set by HLOS */
  uint32_t etb_buf_addr;
  
  /* Region where HLOS will write the l2 cache dump buffer start address */
  uint32_t *l2_cache_dump_buff_ptr;
  
  uint32_t ddr_training_cookie;
  
  /* Cookie that will be used to sync with RPM */
  uint32_t rpm_sync_cookie;
  
  /* Abnormal reset cookie used by UEFI */
  uint32_t abnormal_reset_occurred;
  
  /* Please add new cookie here, do NOT modify or rearrange the existing cookies*/
  uint32_t modemsleeptime;//shijunhan  add
  uint32_t modemawaketime;//shijunhan add to store how long modem keep awake
  uint32_t modemsleep_or_awake;//shijunhan add to indicate modem is sleep or awake,0 sleep, 1 awake,4 means never enter sleep yet

/*zte_modify_20140121 BEGIN*/
  ftm_fusion_info fusion_info;
  uint32_t efs_recovery_flag;

/*zte_modify_20140121 END*/

};

/* 
 * Following structure defines the memory dump cookies tz will place
 * in tz shared imem space.
 */
struct tz_dload_dump_shared_imem_cookie_type
{
  /* Magic number which indicates tz dload dump shared imem has been initialized
     and the content is valid.*/
  uint32_t magic_num;
  
  /* Source address that we should copy from */
  uint32_t *src_addr;
  
  /* Destination address that we should dump to */
  uint32_t *dst_addr;
  
  /* Total size we should copy */
  uint32_t size;


};

/*  Global pointer points to the boot shared imem cookie structure  */
extern struct boot_shared_imem_cookie_type *boot_shared_imem_cookie_ptr;


#endif /* BOOT_SHARED_IMEM_COOKIE_H */

