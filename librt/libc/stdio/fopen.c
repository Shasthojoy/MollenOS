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
 * MollenOS C Library - File Opening & File Creation
 */

/* Includes
 * - System */
#include <os/utils.h>
#include <os/driver/file.h>
#include <os/syscall.h>

/* Includes 
 * - Library */
#include <io.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "local.h"

/* split_oflags
 * Generates WX flags from the stdc opening flags */
unsigned split_oflags(
	_In_ unsigned oflags)
{
	// Variables
    int         wxflags = 0;
    unsigned unsupp; // until we support everything

    if (oflags & _O_APPEND)              wxflags |= WX_APPEND;
    if (oflags & _O_BINARY)              {/* Nothing to do */}
    else if (oflags & _O_TEXT)           wxflags |= WX_TEXT;
    else if (oflags & _O_WTEXT)          wxflags |= WX_TEXT;
    else if (oflags & _O_U16TEXT)        wxflags |= WX_TEXT;
    else if (oflags & _O_U8TEXT)         wxflags |= WX_TEXT;
    else                                 wxflags |= WX_TEXT; // default to TEXT
    if (oflags & _O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(
                    _O_BINARY|_O_TEXT|_O_APPEND|
                    _O_TRUNC|_O_EXCL|_O_CREAT|
                    _O_RDWR|_O_WRONLY|_O_TEMPORARY|
                    _O_NOINHERIT|
                    _O_SEQUENTIAL|_O_RANDOM|_O_SHORT_LIVED|
                    _O_WTEXT|_O_U16TEXT|_O_U8TEXT
                    ))) {
		TRACE(":unsupported oflags 0x%04x\n", unsupp);
	}

	// Done
    return wxflags;
}

/* _open
 * ANSII Version of the fopen. Handles flags and creation flags. */
int _open(
	_In_ __CONST char *file, 
	_In_ int flags,
	...)
{
	// Variables
	FileSystemCode_t Code = 0;
	UUId_t Handle;
	int wxflags = 0;
	int pmode = 0;
	int fd = -1;
	va_list ap;

	// Sanitize input
	if (file == NULL) {
		_set_errno(EINVAL);
		return -1;
	}

	// Extract pmode flags
	if (flags & _O_CREAT) {
		va_start(ap, flags);
		pmode = va_arg(ap, int);
		va_end(ap);
	}

	// Generate WX flags
	wxflags = split_oflags((unsigned int)flags);

	// Invoke os service
	Code = OpenFile(file, _fopts(flags), _faccess(flags), &Handle);
	if (!_fval(Code)) {
		fd = StdioFdAllocate(Handle, wxflags);
		if (flags & _O_WTEXT) {
			get_ioinfo(fd)->exflag |= EF_UTF16|EF_UNK_UNICODE;		
		}
		else if (flags & _O_U16TEXT) {
			get_ioinfo(fd)->exflag |= EF_UTF16;
		}
		else if (flags & _O_U8TEXT) {
			get_ioinfo(fd)->exflag |= EF_UTF8;
		}
	}
	else {
		CloseFile(Handle);
		_fval(Code);
	}

	// Done, return fd
	return fd;
}

/* Information 
"r"	read: Open file for input operations. The file must exist.
"w"	write: Create an empty file for output operations. If a file with the same name already exists, its contents are discarded and the file is treated as a new empty file.
"a"	append: Open file for output at the end of a file. Output operations always write data at the end of the file, expanding it. 
	Repositioning operations (fseek, fsetpos, rewind) are ignored. The file is created if it does not exist.
"r+" read/update: Open a file for update (both for input and output). The file must exist.
"w+" write/update: Create an empty file and open it for update (both for input and output). If a file with the same name already exists its contents are discarded and the file is treated as a new empty file.
"a+" append/update: Open a file for update (both for input and output) with all output operations writing data at the end of the file. 
	 Repositioning operations (fseek, fsetpos, rewind) affects the next input operations, but output operations move the position back to the end of file. The file is created if it does not exist.
*/
FILE *fdopen(int fd, __CONST char *mode)
{
	// Variables
	int open_flags, stream_flags;
	FILE *stream;

	// Sanitize parameters
	if (fd < 0 || mode == NULL) {
		_set_errno(EINVAL);
		return NULL;
	}

	// Split flags
	_fflags(mode, &open_flags, &stream_flags);

	// Allocate a new file instance 
	// and reset the structure
	stream = (FILE*)malloc(sizeof(FILE));
	memset(stream, 0, sizeof(FILE));
	
	// Initialize a handle
	if (StdioFdInitialize(stream, fd, stream_flags) != OsSuccess) {
		free(stream);
		return NULL;
	}
	return stream;
}

/* fopen
 * Opens the file whose name is specified in the parameter filename 
 * and associates it with a stream that can be identified in future 
 * operations by the FILE pointer returned. */
FILE *fopen(
	_In_ __CONST char * filename, 
	_In_ __CONST char * mode)
{
	// Variables
	int open_flags, stream_flags;
	int fd = 0;

	// Sanitize parameters
	if (filename == NULL || mode == NULL) {
		_set_errno(EINVAL);
		return NULL;
	}

	// Split flags
	_fflags(mode, &open_flags, &stream_flags);

	// Open file as file-descriptor
	fd = _open(filename, open_flags, _S_IREAD | _S_IWRITE);
	if (fd == -1) {
		return NULL;
	}

	// Upgrade the fd to a file-stream
	return fdopen(fd, mode);
}
