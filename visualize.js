const fs = require('fs');

// Read the pixel data
const data = JSON.parse(fs.readFileSync('bridge-server/storage/pixel_updates.json', 'utf8'));
const lastUpdate = data.updates[data.updates.length - 1];
const pixels = lastUpdate.pixels;
const width = lastUpdate.width;
const height = lastUpdate.height;

// Create empty canvas
const canvas = Array(height).fill().map(() => Array(width).fill('  '));

// Fill in pixels
pixels.forEach(pixel => {
  // Color is in RGB format: 0xRRGGBB
  const r = (pixel.color >> 16) & 0xFF;  // Red is in the high bits
  const g = (pixel.color >> 8) & 0xFF;   // Green is in the middle
  const b = pixel.color & 0xFF;          // Blue is in the low bits
  canvas[pixel.y][pixel.x] = `\x1b[38;2;${r};${g};${b}m██\x1b[0m`;
});

// Print canvas
console.log('Drawing Visualization:');
canvas.forEach(row => console.log(row.join(''))); 