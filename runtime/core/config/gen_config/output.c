// gen_config/output.c: escribir el toml (printer_print).

static void toml_str(FILE *out, const char *s) {
  fputc('"', out);
  for (; *s; s++) {
    // los saltos de linea y tabuladores como escapes de TOML (el codigo de
    // un <script> los necesita); el resto de controles, fuera
    if (*s == '\n') { fputs("\\n", out); continue; }
    if (*s == '\t') { fputs("\\t", out); continue; }
    if (*s == '\r') { fputs("\\r", out); continue; }
    if (*s == '"' || *s == '\\') fputc('\\', out);
    if ((unsigned char)*s < 0x20) continue;
    fputc(*s, out);
  }
  fputc('"', out);
}

static int is_lib(const char *p) {
  return strncmp(p, "libs/", 5) == 0 || strncmp(p, "lib/", 4) == 0 || strstr(p, "/libs/") || strstr(p, "/lib/");
}

static void print_list(FILE *out, const char *key, char **items, int from, int to) {
  fprintf(out, "%-8s = [", key);
  for (int i = from; i < to; i++) {
    fprintf(out, "%s\n  ", i == from ? "" : ",");
    toml_str(out, items[i]);
  }
  fprintf(out, "%s]\n", to > from ? "\n" : "");
}

static void slug(const char *in, char *out, size_t size) {
  size_t o = 0;
  for (; *in && o < size - 1; in++) {
    if (isalnum((unsigned char)*in)) out[o++] = (char)tolower((unsigned char)*in);
    else if (o > 0 && out[o - 1] != '-') out[o++] = '-';
  }
  while (o > 0 && out[o - 1] == '-') o--;
  out[o] = 0;
  if (!o) snprintf(out, size, "game");
}

// &amp; &lt; &gt; &quot; &#39; del <title>
static void decode_entities(char *s) {
  static const char *ent[][2] = { { "&amp;", "&" }, { "&lt;", "<" }, { "&gt;", ">" },
                                  { "&quot;", "\"" }, { "&#39;", "'" }, { "&apos;", "'" } };
  char *w = s;
  for (char *r = s; *r;) {
    int done = 0;
    for (size_t i = 0; i < sizeof(ent) / sizeof(ent[0]); i++) {
      size_t n = strlen(ent[i][0]);
      if (strncmp(r, ent[i][0], n) == 0) {
        *w++ = ent[i][1][0];
        r += n;
        done = 1;
        break;
      }
    }
    if (!done) *w++ = *r++;
  }
  *w = 0;
}

static void printer_print(FILE *out, const char *dir, GameInfo *gi) {
  decode_entities(gi->title);
  char dirname[1024], name[128];
  snprintf(dirname, sizeof(dirname), "%s", dir);
  normalize(dirname);
  const char *last = strrchr(dirname, '/');
  slug(last ? last + 1 : dirname, name, sizeof(name));

  fprintf(out, "# config.toml generado por: quixjs --gen-config ");
  fprintf(out, "%s\n", dir);
  fprintf(out, "# Se puede editar: es lo que describe el juego (quixjs -c este archivo).\n");
  if (gi->title[0]) fprintf(out, "# %s\n", gi->title);
  fprintf(out, "\n[engine]\n");
  fprintf(out, "name    = ");
  toml_str(out, gi->engine);
  fprintf(out, "\n");
  if (gi->rm_version[0]) {
    fprintf(out, "version = ");
    toml_str(out, gi->rm_version);
    fprintf(out, "\n");
  }
  if (gi->pixi[0]) {
    fprintf(out, "pixi    = ");
    toml_str(out, gi->pixi);
    fprintf(out, "\n");
  }
  fprintf(out, "nwjs    = %s%s\n", gi->nwjs ? "true" : "false",
          gi->nwjs ? "   # package.json de nw.js: require('fs'), process..." : "");
  if (gi->canvas_id[0] && strcmp(gi->canvas_id, "canvas") != 0) {
    fprintf(out, "canvas_id = ");
    toml_str(out, gi->canvas_id);
    fprintf(out, "   # el <canvas> del index.html\n");
  }

  fprintf(out, "\n[game]\n");
  // el titulo: el del menu de --launcher (y el id del juego si no hay id)
  fprintf(out, "title     = ");
  toml_str(out, gi->title[0] ? gi->title : name);
  fprintf(out, "\ngame_dir  = ");
  toml_str(out, gi->base);
  fprintf(out, "\nwidth     = %d   # %s\n", gi->width, gi->size_from);
  fprintf(out, "height    = %d\n", gi->height);


  // RPG Maker en nw.js guarda en dirname(index.html)/save
  const char *save_pattern = NULL;
  if (gi->nwjs && strcmp(gi->engine, "rpgmaker-mv") == 0) save_pattern = "save/*.rpgsave";
  if (gi->nwjs && strcmp(gi->engine, "rpgmaker-mz") == 0) save_pattern = "save/*.rmmzsave";
  if (save_pattern) {
    // RPG Maker guarda en save/, al lado de su index.html (game_dir)
    fprintf(out, "\n# las partidas que pasa Transfer Save\n[saves]\n");
    fprintf(out, "dir   = \"save\"   # la carpeta de las partidas, desde [game] game_dir\n");
    fprintf(out, "files = [");
    toml_str(out, strrchr(save_pattern, '/') + 1);
    fprintf(out, "]   # dentro de dir\n");
  }

  if (gi->font_count > 0) {
    fprintf(out, "\n# fuentes de los @font-face de su CSS\n[fonts]\n");
    for (int i = 0; i < gi->font_count; i++) {
      toml_str(out, gi->font_family[i]);
      fprintf(out, " = ");
      toml_str(out, gi->font_path[i]);
      fprintf(out, "\n");
    }
  }

  // libs: las del principio que estan en una carpeta lib/libs; el resto en
  // includes y el ultimo es entry (el mismo orden que el index.html)
  int libs = 0;
  while (libs < gi->script_count - 1 && is_lib(gi->scripts[libs])) libs++;
  fprintf(out, "\n# los <script> del index.html, en el mismo orden\n[scripts]\n");
  print_list(out, "libs", gi->scripts, 0, libs);
  print_list(out, "includes", gi->scripts, libs, gi->script_count - 1);
  fprintf(out, "%-8s = ", "entry");
  toml_str(out, gi->scripts[gi->script_count - 1]);
  fprintf(out, "\n");

  if (gi->el_count > 0) {
    fprintf(out, "\n# los elementos con id del <body> del index.html: quixjs los crea en el\n"
                 "# body cuando el juego los pide (getElementById)\n"
                 "[html]\nelements = [\n");
    for (int i = 0; i < gi->el_count; i++) {
      const BodyElement *el = &gi->elements[i];
      fprintf(out, "  { tag = ");
      toml_str(out, el->tag);
      fprintf(out, ", id = ");
      toml_str(out, el->id);
      if (el->type[0]) {
        fprintf(out, ", type = ");
        toml_str(out, el->type);
      }
      fprintf(out, " },\n");
    }
    fprintf(out, "]\n");
  }

  if (gi->greenworks) {
    fprintf(out,
      "\n"
      "# The game uses Steam achievements (greenworks.js). quixjs answers for\n"
      "# Steam and writes each achievement the game unlocks, once, to\n"
      "# steam-achievements.txt in the saves folder. Play, read that list and add a\n"
      "# symbol and an [[achievements]] for each id; the title and description\n"
      "# are on the Steam page of the game.\n"
      "#\n"
      "# [symbols]\n"
      "# ach_example = \"path:quix.steam.achievements.GAME_ID\"\n"
      "#\n"
      "# [[achievements]]\n"
      "# id          = \"ach_example\"\n"
      "# title       = \"Title from the Steam page\"\n"
      "# description = \"Description from the Steam page\"\n"
      "# conditions  = [\"ach_example\"]\n");
  }
}
