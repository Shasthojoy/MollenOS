/* MollenOS
 *
 * Copyright 2011 - 2016, Philip Meulengracht
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
 * MollenOS - Server System
 * We have two kinds of servers, internal servers and
 * external servers.
 *
 * Internal Servers:
 * - PE Dll extensions
 * - Run in kernel space
 * - MCore Trusted Code
 * - Has own event threads
 *
 *
 * External Servers:
 * - PE Dll drivers
 * - Run in user space
 * - Untrusted
 * - Has their own event threads
 * 
 */

/* Includes 
 * - System */
#include <Server.h>
#include <Heap.h>
#include <Log.h>

/* Includes 
 * - C-Library */
#include <ds/list.h>
#include <stddef.h>

/* Globals */
List_t *GlbServers = NULL;
Heap_t *GlbServerHeap = NULL;

/* Initialize the Server system 
 * This does not load any actual servers
 * or anything, just inits the server structures */
void ServerInit(void)
{
	/* Initialize lists */
	GlbServers = ListCreate(KeyInteger, LIST_NORMAL);

	/* Initialize the shared memory */
	GlbServerHeap = HeapCreate(MEMORY_LOCATION_SHM, MEMORY_LOCATION_SHM_END, 0);
}

/* Server Memory Operations
 * These include allocation and freeing */
void *ServerMemoryAllocate(size_t Length)
{
	return (void*)HeapAllocate(GlbServerHeap, 
		Length, ALLOCATION_COMMIT, 16, MEMORY_MASK_DEFAULT, "SERVER");
}

/* Server Memory Operations
 * These include allocation and freeing */
void ServerMemoryFree(void *Ptr)
{
	HeapFree(GlbServerHeap, (Addr_t)Ptr);
}