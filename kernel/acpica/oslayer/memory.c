/* MollenOS
 *
 * Copyright 2011 - 2017, Philip Meulengracht
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
 * MollenOS MCore - ACPICA Support Layer (Memory Functions)
 *  - Missing implementations are todo
 */
#define __MODULE "ACPI"
#define __TRACE

/* Includes
 * - (OS) System */
#include <system/addresspace.h>
#include <system/utils.h>
#include <debug.h>
#include <heap.h>

/* Includes
 * - (ACPI) System */
#include <acpi.h>
#include <accommon.h>

/* Definitions
 * - Component Setup */
#define _COMPONENT ACPI_OS_SERVICES
ACPI_MODULE_NAME("oslayer_memory")

/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  Size                - Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation. Null on error.
 *
 * DESCRIPTION: Allocate memory. Algorithm is dependent on the OS.
 *
 *****************************************************************************/
void *
AcpiOsAllocate(
    ACPI_SIZE               Size)
{
    return kmalloc((size_t)Size);
}

/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  Mem                 - Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/
void
AcpiOsFree(
    void *                  Memory)
{
    kfree(Memory);
}

/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  Where               - Physical address of memory to be mapped
 *              Length              - How much memory to map
 *
 * RETURN:      Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/
#if defined(i386)
#include "../../arch/x86/memory.h"
#endif
void *
AcpiOsMapMemory(
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
    // Variables
	uintptr_t Lookup = MmPhyiscalGetSysMappingVirtual((PhysicalAddress_t)Where);

	// There is a few special cases where the mapping doesn't exist, so we must create it 
	if (Lookup == 0) {
        int PageCount = DIVUP(Length, PAGE_SIZE);
        if (PageCount == 1) {
            ACPI_PHYSICAL_ADDRESS PageStart = Where & PAGE_MASK;
            ACPI_PHYSICAL_ADDRESS PageEnd = (Where + Length) & PAGE_MASK;
            if (PageStart != PageEnd) {
                PageCount++;
            }
        }

        // If we are looking for something below the first 4 mb
        // It's already identity mapped
		if (Where >= 0x1000 && Where < TABLE_SPACE_SIZE) {
            Lookup = (uintptr_t)Where;
        }
		else {
			uintptr_t ReservedMem = (uintptr_t)MmReserveMemory(PageCount);
            int i = 0;
            
			for (; i < PageCount; i++) {
				if (!MmVirtualGetMapping(NULL, ReservedMem + (i * PAGE_SIZE)))
					MmVirtualMap(NULL, (Where & PAGE_MASK) + (i * PAGE_SIZE), ReservedMem + (i * PAGE_SIZE), 0);
			}
			return (void*)(ReservedMem + ((uintptr_t)Where & ATTRIBUTE_MASK));
		}
	}
	return (void*)Lookup;
}

/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  Where               - Logical address of memory to be unmapped
 *              Length              - How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping. Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/
void
AcpiOsUnmapMemory(
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
    // Don't unmap anything yet
    // @todo
}

/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetPhysicalAddress
 *
 * PARAMETERS:  LogicalAddress      - Logical address to lookup
 *              PhysicalAddress     - Where to store the result
 *
 * RETURN:      Status Code.
 *
 * DESCRIPTION: Retrieve the physical address of a logical address. Replaces the value.
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsGetPhysicalAddress(
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress)
{
    PhysicalAddress_t Result = AddressSpaceGetMap(
        AddressSpaceGetCurrent(), (VirtualAddress_t)LogicalAddress);
    if (Result != 0) {
        *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)Result;
        return AE_OK;
    }
    return AE_ERROR;
}

/******************************************************************************
 *
 * FUNCTION:    Local cache interfaces
 *
 * DESCRIPTION: Implements cache interfaces via malloc/free for testing
 *              purposes only.
 *
 *****************************************************************************/
#ifndef ACPI_USE_LOCAL_CACHE

ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
    ACPI_MEMORY_LIST        *NewCache;


    NewCache = kmalloc (sizeof (ACPI_MEMORY_LIST));
    if (!NewCache)
    {
        return (AE_NO_MEMORY);
    }

    memset (NewCache, 0, sizeof (ACPI_MEMORY_LIST));
    NewCache->ListName = CacheName;
    NewCache->ObjectSize = ObjectSize;
    NewCache->MaxDepth = MaxDepth;

    *ReturnCache = (ACPI_CACHE_T) NewCache;
    return (AE_OK);
}

ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache)
{
    kfree (Cache);
    return (AE_OK);
}

ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{
    return (AE_OK);
}

void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{
    void *NewObject;

    NewObject = kmalloc (((ACPI_MEMORY_LIST *) Cache)->ObjectSize);
    memset (NewObject, 0, ((ACPI_MEMORY_LIST *) Cache)->ObjectSize);

    return (NewObject);
}

ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
    kfree (Object);
    return (AE_OK);
}

#endif /* ACPI_USE_LOCAL_CACHE */
