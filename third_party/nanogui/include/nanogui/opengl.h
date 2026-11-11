/*
    nanogui/opengl.h -- Pulls in OpenGL, GLAD (if needed), GLFW, and
    NanoVG header files

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include <nanogui/vector.h>

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
#  if defined(NANOGUI_USE_OPENGL)
#    if defined(NANOGUI_GLAD)
#      if defined(NANOGUI_SHARED) && !defined(GLAD_GLAPI_EXPORT)
#        define GLAD_GLAPI_EXPORT
#      endif
#      include <glad/glad.h>
#    else
#      if defined(__APPLE__)
#        define GLFW_INCLUDE_GLCOREARB
#      else
#        define GL_GLEXT_PROTOTYPES
#      endif
#    endif
#  elif defined(NANOGUI_USE_GLES) && NANOGUI_GLES_VERSION == 2
#    define GLFW_INCLUDE_ES2
#  elif defined(NANOGUI_USE_GLES) && NANOGUI_GLES_VERSION == 3
#    define GLFW_INCLUDE_ES3
#  elif defined(NANOGUI_USE_METAL)
#  else
#    error You must select a backend (OpenGL/GLES2/GLES3/Metal)
#  endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef NANOGUI_NO_GLFW
#include <GLFW/glfw3.h>

#define NANOGUI_RELEASE GLFW_RELEASE
#define NANOGUI_PRESS GLFW_PRESS
#define NANOGUI_REPEAT GLFW_REPEAT

#define NANOGUI_KEY_UNKNOWN GLFW_KEY_UNKNOWN

/* Printable keys */
#define NANOGUI_KEY_SPACE              GLFW_KEY_SPACE
#define NANOGUI_KEY_APOSTROPHE         GLFW_KEY_APOSTROPHE  /* ' */
#define NANOGUI_KEY_COMMA              GLFW_KEY_COMMA  /* , */
#define NANOGUI_KEY_MINUS              GLFW_KEY_MINUS  /* - */
#define NANOGUI_KEY_PERIOD             GLFW_KEY_PERIOD  /* . */
#define NANOGUI_KEY_SLASH              GLFW_KEY_SLASH  /* / */
#define NANOGUI_KEY_0                  GLFW_KEY_0
#define NANOGUI_KEY_1                  GLFW_KEY_1
#define NANOGUI_KEY_2                  GLFW_KEY_2
#define NANOGUI_KEY_3                  GLFW_KEY_3
#define NANOGUI_KEY_4                  GLFW_KEY_4
#define NANOGUI_KEY_5                  GLFW_KEY_5
#define NANOGUI_KEY_6                  GLFW_KEY_6
#define NANOGUI_KEY_7                  GLFW_KEY_7
#define NANOGUI_KEY_8                  GLFW_KEY_8
#define NANOGUI_KEY_9                  GLFW_KEY_9
#define NANOGUI_KEY_SEMICOLON          GLFW_KEY_SEMICOLON  /* ; */
#define NANOGUI_KEY_EQUAL              GLFW_KEY_EQUAL  /* = */
#define NANOGUI_KEY_A                  GLFW_KEY_A
#define NANOGUI_KEY_B                  GLFW_KEY_B
#define NANOGUI_KEY_C                  GLFW_KEY_C
#define NANOGUI_KEY_D                  GLFW_KEY_D
#define NANOGUI_KEY_E                  GLFW_KEY_E
#define NANOGUI_KEY_F                  GLFW_KEY_F
#define NANOGUI_KEY_G                  GLFW_KEY_G
#define NANOGUI_KEY_H                  GLFW_KEY_H
#define NANOGUI_KEY_I                  GLFW_KEY_I
#define NANOGUI_KEY_J                  GLFW_KEY_J
#define NANOGUI_KEY_K                  GLFW_KEY_K
#define NANOGUI_KEY_L                  GLFW_KEY_L
#define NANOGUI_KEY_M                  GLFW_KEY_M
#define NANOGUI_KEY_N                  GLFW_KEY_N
#define NANOGUI_KEY_O                  GLFW_KEY_O
#define NANOGUI_KEY_P                  GLFW_KEY_P
#define NANOGUI_KEY_Q                  GLFW_KEY_Q
#define NANOGUI_KEY_R                  GLFW_KEY_R
#define NANOGUI_KEY_S                  GLFW_KEY_S
#define NANOGUI_KEY_T                  GLFW_KEY_T
#define NANOGUI_KEY_U                  GLFW_KEY_U
#define NANOGUI_KEY_V                  GLFW_KEY_V
#define NANOGUI_KEY_W                  GLFW_KEY_W
#define NANOGUI_KEY_X                  GLFW_KEY_X
#define NANOGUI_KEY_Y                  GLFW_KEY_Y
#define NANOGUI_KEY_Z                  GLFW_KEY_Z
#define NANOGUI_KEY_LEFT_BRACKET       GLFW_KEY_LEFT_BRACKET  /* [ */
#define NANOGUI_KEY_BACKSLASH          GLFW_KEY_BACKSLASH  /* \ */
#define NANOGUI_KEY_RIGHT_BRACKET      GLFW_KEY_RIGHT_BRACKET  /* ] */
#define NANOGUI_KEY_GRAVE_ACCENT       GLFW_KEY_GRAVE_ACCENT  /* ` */
#define NANOGUI_KEY_WORLD_1            GLFW_KEY_WORLD_1 /* non-US #1 */
#define NANOGUI_KEY_WORLD_2            GLFW_KEY_WORLD_2 /* non-US #2 */

/* Function keys */
#define NANOGUI_KEY_ESCAPE             GLFW_KEY_ESCAPE
#define NANOGUI_KEY_ENTER              GLFW_KEY_ENTER
#define NANOGUI_KEY_TAB                GLFW_KEY_TAB
#define NANOGUI_KEY_BACKSPACE          GLFW_KEY_BACKSPACE
#define NANOGUI_KEY_INSERT             GLFW_KEY_INSERT
#define NANOGUI_KEY_DELETE             GLFW_KEY_DELETE
#define NANOGUI_KEY_RIGHT              GLFW_KEY_RIGHT
#define NANOGUI_KEY_LEFT               GLFW_KEY_LEFT
#define NANOGUI_KEY_DOWN               GLFW_KEY_DOWN
#define NANOGUI_KEY_UP                 GLFW_KEY_UP
#define NANOGUI_KEY_PAGE_UP            GLFW_KEY_PAGE_UP
#define NANOGUI_KEY_PAGE_DOWN          GLFW_KEY_PAGE_DOWN
#define NANOGUI_KEY_HOME               GLFW_KEY_HOME
#define NANOGUI_KEY_END                GLFW_KEY_END
#define NANOGUI_KEY_CAPS_LOCK          GLFW_KEY_CAPS_LOCK
#define NANOGUI_KEY_SCROLL_LOCK        GLFW_KEY_SCROLL_LOCK
#define NANOGUI_KEY_NUM_LOCK           GLFW_KEY_NUM_LOCK
#define NANOGUI_KEY_PRINT_SCREEN       GLFW_KEY_PRINT_SCREEN
#define NANOGUI_KEY_PAUSE              GLFW_KEY_PAUSE
#define NANOGUI_KEY_F1                 GLFW_KEY_F1
#define NANOGUI_KEY_F2                 GLFW_KEY_F2
#define NANOGUI_KEY_F3                 GLFW_KEY_F3
#define NANOGUI_KEY_F4                 GLFW_KEY_F4
#define NANOGUI_KEY_F5                 GLFW_KEY_F5
#define NANOGUI_KEY_F6                 GLFW_KEY_F6
#define NANOGUI_KEY_F7                 GLFW_KEY_F7
#define NANOGUI_KEY_F8                 GLFW_KEY_F8
#define NANOGUI_KEY_F9                 GLFW_KEY_F9
#define NANOGUI_KEY_F10                GLFW_KEY_F10
#define NANOGUI_KEY_F11                GLFW_KEY_F11
#define NANOGUI_KEY_F12                GLFW_KEY_F12
#define NANOGUI_KEY_F13                GLFW_KEY_F13
#define NANOGUI_KEY_F14                GLFW_KEY_F14
#define NANOGUI_KEY_F15                GLFW_KEY_F15
#define NANOGUI_KEY_F16                GLFW_KEY_F16
#define NANOGUI_KEY_F17                GLFW_KEY_F17
#define NANOGUI_KEY_F18                GLFW_KEY_F18
#define NANOGUI_KEY_F19                GLFW_KEY_F19
#define NANOGUI_KEY_F20                GLFW_KEY_F20
#define NANOGUI_KEY_F21                GLFW_KEY_F21
#define NANOGUI_KEY_F22                GLFW_KEY_F22
#define NANOGUI_KEY_F23                GLFW_KEY_F23
#define NANOGUI_KEY_F24                GLFW_KEY_F24
#define NANOGUI_KEY_F25                GLFW_KEY_F25
#define NANOGUI_KEY_KP_0               GLFW_KEY_KP_0
#define NANOGUI_KEY_KP_1               GLFW_KEY_KP_1
#define NANOGUI_KEY_KP_2               GLFW_KEY_KP_2
#define NANOGUI_KEY_KP_3               GLFW_KEY_KP_3
#define NANOGUI_KEY_KP_4               GLFW_KEY_KP_4
#define NANOGUI_KEY_KP_5               GLFW_KEY_KP_5
#define NANOGUI_KEY_KP_6               GLFW_KEY_KP_6
#define NANOGUI_KEY_KP_7               GLFW_KEY_KP_7
#define NANOGUI_KEY_KP_8               GLFW_KEY_KP_8
#define NANOGUI_KEY_KP_9               GLFW_KEY_KP_9
#define NANOGUI_KEY_KP_DECIMAL         GLFW_KEY_KP_DECIMAL
#define NANOGUI_KEY_KP_DIVIDE          GLFW_KEY_KP_DIVIDE
#define NANOGUI_KEY_KP_MULTIPLY        GLFW_KEY_KP_MULTIPLY
#define NANOGUI_KEY_KP_SUBTRACT        GLFW_KEY_KP_SUBTRACT
#define NANOGUI_KEY_KP_ADD             GLFW_KEY_KP_ADD
#define NANOGUI_KEY_KP_ENTER           GLFW_KEY_KP_ENTER
#define NANOGUI_KEY_KP_EQUAL           GLFW_KEY_KP_EQUAL
#define NANOGUI_KEY_LEFT_SHIFT         GLFW_KEY_LEFT_SHIFT
#define NANOGUI_KEY_LEFT_CONTROL       GLFW_KEY_LEFT_CONTROL
#define NANOGUI_KEY_LEFT_ALT           GLFW_KEY_LEFT_ALT
#define NANOGUI_KEY_LEFT_SUPER         GLFW_KEY_LEFT_SUPER
#define NANOGUI_KEY_RIGHT_SHIFT        GLFW_KEY_RIGHT_SHIFT
#define NANOGUI_KEY_RIGHT_CONTROL      GLFW_KEY_RIGHT_CONTROL
#define NANOGUI_KEY_RIGHT_ALT          GLFW_KEY_RIGHT_ALT
#define NANOGUI_KEY_RIGHT_SUPER        GLFW_KEY_RIGHT_SUPER
#define NANOGUI_KEY_MENU               GLFW_KEY_MENU

#define NANOGUI_KEY_LAST               NANOGUI_KEY_MENU

#define NANOGUI_MOD_SHIFT           GLFW_MOD_SHIFT
#define NANOGUI_MOD_CONTROL         GLFW_MOD_CONTROL
#define NANOGUI_MOD_ALT             GLFW_MOD_ALT
#define NANOGUI_MOD_SUPER           GLFW_MOD_SUPER
#define NANOGUI_MOD_CAPS_LOCK       GLFW_MOD_CAPS_LOCK
#define NANOGUI_MOD_NUM_LOCK        GLFW_MOD_NUM_LOCK

#define NANOGUI_MOUSE_BUTTON_1 GLFW_MOUSE_BUTTON_1
#define NANOGUI_MOUSE_BUTTON_2 GLFW_MOUSE_BUTTON_2

#define NANOGUI_GAMEPAD_BUTTON_A GLFW_GAMEPAD_BUTTON_A
#define NANOGUI_GAMEPAD_BUTTON_B GLFW_GAMEPAD_BUTTON_B
#define NANOGUI_GAMEPAD_BUTTON_X GLFW_GAMEPAD_BUTTON_X
#define NANOGUI_GAMEPAD_BUTTON_Y GLFW_GAMEPAD_BUTTON_Y
#define NANOGUI_GAMEPAD_BUTTON_LEFT_BUMPER GLFW_GAMEPAD_BUTTON_LEFT_BUMPER
#define NANOGUI_GAMEPAD_BUTTON_RIGHT_BUMPER GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER
#define NANOGUI_GAMEPAD_BUTTON_BACK GLFW_GAMEPAD_BUTTON_BACK
#define NANOGUI_GAMEPAD_BUTTON_START GLFW_GAMEPAD_BUTTON_START
#define NANOGUI_GAMEPAD_BUTTON_GUIDE GLFW_GAMEPAD_BUTTON_GUIDE
#define NANOGUI_GAMEPAD_BUTTON_LEFT_THUMB GLFW_GAMEPAD_BUTTON_LEFT_THUMB
#define NANOGUI_GAMEPAD_BUTTON_RIGHT_THUMB GLFW_GAMEPAD_BUTTON_RIGHT_THUMB
#define NANOGUI_GAMEPAD_BUTTON_DPAD_UP GLFW_GAMEPAD_BUTTON_DPAD_UP
#define NANOGUI_GAMEPAD_BUTTON_DPAD_RIGHT GLFW_GAMEPAD_BUTTON_DPAD_RIGHT
#define NANOGUI_GAMEPAD_BUTTON_DPAD_DOWN GLFW_GAMEPAD_BUTTON_DPAD_DOWN
#define NANOGUI_GAMEPAD_BUTTON_DPAD_LEFT GLFW_GAMEPAD_BUTTON_DPAD_LEFT

#define NANOGUI_GAMEPAD_AXIS_LEFT_X GLFW_GAMEPAD_AXIS_LEFT_X
#define NANOGUI_GAMEPAD_AXIS_LEFT_Y GLFW_GAMEPAD_AXIS_LEFT_Y
#define NANOGUI_GAMEPAD_AXIS_RIGHT_X GLFW_GAMEPAD_AXIS_RIGHT_X
#define NANOGUI_GAMEPAD_AXIS_RIGHT_Y GLFW_GAMEPAD_AXIS_RIGHT_Y
#define NANOGUI_GAMEPAD_AXIS_LEFT_TRIGGER GLFW_GAMEPAD_AXIS_LEFT_TRIGGER
#define NANOGUI_GAMEPAD_AXIS_RIGHT_TRIGGER GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER

#else
#if __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#elif __SWITCH__
#include <glad/glad.h>
#elif defined(__LIBRETRO__)
#include "glsym/glsym.h"
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#define NANOGUI_RELEASE 0
#define NANOGUI_PRESS 1
#define NANOGUI_REPEAT 2

#define NANOGUI_KEY_UNKNOWN            -1

/* Printable keys */
#define NANOGUI_KEY_SPACE              32
#define NANOGUI_KEY_APOSTROPHE         39  /* ' */
#define NANOGUI_KEY_COMMA              44  /* , */
#define NANOGUI_KEY_MINUS              45  /* - */
#define NANOGUI_KEY_PERIOD             46  /* . */
#define NANOGUI_KEY_SLASH              47  /* / */
#define NANOGUI_KEY_0                  48
#define NANOGUI_KEY_1                  49
#define NANOGUI_KEY_2                  50
#define NANOGUI_KEY_3                  51
#define NANOGUI_KEY_4                  52
#define NANOGUI_KEY_5                  53
#define NANOGUI_KEY_6                  54
#define NANOGUI_KEY_7                  55
#define NANOGUI_KEY_8                  56
#define NANOGUI_KEY_9                  57
#define NANOGUI_KEY_SEMICOLON          59  /* ; */
#define NANOGUI_KEY_EQUAL              61  /* = */
#define NANOGUI_KEY_A                  65
#define NANOGUI_KEY_B                  66
#define NANOGUI_KEY_C                  67
#define NANOGUI_KEY_D                  68
#define NANOGUI_KEY_E                  69
#define NANOGUI_KEY_F                  70
#define NANOGUI_KEY_G                  71
#define NANOGUI_KEY_H                  72
#define NANOGUI_KEY_I                  73
#define NANOGUI_KEY_J                  74
#define NANOGUI_KEY_K                  75
#define NANOGUI_KEY_L                  76
#define NANOGUI_KEY_M                  77
#define NANOGUI_KEY_N                  78
#define NANOGUI_KEY_O                  79
#define NANOGUI_KEY_P                  80
#define NANOGUI_KEY_Q                  81
#define NANOGUI_KEY_R                  82
#define NANOGUI_KEY_S                  83
#define NANOGUI_KEY_T                  84
#define NANOGUI_KEY_U                  85
#define NANOGUI_KEY_V                  86
#define NANOGUI_KEY_W                  87
#define NANOGUI_KEY_X                  88
#define NANOGUI_KEY_Y                  89
#define NANOGUI_KEY_Z                  90
#define NANOGUI_KEY_LEFT_BRACKET       91  /* [ */
#define NANOGUI_KEY_BACKSLASH          92  /* \ */
#define NANOGUI_KEY_RIGHT_BRACKET      93  /* ] */
#define NANOGUI_KEY_GRAVE_ACCENT       96  /* ` */
#define NANOGUI_KEY_WORLD_1            161 /* non-US #1 */
#define NANOGUI_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define NANOGUI_KEY_ESCAPE             256
#define NANOGUI_KEY_ENTER              257
#define NANOGUI_KEY_TAB                258
#define NANOGUI_KEY_BACKSPACE          259
#define NANOGUI_KEY_INSERT             260
#define NANOGUI_KEY_DELETE             261
#define NANOGUI_KEY_RIGHT              262
#define NANOGUI_KEY_LEFT               263
#define NANOGUI_KEY_DOWN               264
#define NANOGUI_KEY_UP                 265
#define NANOGUI_KEY_PAGE_UP            266
#define NANOGUI_KEY_PAGE_DOWN          267
#define NANOGUI_KEY_HOME               268
#define NANOGUI_KEY_END                269
#define NANOGUI_KEY_CAPS_LOCK          280
#define NANOGUI_KEY_SCROLL_LOCK        281
#define NANOGUI_KEY_NUM_LOCK           282
#define NANOGUI_KEY_PRINT_SCREEN       283
#define NANOGUI_KEY_PAUSE              284
#define NANOGUI_KEY_F1                 290
#define NANOGUI_KEY_F2                 291
#define NANOGUI_KEY_F3                 292
#define NANOGUI_KEY_F4                 293
#define NANOGUI_KEY_F5                 294
#define NANOGUI_KEY_F6                 295
#define NANOGUI_KEY_F7                 296
#define NANOGUI_KEY_F8                 297
#define NANOGUI_KEY_F9                 298
#define NANOGUI_KEY_F10                299
#define NANOGUI_KEY_F11                300
#define NANOGUI_KEY_F12                301
#define NANOGUI_KEY_F13                302
#define NANOGUI_KEY_F14                303
#define NANOGUI_KEY_F15                304
#define NANOGUI_KEY_F16                305
#define NANOGUI_KEY_F17                306
#define NANOGUI_KEY_F18                307
#define NANOGUI_KEY_F19                308
#define NANOGUI_KEY_F20                309
#define NANOGUI_KEY_F21                310
#define NANOGUI_KEY_F22                311
#define NANOGUI_KEY_F23                312
#define NANOGUI_KEY_F24                313
#define NANOGUI_KEY_F25                314
#define NANOGUI_KEY_KP_0               320
#define NANOGUI_KEY_KP_1               321
#define NANOGUI_KEY_KP_2               322
#define NANOGUI_KEY_KP_3               323
#define NANOGUI_KEY_KP_4               324
#define NANOGUI_KEY_KP_5               325
#define NANOGUI_KEY_KP_6               326
#define NANOGUI_KEY_KP_7               327
#define NANOGUI_KEY_KP_8               328
#define NANOGUI_KEY_KP_9               329
#define NANOGUI_KEY_KP_DECIMAL         330
#define NANOGUI_KEY_KP_DIVIDE          331
#define NANOGUI_KEY_KP_MULTIPLY        332
#define NANOGUI_KEY_KP_SUBTRACT        333
#define NANOGUI_KEY_KP_ADD             334
#define NANOGUI_KEY_KP_ENTER           335
#define NANOGUI_KEY_KP_EQUAL           336
#define NANOGUI_KEY_LEFT_SHIFT         340
#define NANOGUI_KEY_LEFT_CONTROL       341
#define NANOGUI_KEY_LEFT_ALT           342
#define NANOGUI_KEY_LEFT_SUPER         343
#define NANOGUI_KEY_RIGHT_SHIFT        344
#define NANOGUI_KEY_RIGHT_CONTROL      345
#define NANOGUI_KEY_RIGHT_ALT          346
#define NANOGUI_KEY_RIGHT_SUPER        347
#define NANOGUI_KEY_MENU               348

#define NANOGUI_KEY_LAST               NANOGUI_KEY_MENU

#define NANOGUI_MOD_SHIFT           0x0001
#define NANOGUI_MOD_CONTROL         0x0002
#define NANOGUI_MOD_ALT             0x0004
#define NANOGUI_MOD_SUPER           0x0008
#define NANOGUI_MOD_CAPS_LOCK       0x0010
#define NANOGUI_MOD_NUM_LOCK        0x0020

#define NANOGUI_MOUSE_BUTTON_1 0
#define NANOGUI_MOUSE_BUTTON_2 1

#define NANOGUI_GAMEPAD_BUTTON_A 0
#define NANOGUI_GAMEPAD_BUTTON_B 1
#define NANOGUI_GAMEPAD_BUTTON_X 2
#define NANOGUI_GAMEPAD_BUTTON_Y 3
#define NANOGUI_GAMEPAD_BUTTON_LEFT_BUMPER 4
#define NANOGUI_GAMEPAD_BUTTON_RIGHT_BUMPER 5
#define NANOGUI_GAMEPAD_BUTTON_BACK 6
#define NANOGUI_GAMEPAD_BUTTON_START 7
#define NANOGUI_GAMEPAD_BUTTON_GUIDE 8
#define NANOGUI_GAMEPAD_BUTTON_LEFT_THUMB 9
#define NANOGUI_GAMEPAD_BUTTON_RIGHT_THUMB 10
#define NANOGUI_GAMEPAD_BUTTON_DPAD_UP 11
#define NANOGUI_GAMEPAD_BUTTON_DPAD_RIGHT 12
#define NANOGUI_GAMEPAD_BUTTON_DPAD_DOWN 13
#define NANOGUI_GAMEPAD_BUTTON_DPAD_LEFT 14

#define NANOGUI_GAMEPAD_AXIS_LEFT_X 0
#define NANOGUI_GAMEPAD_AXIS_LEFT_Y 1
#define NANOGUI_GAMEPAD_AXIS_RIGHT_X 2
#define NANOGUI_GAMEPAD_AXIS_RIGHT_Y 3
#define NANOGUI_GAMEPAD_AXIS_LEFT_TRIGGER 4
#define NANOGUI_GAMEPAD_AXIS_RIGHT_TRIGGER 5

#endif

#if defined(NANOGUI_USE_GLES)
#  if NANOGUI_GLES_VERSION == 2
#    include <GLES2/gl2ext.h>
#  elif NANOGUI_GLES_VERSION == 3
#    include <GLES3/gl2ext.h>
#  endif
#endif

#include <nanovg.h>

// Special treatment of linux Nvidia opengl headers
#if !defined(_WIN32) && !defined(__APPLE__) && defined(NANOGUI_USE_OPENGL)
  #if !defined(GL_UNIFORM_BUFFER)
    #warning NanoGUI suspects you have the NVIDIA OpenGL headers installed.  \
             Compilation will likely fail. If it does, you have two choices: \
             (1) Re-install the mesa-lib_gl header files.                    \
             (2) Compile with NANOGUI_USE_GLAD.
  #endif
#endif

NAMESPACE_BEGIN(nanogui)

/**
 * \brief Determine whether an icon ID is a texture loaded via ``nvg_image_icon``.
 *
 * \rst
 * The implementation defines all ``value < 1024`` as image icons, and
 * everything ``>= 1024`` as an Entypo icon (see :ref:`file_nanogui_entypo.h`).
 * The value ``1024`` exists to provide a generous buffer on how many images
 * may have been loaded by NanoVG.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is an image icon.
 */
inline bool nvg_is_image_icon(int value) { return value < 1024; }

/**
 * \brief Determine whether an icon ID is a font-based icon (e.g. from ``entypo.ttf``).
 *
 * \rst
 * See :func:`nanogui::nvg_is_image_icon` for details.
 * \endrst
 *
 * \param value
 *     The integral value of the icon.
 *
 * \return
 *     Whether or not this is a font icon (from ``entypo.ttf``).
 */
inline bool nvg_is_font_icon(int value) { return value >= 1024; }


/// Check for OpenGL errors and warn if one is found (returns 'true' in that case')
extern NANOGUI_EXPORT bool nanogui_check_glerror(const char *cmd);

NAMESPACE_END(nanogui)
