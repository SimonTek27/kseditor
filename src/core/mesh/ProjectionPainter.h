#pragma once

#include <QObject>
#include <QString>
#include <QVector3D>
#include <QVector2D>
#include <QImage>
#include <QMap>

struct ProjectionStencil {
    QImage image;
    QVector3D position;    // Center position in world space
    QVector3D rotation;    // Euler angles (X, Y, Z)
    QVector3D scale;       // Scale factors (X, Y, Z)
    float opacity = 1.0f;
    bool useAlpha = true;
    bool loop = false;     // Tile/stretch vs repeat
    
    ProjectionStencil() : position(0,0,0), rotation(0,0,0), scale(1,1,1) {}
    ProjectionStencil(const QImage& img, const QVector3D& pos = QVector3D(0,0,0),
                      const QVector3D& rot = QVector3D(0,0,0), const QVector3D& sc = QVector3D(1,1,1))
        : image(img), position(pos), rotation(rot), scale(sc) {}
};

class ProjectionPainter : public QObject
{
    Q_OBJECT
public:
    explicit ProjectionPainter(QObject* parent = nullptr);
    ~ProjectionPainter();
    
    // Stencil management
    void setStencil(const QImage& image);
    QImage stencil() const { return m_stencil; }
    void setStencilPosition(const QVector3D& pos);
    void setStencilRotation(const QVector3D& rot);
    void setStencilScale(const QVector3D& sc);
    void setStencilOpacity(qreal opac);
    void setStencilUseAlpha(bool useAlpha);
    void setStencilLoop(bool loop);
    
    // Projection operations
    // Project stencil onto mesh via raycast from viewport
    int projectStencilToMesh(int objectId, const QVector3D& viewportCenter, 
                             float radius, float strength, int mode,
                             const QVector2D& uvOffset = QVector2D(0,0));
    
    // Clone from source UV to destination UV
    int cloneStencilToPoint(int objectId, const QVector2D& sourceUV, 
                            const QVector2D& destUV, float strength,
                            float blendMode = 1.0f); // 0=additive, 1=multiply, 2=screen, 3=overlay
    
    // Signals
signals:
    void stencilChanged();
    void projectCompleted(int affectedCount);
    void cloneCompleted(int affectedCount);
    
private:
    QImage m_stencil;
    QVector3D m_stencilPos;
    QVector3D m_stencilRot;
    QVector3D m_stencilScale;
    qreal m_stencilOpacity;
    bool m_useAlpha;
    bool m_loop;
    
    // Project ray from viewport point to world space, then to mesh
    bool projectPointToMesh(int objectId, const QVector3D& worldPos, 
                           QVector2D& uv, QVector3D& normal);
    
    // Sample stencil at UV coordinates
    QColor sampleStencilAt(const QVector2D& uv) const;
};