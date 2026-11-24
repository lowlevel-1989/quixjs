// gen_config/html.c: el index.html: los <script>, las fuentes de su CSS, el
// id del <canvas> y los elementos con id del <body>.

// Valor de un atributo dentro de una etiqueta (sin distinguir mayusculas)
static int tag_attr(const char *tag, const char *tag_end, const char *name, char *out, size_t size) {
  size_t nl = strlen(name);
  for (const char *p = tag; p + nl < tag_end; p++) {
    if (strncasecmp(p, name, nl) != 0 || !isspace((unsigned char)p[-1])) continue;
    const char *q = p + nl;
    while (q < tag_end && isspace((unsigned char)*q)) q++;
    if (*q != '=') continue;
    q++;
    while (q < tag_end && isspace((unsigned char)*q)) q++;
    char quote = (*q == '"' || *q == '\'') ? *q++ : 0;
    const char *e = q;
    while (e < tag_end && (quote ? *e != quote : (!isspace((unsigned char)*e) && *e != '>'))) e++;
    size_t len = (size_t)(e - q);
    if (len >= size) len = size - 1;
    memcpy(out, q, len);
    out[len] = 0;
    return 1;
  }
  return 0;
}

static int is_remote(const char *u) {
  return strncmp(u, "http://", 7) == 0 || strncmp(u, "https://", 8) == 0 || strncmp(u, "//", 2) == 0;
}

static void read_css_fonts(const char *base, const char *css_rel, GameInfo *gi) {
  char path[1024], css_dir[1024];
  join(path, sizeof(path), base, css_rel);
  char *css = read_text(path);
  if (!css) return;
  dir_of(css_dir, sizeof(css_dir), css_rel);

  for (char *p = css; (p = strcasestr(p, "@font-face")) != NULL; p++) {
    char *open = strchr(p, '{');
    char *close = open ? strchr(open, '}') : NULL;
    if (!close) break;
    *close = 0;

    char family[128] = "", url[512] = "";
    char *f = strcasestr(open, "font-family");
    if (f && (f = strchr(f, ':'))) {
      f++;
      while (isspace((unsigned char)*f) || *f == '"' || *f == '\'') f++;
      size_t n = strcspn(f, ";\"'\n");
      if (n >= sizeof(family)) n = sizeof(family) - 1;
      memcpy(family, f, n);
      family[n] = 0;
      while (n > 0 && isspace((unsigned char)family[n - 1])) family[--n] = 0;
    }
    char *u = strcasestr(open, "url(");
    if (u) {
      u += 4;
      while (isspace((unsigned char)*u) || *u == '"' || *u == '\'') u++;
      size_t n = strcspn(u, "\"')");
      if (n >= sizeof(url)) n = sizeof(url) - 1;
      memcpy(url, u, n);
      url[n] = 0;
    }
    *close = '}';
    p = close;

    if (!family[0] || !url[0] || is_remote(url) || gi->font_count >= MAX_ITEMS) continue;
    char rel[1024];
    join(rel, sizeof(rel), strcmp(css_dir, ".") ? css_dir : "", url);
    normalize(rel);
    gi->font_family[gi->font_count] = strdup(family);
    gi->font_path[gi->font_count] = strdup(rel);
    gi->font_count++;
  }
  free(css);
}

static void read_html(GameInfo *gi) {
  char *html = read_text(gi->html);
  if (!html) return;

  for (char *p = html; (p = strcasestr(p, "<script")) != NULL; p += 7) {
    char *end = strchr(p, '>');
    if (!end) break;
    char src[512], type[64];
    if (tag_attr(p, end, "src", src, sizeof(src)) && src[0] && !is_remote(src) && gi->script_count < MAX_ITEMS) {
      char *q = strpbrk(src, "?#");
      if (q) *q = 0;
      normalize(src);
      gi->scripts[gi->script_count++] = strdup(src);
      continue;
    }
    // un <script> con el codigo dentro: una data: URL con ese codigo, en su
    // orden (solo JavaScript normal: ni modulos ni plantillas o JSON)
    if (tag_attr(p, end, "src", src, sizeof(src))) continue;
    if (tag_attr(p, end, "type", type, sizeof(type)) && type[0] && strcasecmp(type, "text/javascript") != 0 &&
        strcasecmp(type, "application/javascript") != 0) {
      continue;
    }
    char *close = strcasestr(end + 1, "</script");
    if (!close || gi->script_count >= MAX_ITEMS) continue;
    const char *code = end + 1;
    size_t n = (size_t)(close - code);
    int blank = 1;
    for (size_t i = 0; i < n && blank; i++) blank = isspace((unsigned char)code[i]) != 0;
    if (blank) continue;
    // "data:text/javascript," y el codigo con % como %25 (lo unico que
    // url_decode_data cambia)
    size_t pct = 0;
    for (size_t i = 0; i < n; i++) pct += code[i] == '%';
    char *url = malloc(n + 2 * pct + 32);
    if (!url) continue;
    size_t o = (size_t)sprintf(url, "data:text/javascript,");
    for (size_t i = 0; i < n; i++) {
      if (code[i] == '%') {
        memcpy(url + o, "%25", 3);
        o += 3;
      } else {
        url[o++] = code[i];
      }
    }
    url[o] = 0;
    gi->scripts[gi->script_count++] = url;
  }

  for (char *p = html; (p = strcasestr(p, "<link")) != NULL; p += 5) {
    char *end = strchr(p, '>');
    if (!end) break;
    char rel[64], href[512];
    if (tag_attr(p, end, "rel", rel, sizeof(rel)) && strcasecmp(rel, "stylesheet") == 0 &&
        tag_attr(p, end, "href", href, sizeof(href)) && !is_remote(href)) {
      normalize(href);
      read_css_fonts(gi->base, href, gi);
    }
  }

  if (!gi->title[0]) {
    char *t = strcasestr(html, "<title>");
    if (t) {
      t += 7;
      size_t n = strcspn(t, "<");
      if (n >= sizeof(gi->title)) n = sizeof(gi->title) - 1;
      memcpy(gi->title, t, n);
      gi->title[n] = 0;
    }
  }
  free(html);
}


// el nombre de la etiqueta que empieza en p, en minusculas ("" si no es una)
static void tag_name(const char *p, char *out, size_t size) {
  size_t n = 0;
  while (isalnum((unsigned char)p[n]) && n + 1 < size) {
    out[n] = (char)tolower((unsigned char)p[n]);
    n++;
  }
  out[n] = 0;
}

// los elementos con id del <body>: su etiqueta y su type
static void find_body_elements(const char *html, GameInfo *gi) {
  const char *p = strcasestr(html, "<body");
  while (p && (p = strchr(p + 1, '<')) != NULL && gi->el_count < 32) {
    const char *end = strchr(p, '>');
    if (!end) return;
    char tag[16];
    tag_name(p + 1, tag, sizeof(tag));
    if (strcmp(tag, "script") == 0) {   // su codigo no es HTML
      p = strcasestr(end, "</script");
      continue;
    }
    char id[64] = "";
    if (!tag[0] || strcmp(tag, "body") == 0 || !tag_attr(p, end, "id", id, sizeof(id)) || !id[0])
      continue;
    BodyElement *el = &gi->elements[gi->el_count++];
    memset(el, 0, sizeof(*el));
    snprintf(el->tag, sizeof(el->tag), "%s", tag);
    snprintf(el->id, sizeof(el->id), "%s", id);
    tag_attr(p, end, "type", el->type, sizeof(el->type));
  }
}

// el id del primer <canvas id="..."> del index.html
static void find_canvas_id(const char *html, GameInfo *gi) {
  const char *p = html;
  while ((p = strstr(p, "<canvas")) != NULL) {
    const char *end = strchr(p, '>');
    if (!end) return;
    const char *id = strstr(p, "id=");
    if (id && id < end && (id[3] == '"' || id[3] == '\'')) {
      char q = id[3];
      const char *s = id + 4;
      const char *e = strchr(s, q);
      if (e && e < end && (size_t)(e - s) < sizeof(gi->canvas_id)) {
        memcpy(gi->canvas_id, s, e - s);
        gi->canvas_id[e - s] = 0;
        return;
      }
    }
    p = end;
  }
}
