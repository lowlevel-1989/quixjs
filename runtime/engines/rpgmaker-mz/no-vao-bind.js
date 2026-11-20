// no-vao-bind: para PIXI.systems.GeometrySystem.prototype.bind (PixiJS v5, el
// de RPG Maker MZ).
//
// PixiJS v5 guarda la configuracion de los atributos de cada geometria en un
// VAO (OES_vertex_array_object). Sin la extension (la WebGL de quixjs no la
// tiene) createVertexArray da null, el null se guarda como "su VAO" y no
// cuenta como hecho: cada bind (cada lote de sprites, cada filtro) vuelve a
// pasar por initGeometryVao, que recalcula la firma de la geometria (un
// array y un join), los strides y los buffers, para terminar en
// activateVao. En un menu de Look Outside, initGeometryVao era el ~14% del
// JS, y el ~20% de la memoria que se crea.
//
// Con el hook, sin VAO, la primera vez de cada geometria con cada programa
// va por la original; las siguientes, directamente a lo que acaba haciendo
// la original: activateVao y updateBuffers (los mismos bindBuffer y
// vertexAttribPointer). Si un buffer de la geometria se libero, o la
// geometria se libero (dispose borra su tabla de VAOs, y con ella la marca
// del hook), otra vez la original. Con VAO, la original.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   PixiJS v5.3.12 (GeometrySystem.bind, initGeometryVao), MIT, commit
//   9e2bab0:
//   https://github.com/pixijs/pixijs/blob/9e2bab08bad6b0e8dddac05d65fd290eca7193b1/packages/core/src/geometry/GeometrySystem.ts#L161
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "PIXI.systems.GeometrySystem.prototype.bind" = "rpgmaker-mz/no-vao-bind"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  var sameCode = __quixEngine.sameCode;
  // MODEL: las funciones que espera el hook, tal cual (las de PixiJS
  // v5.3.12, js/libs/pixi.js de RPG Maker MZ 1.8.1), copiadas con
  // tools/rpgmaker_mv_model.py
  var MODEL = {
    'GeometrySystem.bind': function (geometry, shader) {
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
    },
    'GeometrySystem.initGeometryVao': function (geometry, program, incRefCount) {
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
    },

  };
  var proto = PIXI.systems.GeometrySystem.prototype;
  if (
    !sameCode(original, MODEL['GeometrySystem.bind']) ||
    !sameCode(proto.initGeometryVao, MODEL['GeometrySystem.initGeometryVao'])
  )
    return original;

  // cada buffer de la geometria sigue con su buffer de GL
  function buffersReady(geometry, uid) {
    var buffers = geometry.buffers;
    for (var i = 0; i < buffers.length; i++) if (!buffers[i]._glBuffers[uid]) return false;
    return true;
  }

  return function (geometry, shader) {
    if (this.hasVao) return original.call(this, geometry, shader);
    shader = shader || this.renderer.shader.shader;
    var program = shader.program,
      uid = this.CONTEXT_UID,
      key = 'quixNoVao' + program.id;
    var vaos = geometry.glVertexArrayObjects[uid];
    if (vaos && vaos[key] === true && !vaos[program.id] && buffersReady(geometry, uid)) {
      // lo que hace la original sin VAO, sin initGeometryVao
      this._activeGeometry = geometry;
      this._activeVao = null;
      this.activateVao(geometry, program);
      this.updateBuffers();
      return;
    }
    original.call(this, geometry, shader);
    vaos = geometry.glVertexArrayObjects[uid];
    if (vaos && !vaos[program.id]) vaos[key] = true;
  };
}
