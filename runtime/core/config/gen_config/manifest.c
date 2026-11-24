// gen_config/manifest.c: el package.json de nw.js (titulo, tamano de la
// ventana, la pagina de entrada).

static JSValue parse_json_file(JSContext *ctx, const char *path) {
  char *text = read_text(path);
  if (!text) return JS_UNDEFINED;
  JSValue v = JS_ParseJSON(ctx, text, strlen(text), path);
  free(text);
  if (JS_IsException(v)) {
    JSValue e = JS_GetException(ctx);
    JS_FreeValue(ctx, e);
    return JS_UNDEFINED;
  }
  return v;
}

static int get_int(JSContext *ctx, JSValueConst obj, const char *a, const char *b) {
  JSValue x = JS_GetPropertyStr(ctx, obj, a);
  JSValue y = b ? JS_GetPropertyStr(ctx, x, b) : JS_DupValue(ctx, x);
  int r = 0;
  if (JS_IsNumber(y)) JS_ToInt32(ctx, &r, y);
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
  return r;
}

static void get_str(JSContext *ctx, JSValueConst obj, const char *a, const char *b, char *out, size_t size) {
  out[0] = 0;
  JSValue x = JS_GetPropertyStr(ctx, obj, a);
  JSValue y = b ? JS_GetPropertyStr(ctx, x, b) : JS_DupValue(ctx, x);
  if (JS_IsString(y)) {
    const char *s = JS_ToCString(ctx, y);
    if (s) {
      snprintf(out, size, "%s", s);
      JS_FreeCString(ctx, s);
    }
  }
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
}

// package.json de nw.js: { "main": "index.html", "window": { width, height } }
static int read_manifest(JSContext *ctx, const char *dir, GameInfo *gi) {
  char path[1024];
  join(path, sizeof(path), dir, "package.json");
  if (!file_exists(path)) return 0;
  JSValue pkg = parse_json_file(ctx, path);
  if (!JS_IsObject(pkg)) return 0;

  char main[512];
  get_str(ctx, pkg, "main", NULL, main, sizeof(main));
  size_t ml = strlen(main);
  int ok = ml > 5 && strcasecmp(main + ml - 5, ".html") == 0;
  if (ok) {
    join(gi->html, sizeof(gi->html), dir, main);
    normalize(gi->html);
    gi->nwjs = 1;
    int w = get_int(ctx, pkg, "window", "width");
    int h = get_int(ctx, pkg, "window", "height");
    if (w > 0 && h > 0) {
      gi->width = w;
      gi->height = h;
      snprintf(gi->size_from, sizeof(gi->size_from), "package.json");
    }
    get_str(ctx, pkg, "window", "title", gi->title, sizeof(gi->title));
  }
  JS_FreeValue(ctx, pkg);
  return ok;
}
