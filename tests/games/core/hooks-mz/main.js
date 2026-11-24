// Los hooks de RPG Maker MZ (runtime/engines/rpgmaker-mz/hooks.toml) sobre
// lo justo de RPG Maker MZ y PixiJS v5 de mentira. Las funciones que los hooks
// comprueban (sameCode) son las de verdad, tal cual: RPG Maker MZ 1.8.1
// (rmmz_core.js, rmmz_managers.js, rmmz_scenes.js; KADOKAWA, MIT) y PixiJS
// v5.3.12 (GeometrySystem; MIT), copiadas con tools/rpgmaker_mv_model.py.

Array.prototype.clone = function () { return this.slice(0); };

// ---------- PixiJS v5 de mentira ----------
var PIXI = { systems: {}, filters: {} };
// PIXI.Filter: como el de verdad, pone enabled = true al crearse
PIXI.Filter = function (vertex, fragment) {
  this.uniforms = {};
  this.enabled = true;
  this.fragment = fragment;
};

// ColorFilter de RPG Maker MZ
function ColorFilter() {
  this.initialize.apply(this, arguments);
}
ColorFilter.prototype = Object.create(PIXI.Filter.prototype);
ColorFilter.prototype.constructor = ColorFilter;

ColorFilter.prototype.initialize = function() {
    PIXI.Filter.call(this, null, this._fragmentSrc());
    this.uniforms.hue = 0;
    this.uniforms.colorTone = [0, 0, 0, 0];
    this.uniforms.blendColor = [0, 0, 0, 0];
    this.uniforms.brightness = 255;
};

ColorFilter.prototype.setHue = function(hue) {
    this.uniforms.hue = Number(hue);
};

ColorFilter.prototype.setColorTone = function(tone) {
    if (!(tone instanceof Array)) {
        throw new Error("Argument must be an array");
    }
    this.uniforms.colorTone = tone.clone();
};

ColorFilter.prototype.setBlendColor = function(color) {
    if (!(color instanceof Array)) {
        throw new Error("Argument must be an array");
    }
    this.uniforms.blendColor = color.clone();
};

ColorFilter.prototype.setBrightness = function(brightness) {
    this.uniforms.brightness = Number(brightness);
};

ColorFilter.prototype._fragmentSrc = function() {
    const src =
        "varying vec2 vTextureCoord;" +
        "uniform sampler2D uSampler;" +
        "uniform float hue;" +
        "uniform vec4 colorTone;" +
        "uniform vec4 blendColor;" +
        "uniform float brightness;" +
        "vec3 rgbToHsl(vec3 rgb) {" +
        "  float r = rgb.r;" +
        "  float g = rgb.g;" +
        "  float b = rgb.b;" +
        "  float cmin = min(r, min(g, b));" +
        "  float cmax = max(r, max(g, b));" +
        "  float h = 0.0;" +
        "  float s = 0.0;" +
        "  float l = (cmin + cmax) / 2.0;" +
        "  float delta = cmax - cmin;" +
        "  if (delta > 0.0) {" +
        "    if (r == cmax) {" +
        "      h = mod((g - b) / delta + 6.0, 6.0) / 6.0;" +
        "    } else if (g == cmax) {" +
        "      h = ((b - r) / delta + 2.0) / 6.0;" +
        "    } else {" +
        "      h = ((r - g) / delta + 4.0) / 6.0;" +
        "    }" +
        "    if (l < 1.0) {" +
        "      s = delta / (1.0 - abs(2.0 * l - 1.0));" +
        "    }" +
        "  }" +
        "  return vec3(h, s, l);" +
        "}" +
        "vec3 hslToRgb(vec3 hsl) {" +
        "  float h = hsl.x;" +
        "  float s = hsl.y;" +
        "  float l = hsl.z;" +
        "  float c = (1.0 - abs(2.0 * l - 1.0)) * s;" +
        "  float x = c * (1.0 - abs((mod(h * 6.0, 2.0)) - 1.0));" +
        "  float m = l - c / 2.0;" +
        "  float cm = c + m;" +
        "  float xm = x + m;" +
        "  if (h < 1.0 / 6.0) {" +
        "    return vec3(cm, xm, m);" +
        "  } else if (h < 2.0 / 6.0) {" +
        "    return vec3(xm, cm, m);" +
        "  } else if (h < 3.0 / 6.0) {" +
        "    return vec3(m, cm, xm);" +
        "  } else if (h < 4.0 / 6.0) {" +
        "    return vec3(m, xm, cm);" +
        "  } else if (h < 5.0 / 6.0) {" +
        "    return vec3(xm, m, cm);" +
        "  } else {" +
        "    return vec3(cm, m, xm);" +
        "  }" +
        "}" +
        "void main() {" +
        "  vec4 sample = texture2D(uSampler, vTextureCoord);" +
        "  float a = sample.a;" +
        "  vec3 hsl = rgbToHsl(sample.rgb);" +
        "  hsl.x = mod(hsl.x + hue / 360.0, 1.0);" +
        "  hsl.y = hsl.y * (1.0 - colorTone.a / 255.0);" +
        "  vec3 rgb = hslToRgb(hsl);" +
        "  float r = rgb.r;" +
        "  float g = rgb.g;" +
        "  float b = rgb.b;" +
        "  float r2 = colorTone.r / 255.0;" +
        "  float g2 = colorTone.g / 255.0;" +
        "  float b2 = colorTone.b / 255.0;" +
        "  float r3 = blendColor.r / 255.0;" +
        "  float g3 = blendColor.g / 255.0;" +
        "  float b3 = blendColor.b / 255.0;" +
        "  float i3 = blendColor.a / 255.0;" +
        "  float i1 = 1.0 - i3;" +
        "  r = clamp((r / a + r2) * a, 0.0, 1.0);" +
        "  g = clamp((g / a + g2) * a, 0.0, 1.0);" +
        "  b = clamp((b / a + b2) * a, 0.0, 1.0);" +
        "  r = clamp(r * i1 + r3 * i3 * a, 0.0, 1.0);" +
        "  g = clamp(g * i1 + g3 * i3 * a, 0.0, 1.0);" +
        "  b = clamp(b * i1 + b3 * i3 * a, 0.0, 1.0);" +
        "  r = r * brightness / 255.0;" +
        "  g = g * brightness / 255.0;" +
        "  b = b * brightness / 255.0;" +
        "  gl_FragColor = vec4(r, g, b, a);" +
        "}";
    return src;
};

// ---------- ConfigManager y StorageManager de RPG Maker MZ (lo justo) ----------
var saved = {};
var StorageManager = {
  saveObject: function (name, obj) { saved[name] = JSON.parse(JSON.stringify(obj)); }
};
var ConfigManager = { touchUI: true, alwaysDash: false, _isLoaded: false };
ConfigManager.makeData = function () { return { touchUI: this.touchUI, alwaysDash: this.alwaysDash }; };
ConfigManager.applyData = function (config) {
  this.alwaysDash = this.readFlag(config, 'alwaysDash', false);
  this.touchUI = this.readFlag(config, 'touchUI', true);
};

ConfigManager.readFlag = function(config, name, defaultValue) {
    if (name in config) {
        return !!config[name];
    } else {
        return defaultValue;
    }
};

ConfigManager.save = function() {
    StorageManager.saveObject("config", this.makeData());
};

// ---------- Scene_MenuBase, Bitmap y el renderer (lo justo) ----------
var renders = 0, extracts = 0;
PIXI.filters.BlurFilter = function () { this.blur = 8; };
PIXI.Texture = function (base) { this.baseTexture = base; this.destroyed = false; };
PIXI.Texture.prototype.destroy = function () { this.destroyed = true; };
PIXI.Sprite = function (texture) { this.texture = texture; this.filters = null; };
PIXI.RenderTexture = { create: function (w, h) { return { w: w, h: h, destroy: function () { this.gone = true; } }; } };
function Bitmap(w, h) {
  this.width = w;
  this.height = h;
  this.baseTexture = { id: 'bitmap', update: function () {} };
  this.drawn = null;
  var self = this;
  this.context = { drawImage: function (c) { self.drawn = c.pixels; } };
}
Bitmap.prototype.isReady = function () { return true; };
Bitmap.prototype.destroy = function () { this.baseTexture = null; };
var Graphics = {
  app: {
    renderer: {
      render: function (sprite, rt) {
        renders++;
        // la "imagen": de que foto sale y con que filtro
        rt.pixels = sprite.texture.baseTexture.id + '+' + (sprite.filters && sprite.filters[0].blur);
      },
      extract: { canvas: function (rt) { extracts++; return { pixels: rt.pixels, width: rt.w, height: rt.h }; } }
    }
  }
};
function Sprite() { this.bitmap = null; this.filters = null; this.opacity = 255; }
var background = null;
var SceneManager = { backgroundBitmap: function () { return background; } };
function Scene_MenuBase() { this.children = []; }
Scene_MenuBase.prototype.addChild = function (c) { this.children.push(c); };
Scene_MenuBase.prototype.setBackgroundOpacity = function (opacity) { this._backgroundSprite.opacity = opacity; };

Scene_MenuBase.prototype.createBackground = function() {
    this._backgroundFilter = new PIXI.filters.BlurFilter();
    this._backgroundSprite = new Sprite();
    this._backgroundSprite.bitmap = SceneManager.backgroundBitmap();
    this._backgroundSprite.filters = [this._backgroundFilter];
    this.addChild(this._backgroundSprite);
    this.setBackgroundOpacity(192);
};

// ---------- SceneManager e ImageManager de RPG Maker MZ (lo justo) ----------
// (battle-images-release)
Graphics.setStage = function () {};
SceneManager.isSceneChanging = function () { return !!this._nextScene; };
SceneManager.isCurrentSceneBusy = function () { return false; };
SceneManager.onSceneCreate = function () {};
SceneManager._scene = null;
SceneManager._nextScene = null;
SceneManager.onSceneTerminate = function() {
    this._previousScene = this._scene;
    this._previousClass = this._scene.constructor;
    Graphics.setStage(null);
};
SceneManager.changeScene = function() {
    if (this.isSceneChanging() && !this.isCurrentSceneBusy()) {
        if (this._scene) {
            this._scene.terminate();
            this.onSceneTerminate();
        }
        this._scene = this._nextScene;
        this._nextScene = null;
        if (this._scene) {
            this._scene.create();
            this.onSceneCreate();
        }
        if (this._exiting) {
            this.terminate();
        }
    }
};
SceneManager.goto = function (cls) { this._nextScene = new cls(); this.changeScene(); };
function Scene_Map() { this.images = ['img/characters/Sam.png']; }
Scene_Map.prototype.create = function () { for (var i = 0; i < this.images.length; i++) ImageManager.loadBitmap(this.images[i]); };
Scene_Map.prototype.terminate = function () {};
function Scene_Battle() {}
Scene_Battle.prototype.create = function () {
  ImageManager.loadBitmap('img/enemies/Hellcar.png');
  ImageManager.loadBitmap('img/characters/Sam.png');
  ImageManager.loadBitmap('img/system/Window.png');
};
Scene_Battle.prototype.terminate = function () {};
Bitmap.load = function (url) { var b = new Bitmap(8, 8); b.url = url; return b; };
var ImageManager = { _cache: {}, _system: {} };
ImageManager.loadBitmap = function (url) { return this.loadBitmapFromUrl(url); };
ImageManager.loadBitmapFromUrl = function(url) {
    const cache = url.includes("/system/") ? this._system : this._cache;
    if (!cache[url]) {
        cache[url] = Bitmap.load(url);
    }
    return cache[url];
};
ImageManager.clear = function() {
    const cache = this._cache;
    for (const url in cache) {
        cache[url].destroy();
    }
    this._cache = {};
};

// ---------- GeometrySystem de PixiJS v5 sin VAO (lo justo) ----------
// lo que la WebGL recibe, apuntado
var glLog = [];
var byteSizeMap$1 = { 5126: 4 };
function GLBuffer(buffer) { this.buffer = buffer; this.refCount = 0; }
var nextBuffer = 1;
function GeometrySystem() {
  var log = glLog;
  this.gl = {
    createVertexArray: function () { return null; },
    bindVertexArray: function () { return null; },
    createBuffer: function () { return 'buf' + nextBuffer++; },
    bindBuffer: function (t, b) { log.push('bindBuffer ' + b); },
    enableVertexAttribArray: function (l) { log.push('enable ' + l); },
    vertexAttribPointer: function (l, size, type, norm, stride, start) { log.push('pointer ' + [l, size, stride, start].join(',')); }
  };
  this.CONTEXT_UID = 1;
  this.hasVao = false;
  this.managedGeometries = {};
  this.managedBuffers = {};
  this._activeGeometry = null;
  this._activeVao = null;
  this.renderer = { shader: { shader: null } };
  this.inits = 0;
}
GeometrySystem.prototype.checkCompatibility = function () {};
GeometrySystem.prototype.getSignature = function (geometry, program) { return 'g-' + geometry.id; };
GeometrySystem.prototype.activateVao = function (geometry, program) {
  var gl = this.gl;
  if (geometry.indexBuffer) gl.bindBuffer(0, geometry.indexBuffer._glBuffers[this.CONTEXT_UID].buffer);
  for (var j in geometry.attributes) {
    var a = geometry.attributes[j];
    if (!program.attributeData[j]) continue;
    gl.bindBuffer(1, geometry.buffers[a.buffer]._glBuffers[this.CONTEXT_UID].buffer);
    gl.enableVertexAttribArray(program.attributeData[j].location);
    gl.vertexAttribPointer(program.attributeData[j].location, a.size, a.type, false, a.stride, a.start);
  }
};
GeometrySystem.prototype.updateBuffers = function () { glLog.push('update ' + this._activeGeometry.id); };
PIXI.systems.GeometrySystem = GeometrySystem;

GeometrySystem.prototype.bind = function (geometry, shader) {
    shader = shader || this.renderer.shader.shader;
    var gl = this.gl;
    // not sure the best way to address this..
    // currently different shaders require different VAOs for the same geometry
    // Still mulling over the best way to solve this one..
    // will likely need to modify the shader attribute locations at run time!
    var vaos = geometry.glVertexArrayObjects[this.CONTEXT_UID];
    var incRefCount = false;
    if (!vaos) {
        this.managedGeometries[geometry.id] = geometry;
        geometry.disposeRunner.add(this);
        geometry.glVertexArrayObjects[this.CONTEXT_UID] = vaos = {};
        incRefCount = true;
    }
    var vao = vaos[shader.program.id] || this.initGeometryVao(geometry, shader.program, incRefCount);
    this._activeGeometry = geometry;
    if (this._activeVao !== vao) {
        this._activeVao = vao;
        if (this.hasVao) {
            gl.bindVertexArray(vao);
        }
        else {
            this.activateVao(geometry, shader.program);
        }
    }
    // TODO - optimise later!
    // don't need to loop through if nothing changed!
    // maybe look to add an 'autoupdate' to geometry?
    this.updateBuffers();
};

GeometrySystem.prototype.initGeometryVao = function (geometry, program, incRefCount) {
    if (incRefCount === void 0) { incRefCount = true; }
    this.checkCompatibility(geometry, program);
    var gl = this.gl;
    var CONTEXT_UID = this.CONTEXT_UID;
    var signature = this.getSignature(geometry, program);
    var vaoObjectHash = geometry.glVertexArrayObjects[this.CONTEXT_UID];
    var vao = vaoObjectHash[signature];
    if (vao) {
        // this will give us easy access to the vao
        vaoObjectHash[program.id] = vao;
        return vao;
    }
    var buffers = geometry.buffers;
    var attributes = geometry.attributes;
    var tempStride = {};
    var tempStart = {};
    for (var j in buffers) {
        tempStride[j] = 0;
        tempStart[j] = 0;
    }
    for (var j in attributes) {
        if (!attributes[j].size && program.attributeData[j]) {
            attributes[j].size = program.attributeData[j].size;
        }
        else if (!attributes[j].size) {
            console.warn("PIXI Geometry attribute '" + j + "' size cannot be determined (likely the bound shader does not have the attribute)"); // eslint-disable-line
        }
        tempStride[attributes[j].buffer] += attributes[j].size * byteSizeMap$1[attributes[j].type];
    }
    for (var j in attributes) {
        var attribute = attributes[j];
        var attribSize = attribute.size;
        if (attribute.stride === undefined) {
            if (tempStride[attribute.buffer] === attribSize * byteSizeMap$1[attribute.type]) {
                attribute.stride = 0;
            }
            else {
                attribute.stride = tempStride[attribute.buffer];
            }
        }
        if (attribute.start === undefined) {
            attribute.start = tempStart[attribute.buffer];
            tempStart[attribute.buffer] += attribSize * byteSizeMap$1[attribute.type];
        }
    }
    vao = gl.createVertexArray();
    gl.bindVertexArray(vao);
    // first update - and create the buffers!
    // only create a gl buffer if it actually gets
    for (var i = 0; i < buffers.length; i++) {
        var buffer = buffers[i];
        if (!buffer._glBuffers[CONTEXT_UID]) {
            buffer._glBuffers[CONTEXT_UID] = new GLBuffer(gl.createBuffer());
            this.managedBuffers[buffer.id] = buffer;
            buffer.disposeRunner.add(this);
        }
        if (incRefCount) {
            buffer._glBuffers[CONTEXT_UID].refCount++;
        }
    }
    // TODO - maybe make this a data object?
    // lets wait to see if we need to first!
    this.activateVao(geometry, program);
    this._activeVao = vao;
    // add it to the cache!
    vaoObjectHash[program.id] = vao;
    vaoObjectHash[signature] = vao;
    return vao;
};

// la original sin hook, para comparar
var originalBind = GeometrySystem.prototype.bind;
var originalInit = GeometrySystem.prototype.initGeometryVao;

function makeGeometry(id) {
  var vb = { _glBuffers: {}, disposeRunner: { add: function () {} } };
  var ib = { _glBuffers: {}, disposeRunner: { add: function () {} } };
  return {
    id: id,
    buffers: [vb, ib],
    indexBuffer: ib,
    attributes: {
      aVertexPosition: { buffer: 0, size: 2, type: 5126 },
      aTextureCoord: { buffer: 0, size: 2, type: 5126 }
    },
    glVertexArrayObjects: {},
    disposeRunner: { add: function () {} }
  };
}
function makeShader(id) {
  return { program: { id: id, attributeData: { aVertexPosition: { location: 0, size: 2 }, aTextureCoord: { location: 1, size: 2 } } } };
}

requestAnimationFrame(function () {
  T.test('hooks: engine = "rpgmaker-mz"', function () {
    T.equal(ColorFilter.prototype.initialize.__quixHook, 'rpgmaker-mz/render/color-filter-identity-off', 'color-filter-identity-off');
    T.equal(Scene_MenuBase.prototype.createBackground.__quixHook, 'rpgmaker-mz/render/menu-blur-once', 'menu-blur-once');
    T.equal(GeometrySystem.prototype.bind.__quixHook, 'rpgmaker-mz/render/vao-bind-off', 'no-vao-bind');
    T.equal(ConfigManager.readFlag.__quixHook, 'rpgmaker-mz/port/touch-ui-off', 'touch-ui-off (readFlag)');
    T.equal(ConfigManager.save.__quixHook, 'rpgmaker-mz/port/touch-ui-off', 'touch-ui-off (save)');
    T.equal(Scene_Battle.prototype.create.__quixHook, 'rpgmaker/memory/gc-on-transition', 'gc-on-transition (battle)');
    T.equal(Scene_Map.prototype.create.__quixHook, 'rpgmaker/memory/gc-on-transition', 'gc-on-transition (map)');
  });

  T.test('color-filter-identity-off', function () {
    var f = new ColorFilter();
    T.equal(f.enabled, false, 'neutral: off');
    f.setBlendColor([255, 255, 255, 128]);
    T.equal(f.enabled, true, 'a blend color (fade, flash): on');
    f.setBlendColor([255, 255, 255, 0]);
    T.equal(f.enabled, false, 'blend alpha 0 again: off');
    f.setColorTone([0, 0, 0, 10]);
    T.equal(f.enabled, true, 'gray tone: on');
    f.setColorTone([0, 0, 0, 0]);
    f.setHue(30);
    T.equal(f.enabled, true, 'hue: on');
    f.setHue(0);
    f.setBrightness(128);
    T.equal(f.enabled, true, 'brightness: on');
    f.setBrightness(255);
    f.uniforms.colorTone = [-68, -68, 0, 0];
    T.equal(f.enabled, true, 'uniforms changed directly: on');
    f.uniforms.colorTone = [0, 0, 0, 0];
    T.equal(f.enabled, false, 'neutral again: off');
    f.enabled = false;
    f.setBlendColor([0, 0, 0, 255]);
    T.equal(f.enabled, false, 'turned off by the game: stays off');
    f.enabled = true;
    T.equal(f.enabled, true, 'and on again');
    var other = new PIXI.Filter();
    T.equal(other.enabled, true, 'another filter: untouched');
  });

  T.test('menu-blur-once', function () {
    background = new Bitmap(816, 624);
    background.baseTexture.id = 'snap1';
    var a = new Scene_MenuBase();
    a.createBackground();
    var s = a._backgroundSprite;
    T.equal(s.filters, null, 'no filter on the sprite');
    T.equal(s.bitmap.drawn, 'snap1+8', 'the photo with its blur, in a bitmap');
    T.equal(s.opacity, 192, 'same opacity');
    T.equal(renders, 1, 'drawn once');
    var b = new Scene_MenuBase();
    b.createBackground();
    T.equal(b._backgroundSprite.bitmap, s.bitmap, 'a submenu with the same photo: the same blurred bitmap');
    T.equal(renders, 1, 'not drawn again');
    var first = s.bitmap;
    background = new Bitmap(816, 624);
    background.baseTexture.id = 'snap2';
    var c = new Scene_MenuBase();
    c.createBackground();
    T.equal(c._backgroundSprite.bitmap.drawn, 'snap2+8', 'a new photo: blurred again');
    T.equal(first.baseTexture, null, 'the old blurred bitmap is released');
    background = null;
    var d = new Scene_MenuBase();
    d.createBackground();
    T.equal(d._backgroundSprite.filters.length, 1, 'no photo: as RPG Maker MZ');
  });

  T.test('touch-ui-off', function () {
    T.equal(ConfigManager.touchUI, false, 'before loading the config: off');
    ConfigManager.applyData({});
    T.equal(ConfigManager.touchUI, false, 'no config: off');
    ConfigManager.applyData({ touchUI: true, alwaysDash: true });
    T.equal(ConfigManager.touchUI, false, 'a config from the PC with touchUI true: off');
    T.equal(ConfigManager.alwaysDash, true, 'the other options: as saved');
    ConfigManager.touchUI = true;
    ConfigManager.save();
    T.equal(saved.config.quixTouchUI, true, 'saved by quixjs: marked');
    ConfigManager.applyData(saved.config);
    T.equal(ConfigManager.touchUI, true, 'turned on by the player in quixjs: on');
    ConfigManager.touchUI = false;
    ConfigManager.save();
    ConfigManager.applyData(saved.config);
    T.equal(ConfigManager.touchUI, false, 'and off');
  });

  T.test('battle-images-release', function () {
    T.equal(SceneManager.onSceneTerminate.__quixHook, 'rpgmaker-mz/memory/battle-images-release', 'the hook is on');
    SceneManager.goto(Scene_Map);
    var sam = ImageManager._cache['img/characters/Sam.png'];
    SceneManager.goto(Scene_Battle);
    var enemy = ImageManager._cache['img/enemies/Hellcar.png'];
    var windowImg = ImageManager._system['img/system/Window.png'];
    T.ok(enemy && enemy.baseTexture, 'in battle: the enemy is in the cache');
    SceneManager.goto(Scene_Map);
    T.equal(ImageManager._cache['img/enemies/Hellcar.png'], undefined, 'back on the map: the enemy is out of the cache');
    T.equal(enemy.baseTexture, null, 'and destroyed');
    T.equal(ImageManager._cache['img/characters/Sam.png'], sam, 'what the map had stays');
    T.ok(sam.baseTexture, 'not destroyed');
    T.equal(ImageManager._system['img/system/Window.png'], windowImg, 'the system images stay');
    SceneManager.goto(Scene_Battle);
    ImageManager.clear();
    ImageManager.loadBitmap('img/characters/Sam.png');
    var after = ImageManager._cache;
    SceneManager.goto(Scene_Map);
    T.ok(after['img/characters/Sam.png'].baseTexture, 'a transfer in between: the new cache is not touched');
  });

  T.test('no-vao-bind', function () {
    // la misma serie de binds con la original y con el hook: lo mismo a la WebGL
    function run(bind) {
      glLog = [];
      nextBuffer = 1;
      var sys = new GeometrySystem();
      var g1 = makeGeometry(1), g2 = makeGeometry(2);
      var s1 = makeShader(10), s2 = makeShader(11);
      var order = [[g1, s1], [g1, s1], [g2, s1], [g1, s2], [g1, s1], [g2, s1], [g2, s1]];
      for (var i = 0; i < order.length; i++) {
        bind.call(sys, order[i][0], order[i][1]);
        glLog.push('active ' + sys._activeGeometry.id + ' ' + sys._activeVao);
      }
      // un buffer liberado: otra vez la original
      delete g1.buffers[0]._glBuffers[1];
      bind.call(sys, g1, s1);
      glLog.push('refs ' + g1.buffers[0]._glBuffers[1].refCount + ' ' + g2.buffers[0]._glBuffers[1].refCount);
      return glLog.join('\n');
    }
    var before = run(originalBind);
    var after = run(GeometrySystem.prototype.bind);
    T.equal(after, before, 'the same calls to WebGL, in the same order');
    var sys = new GeometrySystem();
    var n = 0;
    sys.initGeometryVao = function () { n++; return originalInit.apply(this, arguments); };
    var g = makeGeometry(3), sh = makeShader(12);
    for (var i = 0; i < 5; i++) sys.bind(g, sh);
    T.equal(n, 1, 'initGeometryVao once per geometry and program');
    sys.hasVao = true;
    sys.gl.createVertexArray = function () { return {}; };
    var g4 = makeGeometry(4);
    sys.bind(g4, sh);
    sys.bind(g4, sh);
    T.equal(n, 2, 'with VAO: the original');
  });
  T.done();
});
