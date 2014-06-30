/* MollenOS
*
* Copyright 2011 - 2014, Philip Meulengracht
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation ? , either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.If not, see <http://www.gnu.org/licenses/>.
*
*
* MollenOS X86-32 USB OHCI Controller Driver
*/

#ifndef _X86_USB_OHCI_H_
#define _X86_USB_OHCI_H_

/* Includes */
#include <crtdefs.h>
#include <stdint.h>
#include <pci.h>

/* Definitions */

/* Structures */

/* Must be 16 byte aligned */
typedef struct _o_endpoint_desc
{
	/* Flags 
	 * Bits 0 - 6: Usb Address of Function
	 * Bits 7 - 10: (Endpoint Nr) Usb Address of this endpoint
	 * Bits 11 - 12: (Direction) Indicates Dataflow, either IN or OUT. (01 -> OUT, 10 -> IN). Otherwise we leave it to the TD descriptor
	 * Bits 13: Speed. 0 indicates full speed, 1 indicates low-speed.
	 * Bits 14: If set, it skips the TD queues and moves on to next EP descriptor
	 * Bits 15: (Format). 0 = Bulk/Control/Interrupt Endpoint. 1 = Isochronous TD format.
	 * Bits 16-26: Maximum Packet Size per data packet.
	 * Bits 27-31: Not used.
	 */
	uint32_t flags;

	/* TD Queue Tail Pointer
	 * Lower 4 bits not used 
	 * If tail == head, no TD will be used 
	 * We queue to the TAIL, always! */
	uint32_t tail_ptr;

	/* TD Queue Head Pointer 
	 * Bits 0: Halted. Indicates that processing of a TD has failed. 
	 * Bits 1: Carry Bit: When a TD is retired this bit is set to last data toggle value.
	 * Bits 2:3 Must be 0. */
	uint32_t head_ptr;

	/* Next EP Descriptor
	 * Lower 4 bits not used */
	uint32_t next_ed;

} ohci_endpoint_desc_t;

/* Must be 16 byte aligned 
 * General Transfer Descriptor */
typedef struct _o_gtransfer_desc
{
	/* Flags
	 * Bits 0-17:  DONT TOUCH.
	 * Bits 18:    If 0, Requires the data to be recieved from an endpoint to exactly fill buffer
	 * Bits 19-20: Direction, 00 = Setup (to ep), 01 = OUT (to ep), 10 = IN (from ep)
	 * Bits 21-23: Interrupt delay count for this TD. This means the HC can delay interrupt a specific amount of frames after TD completion.
	 * Bits 24-25: Data Toggle. It is updated after each successful transmission.
	 * Bits 26-27: Error Count. Updated each transmission that fails. It is 0 on success.
	 * Bits 28-31: Condition Code, if error count is 2 and it fails a third time, this contains error code.
	 * */
	uint32_t flags;

	/* Current Buffer Pointer 
	 * Size must always be lower than the Maximum Packet Size at the endpoint */
	uint32_t cbp;

	/* Next TD 
	 * Lower 4 bits must be 0 (aka 16 byte aligned) */
	uint32_t next_td;

	/* Buffer End 
	 * This is the next 4K page address that can be 
	 * accessed in case of a page boundary crossing 
	 * while using cbp */
	uint32_t buffer_end;

} ohci_gtransfer_desc;

/* Must be 32 byte aligned
 * Isochronous Transfer Descriptor */
typedef struct _o_itransfer_desc
{
	/* Flags
	 * Bits 0-15:	Starting Frame of transaction.
	 * Bits 16-20:	Available
	 * Bits 21-23:	Interrupt Delay
	 * Bits 24-26:	Frame Count in this TD, 0 implies 1, and 7 implies 8. (always +1)
	 * Bits 27:		Reserved
	 * Bits 28-31:	Condition Code (Error Code)
	 * */
	uint32_t flags;

	/* Buffer Page 0 
	 * shall point to first byte of data buffer */
	uint32_t bp0;

	/* Next TD
	* Lower 4 bits must be 0 (aka 16 byte aligned) */
	uint32_t next_td;

	/* Buffer End
	* This is the next 4K page address that can be
	* accessed in case of a page boundary crossing
	* while using cbp */
	uint32_t buffer_end;

	/* Offsets 
	 * Bits 0-10:	Packet Size on IN-transmissions 
	 * Bits 11:		0 Field
	 * Bits 12-15:	Condition Code (Error Code) */
	uint16_t offsets[8];

} ohci_itransfer_desc;

/* Host Controller Communcations Area 
 * must be 256-byte aligned */
typedef struct _o_hcca
{
	/* Interrupt Table 
	 * 32 pointers to interrupt ed's */
	uint32_t interrupt_table[32];

	/* Current Frame Number */
	uint16_t current_frame;

	/* It is 0 */
	uint16_t pad1;

	/* Which head is done, it gets set to this pointer at end of frame
	 * and generates an interrupt */
	uint32_t head_done;

	/* Reserved for HC */
	uint32_t reserved[29];

} ohci_hcca_t;

/* Ohci Interrupt Table */
typedef struct _o_interrupt_table
{
	/* Binary Tree */
	ohci_endpoint_desc_t	ms16[16];
	ohci_endpoint_desc_t	ms8[8];
	ohci_endpoint_desc_t	ms4[4];
	ohci_endpoint_desc_t	ms2[2];
	ohci_endpoint_desc_t	ms1[1];
	ohci_endpoint_desc_t	stop_ed;

} ohci_int_table_t;

/* Register Space */
typedef struct _o_registers
{
	uint32_t	HcRevision;
	uint32_t	HcControl;
	uint32_t	HcCommandStatus;
	uint32_t	HcInterruptStatus;
	uint32_t	HcInterruptEnable;
	uint32_t	HcInterruptDisable;

	uint32_t	HcHCCA;
	uint32_t	HcPeriodCurrentED;
	uint32_t	HcControlHeadED;
	uint32_t	HcControlCurrentED;
	uint32_t	HcBulkHeadED;
	uint32_t	HcBulkCurrentED;
	uint32_t	HcDoneHead;

	uint32_t	HcFmInterval;
	uint32_t	HcFmRemaining;
	uint32_t	HcFmNumber;
	uint32_t	HcPeriodicStart;
	uint32_t	HcLSThreshold;

	/* Max of 15 ports */
	uint32_t	HcRhDescriptorA;
	uint32_t	HcRhDescriptorB;
	uint32_t	HcRhStatus;
	uint32_t	HcRhPortStatus[15];

} ohci_registers_t;

/* Controller Structure */
typedef struct _o_controller
{
	/* Id */
	uint32_t id;

	/* Irq Num */
	uint32_t irq;

	/* Pci Header */
	pci_driver_t *pci_info;

	/* Register Space (Physical) */
	uint32_t control_space;
	uint32_t hcca_space;

	/* Registers */
	volatile ohci_registers_t *registers;
	volatile ohci_hcca_t *hcca;

	/* Interrupt Table */
	ohci_int_table_t *int_table;

} ohci_controller_t;

/* Prototypes */
_CRT_EXTERN void ohci_init(pci_driver_t *device);

#endif // !_X86_USB_OHCI_H_