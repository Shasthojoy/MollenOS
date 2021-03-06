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
 * MollenOS MCore - Server & Process Management
 * - The process/server manager is known as Phoenix
 * - In this file we implement Ash functions
 */
#define __MODULE "ASH1"
//#define __TRACE

/* Includes 
 * - System */
#include <os/driver/file.h>
#include <system/thread.h>
#include <system/utils.h>
#include <process/phoenix.h>
#include <modules/modules.h>
#include <scheduler.h>
#include <threading.h>
#include <debug.h>
#include <heap.h>

/* Includes
 * - Library */
#include <stddef.h>

/* This is the finalizor function for starting
 * up a new base Ash, it finishes setting up the environment
 * and memory mappings, must be called on it's own thread */
void
PhoenixFinishAsh(
    _In_ MCoreAsh_t *Ash)
{
    // Variables
    UUId_t CurrentCpu       = CpuGetCurrentId();
    MCoreThread_t *Thread   = ThreadingGetCurrentThread(CurrentCpu);
    uintptr_t BaseAddress   = 0;
    int LoadedFromInitRD    = 0;

    // Sanitize the loaded path, if we were
    // using the initrd set flags accordingly
    if (MStringFindChars(Ash->Path, "rd:/") != MSTRING_NOT_FOUND) {
        LoadedFromInitRD = 1;
    }

    // Update currently running thread
    Ash->MainThread = Thread->Id;
    Thread->AshId = Ash->Id;

    // Store current address space
    Ash->AddressSpace = AddressSpaceGetCurrent();

    // Setup base address for code data
    BaseAddress = MEMORY_LOCATION_RING3_CODE;

    // Load Executable
    Ash->Executable = PeLoadImage(NULL, Ash->Name, Ash->FileBuffer, 
        Ash->FileBufferLength, &BaseAddress, LoadedFromInitRD);
    Ash->NextLoadingAddress = BaseAddress;

    // Cleanup file buffer
    if (!LoadedFromInitRD) {
        kfree(Ash->FileBuffer);
    }
    Ash->FileBuffer = NULL;

    // Initialize the memory bitmaps
    Ash->Heap = BlockBitmapCreate(AddressSpaceTranslate(Ash->AddressSpace, MEMORY_LOCATION_RING3_HEAP),
        AddressSpaceTranslate(Ash->AddressSpace, MEMORY_LOCATION_RING3_SHM), PAGE_SIZE);
    Ash->Shm = BlockBitmapCreate(AddressSpaceTranslate(Ash->AddressSpace, MEMORY_LOCATION_RING3_SHM),
        AddressSpaceTranslate(Ash->AddressSpace, MEMORY_LOCATION_RING3_IOSPACE), PAGE_SIZE);

    // Create the stack mapping
    AddressSpaceMap(AddressSpaceGetCurrent(), 
    ((MEMORY_LOCATION_RING3_STACK_START - ASH_STACK_INIT) & PAGE_MASK),
        ASH_STACK_INIT, __MASK, AS_FLAG_APPLICATION, NULL);
    Ash->StackStart = MEMORY_LOCATION_RING3_STACK_START;

    // Initialize signal queue
    Ash->SignalQueue = CollectionCreate(KeyInteger);
}

/* PhoenixStartupEntry
 * This is the standard ash-boot function
 * which simply sets up the ash and jumps to userland */
void
PhoenixStartupEntry(
    _In_ void *BasePointer)
{
    // Variables
    MCoreAsh_t *Ash = (MCoreAsh_t*)BasePointer;
    
    // Finish boot-process and go to usermode
    PhoenixFinishAsh(Ash);
    ThreadingEnterUserMode(Ash);
}

/* PhoenixInitializeAsh
 * This function loads the executable and
 * prepares the ash-environment, at this point
 * it won't be completely running yet, it needs
 * its own thread for that. Returns 0 on success */
KERNELAPI
OsStatus_t
KERNELABI
PhoenixInitializeAsh(
    _InOut_ MCoreAsh_t  *Ash, 
    _In_ MString_t      *Path)
{ 
    // Variables
    BufferObject_t *BufferObject    = NULL;
    UUId_t fHandle                  = UUID_INVALID;
    uint8_t *fBuffer                = NULL;
    char *fPath                     = NULL;
    size_t fSize = 0, fRead = 0, fIndex = 0;
    int Index = 0, ShouldFree = 0;

    // Sanitize inputs
    if (Path == NULL) {
        ERROR("Invalid parameters for PhoenixInitializeAsh");
        return OsError;
    }

    // Debug
    TRACE("PhoenixInitializeAsh(%s)", MStringRaw(Path));

    // Zero out structure
    memset(Ash, 0, sizeof(MCoreAsh_t));

    // Open File 
    // We have a special case here
    // in case we are loading from RD
    if (MStringFindChars(Path, "rd:/") != MSTRING_NOT_FOUND) { 
        Ash->Path = MStringCreate((void*)MStringRaw(Path), StrUTF8);
        if (ModulesQueryPath(Path, (void**)&fBuffer, &fSize) != OsSuccess) {
            ERROR("Failed to locate module/file in ramdisk.");
            return OsError;
        }
    }
    else {
        if (OpenFile(MStringRaw(Path), 
            __FILE_MUSTEXIST, __FILE_READ_ACCESS, &fHandle) != FsOk) {
            ERROR("Invalid path given.");
            return OsError;
        }

        // Allocate buffer large enough to read entire file
        GetFileSize(fHandle, &fSize, NULL);
        BufferObject = CreateBuffer(fSize);
        fBuffer = (uint8_t*)kmalloc(fSize);
        fPath = (char*)kmalloc(_MAXPATH);

        // Set that we should free the buffer again
        ShouldFree = 1;

        // Read file and copy path
        memset(fPath, 0, _MAXPATH);
        ReadFile(fHandle, BufferObject, &fIndex, &fRead);
        ReadBuffer(BufferObject, (__CONST void*)fBuffer, fRead, NULL);
        GetFilePath(fHandle, fPath, _MAXPATH);
        Ash->Path = MStringCreate(fPath, StrUTF8);

        // Cleanup
        DestroyBuffer(BufferObject);
        CloseFile(fHandle);
        kfree(fPath);
    }

    // Validate the pe-file buffer
    if (!PeValidate(fBuffer, fSize)) {
        ERROR("Failed to validate the file as a PE-file.");
        if (ShouldFree == 1) {
            kfree(fBuffer);
        }
        return OsError;
    }

    // Update initial members
    Ash->Id = PhoenixGetNextId();
    Ash->Parent = ThreadingGetCurrentThread(CpuGetCurrentId())->AshId;
    Ash->Type = AshBase;
    CriticalSectionConstruct(&Ash->Lock, CRITICALSECTION_PLAIN);

    // Split path, even if a / is not found
    // it won't fail, since -1 + 1 = 0, so we just copy
    // the entire string
    Index = MStringFindReverse(Ash->Path, '/');
    Ash->Name = MStringSubString(Ash->Path, Index + 1, -1);

    // Store members and initialize members
    Ash->FileBuffer = fBuffer;
    Ash->FileBufferLength = fSize;
    Ash->Pipes = CollectionCreate(KeyInteger);
    return OsSuccess;
}

/* PhoenixStartupAsh
 * This is a wrapper for starting up a base Ash
 * and uses <PhoenixInitializeAsh> to setup the env
 * and do validation before starting */
UUId_t
PhoenixStartupAsh(
    _In_ MString_t *Path)
{
    // Variables
    MCoreAsh_t *Ash = NULL;

    // Allocate and initialize instance
    Ash = (MCoreAsh_t*)kmalloc(sizeof(MCoreAsh_t));
    if (PhoenixInitializeAsh(Ash, Path) != OsSuccess) {
        kfree(Ash);
        return UUID_INVALID;
    }

    // Register ash
    PhoenixRegisterAsh(Ash);
    ThreadingCreateThread(MStringRaw(Ash->Name),
        PhoenixStartupEntry, Ash, THREADING_USERMODE);
    return Ash->Id;
}

/* PhoenixOpenAshPipe
 * Creates a new communication pipe available for use. */
OsStatus_t
PhoenixOpenAshPipe(
    _In_ MCoreAsh_t  *Ash, 
    _In_ int          Port, 
    _In_ Flags_t      Flags)
{
    // Variables
    MCorePipe_t *Pipe = NULL;
    DataKey_t Key;

    // Debug
    TRACE("PhoenixOpenAshPipe(Port %i)", Port);

    // Sanitize
    if (Ash == NULL || Port < 0) {
        ERROR("Invalid ash-instance or port id");
        return OsError;
    }

    // Make sure that a pipe on the given Port 
    // doesn't already exist!
    Key.Value = Port;
    CriticalSectionEnter(&Ash->Lock);
    if (CollectionGetDataByKey(Ash->Pipes, Key, 0) != NULL) {
        CriticalSectionLeave(&Ash->Lock);
        WARNING("The requested pipe already exists");
        return OsSuccess;
    }
    CriticalSectionLeave(&Ash->Lock);

    // Create a new pipe and add it to list
    Pipe = PipeCreate(ASH_PIPE_SIZE, Flags);
    
    CriticalSectionEnter(&Ash->Lock);
    CollectionAppend(Ash->Pipes, CollectionCreateNode(Key, Pipe));
    CriticalSectionLeave(&Ash->Lock);

    // Wake sleepers waiting for pipe creations
    SchedulerThreadWakeAll((uintptr_t*)Ash->Pipes);
    return OsSuccess;
}

/* PhoenixWaitAshPipe
 * Waits for a pipe to be opened on the given
 * ash instance. */
OsStatus_t
PhoenixWaitAshPipe(
    _In_ MCoreAsh_t *Ash, 
    _In_ int         Port)
{
    // Variables
    DataKey_t Key;
    int Run = 1;

    // Sanitize input
    if (Ash == NULL) {
        return OsError;
    }

    // Wait for wake-event on pipe
    Key.Value = Port;
    while (Run) {
        CriticalSectionEnter(&Ash->Lock);
        if (CollectionGetDataByKey(Ash->Pipes, Key, 0) != NULL) {
            CriticalSectionLeave(&Ash->Lock);
            break;
        }
        CriticalSectionLeave(&Ash->Lock);
        if (SchedulerThreadSleep((uintptr_t*)Ash->Pipes, 5000) == SCHEDULER_SLEEP_TIMEOUT) {
            ERROR("Failed to wait for open pipe, timeout after 5 seconds.");
            return OsError;
        }
     }
    return OsSuccess;
}

/* PhoenixCloseAshPipe
 * Closes the pipe for the given Ash, and cleansup
 * resources allocated by the pipe. This shutsdown
 * any communication on the port */
OsStatus_t
PhoenixCloseAshPipe(
    _In_ MCoreAsh_t *Ash, 
    _In_ int         Port)
{
    // Variables
    MCorePipe_t *Pipe = NULL;
    DataKey_t Key;

    // Sanitize input
    if (Ash == NULL || Port < 0) {
        return OsSuccess;
    }

    // Lookup pipe
    Key.Value = Port;
    CriticalSectionEnter(&Ash->Lock);
    Pipe = (MCorePipe_t*)CollectionGetDataByKey(Ash->Pipes, Key, 0);
    CriticalSectionLeave(&Ash->Lock);
    if (Pipe == NULL) {
        return OsError;
    }

    // Cleanup pipe and remove node
    PipeDestroy(Pipe);
    CriticalSectionEnter(&Ash->Lock);
    CollectionRemoveByKey(Ash->Pipes, Key);
    return CriticalSectionLeave(&Ash->Lock);
}

/* PhoenixGetAshPipe
 * Retrieves an existing pipe instance for the given ash
 * and port-id. If it doesn't exist, returns NULL. */
MCorePipe_t*
PhoenixGetAshPipe(
    _In_ MCoreAsh_t     *Ash, 
    _In_ int             Port)
{
    // Variables
    MCorePipe_t *Pipe = NULL;
    DataKey_t Key;

    // Sanitize input
    if (Ash == NULL || Port < 0) {
        return NULL;
    }

    // Perform the lookup
    Key.Value = Port;
    CriticalSectionEnter(&Ash->Lock);
    Pipe = (MCorePipe_t*)CollectionGetDataByKey(Ash->Pipes, Key, 0);
    CriticalSectionLeave(&Ash->Lock);
    return Pipe;
}

/* PhoenixGetCurrentAsh
 * Retrives the current ash for the running thread */
MCoreAsh_t*
PhoenixGetCurrentAsh(void)
{
    // Variables
    UUId_t CurrentCpu = UUID_INVALID;

    // Get the ID
    CurrentCpu = CpuGetCurrentId();
    if (ThreadingGetCurrentThread(CurrentCpu) != NULL) {
        if (ThreadingGetCurrentThread(CurrentCpu)->AshId != UUID_INVALID) {
            return PhoenixGetAsh(ThreadingGetCurrentThread(CurrentCpu)->AshId);
        }
        else {
            return NULL;
        }
    }
    else {
        return NULL;
    }
}

/* PhoenixQueryAsh
 * Queries the given ash for information
 * which kind of information is determined by <Function> */
OsStatus_t
PhoenixQueryAsh(
    _In_ MCoreAsh_t         *Ash,
    _In_ AshQueryFunction_t  Function,
    _In_ void               *Buffer,
    _In_ size_t              Length)
{
    // Variables
    OsStatus_t Result   = OsError;
    
    // Sanitize input
    if (Ash == NULL || Buffer == NULL || Length == 0) {
        return OsError;
    }

    // Handle function
    switch (Function) {
        case AshQueryName: {
            size_t BytesToCopy = MIN(MStringSize(Ash->Name), Length);
            memcpy(Buffer, MStringRaw(Ash->Name), BytesToCopy);
            Result = OsSuccess;
        } break;

        // For now, we only support querying of
        // how much memory has been allocated for the heap 
        case AshQueryMemory: {
            *((size_t*)Buffer) = (Ash->Heap->BlocksAllocated * PAGE_SIZE);
            Result = OsSuccess;
        } break;

        case AshQueryParent: {
            UUId_t *bPtr = (UUId_t*)Buffer;
            *bPtr = Ash->Parent;
            Result = OsSuccess;
        } break;
        case AshQueryTopMostParent: {
            UUId_t *bPtr = (UUId_t*)Buffer;
            *bPtr = UUID_INVALID;

            // Iterate up till top parent
            MCoreAsh_t *Parent = Ash;
            while (Parent->Parent != UUID_INVALID) {
                *bPtr = Parent->Parent;
                Parent = PhoenixGetAsh(Parent->Parent);
            }
            Result = OsSuccess;
        } break;

        default:
            break;
    }
    return Result;
}

/* PhoenixCleanupAsh
 * Cleans up a given Ash, freeing all it's allocated resources
 * and unloads it's executables, memory space is not cleaned up
 * must be done by external thread */
void
PhoenixCleanupAsh(
    _In_ MCoreAsh_t *Ash)
{
    // Variables
    CollectionItem_t *fNode = NULL;

    // Strings first
    MStringDestroy(Ash->Name);
    MStringDestroy(Ash->Path);

    // Cleanup signals
    _foreach(fNode, Ash->SignalQueue) {
        kfree(fNode->Data);
    }
    CollectionDestroy(Ash->SignalQueue);

    // Cleanup pipes
    _foreach(fNode, Ash->Pipes) {
        PipeDestroy((MCorePipe_t*)fNode->Data);
    }
    CollectionDestroy(Ash->Pipes);

    // Cleanup memory
    BlockBitmapDestroy(Ash->Shm);
    BlockBitmapDestroy(Ash->Heap);
    PeUnloadImage(Ash->Executable);
    kfree(Ash);
}

