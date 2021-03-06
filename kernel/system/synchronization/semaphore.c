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
 * MollenOS Synchronization
 * - Semaphores implementation, used safe passages known as
 *   critical sections in MCore
 */

/* Includes 
 * - System */
#include <system/thread.h>
#include <system/utils.h>
#include <scheduler.h>
#include <semaphore.h>
#include <heap.h>

/* Includes
 * - Library */
#include <ds/collection.h>
#include <stddef.h>
#include <assert.h>

/* Globals */
static Collection_t *GlbSemaphores = NULL;
static Spinlock_t SemaphoreLock;
int GlbSemaphoreInit = 0;

/* SemaphoreCreate
 * Initializes and allocates a new semaphore
 * Semaphores use safe passages to avoid race-conditions */
Semaphore_t *SemaphoreCreate(int InitialValue)
{
	/* Variables */
	Semaphore_t *Semaphore = NULL;

	/* Allocate a new instance of a semaphore 
	 * and instantiate it */
	Semaphore = (Semaphore_t*)kmalloc(sizeof(Semaphore_t));
	SemaphoreConstruct(Semaphore, InitialValue);

	/* Done! */
	return Semaphore;
}

/* SemaphoreCreateGlobal
 * Creates a global semaphore, identified by it's name
 * and makes sure only one can exist at the time. Returns
 * NULL if one already exists. */
Semaphore_t *SemaphoreCreateGlobal(MString_t *Identifier, int InitialValue)
{
	/* Variables */
	DataKey_t hKey;

	/* First of all, make sure there is no 
	 * conflicting semaphores in system */
	if (Identifier != NULL) {
		if (GlbSemaphoreInit != 1) {
			GlbSemaphores = CollectionCreate(KeyInteger);
			GlbSemaphoreInit = 1;
            SpinlockReset(&SemaphoreLock);
		}

		/* Hash the string */
		hKey.Value = (int)MStringHash(Identifier);

		/* Check list if exists */
        SpinlockAcquire(&SemaphoreLock);
		void *Exists = CollectionGetDataByKey(GlbSemaphores, hKey, 0);
        SpinlockRelease(&SemaphoreLock);

		/* Sanitize the lookup */
		if (Exists != NULL) {
			return NULL;
		}
	}

	/* Allocate a new semaphore */
	Semaphore_t *Semaphore = (Semaphore_t*)kmalloc(sizeof(Semaphore_t));
	SemaphoreConstruct(Semaphore, InitialValue);
	Semaphore->Hash = MStringHash(Identifier);

	/* Add to system list of semaphores if global */
	if (Identifier != NULL)  {
        SpinlockAcquire(&SemaphoreLock);
		CollectionAppend(GlbSemaphores, CollectionCreateNode(hKey, Semaphore));
        SpinlockRelease(&SemaphoreLock);
	}
	return Semaphore;
}

/* SemaphoreDestroy
 * Destroys and frees a semaphore, releasing any
 * resources associated with it */
void SemaphoreDestroy(Semaphore_t *Semaphore)
{
	/* Variables */
	DataKey_t Key;

	/* Sanity 
	 * If it has a global identifier
	 * we want to remove it from list */
	if (Semaphore->Hash != 0) {
		Key.Value = (int)Semaphore->Hash;
		CollectionRemoveByKey(GlbSemaphores, Key);
	}

	/* Wake up all */
	SchedulerThreadWakeAll((uintptr_t*)Semaphore);

	/* Free it */
	kfree(Semaphore);
}

/* SemaphoreConstruct
 * Constructs an already allocated semaphore and resets
 * it's value to the given initial value */
void SemaphoreConstruct(Semaphore_t *Semaphore, int InitialValue)
{
	/* Sanitize the parameters */
	assert(Semaphore != NULL);
	assert(InitialValue >= 0);

	/* Reset values to initial stuff */
	Semaphore->Hash = 0;
	Semaphore->Value = InitialValue;
	Semaphore->Creator = ThreadingGetCurrentThreadId();

	/* Reset safe passage */
	CriticalSectionConstruct(&Semaphore->Lock, CRITICALSECTION_PLAIN);
}

/* SemaphoreP (Wait) 
 * Waits for the semaphore signal with the optional time-out */
OsStatus_t
SemaphoreP(
    Semaphore_t *Semaphore,
    size_t Timeout)
{
	// Decrease the value, and do the sanity check 
	// if we should sleep for events
	CriticalSectionEnter(&Semaphore->Lock);
	Semaphore->Value--;
	if (Semaphore->Value < 0) {
		CriticalSectionLeave(&Semaphore->Lock);
        if (SchedulerThreadSleep((uintptr_t*)Semaphore, Timeout) 
                == SCHEDULER_SLEEP_TIMEOUT) {
            Semaphore->Value++;
            return OsError;
        }
	}
	else {
		CriticalSectionLeave(&Semaphore->Lock);
    }
    return OsSuccess;
}

/* SemaphoreV (Signal) 
 * Signals the semaphore with the given value, default is 1 */
void SemaphoreV(Semaphore_t *Semaphore, int Value)
{
	/* Enter the safe-passage, we do not wish
	 * to be interrupted while doing this */
	CriticalSectionEnter(&Semaphore->Lock);

	/* Increase by the given value 
	 * and check if we should wake up others */
	Semaphore->Value += Value;
	if (Semaphore->Value <= 0) {
		SchedulerThreadWake((uintptr_t*)Semaphore);
	}

	/* Make sure to leave safe passage again! */
	CriticalSectionLeave(&Semaphore->Lock);
}
