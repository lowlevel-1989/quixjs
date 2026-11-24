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
#include "core/url/url.h"
#include "game/symbols/symbols.h"
#include "game/cheats/cheats.h"
#include "backend/toast.h"
#include "js/js.h"
#include "core/config/args.h"
#include "core/config/loader.h"
#include "core/config/printer.h"
#include "core/scripts/load.h"
#include "core/scripts/run.h"
#include "core/timer.h"
#include "core/app_context.h"
#include "core/config/gen_config.h"
#include "core/sandbox.h"
#include "core/debug/input_script.h"
#include "core/debug/debug_server/debug_server.h"
#include "binding/timer.h"
#include "binding/events.h"
#include "binding/canvas.h"
#include "core/worker.h"
#include "binding/image/image.h"
#include "binding/document.h"
#include "binding/window.h"
#include "binding/xhr.h"
#include "binding/quix/quix.h"
#include "binding/polyfill.h"
#include "binding/script.h"
#include "binding/nwjs.h"
#include "binding/webview2.h"
#include "rom/rom.h"
#include "binding/blob.h"
#include "binding/audio/audio.h"
#include "binding/webgl.h"
#include "binding/video.h"
#include "audio/mixer.h"
#include "backend/backend.h"
#include "gfx/gl/gl.h"
#include "gfx/gl/shader_cache.h"
#include "gfx/soft/image_scale.h"
#include "gfx/gl/driver.h"
#include "gfx/soft/image_loader.h"
#include "gfx/soft/raster.h"
#include "gfx/soft/font.h"
#include "launcher/launcher.h"
#include "launcher/ui/ui.h"
#include "launcher/settings.h"
#include "transfer/saves/saves.h"
#include "game/achievements/achievements.h"
#include "game/hooks/hooks.h"
#include "engines/api.h"
#include "core/debug/profiler.h"
#include "core/watchdog.h"
#include "core/debug/memstats.h"
#include "binding/console.h"

#define EXIT_TEST_TIMEOUT 3
#define MAX_SCREEN_CANVASES 8
