// gen_config/engine.c: el motor del juego (RPG Maker MV o MZ y su version)
// y la version de PIXI, por los scripts que carga.

// Primer texto entre comillas despues de "key"
static int quoted_after(const char *text, const char *key, char *out, size_t size) {
  const char *p = strstr(text, key);
  if (!p) return 0;
  p += strlen(key);
  while (*p && *p != '"' && *p != '\'' && *p != '\n') p++;
  if (*p != '"' && *p != '\'') return 0;
  char q = *p++;
  size_t n = strcspn(p, q == '"' ? "\"" : "'");
  if (n >= size) n = size - 1;
  memcpy(out, p, n);
  out[n] = 0;
  return 1;
}

static int number_after(const char *text, const char *key) {
  const char *p = strstr(text, key);
  if (!p) return 0;
  p += strlen(key);
  while (*p && !isdigit((unsigned char)*p) && *p != ';' && *p != '\n') p++;
  return atoi(p);
}

static void detect_engine(GameInfo *gi) {
  char path[1024];
  strcpy(gi->engine, "html5");

  join(path, sizeof(path), gi->base, "js/rmmz_core.js");
  char *core = read_text(path);
  if (core) {
    strcpy(gi->engine, "rpgmaker-mz");
    quoted_after(core, "Utils.RPGMAKER_VERSION", gi->rm_version, sizeof(gi->rm_version));
    free(core);
  } else {
    join(path, sizeof(path), gi->base, "js/rpg_core.js");
    core = read_text(path);
    if (core) {
      strcpy(gi->engine, "rpgmaker-mv");
      quoted_after(core, "Utils.RPGMAKER_VERSION", gi->rm_version, sizeof(gi->rm_version));
      free(core);
    }
  }

  // tamano por defecto del motor si no lo dice package.json
  if (!gi->width) {
    if (strcmp(gi->engine, "rpgmaker-mv") == 0) {
      join(path, sizeof(path), gi->base, "js/rpg_managers.js");
      char *m = read_text(path);
      if (m) {
        gi->width = number_after(m, "SceneManager._screenWidth");
        gi->height = number_after(m, "SceneManager._screenHeight");
        free(m);
        if (gi->width) snprintf(gi->size_from, sizeof(gi->size_from), "js/rpg_managers.js");
      }
    } else if (strcmp(gi->engine, "rpgmaker-mz") == 0) {
      join(path, sizeof(path), gi->base, "data/System.json");
      char *sys = read_text(path);
      if (sys) {
        gi->width = number_after(sys, "\"screenWidth\"");
        gi->height = number_after(sys, "\"screenHeight\"");
        free(sys);
        if (gi->width) snprintf(gi->size_from, sizeof(gi->size_from), "data/System.json");
      }
    }
  }
  if (gi->width <= 0 || gi->height <= 0) {
    gi->width = 640;
    gi->height = 480;
    snprintf(gi->size_from, sizeof(gi->size_from), "por defecto");
  }

  // version de PixiJS por la cabecera de su archivo
  for (int i = 0; i < gi->script_count; i++) {
    const char *name = strrchr(gi->scripts[i], '/');
    name = name ? name + 1 : gi->scripts[i];
    if (strncasecmp(name, "pixi", 4) != 0 || strcasestr(name, "tilemap") || strcasestr(name, "picture") ||
        strcasestr(name, "filter")) continue;
    join(path, sizeof(path), gi->base, gi->scripts[i]);
    FILE *f = fopen(path, "rb");
    if (!f) continue;
    char head[512] = "";
    size_t n = fread(head, 1, sizeof(head) - 1, f);
    head[n] = 0;
    fclose(f);
    char *v = strstr(head, " - v");
    if (v) {
      v += 4;
      size_t len = strspn(v, "0123456789.");
      if (len >= sizeof(gi->pixi)) len = sizeof(gi->pixi) - 1;
      memcpy(gi->pixi, v, len);
      gi->pixi[len] = 0;
      break;
    }
  }
}
