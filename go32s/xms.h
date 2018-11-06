/*
** xms.h -- C bindings for xms API
** Author: Kent Williams william@umaxc.weeg.uiowa.edu
**
** This is an attempt to provide a complete C binding
** for the XMS calls.
**
** All calls will store the error code in xms_error.  I have
** tried to document how the calls work in this file where their
** prototypes are given.
*/
#ifndef __XMS_H
#define __XMS_H

/*
** extended memory block types
*/
typedef unsigned short emb_size_K_t;
typedef unsigned long  emb_off_t;
typedef short emb_handle_t;

/*
** upper memory block types.
*/
typedef unsigned short umb_segment_t;
typedef unsigned short umb_size_t;

/*
** structure defining information returned
** from xms_query_extended_memory
*/
typedef struct _xms_extended_info {
	emb_size_K_t max_free_block;
	emb_size_K_t total_extended_memory;
} xms_extended_info;

/*
** structure defining information returned
** from xms_get_version_info
*/
typedef struct _xms_version_info {
	unsigned xms_ver;
	unsigned xmm_ver;
	unsigned hma_present;
} xms_version_info;

/*
** structure defining information returned from
** xms_get_emb_handle_info
*/
typedef struct _emb_handle_info {
	unsigned char lock_count;
	unsigned char handles_available;
	emb_size_K_t block_size;
} emb_handle_info;

/*
** move parameter block for extended memory moves.
*/
typedef struct _xms_move_param {
	emb_off_t length;
	emb_handle_t source_handle;
	emb_off_t source_offset;
	emb_handle_t dest_handle;
	emb_off_t dest_offset;
} xms_move_param;

/*
** upper memory block info blockblock
*/
typedef struct _umb_info {
	umb_segment_t segment;
	umb_size_t block_size;
} umb_info;

extern char xms_error;
/*
** xms error codes
*/
#define XMS_NOT_IMPLEMENTED		0x80
#define XMS_VDISK_DETECTED		0x81
#define XMS_A20_ERROR			0x82
#define XMS_DRIVER_ERROR		0x8e
#define XMS_FATAL_DRIVER_ERROR	0x8f
#define XMS_NO_HMA				0x90
#define XMS_HMA_INUSE			0x91
#define XMS_HMA_DX_TOO_SMALL	0x92
#define XMS_HMA_NOT_ALLOCATED	0x93
#define XMS_A20_STILL_ENABLED	0x94
#define XMS_EXT_NO_MEM			0xa0
#define XMS_EXT_NO_HANDLES		0xa1
#define XMS_EXT_BAD_HANDLE		0xa2
#define XMS_EXT_BAD_SRC_HANDLE	0xa3
#define XMS_EXT_BAD_SRC_OFFSET	0xa4
#define XMS_EXT_BAD_DST_HANDLE	0xa5
#define XMS_EXT_BAD_DST_OFFSET	0xa6
#define XMS_EXT_BAD_LENGTH		0xa7
#define XMS_EXT_OVERLAP			0xa8
#define XMS_EXT_PARITY			0xa9
#define XMS_EXT_NOT_LOCKED		0xaa
#define XMS_EXT_LOCKED			0xab
#define XMS_EXT_NO_LOCKS		0xac
#define XMS_EXT_LOCK_FAILED		0xad
#define XMS_UMB_SMALLER_AVAIL	0xb0
#define XMS_UMB_NO_MEM			0xb1
#define XMS_UMB_BAD_SEG			0xb2


/* xms function 00H 
** returns pointer to static info block
*/
xms_version_info * xms_get_version_info(void);

/* xms function 01H
** returns 0 on success, non-zero on failure
** if it succeeds, the address of the hma is 0xffff:0010H
** the size parameter is screwy; it is documented to be 0xffff for an
** application,or the actual size needed for a driver.
*/
int xms_hma_allocate(unsigned size);

/* xms function 02H
** returns 0 on success, non-zero on failure
*/
int xms_hma_free(void);

/*
** The following functions diddle the A20 address line.  A20 has to
** be enabled in order to address memory > 1Mbyte, i.e. the HMA
** You are supposed to use the global functions if you are 'the owner'
** of the HMA, the local functions if you are not. You own the HMA
** if your call to xms_hma_allocate succeeded.
** The distinction between global and local aren't really explained
** in the stuff I have;  my inclination would be to try to get the hma
** with xms_hma_allocate and then just use the global functions.
*/
/* xms function 03H */
int xms_global_enable_a20(void);
/* xms function 04H */
int xms_global_disable_a20(void);
/* xms function 05H */
int xms_local_enable_a20(void);
/* xms function 06H */
int xms_local_disable_a20(void);

/* xms function 07H
** returns 1 if the line is enabled, 0 if it isn't.
** actually (!xms_a20_status && !xms_error) would indicate disabled
** status; if the error is non-zero, then the call failed for some screwy
** reason
*/
int xms_a20_status(void);

/* xms function 08H
** this call returns a pointer to a static info area
*/
xms_extended_info *xms_query_extended_memory(void);

/* xms function 09H
** returns an emb handle, or -1 if allocation fails
*/
int xms_emb_allocate(emb_size_K_t size);

/* xms function 0AH
** returns 0 on success non-zero on failure
*/
int xms_emb_free(emb_handle_t handle);

/* xms function 0BH
** returns 0 on success non-zero on failure
*/
int xms_move_emb(xms_move_param *x);

/* xms function 0CH
** returns 32 bit linear address of emb specified by handle,
** 0 otherwise.
*/
emb_off_t xms_lock_emb(emb_handle_t handle);

/* xms function 0DH
** returns 0 on success non-zero on failure
*/
int xms_unlock_emb(emb_handle_t handle);

/* xms function 0EH
** returns poitner to static information buffer, or null on failure
*/
emb_handle_info *xms_get_emb_handle_info(emb_handle_t handle);

/* xms function 0FH
** returns 0 on success, non-zero on failure
*/
int xms_emb_resize(emb_handle_t handle,emb_size_K_t size);

/* xms function 10H 
** returns pointer to static umb info block, or NULL
** if call fails
*/
umb_info * xms_umb_allocate(umb_size_t size);

/* xms function 11H 
** returns 0 on success, non-zero on failure
*/
int xms_umb_free(umb_segment_t segment);

#endif /* XMS_H */
