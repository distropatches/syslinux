#ifndef _PCIDIRECT_H
#define _PCIDIRECT_H

#include <stdint.h>
#include <io.h>

/** @file
 *
 * PCI configuration space access via Type 1 accesses
 *
 */

#define PCIDIRECT_CONFIG_ADDRESS	0xcf8
#define PCIDIRECT_CONFIG_DATA		0xcfc

struct pci_device;

extern void pcidirect_prepare ( struct pci_device *pci, int where );

/**
 * Determine maximum PCI bus number within system
 *
 * @ret max_bus		Maximum bus number
 */
static inline int pcidirect_max_bus ( void ) {
	/* No way to work this out via Type 1 accesses */
	return 0xff;
}

/**
 * Read byte from PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_read_config_byte ( struct pci_device *pci, unsigned int where,
			     uint8_t *value ) {
	pcidirect_prepare ( pci, where );
	*value = inb ( PCIDIRECT_CONFIG_DATA + ( where & 3 ) );
	return 0;
}

/**
 * Read word from PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_read_config_word ( struct pci_device *pci, unsigned int where,
			     uint16_t *value ) {
	pcidirect_prepare ( pci, where );
	*value = inw ( PCIDIRECT_CONFIG_DATA + ( where & 2 ) );
	return 0;
}

/**
 * Read dword from PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_read_config_dword ( struct pci_device *pci, unsigned int where,
			      uint32_t *value ) {
	pcidirect_prepare ( pci, where );
	*value = inl ( PCIDIRECT_CONFIG_DATA );
	return 0;
}

/**
 * Write byte to PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_write_config_byte ( struct pci_device *pci, unsigned int where,
			      uint8_t value ) {
	pcidirect_prepare ( pci, where );
	outb ( value, PCIDIRECT_CONFIG_DATA + ( where & 3 ) );
	return 0;
}

/**
 * Write word to PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_write_config_word ( struct pci_device *pci, unsigned int where,
			      uint16_t value ) {
	pcidirect_prepare ( pci, where );
	outw ( value, PCIDIRECT_CONFIG_DATA + ( where & 2 ) );
	return 0;
}

/**
 * Write dword to PCI configuration space via Type 1 access
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __attribute__ (( always_inline )) int
pcidirect_write_config_dword ( struct pci_device *pci, unsigned int where,
			       uint32_t value ) {
	pcidirect_prepare ( pci, where );
	outl ( value, PCIDIRECT_CONFIG_DATA );
	return 0;
}

#endif /* _PCIDIRECT_H */