#pragma once

#include "CADTypes.h"
#include "FBXParser.h"

namespace CAD {

/**
 * @brief CAD format converter
 * 
 * Converts CAD models between different formats and to FBX.
 */
class Converter {
public:
    Converter();
    
    /**
     * Convert CAD file to FBX scene
     * @param cadFile Input CAD file model
     * @return FBX scene
     */
    ks::FBX::Scene toFBX(const File& cadFile);
    
    /**
     * Set tessellation quality (0.0 - 1.0)
     * @param quality Quality level
     */
    void setTessellationQuality(float quality);
    
    /**
     * Set chordal deviation for surface tessellation
     * @param deviation Chordal deviation value
     */
    void setChordalDeviation(double deviation);
    
    /**
     * Set angular tolerance for surface tessellation
     * @param tolerance Angular tolerance in radians
     */
    void setAngularTolerance(double tolerance);

private:
    float m_tessellationQuality;
    double m_chordalDeviation;
    double m_angularTolerance;
    
    void convertSolid(const Solid& solid, ks::FBX::MeshData& mesh);
    void convertComponent(const Component& component, ks::FBX::MeshData& mesh);
};

} // namespace CAD
