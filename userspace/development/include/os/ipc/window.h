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
 * MollenOS InterProcess Comm Interface
 */

#ifndef _MOLLENOS_IPC_WINDOW_H_
#define _MOLLENOS_IPC_WINDOW_H_

/* Includes
 * - System */
#include <os/virtualkeycodes.h>
#include <os/osdefs.h>
#include <os/ipc/ipc.h>

/* Forward declarations
 * to avoid recursion */
typedef struct _MEventMessage MEventMessage_t;

/* The types of window server messages
 * This is for applications to interact
 * with the window server */
typedef enum _MWindowControlType {
	WindowCtrlCreate,
	WindowCtrlDestroy,
	WindowCtrlInvalidate,
	WindowCtrlQuery,
} MWindowControlType_t;

/* Base structure for window control 
 * messages, the types of control messages
 * are defined above by MWindowControlType_t */
typedef struct _MWindowControl
{
	/* Base */
	MEventMessage_t Header;

	/* Message Type */
	MWindowControlType_t Type;

	/* Available control parameters
	 * used by the control messages */
	size_t LoParam;
	size_t HiParam;
	Rect_t RcParam;

} MWindowControl_t;

/* The different types of input
 * that can be sent input messages
 * from, check the Type field in the
 * structure of an input message */
typedef enum _MInputType {
	InputUnknown = 0,
	InputMouse,
	InputKeyboard,
	InputKeypad,
	InputJoystick,
	InputGamePad,
	InputOther
} MInputType_t;

/* Window input structure, contains
 * information about the recieved input
 * event with button data etc */
typedef struct _MEventInput {
	MInputType_t		Type;
	unsigned			Scancode;
	VKey				Key;
	unsigned			Flags;	/* Flags (Bit-field, see under structure) */
	ssize_t				xRelative;
	ssize_t				yRelative;
	ssize_t				zRelative;
} MEventInput_t;

/* Flags - Event Types */
#define INPUT_BUTTON_RELEASED		0x0
#define INPUT_BUTTON_CLICKED		0x1
#define INPUT_MULTIPLEKEYS			0x2

#endif //!_MOLLENOS_IPC_WINDOW_H_
