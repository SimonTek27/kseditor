#include "WeatherEditorModule.h"
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <cmath>

namespace ks {
namespace weather {

// ============================================================================
// WeatherPreviewRenderer
// ============================================================================

WeatherPreviewRenderer::WeatherPreviewRenderer(QObject* parent)
    : QObject(parent)
{
}

WeatherPreviewRenderer::~WeatherPreviewRenderer() = default;

QImage WeatherPreviewRenderer::renderPreview(const WeatherConfig& config, double time, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect rect(0, 0, width, height);

    // Find the active sequence and interpolate keyframe
    WeatherKeyframe kf;
    QVector<ParticleEffect> particles;
    for (const auto& seq : config.sequences) {
        double seqEnd = seq.startTime + seq.duration;
        if (time >= seq.startTime && time <= seqEnd) {
            kf = WeatherEditor(nullptr).interpolateKeyframe(seq, time);
            particles = getActiveParticles(seq.particleEffects, time);
            break;
        }
    }

    // Derive colors from keyframe values / weather type
    QColor skyTop, skyBottom;
    if (kf.type == "night" || time < 5.0 || time > 21.0) {
        skyTop = QColor(5, 5, 20);
        skyBottom = QColor(15, 15, 40);
    } else if (kf.type == "fog") {
        skyTop = QColor(180, 180, 190);
        skyBottom = QColor(200, 200, 210);
    } else {
        skyTop = interpolateSkyColor({kf}, time);
        skyBottom = lerpColor(skyTop, QColor(200, 210, 230), 0.4);
    }

    // 1. Sky gradient
    drawGradientSky(painter, rect, skyTop, skyBottom);

    // 2. Sun
    double sunAngle = ((time - 6.0) / 12.0) * M_PI;
    QColor sunColor = lerpColor(QColor(255, 200, 50), QColor(255, 100, 30), qBound(0.0, 1.0 - (time - 12.0) / 6.0, 1.0));
    if (time > 6.0 && time < 18.0) {
        drawSun(painter, rect, sunAngle, sunColor);
    }

    // 3. Clouds
    drawClouds(painter, rect, kf.cloudCoverage, time);

    // 4. Fog
    QColor fogColor = interpolateFogColor({kf}, time);
    double fogDensity = kf.values.value("fogDensity", 0.0);
    if (kf.type == "fog") fogDensity = qMax(fogDensity, 0.3);
    drawFog(painter, rect, fogDensity, fogColor);

    // 5. Precipitation
    if (kf.precipitation > 0.0) {
        if (kf.type.contains("rain") || kf.type == "storm") {
            drawRain(painter, rect, kf.precipitation, time);
        } else if (kf.type == "snow" || kf.type == "blizzard") {
            drawSnow(painter, rect, kf.precipitation, time);
        }
    }

    // 6. Particle effects
    drawParticles(painter, rect, particles, time);

    // 7. Lightning for storms
    if (kf.type == "storm" && kf.precipitation > 0.7) {
        drawLightning(painter, rect, kf.precipitation);
    }

    painter.end();
    return img;
}

QImage WeatherPreviewRenderer::renderSky(const WeatherConfig& config, double time, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect rect(0, 0, width, height);

    QColor skyTop, skyBottom;
    if (time < 5.0 || time > 21.0) {
        skyTop = QColor(5, 5, 20);
        skyBottom = QColor(15, 15, 40);
    } else {
        // Interpolate sky color from keyframes
        QVector<WeatherKeyframe> allKeyframes;
        for (const auto& seq : config.sequences) {
            allKeyframes.append(seq.keyframes);
        }
        skyTop = interpolateSkyColor(allKeyframes, time);
        skyBottom = lerpColor(skyTop, QColor(200, 210, 230), 0.4);
    }

    drawGradientSky(painter, rect, skyTop, skyBottom);

    // Draw sun if daytime
    if (time > 6.0 && time < 18.0) {
        double sunAngle = ((time - 6.0) / 12.0) * M_PI;
        QColor sunColor = lerpColor(QColor(255, 200, 50), QColor(255, 100, 30),
                                    qBound(0.0, 1.0 - (time - 12.0) / 6.0, 1.0));
        drawSun(painter, rect, sunAngle, sunColor);
    }

    painter.end();
    return img;
}

QImage WeatherPreviewRenderer::renderParticles(const WeatherConfig& config, double time, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect rect(0, 0, width, height);

    QVector<ParticleEffect> allParticles;
    for (const auto& seq : config.sequences) {
        allParticles.append(getActiveParticles(seq.particleEffects, time));
    }

    drawParticles(painter, rect, allParticles, time);

    // Also draw built-in precipitation
    for (const auto& seq : config.sequences) {
        WeatherKeyframe kf = WeatherEditor(nullptr).interpolateKeyframe(seq, time);
        if (kf.precipitation > 0.0) {
            if (kf.type.contains("rain") || kf.type == "storm") {
                drawRain(painter, rect, kf.precipitation, time);
            } else if (kf.type == "snow" || kf.type == "blizzard") {
                drawSnow(painter, rect, kf.precipitation, time);
            }
        }
    }

    painter.end();
    return img;
}

QColor WeatherPreviewRenderer::interpolateSkyColor(const QVector<WeatherKeyframe>& keyframes, double time)
{
    if (keyframes.isEmpty()) return QColor(135, 206, 235);

    // Find bounding keyframes
    const WeatherKeyframe* prev = nullptr;
    const WeatherKeyframe* next = nullptr;

    for (const auto& kf : keyframes) {
        if (kf.time <= time) {
            prev = &kf;
        } else {
            next = &kf;
            break;
        }
    }

    // If outside range, use nearest
    if (!prev && next) {
        prev = next;
    } else if (prev && !next) {
        next = prev;
    } else if (!prev && !next) {
        return QColor(135, 206, 235);
    }

    // Time-based sky color derivation
    auto skyFromTime = [](double t) -> QColor {
        if (t < 5.0 || t > 21.0) return QColor(10, 10, 30);        // Night
        if (t < 6.5) return QColor(60, 40, 80);                     // Pre-dawn
        if (t < 7.5) return QColor(200, 120, 80);                   // Dawn
        if (t < 9.0) return QColor(150, 180, 220);                  // Morning
        if (t < 16.0) return QColor(135, 206, 235);                 // Midday
        if (t < 17.5) return QColor(150, 180, 220);                 // Afternoon
        if (t < 19.0) return QColor(220, 140, 80);                  // Dusk
        if (t < 20.5) return QColor(80, 50, 70);                    // Post-dusk
        return QColor(10, 10, 30);                                    // Night
    };

    QColor prevColor = skyFromTime(prev->time);
    QColor nextColor = skyFromTime(next->time);

    // Adjust for cloud coverage (darkens sky)
    double avgClouds = (prev->cloudCoverage + next->cloudCoverage) / 2.0;
    double cloudDarken = 1.0 - avgClouds * 0.3;

    if (prev == next) {
        return QColor(prevColor.red() * cloudDarken, prevColor.green() * cloudDarken, prevColor.blue() * cloudDarken);
    }

    double t = (next->time == prev->time) ? 0.0 : (time - prev->time) / (next->time - prev->time);
    t = qBound(0.0, t, 1.0);

    return lerpColor(prevColor, nextColor, t);
}

QColor WeatherPreviewRenderer::interpolateFogColor(const QVector<WeatherKeyframe>& keyframes, double time)
{
    if (keyframes.isEmpty()) return QColor(200, 200, 220);

    const WeatherKeyframe* prev = nullptr;
    const WeatherKeyframe* next = nullptr;

    for (const auto& kf : keyframes) {
        if (kf.time <= time) {
            prev = &kf;
        } else {
            next = &kf;
            break;
        }
    }

    if (!prev && next) prev = next;
    else if (prev && !next) next = prev;
    else if (!prev && !next) return QColor(200, 200, 220);

    // Fog color is influenced by time of day and weather type
    auto fogFromTime = [](double t, const QString& type) -> QColor {
        if (type == "fog") return QColor(200, 200, 210);
        if (t < 5.0 || t > 21.0) return QColor(20, 20, 40);
        if (t < 7.0 || t > 19.0) return QColor(160, 140, 130);
        return QColor(190, 195, 210);
    };

    QColor prevColor = fogFromTime(prev->time, prev->type);
    QColor nextColor = fogFromTime(next->time, next->type);

    if (prev == next) return prevColor;

    double t = (next->time == prev->time) ? 0.0 : (time - prev->time) / (next->time - prev->time);
    t = qBound(0.0, t, 1.0);

    return lerpColor(prevColor, nextColor, t);
}

double WeatherPreviewRenderer::interpolateValue(const QVector<WeatherKeyframe>& keyframes, double time, double (WeatherKeyframe::* member))
{
    if (keyframes.isEmpty()) return 0.0;

    const WeatherKeyframe* prev = nullptr;
    const WeatherKeyframe* next = nullptr;

    for (const auto& kf : keyframes) {
        if (kf.time <= time) {
            prev = &kf;
        } else {
            next = &kf;
            break;
        }
    }

    if (!prev && next) prev = next;
    else if (prev && !next) next = prev;
    else if (!prev && !next) return 0.0;

    double prevVal = prev->*member;
    double nextVal = next->*member;

    if (prev == next) return prevVal;

    double t = (next->time == prev->time) ? 0.0 : (time - prev->time) / (next->time - prev->time);
    t = qBound(0.0, t, 1.0);

    return prevVal + (nextVal - prevVal) * t;
}

QVector<ParticleEffect> WeatherPreviewRenderer::getActiveParticles(const QVector<ParticleEffect>& effects, double time)
{
    QVector<ParticleEffect> active;
    for (const auto& effect : effects) {
        if (effect.intensity > 0.0) {
            active.append(effect);
        }
    }
    return active;
}

QColor WeatherPreviewRenderer::lerpColor(const QColor& a, const QColor& b, double t)
{
    t = qBound(0.0, t, 1.0);
    int r = a.red() + (b.red() - a.red()) * t;
    int g = a.green() + (b.green() - a.green()) * t;
    int bl = a.blue() + (b.blue() - a.blue()) * t;
    int al = a.alpha() + (b.alpha() - a.alpha()) * t;
    return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, bl, 255), qBound(0, al, 255));
}

void WeatherPreviewRenderer::drawGradientSky(QPainter& painter, const QRect& rect, const QColor& top, const QColor& bottom) const
{
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(0.6, lerpColor(top, bottom, 0.5));
    gradient.setColorAt(1.0, bottom);
    painter.fillRect(rect, gradient);
}

void WeatherPreviewRenderer::drawSun(QPainter& painter, const QRect& rect, double sunAngle, const QColor& sunColor) const
{
    // Sun position based on angle (0=horizon, PI/2=zenith)
    double sunX = rect.width() * 0.5 + rect.width() * 0.4 * std::cos(sunAngle - M_PI / 2.0);
    double sunY = rect.height() * 0.3 * (1.0 - std::sin(sunAngle));

    // Outer glow
    QRadialGradient glow(sunX, sunY, 60.0);
    QColor glowColor = lerpColor(sunColor, QColor(255, 255, 200), 0.5);
    glowColor.setAlpha(80);
    glow.setColorAt(0.0, glowColor);
    glowColor.setAlpha(0);
    glow.setColorAt(1.0, glowColor);
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(QPointF(sunX, sunY), 60.0, 60.0);

    // Sun disc
    QRadialGradient disc(sunX, sunY, 20.0);
    disc.setColorAt(0.0, QColor(255, 255, 240));
    disc.setColorAt(0.4, sunColor);
    QColor edgeColor = lerpColor(sunColor, QColor(255, 180, 50), 0.5);
    edgeColor.setAlpha(200);
    disc.setColorAt(1.0, edgeColor);
    painter.setBrush(disc);
    painter.drawEllipse(QPointF(sunX, sunY), 20.0, 20.0);
}

void WeatherPreviewRenderer::drawClouds(QPainter& painter, const QRect& rect, double cloudCoverage, double time) const
{
    if (cloudCoverage <= 0.01) return;

    QRandomGenerator* rng = QRandomGenerator::global();
    int numClouds = static_cast<int>(cloudCoverage * 30) + 5;
    double coverage01 = qBound(0.0, cloudCoverage, 1.0);

    // Cloud color varies with time
    QColor cloudBase;
    if (time < 5.0 || time > 21.0) {
        cloudBase = QColor(30, 30, 50);
    } else if (time < 7.0 || time > 19.0) {
        cloudBase = QColor(180, 140, 120);
    } else {
        cloudBase = QColor(220, 225, 235);
    }

    painter.setPen(Qt::NoPen);

    for (int i = 0; i < numClouds; ++i) {
        double cx = rng->bounded(rect.width());
        double cy = rng->bounded(static_cast<int>(rect.height() * 0.5));
        double w = 40.0 + rng->bounded(80.0) * coverage01;
        double h = 15.0 + rng->bounded(20.0);

        QColor cColor = lerpColor(cloudBase, QColor(255, 255, 255), rng->bounded(100) / 200.0);
        cColor.setAlpha(static_cast<int>(120 * coverage01 + rng->bounded(60)));

        QRadialGradient cloud(cx, cy, w * 0.6);
        cloud.setColorAt(0.0, cColor);
        cColor.setAlpha(0);
        cloud.setColorAt(1.0, cColor);

        painter.setBrush(cloud);
        painter.drawEllipse(QPointF(cx, cy), w, h);
    }
}

void WeatherPreviewRenderer::drawParticles(QPainter& painter, const QRect& rect, const QVector<ParticleEffect>& effects, double time) const
{
    QRandomGenerator* rng = QRandomGenerator::global();

    for (const auto& effect : effects) {
        int count = static_cast<int>(effect.intensity * effect.coverage * 100);

        for (int i = 0; i < count; ++i) {
            double x = rng->bounded(rect.width());
            double y = rng->bounded(rect.height());
            double size = effect.particleSize * (1.0 + rng->bounded(1.0));

            QColor pColor = effect.color;
            pColor.setAlpha(static_cast<int>(effect.intensity * 180));

            painter.setPen(Qt::NoPen);
            painter.setBrush(pColor);
            painter.drawEllipse(QPointF(x, y), size, size);
        }
    }
}

void WeatherPreviewRenderer::drawRain(QPainter& painter, const QRect& rect, double intensity, double time) const
{
    QRandomGenerator* rng = QRandomGenerator::global();
    int numDrops = static_cast<int>(intensity * 200) + 10;

    QPen rainPen(QColor(180, 200, 220, static_cast<int>(100 + intensity * 100)));
    rainPen.setWidthF(1.0 + intensity);
    painter.setPen(rainPen);

    double windOffset = std::sin(time * 0.1) * 5.0 * intensity;

    for (int i = 0; i < numDrops; ++i) {
        double x = rng->bounded(rect.width());
        double y = rng->bounded(rect.height());
        double len = 8.0 + rng->bounded(15.0) * intensity;

        painter.drawLine(QPointF(x, y), QPointF(x + windOffset, y + len));
    }
}

void WeatherPreviewRenderer::drawSnow(QPainter& painter, const QRect& rect, double intensity, double time) const
{
    QRandomGenerator* rng = QRandomGenerator::global();
    int numFlakes = static_cast<int>(intensity * 150) + 10;

    painter.setPen(Qt::NoPen);

    for (int i = 0; i < numFlakes; ++i) {
        double x = rng->bounded(rect.width());
        double y = rng->bounded(rect.height());
        double size = 1.5 + rng->bounded(3.0) * intensity;
        double drift = std::sin(time * 2.0 + i * 0.5) * 3.0;

        QColor flakeColor = QColor(240, 245, 255, static_cast<int>(150 + rng->bounded(100)));
        painter.setBrush(flakeColor);
        painter.drawEllipse(QPointF(x + drift, y), size, size);
    }
}

void WeatherPreviewRenderer::drawFog(QPainter& painter, const QRect& rect, double density, const QColor& color) const
{
    if (density <= 0.01) return;

    QLinearGradient fog(rect.topLeft(), rect.bottomLeft());
    QColor c1 = color;
    c1.setAlpha(static_cast<int>(density * 180));
    QColor c2 = color;
    c2.setAlpha(static_cast<int>(density * 100));

    fog.setColorAt(0.0, QColor(color.red(), color.green(), color.blue(), 0));
    fog.setColorAt(0.3, c1);
    fog.setColorAt(0.7, c2);
    fog.setColorAt(1.0, c1);

    painter.fillRect(rect, fog);
}

void WeatherPreviewRenderer::drawLightning(QPainter& painter, const QRect& rect, double intensity) const
{
    QRandomGenerator* rng = QRandomGenerator::global();

    // Only draw lightning occasionally (simulated by time-based check)
    if (rng->bounded(100) > static_cast<int>(intensity * 30)) return;

    double startX = rect.width() * (0.2 + rng->bounded(60) / 100.0);
    double startY = 0;

    QPen boltPen(QColor(220, 230, 255, 200));
    boltPen.setWidth(2);
    painter.setPen(boltPen);
    painter.setBrush(Qt::NoBrush);

    QVector<QPointF> points;
    points.append(QPointF(startX, startY));

    double x = startX;
    double y = startY;
    int segments = 8 + rng->bounded(6);

    for (int i = 0; i < segments; ++i) {
        x += rng->bounded(30) - 15.0;
        y += rect.height() / segments;
        points.append(QPointF(x, y));

        // Branch
        if (rng->bounded(100) < 25) {
            double bx = x + (rng->bounded(40) - 20.0);
            double by = y + 20.0;
            painter.drawLine(QPointF(x, y), QPointF(bx, by));
        }
    }

    for (int i = 0; i < points.size() - 1; ++i) {
        painter.drawLine(points[i], points[i + 1]);
    }

    // Flash overlay
    QColor flash(255, 255, 255, static_cast<int>(intensity * 30));
    painter.fillRect(rect, flash);
}

// ============================================================================
// WeatherParticleSystem
// ============================================================================

WeatherParticleSystem::WeatherParticleSystem(QObject* parent)
    : QObject(parent)
    , m_rng(QRandomGenerator::global())
{
}

WeatherParticleSystem::~WeatherParticleSystem() = default;

void WeatherParticleSystem::setEmitterConfig(const EmitterConfig& config)
{
    m_config = config;
}

void WeatherParticleSystem::update(double deltaTime)
{
    // Update existing particles
    for (auto it = m_particles.begin(); it != m_particles.end();) {
        updateParticle(*it, deltaTime);
        it->life -= deltaTime;
        if (it->life <= 0.0 || !it->active) {
            it = m_particles.erase(it);
        } else {
            ++it;
        }
    }

    // Spawn new particles
    spawnParticles(deltaTime);
}

void WeatherParticleSystem::render(QPainter& painter, const QRectF& viewport) const
{
    for (const auto& p : m_particles) {
        if (!isInViewport(p, viewport)) continue;

        QColor c = p.color;
        double lifeRatio = p.life / p.maxLife;
        c.setAlpha(static_cast<int>(lifeRatio * 255));

        painter.setPen(Qt::NoPen);
        painter.setBrush(c);
        painter.drawEllipse(p.position, p.size * lifeRatio, p.size * lifeRatio);
    }
}

void WeatherParticleSystem::emitParticles(int count, const QPointF& position)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = position + QPointF(m_rng->bounded(20) - 10.0, m_rng->bounded(10) - 5.0);
        p.velocity = QPointF(
            m_config.wind.x() + (m_rng->bounded(20) - 10.0),
            m_config.gravity.y() + m_rng->bounded(static_cast<int>((m_config.maxSpeed - m_config.minSpeed) * 10)) / 10.0
        );
        p.maxLife = m_config.minLife + m_rng->bounded((m_config.maxLife - m_config.minLife) * 1000) / 1000.0;
        p.life = p.maxLife;
        p.size = m_config.minSize + m_rng->bounded((m_config.maxSize - m_config.minSize) * 100) / 100.0;
        p.color = interpolateGradient(m_rng->bounded(1000) / 1000.0);
        p.active = true;

        m_particles.append(p);
        m_totalSpawned++;
    }
}

void WeatherParticleSystem::clear()
{
    m_particles.clear();
}

void WeatherParticleSystem::setParticleTexture(const QImage& texture)
{
    m_particleTexture = texture;
}

void WeatherParticleSystem::setColorGradient(const QGradient& gradient)
{
    m_colorGradient = gradient;
}

int WeatherParticleSystem::activeParticleCount() const
{
    return m_particles.size();
}

int WeatherParticleSystem::totalSpawned() const
{
    return m_totalSpawned;
}

void WeatherParticleSystem::spawnParticles(double deltaTime)
{
    m_spawnTimer += deltaTime;
    double spawnInterval = 1.0 / m_config.spawnRate;

    while (m_spawnTimer >= spawnInterval) {
        m_spawnTimer -= spawnInterval;
        emitParticles(1, m_config.spawnArea.center());
    }
}

void WeatherParticleSystem::updateParticle(Particle& p, double deltaTime)
{
    p.velocity += m_config.gravity * deltaTime;
    p.velocity += m_config.wind * deltaTime;
    p.position += p.velocity * deltaTime;
    p.rotation += p.rotationSpeed * deltaTime;
}

bool WeatherParticleSystem::isInViewport(const Particle& p, const QRectF& viewport) const
{
    return viewport.contains(p.position);
}

QColor WeatherParticleSystem::interpolateGradient(double t) const
{
    t = qBound(0.0, t, 1.0);

    QLinearGradient linearGrad(0, 0, 1, 0);
    if (m_colorGradient.type() == QGradient::LinearGradient) {
        const QLinearGradient* lg = static_cast<const QLinearGradient*>(&m_colorGradient);
        linearGrad = *lg;
    }

    // Sample the gradient at position t
    QVector<QGradientStop> stops = linearGrad.stops();
    if (stops.isEmpty()) return QColor(255, 255, 255);

    if (stops.size() == 1) return stops.first().second;

    for (int i = 0; i < stops.size() - 1; ++i) {
        if (t >= stops[i].first && t <= stops[i + 1].first) {
            double localT = (stops[i + 1].first == stops[i].first) ? 0.0 :
                (t - stops[i].first) / (stops[i + 1].first - stops[i].first);
            return WeatherPreviewRenderer::lerpColor(stops[i].second, stops[i + 1].second, localT);
        }
    }

    return stops.last().second;
}

} // namespace weather
} // namespace ks
