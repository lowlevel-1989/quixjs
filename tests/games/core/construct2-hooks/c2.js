// lo justo de c2runtime.js (Construct 2): truncateArray y clearArray tal
// cual, y una Layer que dibuja un objeto con efecto directo o con una
// textura aparte (renderEffectChain), con la misma condicion que la de verdad
var cr = {};
cr.truncateArray = function (arr, index)
{
	arr.length = index;
};
cr.clearArray = function (arr)
{
	cr.truncateArray(arr, 0);
};
var drawn = [];
function Layer() { this.layout = { renderEffectChain: function (glw, layer, inst) { drawn.push('chain'); } }; }
Layer.prototype.drawInstanceWithShadersGL = function (inst, glw)
{
	var shaderindex = inst.active_effect_types[0].shaderindex;
	if (inst.active_effect_types.length === 1 && !glw.programUsesCrossSampling(shaderindex) &&
		!glw.programExtendsBox(shaderindex) && !glw.programUsesDest(shaderindex) &&
		inst.opacity === 1 && !inst.type.plugin.must_predraw)
	{
		drawn.push('direct');
	}
	else
	{
		this.layout.renderEffectChain(glw, this, inst, null);
	}
};
// una capa con efectos va a una textura aparte, como Layer.drawGL
Layer.prototype.drawGL = function (glw)
{
	this.render_offscreen = (this.forceOwnTexture || this.opacity !== 1.0 || this.active_effect_types.length > 0 || this.blend_mode !== 0);
	drawn.push(this.render_offscreen ? 'texture' : 'direct');
};
cr.layer = Layer;

// una inclusion de hoja de eventos, tal cual la de Construct 2
var cleaned = [];
function Type(name) {
  this.name = name; this.families = []; this.members = [];
  this.solstack = [{ select_all: true, else_instances: [] }]; this.cur_sol = 0;
}
Type.prototype.pushCleanSol = function () {
  cleaned.push(this.name);
  this.cur_sol++;
  this.solstack[this.cur_sol] = { select_all: true, else_instances: [] };
};
Type.prototype.popSol = function () { this.cur_sol--; };
var runtime = {
  types_by_index: [],
  pushCleanSol: function (list) { for (var i = 0; i < list.length; i++) list[i].pushCleanSol(); },
  popSol: function (list) { for (var i = 0; i < list.length; i++) list[i].popSol(); }
};
function EventInclude(parent, sheet) { this.parent = parent; this.include_sheet = sheet; this.runtime = runtime; }
EventInclude.prototype.run = function ()
{
		if (this.parent)
			this.runtime.pushCleanSol(this.runtime.types_by_index);
    if (!this.include_sheet.hasRun)
        this.include_sheet.run(true);			// from include
		if (this.parent)
			this.runtime.popSol(this.runtime.types_by_index);
};
cr.eventinclude = EventInclude;
