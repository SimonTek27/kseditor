#include "PaintVector.h"
#include <QPainter>
#include <QSvgGenerator>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QtMath>
#include <QRandomGenerator>
#include <QDir>
#include <QTextStream>
#include <QRegExp>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
namespace ks { namespace paint {
void VectorObject::rebuildPathFromNodes(){
    if(nodes.isEmpty()) return;
    QPainterPath p; p.moveTo(nodes[0].pos);
    for(int i=1;i<nodes.size();++i){ auto &n=nodes[i]; auto &pr=nodes[i-1];
        if(pr.handleOut.isNull() && n.handleIn.isNull()) p.lineTo(n.pos);
        else p.cubicTo(pr.pos+pr.handleOut, n.pos+n.handleIn, n.pos);
    }
    path=p;
}
void VectorObject::rebuildNodesFromPath(){
    nodes.clear();
    for(int i=0;i<path.elementCount();++i){ auto e=path.elementAt(i);
        VectorNode n; n.pos=QPointF(e.x,e.y); nodes.append(n);
    }
}
PaintVectorDocument::PaintVectorDocument(QObject* p):QObject(p){ m_pages.append(m_docSize); }
int PaintVectorDocument::addObject(const VectorObject& o){ VectorObject c=o; if(c.id.isEmpty()) c.id=QString("obj%1").arg(m_objects.size()); m_objects.append(c); emit changed(); return m_objects.size()-1; }
bool PaintVectorDocument::removeObject(int idx){ if(idx<0||idx>=m_objects.size()) return false; m_objects.removeAt(idx); emit changed(); return true; }
bool PaintVectorDocument::removeObjectById(const QString& id){ int i=indexOf(id); return i>=0?removeObject(i):false; }
int PaintVectorDocument::indexOf(const QString& id) const{ for(int i=0;i<m_objects.size();++i) if(m_objects[i].id==id) return i; return -1; }
QPainterPath PaintVectorDocument::combinedPath() const{ QPainterPath r; for(auto &o:m_objects) if(o.visible) r.addPath(o.transform.map(o.path)); return r; }
QImage PaintVectorDocument::rasterize(const QSize& size, double dpr) const{
    QSize s=size.isValid()?size:m_docSize;
    QImage img(s*dpr, QImage::Format_ARGB32_Premultiplied); img.fill(Qt::transparent);
    QPainter pr(&img); pr.setRenderHint(QPainter::Antialiasing,true);
    pr.scale(dpr,dpr);
    for(auto &o:m_objects){ if(!o.visible) continue; pr.save(); pr.setTransform(o.transform,true);
        QPen pen(o.style.stroke, o.style.strokeWidth); pen.setStyle(o.style.penStyle); pen.setCapStyle(o.style.capStyle); pen.setJoinStyle(o.style.joinStyle);
        if(!o.style.dashPattern.isEmpty()){ pen.setStyle(Qt::CustomDashLine); pen.setDashPattern(o.style.dashPattern); pen.setDashOffset(o.style.dashOffset); }
        pr.setPen(o.style.strokeNone?Qt::NoPen:pen);
        if(o.style.fillGradient) pr.setBrush(*o.style.fillGradient); else pr.setBrush(o.style.fillNone?Qt::NoBrush:o.style.fill);
        pr.setOpacity(o.style.opacity); pr.drawPath(o.path);
        if(o.type==VectorObjectType::Text && !o.textContent.isEmpty()){ pr.setFont(o.textFont); pr.drawText(o.path.boundingRect(), Qt::AlignLeft, o.textContent); }
        pr.restore();
    }
    return img;
}
bool PaintVectorDocument::importSvg(const QString& t){
    QXmlStreamReader xml(t);
    m_objects.clear(); int pi=0, ri=0, ei=0;
    while(!xml.atEnd()){
        xml.readNext();
        if(xml.isStartElement()){
            QString n=xml.name().toString();
            if(n=="path"){ auto a=xml.attributes(); VectorObject o; o.type=VectorObjectType::Path; o.id=a.value("id").toString(); if(o.id.isEmpty()) o.id=QString("path%1").arg(pi++);
                QPainterPath pp; pp.addRect(QRectF(10+pi*12,10+pi*10,100,60)); o.path=pp;
                QString f=a.value("fill").toString(); QString s=a.value("stroke").toString();
                if(!f.isEmpty()) o.style.fill=QColor(f); if(s=="none") o.style.strokeNone=true; else if(!s.isEmpty()){ o.style.stroke=QColor(s); o.style.strokeNone=false; }
                m_objects.append(o);
            } else if(n=="rect"){ auto a=xml.attributes(); double x=a.value("x").toDouble(), y=a.value("y").toDouble(), w=a.value("width").toDouble(), h=a.value("height").toDouble(); if(w==0) w=100; if(h==0) h=60; VectorObject o=createRect(QRectF(x,y,w,h)); QString id=a.value("id").toString(); if(!id.isEmpty()) o.id=id; else o.id=QString("rect%1").arg(ri++); m_objects.append(o);
            } else if(n=="ellipse"){ auto a=xml.attributes(); double cx=a.value("cx").toDouble(), cy=a.value("cy").toDouble(), rx=a.value("rx").toDouble(), ry=a.value("ry").toDouble(); if(rx==0) rx=50; if(ry==0) ry=30; VectorObject o=createEllipse(QRectF(cx-rx,cy-ry,rx*2,ry*2)); QString id=a.value("id").toString(); if(!id.isEmpty()) o.id=id; else o.id=QString("ellipse%1").arg(ei++); m_objects.append(o); }
        }
    }
    emit changed(); return true;
}
bool PaintVectorDocument::importSvgFile(const QString& path){ QFile f(path); if(!f.open(QIODevice::ReadOnly)) return false; return importSvg(QString::fromUtf8(f.readAll())); }
QString PaintVectorDocument::exportSvg() const{
    QString svg; svg+=QString("<svg xmlns='http://www.w3.org/2000/svg' width='%1' height='%2'>\n").arg(m_docSize.width()).arg(m_docSize.height());
    for(auto &o:m_objects){ if(!o.visible) continue; QString fill=o.style.fillNone?"none":o.style.fill.name(); QString stroke=o.style.strokeNone?"none":o.style.stroke.name();
        if(o.type==VectorObjectType::Rect || o.type==VectorObjectType::Path){ QRectF b=o.path.boundingRect(); svg+=QString("<path id='%1' d='M %2 %3 L %4 %5' fill='%6' stroke='%7' stroke-width='%8' />\n").arg(o.id).arg(b.x()).arg(b.y()).arg(b.right()).arg(b.bottom()).arg(fill).arg(stroke).arg(o.style.strokeWidth); }
        else if(o.type==VectorObjectType::Ellipse){ QRectF b=o.path.boundingRect(); svg+=QString("<ellipse id='%1' cx='%2' cy='%3' rx='%4' ry='%5' fill='%6' stroke='%7'/>\n").arg(o.id).arg(b.center().x()).arg(b.center().y()).arg(b.width()/2).arg(b.height()/2).arg(fill).arg(stroke); }
        else if(o.type==VectorObjectType::Text){ svg+=QString("<text id='%1' x='%2' y='%3' fill='%4' font-family='%5' font-size='%6'>%7</text>\n").arg(o.id).arg(o.path.boundingRect().x()).arg(o.path.boundingRect().y()).arg(fill).arg(o.textFont.family()).arg(o.textFont.pointSize()).arg(o.textContent.toHtmlEscaped()); }
        else svg+=QString("<g id='%1'><path d='M0 0' fill='%2' stroke='%3'/></g>\n").arg(o.id).arg(fill).arg(stroke);
    }
    for(auto &k:m_patterns.keys()){ Q_UNUSED(k); }
    svg+="</svg>"; return svg;
}
bool PaintVectorDocument::exportSvgFile(const QString& path) const{ QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate)) return false; f.write(exportSvg().toUtf8()); return true; }
bool PaintVectorDocument::importPdf(const QString& path){ QFileInfo fi(path); if(!fi.exists()) return false; VectorObject o; o.type=VectorObjectType::Image; o.id="pdf_import"; o.path.addRect(QRectF(0,0,m_docSize.width(),m_docSize.height())); o.svgAttrs["pdf"]=path; addObject(o); return true; }
bool PaintVectorDocument::importEps(const QString& path){ QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) return false; QTextStream ts(&f); QString content=ts.readAll(); f.close(); QPainterPath currentPath; double cx=0, cy=0; bool inPath=false; QStringList tokens=content.split(QRegExp("\\s+")); int i=0; while(i<tokens.size()){ QString t=tokens[i]; if(t=="newpath"){ currentPath=QPainterPath(); inPath=true; } else if(t=="moveto" && i>=2){ double y=tokens[i-1].toDouble(); double x=tokens[i-2].toDouble(); if(inPath) currentPath.moveTo(x,y); cx=x; cy=y; } else if(t=="lineto" && i>=2){ double y=tokens[i-1].toDouble(); double x=tokens[i-2].toDouble(); if(inPath) currentPath.lineTo(x,y); cx=x; cy=y; } else if(t=="curveto" && i>=6){ double y3=tokens[i-1].toDouble(), x3=tokens[i-2].toDouble(); double y2=tokens[i-3].toDouble(), x2=tokens[i-4].toDouble(); double y1=tokens[i-5].toDouble(), x1=tokens[i-6].toDouble(); if(inPath) currentPath.cubicTo(x1,y1,x2,y2,x3,y3); cx=x3; cy=y3; } else if(t=="closepath" && inPath){ currentPath.closeSubpath(); } else if(t=="stroke" || t=="fill" || t=="show"){ if(inPath && !currentPath.isEmpty()){ VectorObject o; o.type=VectorObjectType::Path; o.id=QString("eps%1").arg(m_objects.size()); o.path=currentPath; o.style.fillNone=(t!="fill"); o.style.strokeNone=(t=="fill"); if(t=="fill") o.style.fill=QColor(Qt::black); else o.style.stroke=QColor(Qt::black); addObject(o); } currentPath=QPainterPath(); inPath=false; } i++; } emit changed(); return true; }
VectorObject PaintVectorDocument::createRect(const QRectF& r,double rx,double ry,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Rect; o.id=QString("rect%1").arg(m_objects.size()); if(rx>0||ry>0) o.path.addRoundedRect(r,rx,ry); else o.path.addRect(r); o.style=s; if(s.fill==QColor(0,0,0) && s.stroke==QColor(Qt::transparent)){ o.style.fill=QColor("#4a90e2"); o.style.fillNone=false; } return o; }
VectorObject PaintVectorDocument::createEllipse(const QRectF& r,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Ellipse; o.id=QString("ellipse%1").arg(m_objects.size()); o.path.addEllipse(r); o.style=s; if(o.style.fill==QColor(0,0,0)) o.style.fill=QColor("#4a90e2"); o.style.fillNone=false; return o; }
VectorObject PaintVectorDocument::createStar(const QPointF& c,double r1,double r2,int pts,double phase,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Star; o.id=QString("star%1").arg(m_objects.size()); QPainterPath p; for(int i=0;i<pts*2;++i){ double ang=phase + M_PI*i/pts; double r=(i%2==0)?r1:r2; QPointF pt(c.x()+r*cos(ang), c.y()+r*sin(ang)); if(i==0) p.moveTo(pt); else p.lineTo(pt);} p.closeSubpath(); o.path=p; o.style=s; o.style.fillNone=false; if(o.style.fill==QColor(0,0,0)) o.style.fill=QColor("#e2a14a"); return o; }
VectorObject PaintVectorDocument::createPolygon(const QPointF& c,double r,int sides,double phase,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Polygon; o.id=QString("poly%1").arg(m_objects.size()); QPainterPath p; for(int i=0;i<sides;++i){ double ang=phase + 2*M_PI*i/sides; QPointF pt(c.x()+r*cos(ang), c.y()+r*sin(ang)); if(i==0) p.moveTo(pt); else p.lineTo(pt);} p.closeSubpath(); o.path=p; o.style=s; o.style.fillNone=false; return o; }
VectorObject PaintVectorDocument::createSpiral(const QPointF& c,double t0,double r0,double t1,double r1,double turns,const VectorStyle& s){ Q_UNUSED(t0); Q_UNUSED(t1); VectorObject o; o.type=VectorObjectType::Spiral; o.id=QString("spiral%1").arg(m_objects.size()); QPainterPath p; int segs=int(turns*32); p.moveTo(c.x()+r0, c.y()); for(int i=1;i<=segs;++i){ double t=double(i)/segs; double rr=r0+(r1-r0)*t; double ang=turns*2*M_PI*t; p.lineTo(c.x()+rr*cos(ang), c.y()+rr*sin(ang)); } o.path=p; o.style=s; o.style.strokeNone=false; o.style.fillNone=true; o.style.stroke=QColor("#333"); return o; }
VectorObject PaintVectorDocument::createBox3D(const QRectF& r,double d,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Box3D; o.id=QString("box3d%1").arg(m_objects.size()); QPainterPath p; p.addRect(r); p.moveTo(r.topRight()); p.lineTo(r.topRight()+QPointF(d,-d)); p.lineTo(r.topLeft()+QPointF(d,-d)); p.lineTo(r.topLeft()); o.path=p; o.style=s; o.style.fillNone=false; return o; }
VectorObject PaintVectorDocument::createText(const QPointF& p,const QString& txt,const QFont& f,const VectorStyle& s){ VectorObject o; o.type=VectorObjectType::Text; o.id=QString("text%1").arg(m_objects.size()); o.textContent=txt; o.textFont=f; o.path.addRect(QRectF(p, QSizeF(txt.size()*f.pointSize()*0.6, f.pointSize()*1.2))); o.style=s; o.style.fillNone=false; return o; }
bool PaintVectorDocument::textOnPath(int ti,int pi){ if(ti<0||ti>=m_objects.size()||pi<0||pi>=m_objects.size()) return false; if(m_objects[ti].type!=VectorObjectType::Text) return false; m_objects[ti].textOnPathRef=m_objects[pi].path; emit changed(); return true; }
bool PaintVectorDocument::textInShape(int ti,int si){ if(ti<0||ti>=m_objects.size()||si<0||si>=m_objects.size()) return false; m_objects[ti].svgAttrs["shapeInside"]=m_objects[si].id; emit changed(); return true; }
QPainterPath PaintVectorDocument::booleanOp(int a,int b,VectorBooleanOp op) const{
    if(a<0||a>=m_objects.size()||b<0||b>=m_objects.size()) return QPainterPath();
    QPainterPath pa=m_objects[a].transform.map(m_objects[a].path), pb=m_objects[b].transform.map(m_objects[b].path);
    switch(op){ case VectorBooleanOp::Union: return pa.united(pb); case VectorBooleanOp::Difference: return pa.subtracted(pb); case VectorBooleanOp::Intersection: return pa.intersected(pb); case VectorBooleanOp::Exclusion: { auto u=pa.united(pb); auto i=pa.intersected(pb); return u.subtracted(i);} case VectorBooleanOp::Division: return pa.united(pb); case VectorBooleanOp::CutPath: return pa; } return pa;
}
bool PaintVectorDocument::applyBoolean(int a,int b,VectorBooleanOp op){ if(a<0||a>=m_objects.size()||b<0||b>=m_objects.size()) return false; QPainterPath r=booleanOp(a,b,op); if(r.isEmpty() && op!=VectorBooleanOp::Difference) return false; m_objects[a].path=r; m_objects[a].rebuildNodesFromPath(); if(op!=VectorBooleanOp::CutPath) removeObject(b); emit changed(); return true; }
bool PaintVectorDocument::simplify(int idx,double thr){ if(idx<0||idx>=m_objects.size()) return false; auto &o=m_objects[idx]; QPainterPath np; bool first=true; for(int i=0;i<o.path.elementCount();++i){ auto e=o.path.elementAt(i); QPointF pt(e.x,e.y); if(first){ np.moveTo(pt); first=false; } else { if(QLineF(np.currentPosition(), pt).length() > thr) np.lineTo(pt); } } if(!np.isEmpty()){ o.path=np; o.rebuildNodesFromPath(); emit changed(); } return true; }
bool PaintVectorDocument::inset(int idx,double d){ if(idx<0||idx>=m_objects.size()) return false; QPainterPathStroker s; s.setWidth(d*2); auto p=s.createStroke(m_objects[idx].path); m_objects[idx].path=m_objects[idx].path.subtracted(p); emit changed(); return true; }
bool PaintVectorDocument::outset(int idx,double d){ if(idx<0||idx>=m_objects.size()) return false; QPainterPathStroker s; s.setWidth(d*2); auto st=s.createStroke(m_objects[idx].path); m_objects[idx].path=m_objects[idx].path.united(st); emit changed(); return true; }
bool PaintVectorDocument::dynamicOffset(int idx,double d){ return outset(idx,d); }
bool PaintVectorDocument::strokeToPath(int idx){ if(idx<0||idx>=m_objects.size()) return false; auto &o=m_objects[idx]; if(o.style.strokeNone) return false; QPainterPathStroker s; s.setWidth(o.style.strokeWidth); s.setCapStyle(o.style.capStyle); s.setJoinStyle(o.style.joinStyle); s.setDashPattern(o.style.dashPattern); o.path=s.createStroke(o.path); o.style.fill=o.style.stroke; o.style.fillNone=false; o.style.strokeNone=true; emit changed(); return true; }
bool PaintVectorDocument::objectToPath(int idx){ if(idx<0||idx>=m_objects.size()) return false; m_objects[idx].type=VectorObjectType::Path; m_objects[idx].rebuildNodesFromPath(); emit changed(); return true; }
bool PaintVectorDocument::breakApart(int idx){ if(idx<0||idx>=m_objects.size()) return false; auto o=m_objects[idx]; auto polys=o.path.toSubpathPolygons(); if(polys.size()<=1) return false; removeObject(idx); for(auto &poly:polys){ VectorObject n; n.type=VectorObjectType::Path; n.id=QString("%1_part%2").arg(o.id).arg(&poly-&polys[0]); QPainterPath pp; pp.addPolygon(poly); n.path=pp; n.style=o.style; addObject(n); } return true; }
bool PaintVectorDocument::combine(const QVector<int>& ids){ if(ids.size()<2) return false; QPainterPath u; VectorStyle s=m_objects[ids[0]].style; for(int id:ids) if(id>=0&&id<m_objects.size()) u.addPath(m_objects[id].transform.map(m_objects[id].path)); QVector<int> sorted=ids; std::sort(sorted.begin(), sorted.end(), std::greater<int>()); for(int id:sorted) removeObject(id); VectorObject n; n.type=VectorObjectType::Path; n.id=QString("combined%1").arg(m_objects.size()); n.path=u; n.style=s; n.style.fillNone=false; addObject(n); return true; }
QPainterPath PaintVectorDocument::traceBitmap(const QImage& img,double thr,bool color,int colors){
    Q_UNUSED(color); Q_UNUSED(colors);
    QImage g=img.convertToFormat(QImage::Format_Grayscale8);
    QPainterPath p; bool in=false; QPointF start;
    for(int y=0;y<g.height(); y+=4){ for(int x=0;x<g.width(); x+=4){ int v=qGray(g.pixel(x,y)); bool b=v < thr; if(b && !in){ start=QPointF(x,y); in=true; } if(!b && in){ p.addRect(QRectF(start, QPointF(x,y))); in=false; } } }
    return p;
}
int PaintVectorDocument::addClone(const QString& src,const QTransform& t){ int i=indexOf(src); if(i<0) return -1; VectorObject c=m_objects[i]; c.id=QString("%1_clone%2").arg(src).arg(m_objects.size()); c.type=VectorObjectType::Clone; c.cloneSourceId=src; c.transform=t; return addObject(c); }
bool PaintVectorDocument::tiledClones(int src,int rows,int cols,double dx,double dy){ if(src<0||src>=m_objects.size()) return false; QString sid=m_objects[src].id; for(int r=0;r<rows;++r) for(int c=0;c<cols;++c){ if(r==0&&c==0) continue; QTransform t; t.translate(c*dx, r*dy); addClone(sid,t); } return true; }
bool PaintVectorDocument::sprayClones(int src,int count,const QRectF& area,double mn,double mx){ if(src<0||src>=m_objects.size()) return false; QString sid=m_objects[src].id; for(int i=0;i<count;++i){ double x=area.x()+QRandomGenerator::global()->bounded(area.width()); double y=area.y()+QRandomGenerator::global()->bounded(area.height()); double s=mn+(mx-mn)*QRandomGenerator::global()->generateDouble(); QTransform t; t.translate(x,y); t.scale(s,s); addClone(sid,t); } return true; }
void PaintVectorDocument::setGradient(int idx,QGradient* g,bool isFill){ if(idx<0||idx>=m_objects.size()||!g) return; if(isFill){ delete m_objects[idx].style.fillGradient; m_objects[idx].style.fillGradient=g; } else { delete m_objects[idx].style.strokeGradient; m_objects[idx].style.strokeGradient=g; } emit changed(); }
void PaintVectorDocument::applyLpe(int idx){ auto &o=m_objects[idx]; if(o.lpe==VectorPathEffect::Simplify) simplify(idx, o.lpeParams.value("threshold",2.0).toDouble()); else if(o.lpe==VectorPathEffect::Inset) inset(idx, o.lpeParams.value("distance",5).toDouble()); else if(o.lpe==VectorPathEffect::Outset) outset(idx, o.lpeParams.value("distance",5).toDouble()); else if(o.lpe==VectorPathEffect::Envelope) { /* envelope deform stub */ } else if(o.lpe==VectorPathEffect::PowerStroke) strokeToPath(idx); }
void PaintVectorDocument::applyFilters(QImage& img,const QString& fid) const{ if(!m_filterObjs.contains(fid)) return; auto &f=m_filterObjs[fid]; for(auto &prim:f.primitives){ if(prim.type==VectorFilterType::GaussianBlur || prim.type==VectorFilterType::Blur){ double sx=prim.params.value("stdDeviation","2").toDouble(); double sy=prim.params.value("stdDeviationY",sx).toDouble(); int rad=qMax(1,int(qMax(sx,sy)*3)); QImage tmp=img; for(int i=0;i<rad;++i){ QPainter p(&tmp); p.setOpacity(0.5); p.drawImage(1,0,img); p.drawImage(-1,0,img); p.drawImage(0,1,img); p.drawImage(0,-1,img); p.end(); img=tmp; } } else if(prim.type==VectorFilterType::Morphology){ double rx=prim.params.value("radius","1").toDouble(); double ry=prim.params.value("radiusY",rx).toDouble(); int rad=qMax(1,int(rx)); for(int i=0;i<rad;++i){ QImage tmp=img; QPainter p(&tmp); p.drawImage(1,0,img); p.drawImage(-1,0,img); p.drawImage(0,1,img); p.drawImage(0,-1,img); p.end(); img=tmp; } Q_UNUSED(ry); } else if(prim.type==VectorFilterType::Offset){ int dx=prim.params.value("dx","0").toInt(); int dy=prim.params.value("dy","0").toInt(); QImage tmp=img.size(), QImage::Format_ARGB32; tmp.fill(Qt::transparent); QPainter p(&tmp); p.drawImage(dx,dy,img); p.end(); img=tmp; } else if(prim.type==VectorFilterType::Flood){ QColor fc=prim.params.value("flood-color","black").value<QColor>(); double fo=prim.params.value("flood-opacity","1").toDouble(); img.fill(fc); QPainter p(&img); p.setOpacity(fo); p.end(); } else if(prim.type==VectorFilterType::ColorMatrix){ QVariantList m=prim.params.value("matrix").toList(); if(m.size()>=20){ QImage tmp=img; for(int y=0;y<img.height();++y){ QRgb* l=reinterpret_cast<QRgb*>(tmp.scanLine(y)); for(int x=0;x<img.width();++x){ double r=qRed(l[x])/255.0, g=qGreen(l[x])/255.0, b=qBlue(l[x])/255.0, a=qAlpha(l[x])/255.0; double nr=m[0].toDouble()*r+m[1].toDouble()*g+m[2].toDouble()*b+m[3].toDouble()*a+m[4].toDouble(); double ng=m[5].toDouble()*r+m[6].toDouble()*g+m[7].toDouble()*b+m[8].toDouble()*a+m[9].toDouble(); double nb=m[10].toDouble()*r+m[11].toDouble()*g+m[12].toDouble()*b+m[13].toDouble()*a+m[14].toDouble(); double na=m[15].toDouble()*r+m[16].toDouble()*g+m[17].toDouble()*b+m[18].toDouble()*a+m[19].toDouble(); l[x]=qRgba(qBound(0,int(nr*255),255),qBound(0,int(ng*255),255),qBound(0,int(nb*255),255),qBound(0,int(na*255),255)); } } img=tmp; } } else if(prim.type==VectorFilterType::Composite){ QString op=prim.params.value("operator","over").toString(); Q_UNUSED(op); } else if(prim.type==VectorFilterType::DisplacementMap){ double sx=prim.params.value("scale","0").toDouble(); Q_UNUSED(sx); } } }
QString PaintVectorDocument::createMeshGradient(int idx,const QVector<VectorMeshPatch>& patches){ if(idx<0||idx>=m_objects.size()) return ""; m_objects[idx].style.gradientType=VectorGradientType::Mesh; m_objects[idx].style.meshPatches=patches; emit changed(); return QString("mesh%1").arg(idx); }
QString PaintVectorDocument::createFilter(const QString& id,const QVector<VectorFilterPrimitive>& prims){ VectorFilter f; f.id=id; f.primitives=prims; m_filterObjs[id]=f; m_filters[id]=filterSvg(id); emit changed(); return id; }
bool PaintVectorDocument::applyFilterToObject(int idx,const QString& fid){ if(idx<0||idx>=m_objects.size()||!m_filterObjs.contains(fid)) return false; m_objects[idx].style.filterId=fid; emit changed(); return true; }
QString PaintVectorDocument::filterSvg(const QString& id) const{ if(!m_filterObjs.contains(id)) return ""; QString s=QString("<filter id='%1'>").arg(id); for(auto &p:m_filterObjs[id].primitives){ s+=QString("<feGaussianBlur in='%1' stdDeviation='%2'/>").arg(p.in).arg(p.params.value("stdDeviation","2")); } s+="</filter>"; return s; }
QString PaintVectorDocument::addExtension(const VectorExtension& ext){ m_extensions[ext.id]=ext; emit changed(); return ext.id; }
bool PaintVectorDocument::runExtension(const QString& id,const QVariantMap& args){ if(!m_extensions.contains(id)) return false; Q_UNUSED(args); emit changed(); return true; }
QStringList PaintVectorDocument::extensionIds() const{ return m_extensions.keys(); }
bool PaintVectorDocument::removeExtension(const QString& id){ return m_extensions.remove(id)>0; }
QVariantMap PaintVectorDocument::extensionInfo(const QString& id) const{ if(!m_extensions.contains(id)) return {}; QVariantMap m; auto &e=m_extensions[id]; m["id"]=e.id; m["name"]=e.name; m["script"]=e.scriptPath; m["menu"]=e.menuLabel; m["enabled"]=e.enabled; return m; }
bool PaintVectorDocument::cmsConvert(const QString& src,const QString& dst){ Q_UNUSED(src); m_cmsProfile=dst; emit changed(); return true; }
QStringList PaintVectorDocument::swatches() const{ return m_swatches.keys(); }
bool PaintVectorDocument::addSwatch(const VectorSwatch& s){ m_swatches[s.id]=s; emit changed(); return true; }
bool PaintVectorDocument::removeSwatch(const QString& id){ return m_swatches.remove(id)>0; }
QColor PaintVectorDocument::swatchColor(const QString& id) const{ return m_swatches.value(id).color; }
bool PaintVectorDocument::batchExport(const QString& outDir,const QString& fmt,int dpi) const{ QDir dir(outDir); if(!dir.exists()) QDir().mkpath(outDir); bool allOk=true; for(int p=0;p<m_pages.size();++p){ QImage img=rasterize(m_pages[p],dpi/72.0); QString fname=QString("%1/page_%2.%3").arg(outDir).arg(p+1,3,10,QChar('0')).arg(fmt); if(fmt=="png") allOk &= img.save(fname,"PNG"); else if(fmt=="jpg" || fmt=="jpeg") allOk &= img.save(fname,"JPEG",95); else if(fmt=="bmp") allOk &= img.save(fname,"BMP"); else if(fmt=="tiff" || fmt=="tif") allOk &= img.save(fname,"TIFF"); else allOk &= img.save(fname,"PNG"); } return allOk; }
QString PaintVectorDocument::commandLineExport(const QString& svgPath,const QString& outPath,const QString& opts) const{ PaintVectorDocument tmp; if(!tmp.importSvgFile(svgPath)) return QString("error:failed_to_load %1").arg(svgPath); int dpi=300; QStringList parts=opts.split(' '); for(auto &p:parts){ if(p.startsWith("-dpi=")) dpi=p.mid(5).toInt(); } QString fmt="png"; if(outPath.endsWith(".jpg")||outPath.endsWith(".jpeg")) fmt="jpg"; else if(outPath.endsWith(".bmp")) fmt="bmp"; else if(outPath.endsWith(".tiff")||outPath.endsWith(".tif")) fmt="tiff"; QImage img=tmp.rasterize(tmp.docSize(),dpi/72.0); bool ok=false; if(fmt=="png") ok=img.save(outPath,"PNG"); else if(fmt=="jpg") ok=img.save(outPath,"JPEG",95); else if(fmt=="bmp") ok=img.save(outPath,"BMP"); else if(fmt=="tiff") ok=img.save(outPath,"TIFF"); else ok=img.save(outPath,"PNG"); return ok?QString("ok:%1").arg(outPath):QString("error:save_failed %1").arg(outPath); }
QStringList PaintVectorDocument::documentTemplates() const{ return m_templates.keys(); }
bool PaintVectorDocument::saveAsTemplate(const QString& name){ m_templates[name]=exportSvg(); return true; }
bool PaintVectorDocument::loadTemplate(const QString& name){ if(!m_templates.contains(name)) return false; return importSvg(m_templates[name]); }
QVariantMap PaintVectorDocument::documentProperties() const{ QVariantMap m; m["width"]=m_docSize.width(); m["height"]=m_docSize.height(); m["pages"]=m_pages.size(); for(auto k:m_props.keys()) m[k]=m_props[k]; return m; }
QVariantMap PaintVectorDocument::objectProperties(int idx) const{ if(idx<0||idx>=m_objects.size()) return {}; QVariantMap m; auto &o=m_objects[idx]; m["id"]=o.id; m["type"]=int(o.type); m["fill"]=o.style.fill.name(); m["stroke"]=o.style.stroke.name(); m["strokeWidth"]=o.style.strokeWidth; m["opacity"]=o.style.opacity; m["visible"]=o.visible; m["locked"]=o.locked; return m; }
bool PaintVectorDocument::setObjectProperty(int idx,const QString& k,const QVariant& v){ if(idx<0||idx>=m_objects.size()) return false; auto &o=m_objects[idx]; if(k=="fill") o.style.fill=QColor(v.toString()); else if(k=="stroke") o.style.stroke=QColor(v.toString()); else if(k=="strokeWidth") o.style.strokeWidth=v.toDouble(); else if(k=="opacity") o.style.opacity=v.toDouble(); else if(k=="visible") o.visible=v.toBool(); else if(k=="locked") o.locked=v.toBool(); else o.svgAttrs[k]=v.toString(); emit changed(); return true; }
bool PaintVectorDocument::alignObjects(const QVector<int>& ids,int t){ if(ids.size()<2) return false; double minX=1e9, maxX=-1e9, minY=1e9, maxY=-1e9; for(int id:ids){ auto r=m_objects[id].bounds(); minX=qMin(minX,r.x()); maxX=qMax(maxX,r.right()); minY=qMin(minY,r.y()); maxY=qMax(maxY,r.bottom()); } for(int id:ids){ auto &o=m_objects[id]; auto r=o.bounds(); QTransform tr; if(t==0) tr.translate(minX - r.x(),0); else if(t==1) tr.translate((minX+maxX)/2 - r.center().x(),0); else if(t==2) tr.translate(maxX - r.right(),0); else if(t==3) tr.translate(0, minY - r.y()); else if(t==4) tr.translate(0, (minY+maxY)/2 - r.center().y()); else if(t==5) tr.translate(0, maxY - r.bottom()); o.transform=tr*o.transform; } emit changed(); return true; }
bool PaintVectorDocument::distributeObjects(const QVector<int>& ids,int dir,double gap){ if(ids.size()<2) return false; QVector<int> s=ids; std::sort(s.begin(), s.end(), [&](int a,int b){ return dir==0? m_objects[a].bounds().x() < m_objects[b].bounds().x() : m_objects[a].bounds().y() < m_objects[b].bounds().y(); }); double pos=0; bool first=true; for(int id:s){ auto &o=m_objects[id]; auto r=o.bounds(); if(first){ pos=dir==0? r.right()+gap : r.bottom()+gap; first=false; continue; } double cur = dir==0? r.x() : r.y(); QTransform tr; tr.translate(dir==0? pos-cur : 0, dir==0?0:pos-cur); o.transform=tr*o.transform; pos+=dir==0? r.width()+gap : r.height()+gap; } emit changed(); return true; }
bool PaintVectorDocument::arrangeObjects(const QVector<int>& ids,int cols,double px,double py){ if(ids.isEmpty()||cols<=0) return false; for(int i=0;i<ids.size();++i){ auto &o=m_objects[ids[i]]; int c=i%cols, r=i/cols; QTransform tr; tr.translate(c*(o.bounds().width()+px), r*(o.bounds().height()+py)); o.transform=tr*o.transform; } emit changed(); return true; }
QPointF PaintVectorDocument::snapPoint(const QPointF& p) const{ if(!m_snapEnabled) return p; return QPointF(qRound(p.x()/m_gridSize)*m_gridSize, qRound(p.y()/m_gridSize)*m_gridSize); }
} }
