#include "PaintCanvasWidget.h"
#include "PaintDocument.h"
#include "PaintPainter.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPolygon>
#include <QBrush>
#include <cmath>

namespace ks {
namespace paint {

PaintCanvasWidget::PaintCanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);

    // Marching ants timer
    m_marchingAntsTimer = new QTimer(this);
    m_marchingAntsTimer->setInterval(100); // 10 FPS for smooth marching
    connect(m_marchingAntsTimer, &QTimer::timeout, this, [this]() {
        m_marchingAntsOffset += 2.0;
        if (m_marchingAntsOffset > 16.0) m_marchingAntsOffset = 0.0;
        update();
    });
}

void PaintCanvasWidget::setDocument(PaintDocument* doc)
{
    m_document = doc;
    if (m_document) {
        connect(m_document, &PaintDocument::selectionChanged, this, [this]() {
            if (m_document && m_document->hasSelection()) {
                startMarchingAnts();
            } else {
                stopMarchingAnts();
            }
            update();
        });
    }
    zoomToFit();
    update();
}

void PaintCanvasWidget::setTool(PaintTool tool)
{
    m_tool = tool;
    if (!paintToolIsSelect(tool)) {
        m_selecting = false;
    }
    update();
}

void PaintCanvasWidget::setPrimaryColor(const QColor& color)
{
    m_primaryColor = color;
    update();
}

void PaintCanvasWidget::setSecondaryColor(const QColor& color)
{
    m_secondaryColor = color;
    update();
}

void PaintCanvasWidget::setBrushSize(float size)
{
    m_brushSize = qMax(1.0f, size);
    update();
}

void PaintCanvasWidget::setBrushHardness(float hardness)
{
    m_brushHardness = qBound(0.0f, hardness, 1.0f);
    update();
}

void PaintCanvasWidget::setBrushOpacity(float opacity)
{
    m_brushOpacity = qBound(0.0f, opacity, 1.0f);
    update();
}

void PaintCanvasWidget::setBrushFlow(float flow)
{
    m_brushFlow = qBound(0.0f, flow, 1.0f);
    update();
}

void PaintCanvasWidget::setBrushStrength(float strength)
{
    m_brushStrength = qBound(0.0f, strength, 1.0f);
    update();
}

void PaintCanvasWidget::setCloneSource(const QPoint& pos)
{
    m_cloneSource = pos;
    m_hasCloneSource = true;
}

void PaintCanvasWidget::setZoom(float zoom)
{
    m_zoom = qBound(0.02f, zoom, 64.0f);
    emit zoomChanged(m_zoom);
    update();
}

void PaintCanvasWidget::zoomToFit()
{
    if (!m_document || !m_document->hasDocument()) return;
    if (width() <= 0 || height() <= 0) return;
    float zx = float(width()) / float(m_document->width());
    float zy = float(height()) / float(m_document->height());
    setZoom(qMin(zx, zy) * 0.9f);
    m_pan = QPointF(m_document->width() / 2.0f, m_document->height() / 2.0f);
    update();
}

void PaintCanvasWidget::zoomIn()
{
    setZoom(m_zoom * 1.25f);
}

void PaintCanvasWidget::zoomOut()
{
    setZoom(m_zoom / 1.25f);
}

void PaintCanvasWidget::zoom100()
{
    setZoom(1.0f);
    m_pan = QPointF(m_document ? m_document->width() / 2.0f : 0, m_document ? m_document->height() / 2.0f : 0);
    update();
}

QPointF PaintCanvasWidget::viewToImage(const QPoint& viewPos) const
{
    return QPointF(m_pan.x() + (viewPos.x() - width() / 2.0) / m_zoom,
                   m_pan.y() + (viewPos.y() - height() / 2.0) / m_zoom);
}

QPoint PaintCanvasWidget::imagePosFromView(const QPoint& viewPos) const
{
    return viewToImage(viewPos).toPoint();
}

QPoint PaintCanvasWidget::imageToView(const QPointF& imagePos) const
{
    return QPoint(int(width() / 2.0 + (imagePos.x() - m_pan.x()) * m_zoom),
                  int(height() / 2.0 + (imagePos.y() - m_pan.y()) * m_zoom));
}

QRect PaintCanvasWidget::imageRectToView(const QRect& imageRect) const
{
    QPoint tl = imageToView(imageRect.topLeft());
    QPoint br = imageToView(QPointF(imageRect.right() + 1, imageRect.bottom() + 1));
    return QRect(tl, br);
}

void PaintCanvasWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 34));

    if (!m_document || !m_document->hasDocument()) {
        p.setPen(QColor(120, 120, 130));
        p.drawText(rect(), Qt::AlignCenter, tr("Open an image or create a new paint to start painting"));
        return;
    }

    QSize imgSize = m_document->size();
    QPointF topLeft = viewToImage(QPoint(0, 0));
    QPointF bottomRight = viewToImage(QPoint(width(), height()));

    // Checkerboard / background for transparent areas
    QRect fullRect = imageRectToView(QRect(0, 0, imgSize.width(), imgSize.height()));
    p.save();
    p.setBrush(QColor(38, 38, 42));
    p.setPen(QPen(QColor(48, 48, 54), 1));
    p.drawRect(fullRect);
    p.restore();

    QImage composite = m_document->composite();
    if (!composite.isNull()) {
        p.drawImage(QRect(fullRect.topLeft(), fullRect.size() * (m_zoom > 0 ? 1.0 : 1.0)), composite);
    }

    drawSelection(p);
    drawFreeSelectPreview(p);
    drawBrushCursor(p);
}

void PaintCanvasWidget::drawCheckerboard(QPainter& p, const QRect& rect)
{
    const int cell = 8;
    int startX = int(std::floor(rect.left() / float(cell))) * cell;
    int startY = int(std::floor(rect.top() / float(cell))) * cell;
    for (int y = startY; y < rect.bottom(); y += cell) {
        for (int x = startX; x < rect.right(); x += cell) {
            bool dark = ((x / cell) + (y / cell)) % 2 == 0;
            p.fillRect(x, y, cell, cell, dark ? QColor(200, 200, 200) : QColor(255, 255, 255));
        }
    }
}

void PaintCanvasWidget::drawBrushCursor(QPainter& p)
{
    if (!m_document || !m_document->hasDocument()) return;
    if (!(paintToolIsPaint(m_tool) || m_tool == PaintTool::Healing || m_tool == PaintTool::Clone)) return;
    if (!underMouse()) return;

    QPoint cursor = mapFromGlobal(QCursor::pos());
    if (!rect().contains(cursor)) return;

    int radius = qMax(1, int(m_brushSize * m_zoom));
    p.save();
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255, 255, 255, 220), 1, Qt::DotLine));
    p.drawEllipse(cursor, radius, radius);
    p.setPen(QPen(QColor(0, 0, 0, 180), 1, Qt::DotLine));
    p.drawEllipse(cursor, radius - 1, radius - 1);
    p.restore();
}

// Free select polygon preview during drawing
void PaintCanvasWidget::drawFreeSelectPreview(QPainter& p)
{
    if (!m_document || !m_document->hasDocument()) return;
    if (m_tool != PaintTool::FreeSelect) return;
    if (!m_selecting || m_freeSelectPoints.size() < 2) return;

    QRect viewRect = imageRectToView(QRect(0, 0, m_document->width(), m_document->height()));

    // Draw polygon lines
    QPen pen(QColor(255, 255, 255, 200), 1.5, Qt::DashLine);
    pen.setDashPattern({ 4, 4 });
    p.save();
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QTransform transform;
    transform.translate(viewRect.left(), viewRect.top());
    transform.scale(m_zoom, m_zoom);

    QPointF prev = transform.map(m_freeSelectPoints.first());
    for (int i = 1; i < m_freeSelectPoints.size(); ++i) {
        QPointF curr = transform.map(m_freeSelectPoints[i]);
        p.drawLine(prev, curr);
        prev = curr;
    }

    // Draw closing line to current cursor position
    QPoint cursorPos = mapFromGlobal(QCursor::pos());
    if (rect().contains(cursorPos)) {
        QPointF curr = transform.map(imagePosFromView(cursorPos));
        p.drawLine(prev, curr);
    }

    p.restore();

    // Draw vertex points
    p.save();
    p.setPen(QPen(QColor(255, 255, 255), 2));
    p.setBrush(QColor(0, 100, 255, 200));
    for (const QPoint& pt : m_freeSelectPoints) {
        QPointF v = transform.map(pt);
        p.drawEllipse(v, 4, 4);
    }
    p.restore();
}

void PaintCanvasWidget::drawSelection(QPainter& p)
{
    if (!m_document || !m_document->hasDocument()) return;
    if (!m_document->hasSelection()) return;

    // Draw marching ants along the selection path
    if (!m_selectionPath.isEmpty()) {
        QRect viewRect = imageRectToView(QRect(0, 0, m_document->width(), m_document->height()));

        // Transform the path to view coordinates
        QTransform transform;
        transform.translate(viewRect.left(), viewRect.top());
        transform.scale(m_zoom, m_zoom);
        QPainterPath viewPath = transform.map(m_selectionPath);

        // Draw marching ants: two offset dashed lines for contrast
        QPen antPen1(QColor(255, 255, 255, 200), 1.5, Qt::DashLine);
        antPen1.setDashPattern({ 6, 6 });
        antPen1.setDashOffset(m_marchingAntsOffset);

        QPen antPen2(QColor(0, 0, 0, 150), 1.5, Qt::DashLine);
        antPen2.setDashPattern({ 6, 6 });
        antPen2.setDashOffset(m_marchingAntsOffset + 8); // Offset for contrast

        p.save();
        p.setPen(antPen1);
        p.drawPath(viewPath);
        p.setPen(antPen2);
        p.drawPath(viewPath);
        p.restore();
    }
}

void PaintCanvasWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_document || !m_document->hasDocument()) return;
    setFocus();

    QPoint imagePos = imagePosFromView(event->pos());

    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && m_spacePressed)) {
        m_panning = true;
        m_panLastView = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    switch (m_tool) {
    case PaintTool::Pan:
        m_panning = true;
        m_panLastView = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    case PaintTool::Zoom: {
        if (event->modifiers() & Qt::ControlModifier) zoomOut();
        else zoomIn();
        return;
    }
    case PaintTool::Move: {
        // Move current layer
        m_dragging = true;
        m_dragStartView = event->pos();
        m_dragStartImage = imagePos;
        m_document->pushUndo();
        return;
    }
    case PaintTool::ColorPicker: {
        pickColor(imagePos);
        return;
    }
    case PaintTool::RectSelect:
    case PaintTool::EllipseSelect: {
        m_selecting = true;
        m_dragStartView = event->pos();
        m_dragStartImage = imagePos;
        return;
    }
    case PaintTool::FreeSelect: {
        m_selecting = true;
        m_freeSelectPoints.append(imagePos);
        if (event->modifiers() & Qt::ControlModifier) {
            // close polygon
            applyFreeSelection(m_freeSelectPoints, false);
            m_freeSelectPoints.clear();
            m_selecting = false;
        }
        return;
    }
    case PaintTool::FuzzySelect: {
        applyFuzzySelection(imagePos, event->modifiers() & Qt::ShiftModifier);
        emit selectionChanged();
        return;
    }
    case PaintTool::Clone: {
        if (event->modifiers() & Qt::ControlModifier) {
            m_cloneSource = imagePos;
            m_hasCloneSource = true;
            emit statusMessage(tr("Clone source set"));
            return;
        }
        break;
    }
    case PaintTool::BucketFill: {
        QImage layer = m_document->currentLayerImage();
        if (layer.isNull()) return;
        PaintPainter::fill(layer, m_document->selectionMask(), imagePos, m_primaryColor, m_fuzzyTolerance);
        m_document->setCurrentLayerImage(layer);
        emit imageEdited();
        return;
    }
    case PaintTool::Gradient: {
        m_dragging = true;
        m_dragStartView = event->pos();
        m_dragStartImage = imagePos;
        return;
    }
    case PaintTool::Text: {
        QPoint imagePos = imagePosFromView(event->pos());
        emit textToolClicked(imagePos);
        return;
    }
    default:
        break;
    }

    // Start a paint stroke
    if (paintToolIsPaint(m_tool)) {
        m_dragging = true;
        m_dragStartView = event->pos();
        m_dragStartImage = imagePos;
        m_lastImage = imagePos;
        m_strokeActive = true;
        PaintBrush defaultBrush;
        defaultBrush.tool = m_tool;
        defaultBrush.radius = m_brushSize;
        defaultBrush.hardness = m_brushHardness;
        defaultBrush.opacity = m_brushOpacity;
        defaultBrush.flow = m_brushFlow;
        defaultBrush.strength = m_brushStrength;
        defaultBrush.pressure = 1.0f;
        defaultBrush.color = m_primaryColor;
        defaultBrush.secondaryColor = m_secondaryColor;
        defaultBrush.cloneSource = m_hasCloneSource ? m_cloneSource : imagePos;
        defaultBrush.hasCloneSource = m_hasCloneSource;
        defaultBrush.eraseAlpha = (m_tool == PaintTool::Eraser);
        startStroke(imagePos, defaultBrush);
    }
}

void PaintCanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_document || !m_document->hasDocument()) return;

    if (m_panning) {
        QPoint delta = event->pos() - m_panLastView;
        m_panLastView = event->pos();
        m_pan -= QPointF(delta.x() / m_zoom, delta.y() / m_zoom);
        update();
        return;
    }

    if (m_dragging) {
        QPoint imagePos = imagePosFromView(event->pos());

        switch (m_tool) {
        case PaintTool::Move: {
            QPoint delta = imagePos - m_dragStartImage;
            if (m_document->currentLayer()) {
                m_document->setLayerOffset(m_document->currentLayerIndex(),
                    QPoint(m_document->currentLayer()->offsetX + delta.x(),
                           m_document->currentLayer()->offsetY + delta.y()));
                m_dragStartImage = imagePos;
            }
            return;
        }
        case PaintTool::RectSelect:
        case PaintTool::EllipseSelect: {
            m_selectionRect = QRect(m_dragStartImage, imagePos).normalized();
            update();
            return;
        }
        case PaintTool::Gradient: {
            // show gradient preview
            update();
            return;
        }
        case PaintTool::FreeSelect: {
            m_freeSelectPoints.append(imagePos);
            update();
            return;
        }
        default:
            break;
        }

        if (m_strokeActive && paintToolIsPaint(m_tool)) {
            PaintBrush updateBrush;
            updateBrush.tool = m_tool;
            updateBrush.radius = m_brushSize;
            updateBrush.hardness = m_brushHardness;
            updateBrush.opacity = m_brushOpacity;
            updateBrush.flow = m_brushFlow;
            updateBrush.strength = m_brushStrength;
            updateBrush.pressure = 1.0f;
            updateBrush.color = m_primaryColor;
            updateBrush.secondaryColor = m_secondaryColor;
            updateBrush.cloneSource = m_hasCloneSource ? m_cloneSource : m_lastImage;
            updateBrush.hasCloneSource = m_hasCloneSource;
            updateBrush.eraseAlpha = (m_tool == PaintTool::Eraser);
            updateStroke(imagePos, updateBrush);
        }
        return;
    }

    if (paintToolIsPaint(m_tool)) {
        emit statusMessage(tr("%1  |  %2, %3").arg(paintToolDisplayName(m_tool)).arg(imagePosFromView(event->pos()).x()).arg(imagePosFromView(event->pos()).y()));
    }
    update();
}

void PaintCanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    if (m_panning) {
        m_panning = false;
        unsetCursor();
        return;
    }

    if (m_dragging) {
        QPoint imagePos = imagePosFromView(event->pos());

        switch (m_tool) {
        case PaintTool::RectSelect:
        case PaintTool::EllipseSelect: {
            m_selectionRect = QRect(m_dragStartImage, imagePos).normalized();
            applySelectionRect(m_selectionRect);
            m_dragging = false;
            emit selectionChanged();
            return;
        }
        case PaintTool::Gradient: {
            QImage layer = m_document->currentLayerImage();
            if (!layer.isNull()) {
                PaintPainter::gradient(layer, m_document->selectionMask(), m_dragStartImage, imagePos,
                                      m_primaryColor, m_secondaryColor, m_radialGradient);
                m_document->setCurrentLayerImage(layer);
                emit imageEdited();
            }
            m_dragging = false;
            return;
        }
        case PaintTool::Move:
            m_dragging = false;
            return;
        case PaintTool::FreeSelect:
            m_dragging = false;
            return;
        default:
            break;
        }

        if (m_strokeActive) {
        PaintBrush endBrush;
        endBrush.tool = m_tool;
        endBrush.radius = m_brushSize;
        endBrush.hardness = m_brushHardness;
        endBrush.opacity = m_brushOpacity;
        endBrush.flow = m_brushFlow;
        endBrush.strength = m_brushStrength;
        endBrush.pressure = 1.0f;
        endBrush.color = m_primaryColor;
        endBrush.secondaryColor = m_secondaryColor;
        endBrush.cloneSource = m_hasCloneSource ? m_cloneSource : m_lastImage;
        endBrush.hasCloneSource = m_hasCloneSource;
        endBrush.eraseAlpha = (m_tool == PaintTool::Eraser);
        updateStroke(imagePos, endBrush);
        endStroke();
        }
        m_dragging = false;
        emit documentInteractionFinished();
    }
}

void PaintCanvasWidget::tabletEvent(QTabletEvent* event)
{
    if (!m_document || !m_document->hasDocument()) {
        QWidget::tabletEvent(event);
        return;
    }

    QPoint imagePos = imagePosFromView(event->position().toPoint());
    float pressure = event->pressure();
    float tiltX = event->rotation();
    float tiltY = 0.0f;

    switch (event->type()) {
    case QTabletEvent::TabletPress: {
        setFocus();
        m_dragging = true;
        m_dragStartView = event->pos();
        m_dragStartImage = imagePos;
        m_lastImage = imagePos;
        m_strokeActive = true;

        PaintBrush brush;
        brush.tool = m_tool;
        brush.radius = m_brushSize;
        brush.hardness = m_brushHardness;
        brush.opacity = m_brushOpacity;
        brush.flow = m_brushFlow;
        brush.strength = m_brushStrength;
        brush.pressure = pressure;
        brush.tiltX = tiltX;
        brush.tiltY = tiltY;
        brush.color = m_primaryColor;
        brush.secondaryColor = m_secondaryColor;
        brush.cloneSource = m_hasCloneSource ? m_cloneSource : imagePos;
        brush.hasCloneSource = m_hasCloneSource;
        brush.eraseAlpha = (m_tool == PaintTool::Eraser);

        QImage layer = m_document->currentLayerImage();
        if (!layer.isNull()) {
            switch (m_tool) {
            case PaintTool::Smudge:
                break;
            case PaintTool::Blur:
                PaintPainter::blurAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
                break;
            case PaintTool::Sharpen:
                PaintPainter::sharpenAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
                break;
            case PaintTool::Dodge:
                PaintPainter::dodgeAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
                break;
            case PaintTool::Burn:
                PaintPainter::burnAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
                break;
            case PaintTool::Clone:
                if (m_hasCloneSource)
                    PaintPainter::cloneAt(layer, m_document->selectionMask(), imagePos, m_cloneSource, m_brushSize, m_brushOpacity);
                break;
            case PaintTool::Healing:
                if (m_hasCloneSource)
                    PaintPainter::healAt(layer, m_document->selectionMask(), imagePos, m_cloneSource, m_brushSize, m_brushOpacity);
                break;
            default:
                PaintPainter::paintAt(layer, m_document->selectionMask(), imagePos, brush);
                break;
            }
            m_document->setCurrentLayerImage(layer);
            emit imageEdited();
        }
        startStroke(imagePos, brush);
        break;
    }
    case QTabletEvent::TabletMove: {
        if (m_dragging && m_strokeActive && paintToolIsPaint(m_tool)) {
            PaintBrush updateBrush;
            updateBrush.tool = m_tool;
            updateBrush.radius = m_brushSize;
            updateBrush.hardness = m_brushHardness;
            updateBrush.opacity = m_brushOpacity;
            updateBrush.flow = m_brushFlow;
            updateBrush.strength = m_brushStrength;
            updateBrush.pressure = pressure;
            updateBrush.tiltX = tiltX;
            updateBrush.tiltY = tiltY;
            updateBrush.color = m_primaryColor;
            updateBrush.secondaryColor = m_secondaryColor;
            updateBrush.cloneSource = m_hasCloneSource ? m_cloneSource : m_lastImage;
            updateBrush.hasCloneSource = m_hasCloneSource;
            updateBrush.eraseAlpha = (m_tool == PaintTool::Eraser);
            updateStroke(imagePos, updateBrush);
        }
        if (paintToolIsPaint(m_tool)) {
            emit statusMessage(tr("%1  |  %2, %3").arg(paintToolDisplayName(m_tool)).arg(imagePosFromView(event->position().toPoint()).x()).arg(imagePosFromView(event->position().toPoint()).y()));
        }
        update();
        break;
    }
    case QTabletEvent::TabletRelease: {
        if (m_strokeActive) {
            PaintBrush endBrush;
            endBrush.tool = m_tool;
            endBrush.radius = m_brushSize;
            endBrush.hardness = m_brushHardness;
            endBrush.opacity = m_brushOpacity;
            endBrush.flow = m_brushFlow;
            endBrush.strength = m_brushStrength;
            endBrush.pressure = pressure;
            endBrush.tiltX = tiltX;
            endBrush.tiltY = tiltY;
            endBrush.color = m_primaryColor;
            endBrush.secondaryColor = m_secondaryColor;
            endBrush.cloneSource = m_hasCloneSource ? m_cloneSource : m_lastImage;
            endBrush.hasCloneSource = m_hasCloneSource;
            endBrush.eraseAlpha = (m_tool == PaintTool::Eraser);
            updateStroke(imagePos, endBrush);
            endStroke();
        }
        m_dragging = false;
        m_strokeActive = false;
        emit documentInteractionFinished();
        break;
    }
    default:
        break;
    }
    event->accept();
}

void PaintCanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (!m_document || !m_document->hasDocument()) return;
    int delta = event->angleDelta().y();
    if (delta == 0) return;
    float factor = delta > 0 ? 1.15f : 1.0f / 1.15f;
    setZoom(m_zoom * factor);
    update();
    event->accept();
}

void PaintCanvasWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
    }
    QWidget::keyPressEvent(event);
}

void PaintCanvasWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space) {
        m_spacePressed = false;
        unsetCursor();
    }
    QWidget::keyReleaseEvent(event);
}

void PaintCanvasWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    unsetCursor();
}

void PaintCanvasWidget::startStroke(const QPoint& imagePos, const PaintBrush& brush)
{
    if (!m_document || !m_document->currentLayer()) return;
    m_document->pushUndo();

    PaintBrush b = brush;
    if (b.radius <= 0) b.radius = m_brushSize;
    if (b.hardness < 0.0f) b.hardness = m_brushHardness;
    if (b.opacity < 0.0f) b.opacity = m_brushOpacity;
    if (b.flow < 0.0f) b.flow = m_brushFlow;
    if (b.strength < 0.0f) b.strength = m_brushStrength;
    if (b.tool == PaintTool::None) b.tool = m_tool;
    if (!b.hasCloneSource && m_hasCloneSource) b.cloneSource = m_cloneSource;
    b.hasCloneSource = m_hasCloneSource || b.hasCloneSource;

    m_brush = b;

    QImage layer = m_document->currentLayerImage();
    if (layer.isNull()) return;

    switch (m_tool) {
    case PaintTool::Smudge:
        break;
    case PaintTool::Blur:
        PaintPainter::blurAt(layer, m_document->selectionMask(), imagePos, b.radius, b.strength);
        break;
    case PaintTool::Sharpen:
        PaintPainter::sharpenAt(layer, m_document->selectionMask(), imagePos, b.radius, b.strength);
        break;
    case PaintTool::Dodge:
        PaintPainter::dodgeAt(layer, m_document->selectionMask(), imagePos, b.radius, b.strength);
        break;
    case PaintTool::Burn:
        PaintPainter::burnAt(layer, m_document->selectionMask(), imagePos, b.radius, b.strength);
        break;
    case PaintTool::Clone:
        if (b.hasCloneSource)
            PaintPainter::cloneAt(layer, m_document->selectionMask(), imagePos, b.cloneSource, b.radius, b.opacity);
        break;
    case PaintTool::Healing:
        if (b.hasCloneSource)
            PaintPainter::healAt(layer, m_document->selectionMask(), imagePos, b.cloneSource, b.radius, b.opacity);
        break;
    default:
        PaintPainter::paintAt(layer, m_document->selectionMask(), imagePos, b);
        break;
    }

    m_document->setCurrentLayerImage(layer);
    emit imageEdited();
}

void PaintCanvasWidget::updateStroke(const QPoint& imagePos, const PaintBrush& brush)
{
    if (!m_document || !m_document->currentLayer()) return;
    if (!m_strokeActive) return;

    PaintBrush b = brush;
    if (b.radius <= 0) b.radius = m_brushSize;
    if (b.hardness < 0.0f) b.hardness = m_brushHardness;
    if (b.opacity < 0.0f) b.opacity = m_brushOpacity;
    if (b.flow < 0.0f) b.flow = m_brushFlow;
    if (b.strength < 0.0f) b.strength = m_brushStrength;
    if (b.tool == PaintTool::None) b.tool = m_tool;
    if (!b.hasCloneSource && m_hasCloneSource) b.cloneSource = m_cloneSource;
    b.hasCloneSource = m_hasCloneSource || b.hasCloneSource;

    QImage layer = m_document->currentLayerImage();
    if (layer.isNull()) return;

    switch (m_tool) {
    case PaintTool::Blur:
        PaintPainter::blurAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
        break;
    case PaintTool::Sharpen:
        PaintPainter::sharpenAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
        break;
    case PaintTool::Dodge:
        PaintPainter::dodgeAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
        break;
    case PaintTool::Burn:
        PaintPainter::burnAt(layer, m_document->selectionMask(), imagePos, m_brushSize, m_brushStrength);
        break;
    case PaintTool::Clone:
        if (b.hasCloneSource)
            PaintPainter::cloneAt(layer, m_document->selectionMask(), imagePos, b.cloneSource, b.radius, b.opacity);
        break;
    case PaintTool::Healing:
        if (b.hasCloneSource)
            PaintPainter::healAt(layer, m_document->selectionMask(), imagePos, b.cloneSource, b.radius, b.opacity);
        break;
    default:
        PaintPainter::paintLine(layer, m_document->selectionMask(), m_lastImage, imagePos, b);
        break;
    }

    m_lastImage = imagePos;
    m_document->setCurrentLayerImage(layer);
    emit imageEdited();
}

void PaintCanvasWidget::endStroke()
{
    m_strokeActive = false;
}

void PaintCanvasWidget::pickColor(const QPoint& imagePos)
{
    if (!m_document) return;
    QImage composite = m_document->composite();
    if (composite.isNull()) return;
    if (imagePos.x() < 0 || imagePos.y() < 0 || imagePos.x() >= composite.width() || imagePos.y() >= composite.height()) return;
    QColor color = composite.pixelColor(imagePos);
    m_primaryColor = color;
    emit colorPicked(color);
    emit statusMessage(tr("Color: %1").arg(color.name()));
}

void PaintCanvasWidget::applySelectionRect(const QRect& rect)
{
    if (!m_document) return;
    QRect clamped = rect.intersected(QRect(0, 0, m_document->width(), m_document->height()));

    QImage mask = QImage(m_document->size(), QImage::Format_ARGB32);
    mask.fill(Qt::transparent);

    QPainter p(&mask);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 255));
    if (m_tool == PaintTool::EllipseSelect)
        p.drawEllipse(clamped);
    else
        p.drawRect(clamped);
    p.end();

    m_document->setSelectionMask(mask);
    update();
    startMarchingAnts();
}

void PaintCanvasWidget::applyFuzzySelection(const QPoint& pos, bool subtract)
{
    if (!m_document) return;
    QImage composite = m_document->composite();
    if (composite.isNull()) return;
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= composite.width() || pos.y() >= composite.height()) return;

    QImage mask(m_document->size(), QImage::Format_ARGB32);
    mask.fill(Qt::transparent);

    QRgb target = composite.pixel(pos);
    int tol = int(m_fuzzyTolerance * 255.0f);

    QVector<QPoint> stack;
    stack.append(pos);
    QImage visited(mask.size(), QImage::Format_Indexed8);
    visited.fill(0);

    while (!stack.isEmpty()) {
        QPoint p = stack.takeLast();
        if (p.x() < 0 || p.y() < 0 || p.x() >= composite.width() || p.y() >= composite.height()) continue;
        if (visited.pixel(p.x(), p.y())) continue;
        visited.setPixel(p.x(), p.y(), 1);

        QRgb cur = composite.pixel(p.x(), p.y());
        if (qAbs(qRed(cur) - qRed(target)) > tol ||
            qAbs(qGreen(cur) - qGreen(target)) > tol ||
            qAbs(qBlue(cur) - qBlue(target)) > tol) continue;

        mask.setPixel(p.x(), p.y(), qRgba(0, 0, 0, 255));
        stack.append(QPoint(p.x() - 1, p.y()));
        stack.append(QPoint(p.x() + 1, p.y()));
        stack.append(QPoint(p.x(), p.y() - 1));
        stack.append(QPoint(p.x(), p.y() + 1));
    }

    if (subtract && m_document->hasSelection()) {
        QImage existing = m_document->selectionMask();
        for (int y = 0; y < mask.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(mask.scanLine(y));
            const QRgb* ex = reinterpret_cast<const QRgb*>(existing.constScanLine(y));
            for (int x = 0; x < mask.width(); ++x) {
                // Keep only pixels that are in existing but NOT in the new fuzzy region
                bool inExisting = qAlpha(ex[x]) > 0;
                bool inFuzzy = qAlpha(line[x]) > 0;
                line[x] = qRgba(0, 0, 0, (inExisting && !inFuzzy) ? 255 : 0);
            }
        }
    }

    m_document->setSelectionMask(mask);
    update();
    startMarchingAnts();
}

void PaintCanvasWidget::applyFreeSelection(const QList<QPoint>& poly, bool subtract)
{
    Q_UNUSED(subtract);
    if (!m_document) return;
    QImage mask(m_document->size(), QImage::Format_ARGB32);
    mask.fill(Qt::transparent);
    if (poly.size() < 3) return;

    QPolygon polygon;
    for (const QPoint& p : poly) polygon << p;

    QPainter p(&mask);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 255));
    p.drawPolygon(polygon);
    p.end();

    m_document->setSelectionMask(mask);
    update();
}

// Marching ants animation
void PaintCanvasWidget::updateSelectionPath()
{
    if (!m_document || !m_document->hasSelection()) {
        m_selectionPath = QPainterPath();
        return;
    }

    QImage sel = m_document->selectionMask();
    if (sel.isNull()) {
        m_selectionPath = QPainterPath();
        return;
    }

    // Trace the selection boundary using a simple contour tracing
    int w = sel.width();
    int h = sel.height();
    QVector<QPoint> boundary;

    // Marching squares algorithm to find boundary
    for (int y = 0; y < h - 1; ++y) {
        for (int x = 0; x < w - 1; ++x) {
            bool tl = sel.pixelColor(x, y).alpha() > 128;
            bool tr = sel.pixelColor(x + 1, y).alpha() > 128;
            bool bl = sel.pixelColor(x, y + 1).alpha() > 128;
            bool br = sel.pixelColor(x + 1, y + 1).alpha() > 128;

            int cell = (tl ? 8 : 0) | (tr ? 4 : 0) | (bl ? 2 : 0) | (br ? 1 : 0);
            if (cell != 0 && cell != 15) {
                // This cell has a boundary
                if (tl && !tr) boundary.append(QPoint(x + 1, y));
                if (tr && !br) boundary.append(QPoint(x + 1, y + 1));
                if (br && !bl) boundary.append(QPoint(x, y + 1));
                if (bl && !tl) boundary.append(QPoint(x, y));
            }
        }
    }

    if (!boundary.isEmpty()) {
        m_selectionPath.moveTo(boundary.first());
        for (int i = 1; i < boundary.size(); ++i) {
            m_selectionPath.lineTo(boundary[i]);
        }
        m_selectionPath.closeSubpath();
    } else {
        m_selectionPath = QPainterPath();
    }
}

void PaintCanvasWidget::startMarchingAnts()
{
    if (m_document && m_document->hasSelection()) {
        updateSelectionPath();
        if (!m_selectionPath.isEmpty()) {
            m_marchingAntsTimer->start();
        }
    }
}

void PaintCanvasWidget::stopMarchingAnts()
{
    m_marchingAntsTimer->stop();
}

} // namespace paint
} // namespace ks
