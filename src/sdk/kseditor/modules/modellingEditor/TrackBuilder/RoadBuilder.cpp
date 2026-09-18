#include "RoadBuilder.h"
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace ks { namespace track {

RoadBuilder::RoadBuilder(QObject* parent) : QObject(parent) {}

// ============================================================================
// Catmull-Rom spline
// ============================================================================
QVector3D RoadBuilder::catmullRom(QVector3D p0,QVector3D p1,
                                   QVector3D p2,QVector3D p3,float t) const
{
    float t2=t*t, t3=t2*t;
    return 0.5f*((2.f*p1)
               + (-p0+p2)*t
               + (2.f*p0 - 5.f*p1 + 4.f*p2 - p3)*t2
               + (-p0 + 3.f*p1 - 3.f*p2 + p3)*t3);
}

QVector3D RoadBuilder::evalSpline(const QVector<SplinePoint>& pts, float t) const
{
    if (pts.isEmpty()) return {};
    if (pts.size()==1) return pts[0].position;
    int n=pts.size()-1;
    float seg=t*n;
    int i=qBound(0,(int)seg,n-1);
    float lt=seg-i;
    int i0=qMax(0,i-1),i1=i,i2=qMin(n,i+1),i3=qMin(n,i+2);
    return catmullRom(pts[i0].position,pts[i1].position,
                      pts[i2].position,pts[i3].position,lt);
}

float RoadBuilder::lerpAtT(const QVector<SplinePoint>& pts, float t,
                             std::function<float(const SplinePoint&)> get) const
{
    if (pts.isEmpty()) return 0.f;
    if (pts.size()==1) return get(pts[0]);
    int n=pts.size()-1;
    float seg=t*n;
    int i=qBound(0,(int)seg,n-1);
    float lt=seg-i;
    return get(pts[i])*(1.f-lt)+get(pts[qMin(n,i+1)])*lt;
}

float RoadBuilder::splineWidth(const QVector<SplinePoint>& pts, float t) const
{
    return lerpAtT(pts,t,[](const SplinePoint& p){return p.width;});
}
float RoadBuilder::splineCamberL(const QVector<SplinePoint>& pts, float t) const
{
    return lerpAtT(pts,t,[](const SplinePoint& p){return p.camberL;});
}
float RoadBuilder::splineCamberR(const QVector<SplinePoint>& pts, float t) const
{
    return lerpAtT(pts,t,[](const SplinePoint& p){return p.camberR;});
}

// ============================================================================
// Centre-line tessellation
// ============================================================================
QVector<QVector3D> RoadBuilder::tessellateCentreLine(const Road& road) const
{
    if (road.points.size()<2) return {};
    // Estimate total arc length with coarse sampling
    int coarse=100;
    float arcLen=0.f;
    QVector3D prev=evalSpline(road.points,0.f);
    for (int i=1;i<=coarse;++i) {
        QVector3D cur=evalSpline(road.points,float(i)/coarse);
        arcLen+=(cur-prev).length();
        prev=cur;
    }
    int steps=qMax(2,int(arcLen/road.tessResolution));
    QVector<QVector3D> cl;
    cl.reserve(steps+1);
    for (int i=0;i<=steps;++i)
        cl.append(evalSpline(road.points,float(i)/steps));
    return cl;
}

// ============================================================================
// Build Road Mesh
// ============================================================================
RoadMesh RoadBuilder::buildRoad(const Road& road) const
{
    RoadMesh mesh;
    mesh.roadId=road.id;
    if (road.points.size()<2) return mesh;

    QVector<QVector3D> cl=tessellateCentreLine(road);
    int steps=cl.size()-1;
    mesh.centreLine=cl;
    mesh.centreNormals.resize(cl.size());
    mesh.cumulativeDist.resize(cl.size(),0.f);

    // Cumulative arc length
    for (int i=1;i<cl.size();++i) {
        mesh.cumulativeDist[i]=mesh.cumulativeDist[i-1]+(cl[i]-cl[i-1]).length();
    }
    mesh.totalLength=mesh.cumulativeDist.last();

    // Columns: left edge, left crown, centre, right crown, right edge  (5)
    const int COLS=5;
    // col offsets:  -1, -0.5, 0, 0.5, 1  (normalised to half-width)
    const float colT[COLS]={-1.f,-0.5f,0.f,0.5f,1.f};

    mesh.vertices.reserve((steps+1)*COLS);

    for (int row=0;row<=steps;++row) {
        float t=float(row)/steps;
        QVector3D cpt=cl[row];

        // Forward tangent
        QVector3D fwd;
        if (row<steps) fwd=(cl[row+1]-cl[row]);
        else           fwd=(cl[row]-cl[row-1]);
        fwd.setY(0); fwd.normalize();
        QVector3D right=QVector3D::crossProduct(fwd,{0,1,0}).normalized();

        // Centre-line normal (up + road slope)
        mesh.centreNormals[row]=QVector3D::crossProduct(right,fwd).normalized();

        float hw   = splineWidth(road.points,t);          // half-width
        float camL = splineCamberL(road.points,t);        // deg
        float camR = splineCamberR(road.points,t);
        float crown= road.profile==RoadProfile::Crowned ? road.crownHeight : 0.f;
        float dist = mesh.cumulativeDist[row];

        // Snap Y from terrain if available
        if (m_snapToTerrain && m_terrainSample) {
            cpt.setY(m_terrainSample(cpt.x(),cpt.z()));
        }

        // Bridge offset
        if (road.isBridge) cpt.setY(cpt.y()+road.bridgeHeight);

        for (int c=0;c<COLS;++c) {
            float lateT=colT[c];     // -1..1
            float latDist=lateT*hw;
            QVector3D pos=cpt+right*latDist;

            // Height from camber + crown
            float h;
            if (lateT<=0.f) {
                // left side
                float camberH=std::tan(camL*float(M_PI)/180.f)*(-latDist);
                float crownH = crown*(1.f-(-lateT));
                h=camberH+crownH;
            } else {
                float camberH=std::tan(camR*float(M_PI)/180.f)*latDist;
                float crownH = crown*(1.f-lateT);
                h=camberH+crownH;
            }
            pos.setY(pos.y()+h);

            // Normal (basic up, refined later)
            QVector3D norm={0,1,0};

            RoadVertex v;
            v.position=pos;
            v.normal=norm;
            v.uv={dist/hw,  (lateT+1.f)*0.5f * road.textureScale};
            v.laneT=(lateT+1.f)*0.5f;
            v.distAlong=dist;
            mesh.vertices.append(v);
        }
    }

    // Triangle indices (quad strip)
    mesh.indices.reserve(steps*COLS*6);
    for (int row=0;row<steps;++row) {
        for (int c=0;c<COLS-1;++c) {
            int a=row*COLS+c, b=a+1, c_=a+COLS, d=c_+1;
            mesh.indices<<a<<b<<c_<<b<<d<<c_;
        }
    }

    // Recompute normals from triangles
    QVector<QVector3D> normAcc(mesh.vertices.size(),{0,0,0});
    for (int i=0;i+2<mesh.indices.size();i+=3) {
        int ia=mesh.indices[i],ib=mesh.indices[i+1],ic=mesh.indices[i+2];
        QVector3D e1=mesh.vertices[ib].position-mesh.vertices[ia].position;
        QVector3D e2=mesh.vertices[ic].position-mesh.vertices[ia].position;
        QVector3D n=QVector3D::crossProduct(e1,e2);
        normAcc[ia]+=n; normAcc[ib]+=n; normAcc[ic]+=n;
    }
    for (int i=0;i<mesh.vertices.size();++i)
        mesh.vertices[i].normal=normAcc[i].normalized();

    return mesh;
}

// ============================================================================
// Build Kerb Mesh
// ============================================================================
KerbMesh RoadBuilder::buildKerb(const Kerb& kerb, const RoadMesh& road) const
{
    KerbMesh km;
    km.kerbId=kerb.id;
    if (road.centreLine.isEmpty()) return km;

    int n=road.centreLine.size();
    float kw=kerb.width;
    float kh=kerb.height;
    bool left=kerb.leftSide;

    // Determine which road points to cover
    QSet<int> active;
    if (kerb.pointIndices.isEmpty())
        for (int i=0;i<n;++i) active.insert(i);
    else
        for (int i:kerb.pointIndices) active.insert(i);

    // 3 vertices per cross-section: inner, top, outer
    for (int row=0;row<n;++row) {
        if (!active.contains(row)) continue;

        QVector3D cpt=road.centreLine[row];
        QVector3D norm=road.centreNormals[row];

        // Lateral direction
        QVector3D fwd;
        if (row+1<n) fwd=road.centreLine[row+1]-cpt;
        else         fwd=cpt-road.centreLine[row-1];
        fwd.setY(0); fwd.normalize();
        QVector3D right=QVector3D::crossProduct(fwd,{0,1,0}).normalized();
        if (!left) right=-right;

        // Road edge position for this row
        // Find the outermost vertex of the parent road on the correct side
        int baseCol=0;
        float colsPerRow = (float)road.vertices.size() / n;
        int bestCol = 0;
        float bestT = left ? 1.0f : 0.0f;
        for (int c = 0; c < (int)colsPerRow; ++c) {
            int idx = row * (int)colsPerRow + c;
            if (idx < road.vertices.size()) {
                float t = road.vertices[idx].laneT;
                if (left && t < bestT) { bestT = t; bestCol = c; }
                if (!left && t > bestT) { bestT = t; bestCol = c; }
            }
        }
        int edgeIdx = row * (int)colsPerRow + bestCol;
        QVector3D edge;
        if (edgeIdx < road.vertices.size()) {
            edge = road.vertices[edgeIdx].position;
        } else {
            // Fallback: estimate road half-width from first row
            float halfWidth = 5.0f;
            if (!road.vertices.isEmpty() && n > 1) {
                float totalWidth = 0;
                for (int c = 0; c < (int)colsPerRow - 1; ++c) {
                    int a = c, b = c + 1;
                    totalWidth += (road.vertices[b].position - road.vertices[a].position).length();
                }
                halfWidth = totalWidth * 0.5f;
            }
            edge = cpt + right * (left ? -halfWidth : halfWidth);
        }

        // 3 cross section points
        QVector3D inner=edge;
        QVector3D top  =edge+right*kw + QVector3D(0,kh,0);
        QVector3D outer=edge+right*kw*1.5f;

        auto addV=[&](QVector3D pos, QVector2D uv){
            RoadVertex v; v.position=pos; v.normal={0,1,0}; v.uv=uv;
            km.vertices.append(v);
        };
        float u=road.cumulativeDist.value(row,0.f);
        addV(inner,{u,0.f});
        addV(top,  {u,0.5f});
        addV(outer,{u,1.f});
    }

    // Build triangle strip
    int rows=km.vertices.size()/3;
    for (int row=0;row<rows-1;++row) {
        for (int c=0;c<2;++c) {
            int a=row*3+c, b=a+1, cc=a+3, d=cc+1;
            km.indices<<a<<b<<cc<<b<<d<<cc;
        }
    }
    return km;
}

// ============================================================================
// Build Wall Mesh
// ============================================================================
RoadMesh RoadBuilder::buildWall(const Wall& wall, const RoadMesh* snapRoad) const
{
    RoadMesh mesh;
    mesh.roadId=wall.id;

    QVector<QVector3D> pts=wall.points;

    // If snapping to road, build points from road edge
    if (wall.snapToRoad && snapRoad && !snapRoad->centreLine.isEmpty()) {
        pts.clear();
        int n=snapRoad->centreLine.size();
        float colsPerRow = (float)snapRoad->vertices.size() / n;
        for (int i=0;i<n;++i) {
            QVector3D cpt=snapRoad->centreLine[i];
            QVector3D fwd;
            if (i+1<n) fwd=snapRoad->centreLine[i+1]-cpt;
            else       fwd=cpt-snapRoad->centreLine[i-1];
            fwd.setY(0); fwd.normalize();
            QVector3D right=QVector3D::crossProduct(fwd,{0,1,0}).normalized();
            if (!wall.snapLeft) right=-right;

            // Find actual road edge vertex
            float bestT = wall.snapLeft ? 1.0f : 0.0f;
            int bestCol = 0;
            for (int c = 0; c < (int)colsPerRow; ++c) {
                int idx = i * (int)colsPerRow + c;
                if (idx < snapRoad->vertices.size()) {
                    float t = snapRoad->vertices[idx].laneT;
                    if (wall.snapLeft && t < bestT) { bestT = t; bestCol = c; }
                    if (!wall.snapLeft && t > bestT) { bestT = t; bestCol = c; }
                }
            }
            int edgeIdx = i * (int)colsPerRow + bestCol;
            if (edgeIdx < snapRoad->vertices.size()) {
                pts.append(snapRoad->vertices[edgeIdx].position);
            } else {
                pts.append(cpt + right * (wall.snapLeft ? -5.0f : 5.0f));
            }
        }
    }

    if (pts.size()<2) return mesh;

    float h=wall.height;
    for (int i=0;i<pts.size();++i) {
        RoadVertex bot,top;
        bot.position=pts[i];
        top.position=pts[i]+QVector3D(0,h,0);
        float u=float(i)/(pts.size()-1)*10.f;
        bot.uv={u,0}; top.uv={u,1};
        bot.normal=top.normal={0,0,1}; // refined below
        mesh.vertices<<bot<<top;
    }
    for (int i=0;i<pts.size()-1;++i) {
        int a=i*2,b=a+1,c=a+2,d=a+3;
        mesh.indices<<a<<b<<c<<b<<d<<c;
    }
    return mesh;
}

// ============================================================================
// Build Surface mesh (polygon triangulation: ear-clipping)
// ============================================================================
RoadMesh RoadBuilder::buildSurface(const Surface& surface,
                                    std::function<float(float,float)> heightFn) const
{
    RoadMesh mesh;
    mesh.roadId=surface.id;
    const auto& poly=surface.polygon;
    if (poly.size()<3) return mesh;

    // Build vertex list
    for (const auto& p:poly) {
        RoadVertex v;
        float y=heightFn?heightFn(p.x(),p.y()):surface.elevation;
        v.position={p.x(),y,p.y()};
        v.normal={0,1,0};
        v.uv={p.x()*0.1f,p.y()*0.1f};
        mesh.vertices.append(v);
    }

    // Ear-clipping triangulation
    QVector<int> idx;
    for (int i=0;i<poly.size();++i) idx.append(i);

    auto cross2=[](QVector2D a,QVector2D b,QVector2D c){
        return (b.x()-a.x())*(c.y()-a.y())-(b.y()-a.y())*(c.x()-a.x());
    };
    auto inTriangle=[&](QVector2D p,QVector2D a,QVector2D b,QVector2D c)->bool{
        float d1=cross2(p,a,b),d2=cross2(p,b,c),d3=cross2(p,c,a);
        bool hn=(d1<0)||(d2<0)||(d3<0);
        bool hp=(d1>0)||(d2>0)||(d3>0);
        return !(hn&&hp);
    };

    while (idx.size()>=3) {
        bool found=false;
        for (int i=0;i<idx.size();++i) {
            int a=idx[i], b=idx[(i+1)%idx.size()], c=idx[(i+2)%idx.size()];
            QVector2D pa=poly[a],pb=poly[b],pc=poly[c];
            if (cross2(pa,pb,pc)<=0.f) continue; // not ear
            bool ear=true;
            for (int j=0;j<idx.size();++j) {
                int jj=idx[j];
                if (jj==a||jj==b||jj==c) continue;
                if (inTriangle(poly[jj],pa,pb,pc)){ ear=false; break; }
            }
            if (ear) {
                mesh.indices<<a<<b<<c;
                idx.removeAt((i+1)%idx.size());
                found=true;
                break;
            }
        }
        if (!found) break; // degenerate polygon
    }
    return mesh;
}

// ============================================================================
// Auto AI line (simple: follow road centre-lines in order, smooth)
// ============================================================================
AILine RoadBuilder::autoAILine(const QVector<RoadMesh>& roads, bool closeLoop) const
{
    AILine ai;
    ai.id="auto_ai";
    ai.isLoop=closeLoop;
    for (const auto& rm:roads) {
        for (int i=0;i<rm.centreLine.size();++i) {
            AILinePoint p;
            p.position=rm.centreLine[i];
            // Estimate speed from local curvature
            float spd=120.f;
            if (i>0&&i<rm.centreLine.size()-1) {
                QVector3D d1=rm.centreLine[i]-rm.centreLine[i-1];
                QVector3D d2=rm.centreLine[i+1]-rm.centreLine[i];
                float angle=std::acos(qBound(-1.f,QVector3D::dotProduct(d1.normalized(),d2.normalized()),1.f));
                spd=qBound(40.f,120.f-angle*200.f,200.f);
            }
            p.speed=spd;
            p.width=3.f;
            ai.points.append(p);
        }
    }
    return ai;
}

}} // namespace ks::track
