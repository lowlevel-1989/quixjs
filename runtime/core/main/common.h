/*
 * quixjs - A lightweight JS runtime using SDL2 and JavaScriptCore
 * Copyright (C) 2025  Vinicio Valbuena (lowlevel.1989)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

// Lo que comparten las partes de main(): los #include y las constantes. Ver
// README.md.

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "platform/platform.h"
#include "core/url.h"
#include "symbols/symbols.h"
#include "cheats/cheats.h"
#include "backend/toast.h"
#include "js/js.h"
#include "core/args.h"
#include "core/config_loader.h"
#include "core/printer.h"
#include "core/compiler.h"
#include "core/runner.h"
#include "core/timer.h"
#include "core/app_context.h"
#include "core/gen_config.h"
#include "core/sandbox.h"
#include "core/input_script.h"
#include "core/debug_server.h"
#include "binding/timer.h"
#include "binding/events.h"
#include "binding/canvas.h"
#include "core/worker.h"
#include "binding/image.h"
#include "binding/document.h"
#include "binding/window.h"
#include "binding/xhr.h"
#include "binding/quix.h"
#include "binding/lan.h"
#include "binding/polyfill.h"
#include "binding/script.h"
#include "binding/nwjs.h"
#include "binding/blob.h"
#include "binding/audio.h"
#include "binding/webgl.h"
#include "binding/video.h"
#include "audio/mixer.h"
#include "backend/backend.h"
#include "gfx/gl.h"
#include "gfx/shader_cache.h"
#include "gfx/image_scale.h"
#include "gfx/driver.h"
#include "gfx/image_loader.h"
#include "gfx/raster.h"
#include "gfx/font.h"
#include "launcher/launcher.h"
#include "launcher/ui.h"
#include "launcher/settings.h"
#include "transfer/saves.h"
#include "achievements/achievements.h"
#include "hooks/hooks.h"
#include "engines/engine_api.h"
#include "core/profiler.h"
#include "core/watchdog.h"
#include "core/memstats.h"
#include "binding/console.h"

#define EXIT_TEST_TIMEOUT 3
#define MAX_SCREEN_CANVASES 8
