// render/render-scale: para Graphics._updateRenderer (RPG Maker MV con WebGL).
//
// Con [game] render_scale (un numero entre 0 y 1, o "auto": la escala con la
// que el juego cabe en la pantalla), PIXI dibuja el juego a esa resolucion:
// el renderer tiene esa resolution y el canvas mide eso (en la RG CubeXX,
// 720x720, Firework de 1280x720 se dibuja a 720x405, lo que se ve). La logica
// del juego no cambia: Graphics.width y height, las coordenadas y las
// imagenes siguen a 1280x720; PIXI lo escala al dibujar. Los filtros (el
// tono de pantalla, los desenfoques) tambien se dibujan a esa resolucion.
// Las fotos de la pantalla (snap) siguen a la resolucion del juego.
//
// Los textos y las imagenes estan a la resolucion del juego y la GPU los
// reduce al dibujar. Un Bitmap de RPG Maker MV es NEAREST (smooth = false):
// reducido asi, se salta pixeles y la letra sale deformada (Firework: la
// lista de partidas, UIFont-Heavy de 20 px, queda en ~11; y los botones de
// abajo, una imagen). Por eso, al subir uno a la GPU, la reduccion
// (MIN_FILTER) pasa a LINEAR: en cualquier GPU, sin mipmaps (con ellos la
// letra salia mas apagada, y gastan memoria). La ampliacion (MAG_FILTER) no
// cambia. Los tilesets (img/tilesets/) no: sus baldosas se mezclarian en los
// bordes y el mapa tendria rayas.
//
// Firework (pantalla completa ~10 veces por imagen) en una RG CubeXX: ver
// games/firework-shiying/port.toml. Sin render_scale (lo normal), el hook no
// cambia nada.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_core/Graphics.js (_updateRenderer)
//
// Uso (config.toml o port.toml):
//   [game]
//   render_scale = "auto"
//   [hooks]
//   "Graphics._updateRenderer" = "rpgmaker-mv/render/render-scale"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/game/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: la funcion que espera el hook, tal cual (la de RPG Maker MV)
  var MODEL = {
    'Graphics._updateRenderer': function() {
        if (this._renderer) {
            this._renderer.resize(this._width, this._height);
        }
    }
  };
  if (!quix.engine.sameCode(original, MODEL['Graphics._updateRenderer'])) return original;
  var s = typeof quix === 'object' && quix.gpu ? quix.gpu.renderScale : 1;
  if (!(s > 0 && s < 1) || typeof PIXI !== 'object') return original;

  // los filtros, a la resolucion del renderer (su textura intermedia). La
  // clase no se exporta en PIXI 4: la del filterManager del renderer
  function filtersAtScale(fm) {
    var proto = fm && Object.getPrototypeOf(fm);
    if (!proto || typeof proto.pushFilter !== 'function' || proto.pushFilter.__quixScale) return;
    var push = proto.pushFilter;
    proto.pushFilter = function (target, filters) {
      var res = this.renderer.resolution;
      if (res < 1 && filters) {
        for (var i = 0; i < filters.length; i++) if (filters[i].resolution === 1) filters[i].resolution = res;
      }
      return push.apply(this, arguments);
    };
    proto.pushFilter.__quixScale = true;
  }

  // los textos y las imagenes (menos los tilesets), reducidos con LINEAR al
  // subirlos (el textureManager de PIXI 4)
  function minifyLinear(r) {
    var tm = r.textureManager;
    if (!tm || typeof tm.updateTexture !== 'function' || tm.updateTexture.__quixMin) return;
    var gl = r.gl, update = tm.updateTexture;
    tm.updateTexture = function (texture) {
      var glt = update.apply(this, arguments);
      var base = texture && (texture.baseTexture || texture);
      var src = base && base.source;
      var url = (base && base.imageUrl) || (src && src.src) || '';
      if (glt && src && !/img\/tilesets\//i.test(url)) {
        glt.bind();
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      }
      return glt;
    };
    tm.updateTexture.__quixMin = true;
  }

  var hooked = function () {
    var r = this._renderer;
    if (r && r.gl && r.rootRenderTarget && r.resolution !== s) {
      r.resolution = s;
      r.rootRenderTarget.resolution = s;
      filtersAtScale(r.filterManager);
      minifyLinear(r);
    }
    return original.apply(this, arguments);
  };
  // el renderer se crea despues (Graphics.initialize, con _createRenderer) y
  // RPG Maker solo llama a _updateRenderer si cambia el tamano: tambien al
  // crearlo. Si ya existe, ahora
  if (typeof Graphics === 'function') {
    if (Graphics._renderer) {
      hooked.call(Graphics);
    } else if (typeof Graphics._createRenderer === 'function') {
      var create = Graphics._createRenderer;
      Graphics._createRenderer = function () {
        var r = create.apply(this, arguments);
        hooked.call(this);
        return r;
      };
    }
  }
  return hooked;
}
