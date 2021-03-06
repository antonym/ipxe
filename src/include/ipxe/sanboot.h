#ifndef _IPXE_SANBOOT_H
#define _IPXE_SANBOOT_H

/** @file
 *
 * iPXE sanboot API
 *
 * The sanboot API provides methods for hooking, unhooking,
 * describing, and booting from SAN devices.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/api.h>
#include <ipxe/refcnt.h>
#include <ipxe/list.h>
#include <ipxe/uri.h>
#include <ipxe/retry.h>
#include <ipxe/blockdev.h>
#include <config/sanboot.h>

/** A SAN device */
struct san_device {
	/** Reference count */
	struct refcnt refcnt;
	/** List of SAN devices */
	struct list_head list;

	/** SAN device URI */
	struct uri *uri;
	/** Drive number */
	unsigned int drive;

	/** Underlying block device interface */
	struct interface block;
	/** Current device status */
	int block_rc;

	/** Command interface */
	struct interface command;
	/** Command timeout timer */
	struct retry_timer timer;
	/** Command status */
	int command_rc;

	/** Raw block device capacity */
	struct block_device_capacity capacity;
	/** Block size shift
	 *
	 * To allow for emulation of CD-ROM access, this represents
	 * the left-shift required to translate from exposed logical
	 * I/O blocks to underlying blocks.
	 */
	unsigned int blksize_shift;
	/** Drive is a CD-ROM */
	int is_cdrom;

	/** Driver private data */
	void *priv;
};

/**
 * Calculate static inline sanboot API function name
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 * @ret _subsys_func	Subsystem API function
 */
#define SANBOOT_INLINE( _subsys, _api_func ) \
	SINGLE_API_INLINE ( SANBOOT_PREFIX_ ## _subsys, _api_func )

/**
 * Provide a sanboot API implementation
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 * @v _func		Implementing function
 */
#define PROVIDE_SANBOOT( _subsys, _api_func, _func ) \
	PROVIDE_SINGLE_API ( SANBOOT_PREFIX_ ## _subsys, _api_func, _func )

/**
 * Provide a static inline sanboot API implementation
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 */
#define PROVIDE_SANBOOT_INLINE( _subsys, _api_func ) \
	PROVIDE_SINGLE_API_INLINE ( SANBOOT_PREFIX_ ## _subsys, _api_func )

/* Include all architecture-independent sanboot API headers */
#include <ipxe/null_sanboot.h>
#include <ipxe/efi/efi_block.h>

/* Include all architecture-dependent sanboot API headers */
#include <bits/sanboot.h>

/**
 * Hook SAN device
 *
 * @v uri		URI
 * @v drive		Drive number
 * @ret drive		Drive number, or negative error
 */
int san_hook ( struct uri *uri, unsigned int drive );

/**
 * Unhook SAN device
 *
 * @v drive		Drive number
 */
void san_unhook ( unsigned int drive );

/**
 * Attempt to boot from a SAN device
 *
 * @v drive		Drive number
 * @ret rc		Return status code
 */
int san_boot ( unsigned int drive );

/**
 * Describe SAN device for SAN-booted operating system
 *
 * @v drive		Drive number
 * @ret rc		Return status code
 */
int san_describe ( unsigned int drive );

extern struct list_head san_devices;

/** Iterate over all SAN devices */
#define for_each_sandev( sandev ) \
	list_for_each_entry ( (sandev), &san_devices, list )

/** There exist some SAN devices
 *
 * @ret existence	Existence of SAN devices
 */
static inline int have_sandevs ( void ) {
	return ( ! list_empty ( &san_devices ) );
}

/**
 * Get reference to SAN device
 *
 * @v sandev		SAN device
 * @ret sandev		SAN device
 */
static inline __attribute__ (( always_inline )) struct san_device *
sandev_get ( struct san_device *sandev ) {
	ref_get ( &sandev->refcnt );
	return sandev;
}

/**
 * Drop reference to SAN device
 *
 * @v sandev		SAN device
 */
static inline __attribute__ (( always_inline )) void
sandev_put ( struct san_device *sandev ) {
	ref_put ( &sandev->refcnt );
}

/**
 * Calculate SAN device block size
 *
 * @v sandev		SAN device
 * @ret blksize		Sector size
 */
static inline size_t sandev_blksize ( struct san_device *sandev ) {
	return ( sandev->capacity.blksize << sandev->blksize_shift );
}

/**
 * Calculate SAN device capacity
 *
 * @v sandev		SAN device
 * @ret blocks		Number of blocks
 */
static inline uint64_t sandev_capacity ( struct san_device *sandev ) {
	return ( sandev->capacity.blocks >> sandev->blksize_shift );
}

/**
 * Check if SAN device needs to be reopened
 *
 * @v sandev		SAN device
 * @ret needs_reopen	SAN device needs to be reopened
 */
static inline int sandev_needs_reopen ( struct san_device *sandev ) {
	return ( sandev->block_rc != 0 );
}

extern struct san_device * sandev_find ( unsigned int drive );
extern int sandev_reopen ( struct san_device *sandev );
extern int sandev_reset ( struct san_device *sandev );
extern int sandev_rw ( struct san_device *sandev, uint64_t lba,
		       unsigned int count, userptr_t buffer,
		       int ( * block_rw ) ( struct interface *control,
					    struct interface *data,
					    uint64_t lba, unsigned int count,
					    userptr_t buffer, size_t len ) );
extern struct san_device * alloc_sandev ( struct uri *uri, size_t priv_size );
extern int register_sandev ( struct san_device *sandev );
extern void unregister_sandev ( struct san_device *sandev );
extern unsigned int san_default_drive ( void );

#endif /* _IPXE_SANBOOT_H */
