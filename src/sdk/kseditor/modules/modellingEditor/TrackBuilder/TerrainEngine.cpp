#include "TerrainEngine.h"
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <random>

namespace ks { namespace track {

TerrainEngine::TerrainEngine(QObject* parent) : QObject(parent) {}

// ============================================================================
void TerrainEngine::init(const TerrainConfig& config)
{
    m_cfg = config;
    int n = config.gridWidth * config.gridHeight;
    m_heightmap.assign(n, 0.f);
    m_normals.assign(n, QVector3D(0,1,0));
    m_layers.clear();
    // Default base layer
    addLayer("Base", "terrain_base", 20.f);
    for (auto& l : m_layers) l.weights.assign(n, 1.f);
}

void TerrainEngine::reset()
{
    init(m_cfg);
    emit modified(QRect(0,0,m_cfg.gridWidth,m_cfg.gridHeight));
}

// ============================================================================
// Height access
// ============================================================================
float TerrainEngine::getHeight(int x, int z) const
{
    if (x<0||x>=m_cfg.gridWidth||z<0||z>=m_cfg.gridHeight) return 0.f;
    return m_heightmap[z*m_cfg.gridWidth+x];
}

void TerrainEngine::setHeight(int x, int z, float h)
{
    if (x<0||x>=m_cfg.gridWidth||z<0||z>=m_cfg.gridHeight) return;
    m_heightmap[z*m_cfg.gridWidth+x] = qBound(m_cfg.minElevation, h, m_cfg.maxElevation);
}

void TerrainEngine::gridFromWorld(float wx, float wz, int& gx, int& gz) const
{
    gx = int((wx / m_cfg.worldWidth  + 0.5f) * (m_cfg.gridWidth  - 1));
    gz = int((wz / m_cfg.worldHeight + 0.5f) * (m_cfg.gridHeight - 1));
}

void TerrainEngine::worldFromGrid(int gx, int gz, float& wx, float& wz) const
{
    wx = (float(gx) / (m_cfg.gridWidth  - 1) - 0.5f) * m_cfg.worldWidth;
    wz = (float(gz) / (m_cfg.gridHeight - 1) - 0.5f) * m_cfg.worldHeight;
}

float TerrainEngine::getHeightWorld(float wx, float wz) const
{
    float nx = (wx/m_cfg.worldWidth + 0.5f) * (m_cfg.gridWidth  - 1);
    float nz = (wz/m_cfg.worldHeight + 0.5f) * (m_cfg.gridHeight - 1);
    int x0=int(nx), z0=int(nz), x1=x0+1, z1=z0+1;
    x1=qMin(x1,m_cfg.gridWidth-1); z1=qMin(z1,m_cfg.gridHeight-1);
    float fx=nx-x0, fz=nz-z0;
    float h00=getHeight(x0,z0), h10=getHeight(x1,z0);
    float h01=getHeight(x0,z1), h11=getHeight(x1,z1);
    return (h00*(1-fx)+h10*fx)*(1-fz) + (h01*(1-fx)+h11*fx)*fz;
}

void TerrainEngine::setHeightWorld(float wx, float wz, float h)
{
    int gx,gz; gridFromWorld(wx,wz,gx,gz);
    setHeight(gx,gz,h);
}

// ============================================================================
// Brush falloff
// ============================================================================
float TerrainEngine::falloff(float distNorm) const
{
    // Smooth falloff: 1 at centre, 0 at edge; controlled by m_brushFalloff
    float t = qBound(0.f, 1.f - distNorm, 1.f);
    float f = m_brushFalloff;
    if (f < 0.01f) return t > 0.f ? 1.f : 0.f;
    return std::pow(t, 1.f / f);
}

// ============================================================================
// Apply brush
// ============================================================================
void TerrainEngine::applyBrush(float wx, float wz)
{
    int cx,cz; gridFromWorld(wx,wz,cx,cz);
    float cellW = m_cfg.worldWidth  / m_cfg.gridWidth;
    float cellH = m_cfg.worldHeight / m_cfg.gridHeight;
    int radiusG = int(std::ceil(m_brushRadius / qMin(cellW,cellH)));

    int x0=qMax(0,cx-radiusG), x1=qMin(m_cfg.gridWidth-1,cx+radiusG);
    int z0=qMax(0,cz-radiusG), z1=qMin(m_cfg.gridHeight-1,cz+radiusG);

    float dt = 0.016f * m_brushStrength;  // ~60fps delta

    static std::mt19937 rng(42);
    static std::uniform_real_distribution<float> dist(-1.f,1.f);

    for (int z=z0;z<=z1;++z) {
        for (int x=x0;x<=x1;++x) {
            float bx,bz; worldFromGrid(x,z,bx,bz);
            float d = std::sqrt((bx-wx)*(bx-wx)+(bz-wz)*(bz-wz));
            if (d > m_brushRadius) continue;
            float w = falloff(d/m_brushRadius) * dt;
            float h = getHeight(x,z);
            switch (m_brushMode) {
            case BrushMode::Raise:
                setHeight(x,z, h + w * 20.f);
                break;
            case BrushMode::Lower:
                setHeight(x,z, h - w * 20.f);
                break;
            case BrushMode::Smooth: {
                float avg = 0.f; int cnt=0;
                for (int dz=-1;dz<=1;++dz) for (int dx=-1;dx<=1;++dx) {
                    float s=getHeight(x+dx,z+dz); avg+=s; ++cnt;
                }
                avg/=cnt;
                setHeight(x,z, h + (avg-h)*w*5.f);
                break;
            }
            case BrushMode::Flatten:
                setHeight(x,z, h + (m_flattenTarget-h)*w*5.f);
                break;
            case BrushMode::Noise:
                setHeight(x,z, h + dist(rng)*w*10.f);
                break;
            case BrushMode::Ramp:
                // Ramp handled externally via flattenRegion
                break;
            case BrushMode::Erosion:
                // Basic thermal erosion step
                for (int dz=-1;dz<=1;++dz) for (int dx=-1;dx<=1;++dx) {
                    if(dx==0&&dz==0) continue;
                    float nb=getHeight(x+dx,z+dz);
                    float diff=h-nb;
                    if (diff>0.5f) {
                        float mv=diff*0.05f*w;
                        setHeight(x,z,h-mv);
                        setHeight(x+dx,z+dz,nb+mv);
                        h=getHeight(x,z);
                    }
                }
                break;
            }
        }
    }
    emit modified(QRect(x0,z0,x1-x0+1,z1-z0+1));
}

// ============================================================================
// One-shot operations
// ============================================================================
void TerrainEngine::flattenRegion(float wx, float wz, float radius, float targetH)
{
    float old=m_brushStrength, oldT=m_flattenTarget;
    BrushMode oldM=m_brushMode;
    m_brushMode=BrushMode::Flatten; m_brushStrength=1.f; m_flattenTarget=targetH;
    // Multiple passes for full flatten
    for (int i=0;i<10;++i) applyBrush(wx,wz);
    m_brushMode=oldM; m_brushStrength=old; m_flattenTarget=oldT;
}

void TerrainEngine::addNoise(float wx, float wz, float radius, float amplitude, float frequency)
{
    int cx,cz; gridFromWorld(wx,wz,cx,cz);
    float cellW=m_cfg.worldWidth/m_cfg.gridWidth;
    int rg=int(std::ceil(radius/cellW));

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> nd(-1.f,1.f);

    for (int z=qMax(0,cz-rg);z<=qMin(m_cfg.gridHeight-1,cz+rg);++z) {
        for (int x=qMax(0,cx-rg);x<=qMin(m_cfg.gridWidth-1,cx+rg);++x) {
            float bx,bz; worldFromGrid(x,z,bx,bz);
            float d=std::sqrt((bx-wx)*(bx-wx)+(bz-wz)*(bz-wz));
            if (d>radius) continue;
            float f=falloff(d/radius);
            // Simple value noise
            float nx = bx*frequency*0.01f;
            float nz = bz*frequency*0.01f;
            float n = std::sin(nx*1.3f)*std::cos(nz*1.7f)*0.5f
                    + std::sin(nx*2.7f+0.5f)*std::cos(nz*3.1f+0.3f)*0.25f
                    + std::sin(nx*5.3f+1.1f)*std::cos(nz*6.3f+0.9f)*0.125f;
            setHeight(x,z, getHeight(x,z)+n*amplitude*f);
        }
    }
}

void TerrainEngine::erode(int iterations)
{
    for (int it=0;it<iterations;++it) {
        for (int z=1;z<m_cfg.gridHeight-1;++z) {
            for (int x=1;x<m_cfg.gridWidth-1;++x) {
                float h=getHeight(x,z);
                float minNb=h; int mx=x,mz=z;
                auto check=[&](int nx,int nz){ float nh=getHeight(nx,nz); if(nh<minNb){minNb=nh;mx=nx;mz=nz;} };
                check(x-1,z);check(x+1,z);check(x,z-1);check(x,z+1);
                if (minNb<h-0.5f) {
                    float mv=(h-minNb)*0.1f;
                    setHeight(x,z,h-mv);
                    setHeight(mx,mz,minNb+mv);
                }
            }
        }
    }
}

void TerrainEngine::hydraulicErode(int iterations)
{
    // Simplified hydraulic erosion: deposit/pick based on slope
    QVector<float> water(m_heightmap.size(), 0.01f);
    QVector<float> sediment(m_heightmap.size(), 0.f);
    float ks_=0.01f, kd=0.01f, ke=0.05f, kc=0.1f;

    for (int it=0;it<iterations;++it) {
        for (int z=1;z<m_cfg.gridHeight-1;++z) {
            for (int x=1;x<m_cfg.gridWidth-1;++x) {
                float h=getHeight(x,z);
                float w=water[z*m_cfg.gridWidth+x];
                // Find steepest descent
                float bestDH=-1.f; int bx=x,bz=z;
                auto check=[&](int nx,int nz){
                    float dh=(h+w)-(getHeight(nx,nz)+water[nz*m_cfg.gridWidth+nx]);
                    if(dh>bestDH){bestDH=dh;bx=nx;bz=nz;}
                };
                check(x-1,z);check(x+1,z);check(x,z-1);check(x,z+1);
                if (bestDH>0.f) {
                    float flow=qMin(w, bestDH*0.5f);
                    water[z*m_cfg.gridWidth+x]-=flow;
                    water[bz*m_cfg.gridWidth+bx]+=flow;
                    float cap=kc*flow*bestDH;
                    float sed=sediment[z*m_cfg.gridWidth+x];
                    if (sed>cap) {
                        float dep=(sed-cap)*kd;
                        setHeight(x,z,h+dep);
                        sediment[z*m_cfg.gridWidth+x]-=dep;
                    } else {
                        float pick=qMin(ks_*(cap-sed), h*ke);
                        setHeight(x,z,h-pick);
                        sediment[z*m_cfg.gridWidth+x]+=pick;
                    }
                }
            }
        }
    }
}

void TerrainEngine::normalise(float targetMin, float targetMax)
{
    auto [mn,mx]=std::minmax_element(m_heightmap.begin(),m_heightmap.end());
    float range=*mx-*mn;
    if (range<1e-4f) return;
    float s=(targetMax-targetMin)/range;
    for (float& h:m_heightmap) h=(h-*mn)*s+targetMin;
}

// ============================================================================
// Normals
// ============================================================================
void TerrainEngine::recalcNormals()
{
    m_normals.resize(m_cfg.gridWidth*m_cfg.gridHeight);
    float dx=m_cfg.worldWidth/(m_cfg.gridWidth-1);
    float dz=m_cfg.worldHeight/(m_cfg.gridHeight-1);
    for (int z=0;z<m_cfg.gridHeight;++z) {
        for (int x=0;x<m_cfg.gridWidth;++x) {
            float l=getHeight(qMax(0,x-1),z);
            float r=getHeight(qMin(m_cfg.gridWidth-1,x+1),z);
            float u=getHeight(x,qMax(0,z-1));
            float d_=getHeight(x,qMin(m_cfg.gridHeight-1,z+1));
            QVector3D n(-(r-l)/(2*dx), 1.f, -(d_-u)/(2*dz));
            m_normals[z*m_cfg.gridWidth+x]=n.normalized();
        }
    }
    emit normalsRecalculated();
}

QVector3D TerrainEngine::normalAt(int x,int z) const
{
    if (m_normals.isEmpty()||x<0||x>=m_cfg.gridWidth||z<0||z>=m_cfg.gridHeight)
        return {0,1,0};
    return m_normals[z*m_cfg.gridWidth+x];
}

QVector3D TerrainEngine::normalAtWorld(float wx,float wz) const
{
    int gx,gz; gridFromWorld(wx,wz,gx,gz);
    return normalAt(gx,gz);
}

// ============================================================================
// Import
// ============================================================================
bool TerrainEngine::importFromImage(const QImage& img, float minH, float maxH)
{
    if (img.isNull()) return false;
    QImage grey=img.convertToFormat(QImage::Format_Grayscale8)
                    .scaled(m_cfg.gridWidth,m_cfg.gridHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    float range=maxH-minH;
    for (int z=0;z<m_cfg.gridHeight;++z)
        for (int x=0;x<m_cfg.gridWidth;++x)
            setHeight(x,z, minH + float(qGray(grey.pixel(x,z)))/255.f * range);
    recalcNormals();
    emit modified(QRect(0,0,m_cfg.gridWidth,m_cfg.gridHeight));
    return true;
}

bool TerrainEngine::importFromSRTM(const QString& filePath)
{
    // SRTM .hgt: big-endian int16, 1201x1201 or 3601x3601
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data=f.readAll();
    int expected1201 = 1201*1201*2;
    int expected3601 = 3601*3601*2;
    int side = (data.size()==expected3601) ? 3601 : 1201;
    if (data.size()!=side*side*2) { qWarning()<<"Invalid SRTM size"; return false; }

    auto readInt16BE=[&](int i)->float{
        uchar a=data[i*2], b=data[i*2+1];
        int16_t v=(int16_t)((a<<8)|b);
        return (v==-32768) ? 0.f : float(v);
    };

    // Resample to our grid
    for (int z=0;z<m_cfg.gridHeight;++z) {
        for (int x=0;x<m_cfg.gridWidth;++x) {
            float sx=float(x)/(m_cfg.gridWidth-1)*(side-1);
            float sz=float(z)/(m_cfg.gridHeight-1)*(side-1);
            int ix=int(sx), iz=int(sz);
            float fx=sx-ix, fz=sz-iz;
            float h00=readInt16BE(iz*side+ix);
            float h10=readInt16BE(iz*side+qMin(ix+1,side-1));
            float h01=readInt16BE(qMin(iz+1,side-1)*side+ix);
            float h11=readInt16BE(qMin(iz+1,side-1)*side+qMin(ix+1,side-1));
            setHeight(x,z,(h00*(1-fx)+h10*fx)*(1-fz)+(h01*(1-fx)+h11*fx)*fz);
        }
    }
    recalcNormals();
    emit modified(QRect(0,0,m_cfg.gridWidth,m_cfg.gridHeight));
    return true;
}

bool TerrainEngine::importFromRaw(const QVector<float>& data, int w, int h)
{
    if (data.size()!=w*h) return false;
    // Resample to grid
    for (int z=0;z<m_cfg.gridHeight;++z) {
        for (int x=0;x<m_cfg.gridWidth;++x) {
            float sx=float(x)/(m_cfg.gridWidth-1)*(w-1);
            float sz=float(z)/(m_cfg.gridHeight-1)*(h-1);
            int ix=int(sx),iz=int(sz);
            float fx=sx-ix,fz=sz-iz;
            float h00=data[iz*w+ix],h10=data[iz*w+qMin(ix+1,w-1)];
            float h01=data[qMin(iz+1,h-1)*w+ix],h11=data[qMin(iz+1,h-1)*w+qMin(ix+1,w-1)];
            setHeight(x,z,(h00*(1-fx)+h10*fx)*(1-fz)+(h01*(1-fx)+h11*fx)*fz);
        }
    }
    recalcNormals();
    emit modified(QRect(0,0,m_cfg.gridWidth,m_cfg.gridHeight));
    return true;
}

// ============================================================================
// Texture paint
// ============================================================================
int TerrainEngine::addLayer(const QString& name, const QString& textureId, float uvScale)
{
    TerrainLayer l;
    l.name=name; l.textureId=textureId; l.uvScale=uvScale;
    l.weights.assign(m_cfg.gridWidth*m_cfg.gridHeight, 0.f);
    m_layers.append(l);
    return m_layers.size()-1;
}

void TerrainEngine::removeLayer(int index)
{
    if (index>=0&&index<m_layers.size()) m_layers.removeAt(index);
}

void TerrainEngine::paintLayer(int li, float wx, float wz, float radius, float opacity)
{
    if (li<0||li>=m_layers.size()) return;
    auto& layer=m_layers[li];
    int cx,cz; gridFromWorld(wx,wz,cx,cz);
    int rg=int(std::ceil(radius/(m_cfg.worldWidth/m_cfg.gridWidth)));
    for (int z=qMax(0,cz-rg);z<=qMin(m_cfg.gridHeight-1,cz+rg);++z) {
        for (int x=qMax(0,cx-rg);x<=qMin(m_cfg.gridWidth-1,cx+rg);++x) {
            float bx,bz_; worldFromGrid(x,z,bx,bz_);
            float d=std::sqrt((bx-wx)*(bx-wx)+(bz_-wz)*(bz_-wz));
            if (d>radius) continue;
            float w=falloff(d/radius)*opacity;
            int idx=z*m_cfg.gridWidth+x;
            layer.weights[idx]=qBound(0.f,layer.weights[idx]+w,1.f);
        }
    }
}

void TerrainEngine::autoMaskBySlope(int li, float minSlope, float maxSlope)
{
    if (li<0||li>=m_layers.size()) return;
    auto& layer=m_layers[li];
    for (int z=0;z<m_cfg.gridHeight;++z) {
        for (int x=0;x<m_cfg.gridWidth;++x) {
            QVector3D n=normalAt(x,z);
            float slope=std::acos(qBound(-1.f,n.y(),1.f))*180.f/float(M_PI);
            float t=(slope-minSlope)/(maxSlope-minSlope);
            layer.weights[z*m_cfg.gridWidth+x]=qBound(0.f,t,1.f);
        }
    }
}

void TerrainEngine::autoMaskByRoad(int li, float roadBuffer, const QVector<Road>& roads)
{
    if (li<0||li>=m_layers.size()) return;
    auto& wts=m_layers[li].weights;
    std::fill(wts.begin(),wts.end(),0.f);
    for (int z=0;z<m_cfg.gridHeight;++z) {
        for (int x=0;x<m_cfg.gridWidth;++x) {
            float wx,wz_; worldFromGrid(x,z,wx,wz_);
            for (const auto& road:roads) {
                for (const auto& pt:road.points) {
                    float d=QVector2D(wx-pt.position.x(),wz_-pt.position.z()).length();
                    if (d<roadBuffer+pt.width) {
                        float t=1.f-qBound(0.f,(d-pt.width)/roadBuffer,1.f);
                        wts[z*m_cfg.gridWidth+x]=qMax(wts[z*m_cfg.gridWidth+x],t);
                    }
                }
            }
        }
    }
}

// ============================================================================
// Export
// ============================================================================
QImage TerrainEngine::toHeightmapImage(int resolution) const
{
    int w=(resolution>0)?resolution:m_cfg.gridWidth;
    int h=(resolution>0)?resolution:m_cfg.gridHeight;
    QImage img(w,h,QImage::Format_Grayscale16);
    float range=m_cfg.maxElevation-m_cfg.minElevation;
    if (range<1.f) range=1.f;
    for (int z=0;z<h;++z) {
        for (int x=0;x<w;++x) {
            float wx=float(x)/(w-1)*m_cfg.worldWidth-m_cfg.worldWidth*0.5f;
            float wz=float(z)/(h-1)*m_cfg.worldHeight-m_cfg.worldHeight*0.5f;
            float hv=getHeightWorld(wx,wz);
            uint16_t v=uint16_t(qBound(0.f,(hv-m_cfg.minElevation)/range,1.f)*65535.f);
            reinterpret_cast<uint16_t*>(img.scanLine(z))[x]=v;
        }
    }
    return img;
}

}} // namespace ks::track
