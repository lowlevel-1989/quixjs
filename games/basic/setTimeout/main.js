let frameCount = 0;
let lastTime = Date.now();

function loop() {
  frameCount++;

  const now = Date.now();
  const delta = now - lastTime;

  if (delta >= 1000) {
    const fps = frameCount / (delta / 1000);
    console.log(`FPS: ${fps.toFixed(2)}`);

    // Reset para el siguiente segundo
    frameCount = 0;
    lastTime = now;
  }

  setTimeout(loop, 1000 / 30);
}

loop();

