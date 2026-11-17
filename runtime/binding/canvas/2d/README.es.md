# 2d

[English](README.md) | Español

CanvasRenderingContext2D. El código está repartido por temas: `context2d.c`
tiene la clase y la lista de métodos y propiedades, y cada uno de los demás
archivos tiene las funciones de un tema. Los pixeles los dibuja `gfx/` (paths,
máscaras, pinturas) sobre la superficie del canvas.

| archivo        | de qué se encarga |
|----------------|-------------------|
| `context2d.h`  | `Context2D` y `Context2DState` (matriz, estilos, clip, fuente, sombra...) y las funciones de cada archivo |
| `context2d.c`  | la clase del contexto y su lista de métodos y propiedades |
| `state.c`      | save / restore (pila de estados), reset |
| `style.c`      | fillStyle, strokeStyle, globalAlpha, lineWidth, globalCompositeOperation...; CanvasGradient y CanvasPattern |
| `transform.c`  | translate, rotate, scale, transform, setTransform (también con un DOMMatrix), resetTransform, getTransform |
| `path.c`       | beginPath, moveTo, lineTo, rect, arc, arcTo, ellipse, curvas cuadráticas y bezier, isPointInPath |
| `draw.c`       | fillRect, strokeRect, clearRect, fill, stroke, clip |
| `image.c`      | drawImage, ImageData, getImageData, putImageData, createImageData |
| `text.c`       | font, textAlign, textBaseline, measureText, fillText, strokeText |

## Notas

- Los puntos de un path se guardan ya transformados (coordenadas de
  dispositivo), como dice el spec: la matriz que cuenta es la del momento de
  `moveTo` / `lineTo`. Las curvas se aplanan con un error máximo de 0.25 px.
- `save()` guarda hasta 32 estados. Los que sobran se cuentan para que los
  `restore()` queden equilibrados, pero no guardan estado.
- Un `clip()` nuevo es la intersección con el anterior.
- Los valores que no son un color válido se ignoran, como en el navegador.
- Falta todavía: líneas discontinuas (`setLineDash` se acepta, las líneas se
  dibujan continuas) y texto con gradientes o patrones (usa el color del
  primer stop, o negro).
