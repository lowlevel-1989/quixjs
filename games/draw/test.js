var frameCount = 0;
var lastTime = Date.now();
var color = getRandomColor();
var lastColorChange = Date.now();

const canvas = document.createElement("canvas");
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

document.body.appendChild(canvas);

const ctx = canvas.getContext("2d");

const boxWidth  = 25;
const boxHeight = 25;

var x = Math.random() * (canvas.width - boxWidth);
var y = Math.random() * (canvas.height - boxHeight);

var dx = 4;
var dy = 3;

function getRandomColor() {
  const r = Math.floor(Math.random() * 256);
  const g = Math.floor(Math.random() * 256);
  const b = Math.floor(Math.random() * 256);
  return `rgb(${r},${g},${b})`;
}

function loop() {
  frameCount++;

  const now = Date.now();
  const delta = now - lastTime;

  if (delta >= 1000) {
    const fps = frameCount / (delta / 1000);
    console.log(`FPS: ${fps.toFixed(2)}`);
    frameCount = 0;
    lastTime = now;
  }

  if (now - lastColorChange >= 2000) {
    color = getRandomColor();
    lastColorChange = now;
  }

  // Mover la "pelota"
  x += dx;
  y += dy;

  console.log(x, y)


  if (x <= 0 || x + boxWidth >= canvas.width) {
    dx = (Math.random() * 6 + 2) * (dx > 0 ? -1 : 1); // Nuevo dx aleatorio con dirección opuesta
    dy = (Math.random() * 6 - 3);                     // Nuevo dy aleatorio entre -3 y +3
    color = getRandomColor();
  }

  if (y <= 0 || y + boxHeight >= canvas.height) {
    dy = (Math.random() * 6 + 2) * (dy > 0 ? -1 : 1); // Nuevo dy aleatorio con dirección opuesta
    dx = (Math.random() * 6 - 3);                     // Nuevo dx aleatorio entre -3 y +3
    color = getRandomColor();
  }

  console.log(x, y)
  // ctx.clearRect(0, 0, canvas.width, canvas.height);

  ctx.fillStyle = color;
  ctx.fillRect(x, y, boxWidth, boxHeight);

  //setTimeout(loop, 1000 / 30); // 30 FPS como antes
  requestAnimationFrame(loop);
}
//loop();
requestAnimationFrame(loop);

