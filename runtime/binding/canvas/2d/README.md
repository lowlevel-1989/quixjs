# 2d

English | [Español](README.es.md)

CanvasRenderingContext2D. The code is split by topic: `context2d.c` has the
class and the list of methods and properties, and each other file has the
functions of one topic. The pixels are drawn by `gfx/` (paths, masks,
paints) on the surface of the canvas.

| file           | what it does |
|----------------|--------------|
| `context2d.h`  | `Context2D` and `Context2DState` (matrix, styles, clip, font, shadow...) and the functions of every file |
| `context2d.c`  | the context class and its list of methods and properties |
| `state.c`      | save / restore (stack of states), reset |
| `style.c`      | fillStyle, strokeStyle, globalAlpha, lineWidth, globalCompositeOperation...; CanvasGradient and CanvasPattern |
| `transform.c`  | translate, rotate, scale, transform, setTransform (also with a DOMMatrix), resetTransform, getTransform |
| `path.c`       | beginPath, moveTo, lineTo, rect, arc, arcTo, ellipse, quadratic and bezier curves, isPointInPath |
| `draw.c`       | fillRect, strokeRect, clearRect, fill, stroke, clip |
| `image.c`      | drawImage, ImageData, getImageData, putImageData, createImageData |
| `text.c`       | font, textAlign, textBaseline, measureText, fillText, strokeText |

## Notes

- The points of a path are saved already transformed (device coordinates),
  as the spec says: the matrix that counts is the one at the time of
  `moveTo` / `lineTo`. Curves are flattened with a maximum error of 0.25 px.
- `save()` keeps up to 32 states. The extra ones are counted so that the
  `restore()` calls stay balanced, but they do not save state.
- A new `clip()` is the intersection with the previous one.
- Values that are not a valid color are ignored, as in the browser.
- Not done yet: dashed lines (`setLineDash` is accepted, lines are drawn
  solid) and text with gradients or patterns (it uses the color of the first
  stop, or black).
