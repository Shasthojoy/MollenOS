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
* MollenOS Window - Sapphire
*/

/* Includes */
#include <SDL.h>
#include <SDL_image.h>
#include "Scene.h"
#include "Window.h"

/* CLib */
#include <string.h>
#include <stdlib.h>

/* Constructor
 * Allocates a new window of the given
 * dimensions and initializes it */
Window_t *WindowCreate(IpcComm_t Owner, Rect_t *Dimensions, int Flags, SDL_Renderer *Renderer)
{
	/* Allocate a new window instance */
	Window_t *Window = (Window_t*)malloc(sizeof(Window_t));
	void *mPixels = NULL;
	int mPitch = 0;
	
	/* Set initial stuff */
	Window->Id = 0;
	Window->Owner = Owner;
	Window->Flip = SDL_FLIP_NONE;
	Window->Rotation = 0.0;
	Window->zIndex = 1000;

	/* Convert dims */
	Window->Dimensions.x = Dimensions->x;
	Window->Dimensions.y = Dimensions->y;
	Window->Dimensions.h = Dimensions->h;
	Window->Dimensions.w = Dimensions->w;

	/* Allocate a texture */
	Window->Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, 
		SDL_TEXTUREACCESS_STREAMING, Dimensions->w, Dimensions->h);

	/* Get texture information */
	SDL_LockTexture(Window->Texture, NULL, &mPixels, &mPitch);

	/* Allocate a user-backbuffer */
	Window->Backbuffer = malloc(Dimensions->h * mPitch);
	memset(Window->Backbuffer, 0xFFFFFFFF, Dimensions->h * mPitch);

	/* Copy pixels */
	memcpy(mPixels, Window->Backbuffer, Dimensions->h * mPitch);

	/* Unlock texture */
	SDL_UnlockTexture(Window->Texture);

	/* Update size */
	Window->BackbufferSize = Dimensions->h * mPitch;

	/* Done */
	return Window;
}

/* Destructor
 * Cleans up and releases
 * resources allocated */
void WindowDestroy(Window_t *Window)
{
	/* Sanity */
	if (Window == NULL)
		return;

	/* Free sdl-stuff */
	SDL_DestroyTexture(Window->Texture);

	/* Free resources */
	free(Window->Backbuffer);
	free(Window);
}

/* Update
 * Updates all neccessary state and
 * buffers before rendering */
void WindowUpdate(Window_t *Window)
{
	/* Variables needed for update */
	void *mPixels = NULL;
	int mPitch = 0;

	/* Sanity */
	if (Window == NULL)
		return;

	/* Lock texture */
	SDL_LockTexture(Window->Texture, NULL, &mPixels, &mPitch);

	/* Copy pixels */
	memcpy(mPixels, Window->Backbuffer, Window->Dimensions.h * mPitch);

	/* Unlock texture */
	SDL_UnlockTexture(Window->Texture);
}

/* Render
 * Renders the window to the
 * given renderer */
void WindowRender(Window_t *Window, SDL_Renderer *Renderer)
{
	/* Sanity */
	if (Window == NULL)
		return;

	/* Copy window texture to render */
	SDL_RenderCopyEx(Renderer, Window->Texture, NULL, &Window->Dimensions, 
		Window->Rotation, NULL, Window->Flip);
}