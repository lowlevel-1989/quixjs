// Crear canvas dinámicamente
const canvas = document.createElement('canvas');
canvas.width  = 640;
canvas.height = 480;

// Agregar al body
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

// Cargar la imagen
const bunny = new Image();
bunny.src = 'bunny.png';

bunny.onload = function () {
  // Dibuja la imagen cada 100ms
  setTimeout(() => {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const x = canvas.width / 2 - bunny.width / 2;
    const y = canvas.height / 2 - bunny.height / 2;

    ctx.drawImage(bunny, x, y);
  }, 100);
};

