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
* MollenOS C Library - Standard OS Header
*/

#ifndef __MOLLENOS_CLIB__
#define __MOLLENOS_CLIB__

/* C-Library - Includes */
#include <crtdefs.h>

/* OS - Includes */
#include <os/Ipc.h>

/* Definitons */

/* The max path to expect from mollenos 
 * file/path operations */
#ifndef MPATH_MAX
#define MPATH_MAX		512
#endif

/* Typedef the process id type */
#ifndef MPROCESS_ID
#define MPROCESS_ID
typedef unsigned int PId_t;
#endif

/* This describes a window handle 
 * used by UI functions */
#ifndef MWNDHANDLE
#define MWNDHANDLE
typedef void *WndHandle_t;
#endif

/* Enumerators */
typedef enum _MollenOSDeviceType
{
	DeviceUnknown = 0,
	DeviceCpu,
	DeviceCpuCore,
	DeviceController,
	DeviceBus,
	DeviceClock,
	DeviceTimer,
	DevicePerfTimer,
	DeviceInput,
	DeviceStorage,
	DeviceVideo

} MollenOSDeviceType_t;

/* Environment Paths */
typedef enum _EnvironmentPaths
{
	/* The default */
	PathCurrentWorkingDir = 0,

	/* Application Paths */
	PathApplicationBase,
	PathApplicationData,

	/* System Directories */
	PathSystemBase,
	PathSystemDirectory,

	/* Shared Directories */
	PathCommonBin,
	PathCommonDocuments,
	PathCommonInclude,
	PathCommonLib,
	PathCommonMedia,

	/* User Directories */
	PathUserBase,

	/* Special Directory Count */
	PathEnvironmentCount

} EnvironmentPath_t;

/* Structures */
typedef struct _MollenOSVideoDescriptor
{
	/* Framebuffer */
	void *FrameBufferAddr;

	/* Mode Information */
	size_t BytesPerScanline;
	size_t Height;
	size_t Width;
	int Depth;

	/* Pixel Information */
	int RedPosition;
	int BluePosition;
	int GreenPosition;
	int ReservedPosition;

	int RedMask;
	int BlueMask;
	int GreenMask;
	int ReservedMask;

} MollenOSVideoDescriptor_t;

/* Define the standard os 
 * rectangle used for ui 
 * operations */
#ifndef MRECTANGLE_DEFINED
#define MRECTANGLE_DEFINED
typedef struct _mRectangle
{
	/* Origin */
	int x, y;

	/* Size */
	int h, w;

} Rect_t;
#endif

/***********************
* IPC Prototypes
***********************/

/* IPC - Peek - NO BLOCK
 * This returns -1 if there is no new messages
 * in the message-queue, otherwise it returns 0
 * and fills the base structures with information about
 * the message */
_MOS_API int MollenOSMessagePeek(MEventMessage_t *Message);

/* IPC - Read/Wait - BLOCKING OP
 * This returns -1 if something went wrong reading
 * a message from the message queue, otherwise it returns 0
 * and fills the structures with information about
 * the message */
_MOS_API int MollenOSMessageWait(MEventMessage_t *Message);

/* IPC - Write
 * Returns -1 if message failed to send
 * Returns -2 if message-target didn't exist
 * Returns 0 if message was sent correctly to target */
_MOS_API int MollenOSMessageSend(IpcComm_t Target, void *Message, size_t MessageLength);

/***********************
* Process Prototypes
***********************/

/* Process Spawn
 * This function spawns a new process
 * in it's own address space, and returns
 * the new process id
 * If startup is failed, the returned value
 * is 0xFFFFFFFF */
_MOS_API PId_t ProcessSpawn(const char *Path, const char *Arguments);

/* Process Join
 * Attaches to a running process
 * and waits for the process to quit
 * the return value is the return code
 * from the target process */
_MOS_API int ProcessJoin(PId_t ProcessId);

/* Process Kill
 * Kills target process id
 * On error, returns -1, or if the
 * kill was succesful, returns 0 */
_MOS_API int ProcessKill(PId_t ProcessId);

/***********************
* Memory Prototypes
***********************/

/* Memory - Share 
 * This shares a piece of memory with the 
 * target process. The function returns NULL
 * on failure to share the piece of memory
 * otherwise it returns the new buffer handle
 * that can be accessed by the other process */
_MOS_API void *MollenOSMemoryShare(IpcComm_t Target, void *Buffer, size_t Size);

/* Memory - Unshare 
 * This takes a previous shared memory handle 
 * and unshares it again from the target process
 * The function returns 0 if unshare was succesful, 
 * otherwise -1 */
_MOS_API int MollenOSMemoryUnshare(IpcComm_t Target, void *MemoryHandle, size_t Size);

/***********************
* Device Prototypes
***********************/
_MOS_API int MollenOSDeviceQuery(MollenOSDeviceType_t Type, int Request, void *Buffer, size_t Length);

/***********************
* Shared Library Prototypes
***********************/

/* Load a shared object given a path
 * path must exists otherwise NULL is returned */
_MOS_API void *SharedObjectLoad(const char *SharedObject);

/* Load a function-address given an shared object
 * handle and a function name, function must exist
 * otherwise null is returned */
_MOS_API void *SharedObjectGetFunction(void *Handle, const char *Function);

/* Unloads a valid shared object handle
 * returns 0 on success */
_MOS_API void SharedObjectUnload(void *Handle);

/***********************
* Environment Prototypes
***********************/

/* Resolve Environmental Path
 * Resolves the environmental type
 * to an valid absolute path */
_MOS_API void EnvironmentResolve(EnvironmentPath_t SpecialPath, char *StringBuffer);

/***********************
* Screen Prototypes
***********************/

/* This function returns screen geomemtry
 * descriped as a rectangle structure, which
 * must be pre-allocated */
_MOS_API void MollenOSGetScreenGeometry(Rect_t *Rectangle);

/***********************
* Ui Prototypes
***********************/

/* UiCreateWindow
 * Creates a window of the given
 * dimensions and flags. The returned
 * value is the id of the newly created
 * window. Returns NULL on failure */
_MOS_API WndHandle_t UiCreateWindow(Rect_t *Dimensions, int Flags);

/* UiDestroyWindow 
 * Destroys a given window 
 * and frees the resources associated
 * with it. Returns 0 on success */
_MOS_API int UiDestroyWindow(WndHandle_t Handle);

/* UiQueryDimensions 
 * Queries the dimensions of a window
 * handle */
_MOS_API int UiQueryDimensions(WndHandle_t Handle, Rect_t *Dimensions);

/* UiQueryBackbuffer 
 * Queries the backbuffer handle of a window 
 * that can be used for direct pixel access to it */
_MOS_API int UiQueryBackbuffer(WndHandle_t Handle, void **Backbuffer, size_t *BackbufferSize);

/* UiInvalidateRect
 * Invalides a region of the window
 * based on relative coordinates in the window 
 * if its called with NULL as dimensions it 
 * invalidates all */
_MOS_API void UiInvalidateRect(WndHandle_t Handle, Rect_t *Rect);


/***********************
* System Misc Prototypes
* - No functions here should
*   be called manually
*   they are automatically used
*   by systems
***********************/
_MOS_API void UiConnect(void);
_MOS_API void UiDisconnect(void);
_MOS_API int MollenOSSignalWait(size_t Timeout);
_MOS_API int MollenOSSignalWake(IpcComm_t Target);
_MOS_API void MollenOSSystemLog(const char *Message);
_MOS_API int MollenOSEndBoot(void);
_MOS_API int MollenOSRegisterWM(void);

#endif //!__MOLLENOS_CLIB__