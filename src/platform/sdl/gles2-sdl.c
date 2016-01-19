/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "main.h"

#include "gl-common.h"

#include <malloc.h>

static bool mSDLGLES2Init(struct SDLSoftwareRenderer* renderer);
static void mSDLGLES2RunloopGBA(struct SDLSoftwareRenderer* renderer, void* user);
static void mSDLGLES2Deinit(struct SDLSoftwareRenderer* renderer);

void mSDLGLES2Create(struct mSDLRenderer* renderer) {
	renderer->init = mSDLGLES2Init;
	renderer->deinit = mSDLGLES2Deinit;
	renderer->runloop = mSDLGLES2RunloopGBA;
}

bool mSDLGLES2Init(struct SDLSoftwareRenderer* renderer) {
#ifdef BUILD_RASPI
	bcm_host_init();
	renderer->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	int major, minor;
	if (EGL_FALSE == eglInitialize(renderer->display, &major, &minor)) {
		printf("Failed to initialize EGL");
		return false;
	}

	if (EGL_FALSE == eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("Failed to get GLES API");
		return false;
	}

	const EGLint requestConfig[] = {
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 5,
		EGL_BLUE_SIZE, 5,
		EGL_ALPHA_SIZE, 1,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLConfig config;
	EGLint numConfigs;

	if (EGL_FALSE == eglChooseConfig(renderer->display, requestConfig, &config, 1, &numConfigs)) {
		printf("Failed to choose EGL config\n");
		return false;
	}

	const EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	int dispWidth = 240, dispHeight = 160, adjWidth;
	renderer->context = eglCreateContext(renderer->display, config, EGL_NO_CONTEXT, contextAttributes);
	graphics_get_display_size(0, &dispWidth, &dispHeight);
	adjWidth = dispHeight / 2 * 3;

	DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
	DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

	VC_RECT_T destRect = {
		.x = (dispWidth - adjWidth) / 2,
		.y = 0,
		.width = adjWidth,
		.height = dispHeight
	};

	VC_RECT_T srcRect = {
		.x = 0,
		.y = 0,
		.width = 240 << 16,
		.height = 160 << 16
	};

	DISPMANX_ELEMENT_HANDLE_T element = vc_dispmanx_element_add(update, display, 0, &destRect, 0, &srcRect, DISPMANX_PROTECTION_NONE, 0, 0, 0);
	vc_dispmanx_update_submit_sync(update);

	renderer->window.element = element;
	renderer->window.width = dispWidth;
	renderer->window.height = dispHeight;

	renderer->surface = eglCreateWindowSurface(renderer->display, config, &renderer->window, 0);
	if (EGL_FALSE == eglMakeCurrent(renderer->display, renderer->surface, renderer->surface, renderer->context)) {
		return false;
	}
#else
	mSDLGLCommonInit(renderer);
#endif

	renderer->d.outputBuffer = memalign(16, VIDEO_HORIZONTAL_PIXELS * VIDEO_VERTICAL_PIXELS * 4);
	renderer->d.outputBufferStride = VIDEO_HORIZONTAL_PIXELS;

	GBAGLES2ContextCreate(&renderer->gl2);
	renderer->gl2.d.user = renderer;
	renderer->gl2.d.lockAspectRatio = renderer->lockAspectRatio;
	renderer->gl2.d.filter = renderer->filter;
	renderer->gl2.d.swap = mSDLGLCommonSwap;
	renderer->gl2.d.init(&renderer->gl2.d, 0);
	return true;
}

void mSDLGLES2Runloop(struct mSDLRenderer* renderer, void* user) {
	struct GBAThread* context = user;
	SDL_Event event;
	struct VideoBackend* v = &renderer->gl2.d;

	while (context->state < THREAD_EXITING) {
		while (SDL_PollEvent(&event)) {
			GBASDLHandleEvent(context, &renderer->player, &event);
		}

		if (GBASyncWaitFrameStart(&context->sync)) {
			v->postFrame(v, renderer->d.outputBuffer);
		}
		GBASyncWaitFrameEnd(&context->sync);
		v->drawFrame(v);
#ifdef BUILD_RASPI
		eglSwapBuffers(renderer->display, renderer->surface);
#else
		v->swap(v);
#endif
	}
}

void mSDLGLES2Deinit(struct mSDLRenderer* renderer) {
	if (renderer->gl2.d.deinit) {
		renderer->gl2.d.deinit(&renderer->gl2.d);
	}
#ifdef BUILD_RASPI
	eglMakeCurrent(renderer->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(renderer->display, renderer->surface);
	eglDestroyContext(renderer->display, renderer->context);
	eglTerminate(renderer->display);
	bcm_host_deinit();
#elif SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GL_DeleteContext(renderer->glCtx);
#endif
	free(renderer->d.outputBuffer);
}
