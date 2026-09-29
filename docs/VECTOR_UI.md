# Vector UI rendering

## API
```cpp
VectorPath path;
path.moveTo(100, 100);
path.lineTo(200, 80);
path.cubicTo(250, 60, 280, 140, 320, 120);
path.quadTo(360, 100, 400, 150);
path.arcTo(450, 150, 40, 0, 270);
path.close();

StrokeStyle stroke;
stroke.color = Color::rgb(80, 200, 255);
stroke.width = 3.f;
stroke.closed = true;
ui.addPathStroke(path, stroke);

FillStyle fill;
fill.color = Color::rgba(0.1f, 0.3f, 0.5f, 0.4f);
ui.addPathFill(path, fill);

// helpers
path.clear();
path.addRoundRect(40, 40, 200, 120, 12);
path.addCircle(640, 360, 50);
```

## Tessellation
| Element | Method |
|---------|--------|
| Line | segment |
| Quad / Cubic | adaptive subdivision (tol ~0.5 px) |
| Arc | fixed angular steps |
| Stroke | thick triangle pairs + simple miter |
| Fill | triangle fan (convex) |

## GPU
Same UI pipeline: white atlas texel, alpha blend. No extra shaders required.
