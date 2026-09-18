#include "TextureAtlas.h"
#include <QFile>
#include <QPainter>
#include <QDebug>
#include <QRandomGenerator>

namespace ks {

TextureAtlas* TextureAtlas::s_instance = nullptr;

TextureAtlas::TextureAtlas(QObject* parent)
    : QObject(parent)
{}

TextureAtlas* TextureAtlas::instance() {
    if (!s_instance) {
        s_instance = new TextureAtlas();
    }
    return s_instance;
}

void TextureAtlas::addTexture(const QString& name, const QString& imagePath) {
    AtlasTexture tex;
    tex.name = name;
    
    if (tex.image.load(imagePath)) {
        m_textures.append(tex);
        qDebug() << "Added texture:" << name << "size:" << tex.image.size();
    } else {
        qWarning() << "Failed to load texture:" << imagePath;
    }
}

void TextureAtlas::clearTextures() {
    m_textures.clear();
    m_atlasImage = QImage();
}

bool TextureAtlas::generateAtlas(int atlasSize, int padding) {
    if (m_textures.isEmpty()) {
        qWarning() << "No textures to pack";
        return false;
    }
    
    m_atlasSize = QSize(atlasSize, atlasSize);
    m_atlasImage = QImage(m_atlasSize, QImage::Format_RGBA8888);
    m_atlasImage.fill(Qt::transparent);
    
    if (!packTextures(atlasSize, padding)) {
        emit generationComplete(false);
        return false;
    }
    
    emit generationComplete(true);
    return true;
}

bool TextureAtlas::packTextures(int atlasSize, int padding) {
    QPainter painter(&m_atlasImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    
    int x = padding;
    int y = padding;
    int rowHeight = 0;
    
    for (int i = 0; i < m_textures.size(); ++i) {
        AtlasTexture& tex = m_textures[i];
        
        if (x + tex.image.width() + padding > atlasSize) {
            x = padding;
            y += rowHeight + padding;
            rowHeight = 0;
        }
        
        if (y + tex.image.height() + padding > atlasSize) {
            qWarning() << "Atlas too small for textures";
            return false;
        }
        
        tex.region = QRect(x, y, tex.image.width(), tex.image.height());
        painter.drawImage(x, y, tex.image);
        
        x += tex.image.width() + padding;
        rowHeight = qMax(rowHeight, tex.image.height());
        
        emit generationProgress(i + 1, m_textures.size());
    }
    
    return true;
}

QList<QRect> TextureAtlas::calculatePacking(int width, int height) {
    QList<QRect> rects;
    int x = 4, y = 4, rowH = 0;
    for (int i = 0; i < m_textures.size(); ++i) {
        QSize s = m_textures[i].image.size();
        if (s.width() <= 0 || s.height() <= 0) continue;
        if (x + s.width() + 4 > width) { x = 4; y += rowH + 4; rowH = 0; }
        if (y + s.height() + 4 > height) break;
        rects.append(QRect(x, y, s.width(), s.height()));
        x += s.width() + 4;
        rowH = qMax(rowH, s.height());
    }
    return rects;
}

bool TextureAtlas::saveAtlas(const QString& outputPath) {
    return m_atlasImage.save(outputPath);
}

bool TextureAtlas::saveUVMap(const QString& outputPath) {
    QImage uvMap(m_atlasSize, QImage::Format_RGBA8888);
    uvMap.fill(Qt::black);
    
    QPainter painter(&uvMap);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    
    for (const AtlasTexture& tex : m_textures) {
        QColor color(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), 255);
        painter.fillRect(tex.region, color);
    }
    
    return uvMap.save(outputPath);
}

}