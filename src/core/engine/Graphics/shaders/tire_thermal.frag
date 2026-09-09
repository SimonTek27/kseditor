#version 450

// Tire Thermal Mesh Fragment Shader
// Colors tire based on temperature distribution: core, surface, and wear

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_uv;
in float v_coreTemp;
in float v_surfaceTemp;
in float v_wearLevel;

// Uniforms for thermal coloring
uniform vec3 coldColor = vec3(0.0, 0.2, 0.8);     // Blue - cold (core < 60°C)
uniform vec3 normalColor = vec3(0.5, 0.7, 1.0);   // Normal temp (60-100°C)
uniform vec3 hotColor = vec3(1.0, 0.5, 0.0);      // Overheated (100-120°C)
uniform vec3 overheatedColor = vec3(1.0, 0.2, 0.0); // Severe (120°+)
uniform vec3 wearColor = vec3(0.9, 0.9, 0.9);      // Worn appearance

// Temperature ranges
const float COLD_THRESH = 60.0;
const float NORMAL_THRESH = 100.0;
const float OVERHEATED_THRESH = 120.0;

void main() {
    // Normalize normal for lighting (optional, for visual depth)
    vec3 N = normalize(v_normal);

    // --- Temperature-based color mapping ---

    // Start with base color based on surface temperature
    float temp = v_surfaceTemp;

    // Clamp temperature
    temp = clamp(temp, -50.0, 160.0);

    // Map temperature to normalized 0-1 factor
    // -50°C → 0.0, 60°C → 0.0, 100°C → 0.5, 120°C → 0.8, 160°C → 1.0
    float tNorm;
    if (temp <= COLD_THRESH) {
        tNorm = 0.0;
    } else if (temp >= OVERHEATED_THRESH) {
        tNorm = 1.0;
    } else {
        // Linear interpolation between normal and overheated thresholds
        tNorm = (temp - COLD_THRESH) / (OVERHEATED_THRESH - COLD_THRESH);
        tNorm = clamp(tNorm, 0.0, 1.0);
    }

    // Interpolate color based on temperature
    vec3 baseColor = mix(normalColor, overheatedColor, tNorm);

    // Apply wear effect - worn tires lose saturation and gain gray
    if (v_wearLevel > 0.0) {
        // Desaturate based on wear level
        float gray = dot(baseColor, vec3(0.3, 0.59, 0.11));  // luminance
        baseColor = mix(baseColor, vec3(gray), v_wearLevel * 0.7);
    }

    // Core temperature influence - affects center line color intensity
    float coreInfluence = clamp(v_coreTemp / 120.0, 0.0, 1.0);
    // Slightly shift color based on core temp
    baseColor = mix(baseColor, baseColor + vec3(0.1, -0.05, -0.1), coreInfluence * 0.2);

    // Final color with optional ambient lighting term
    float diffuse = max(dot(N, vec3(0.5, 1.0, 0.5)), 0.1);
    vec3 finalColor = baseColor * diffuse * 0.8 + vec3(0.1, 0.1, 0.1);

    gl_FragColor = vec4(finalColor, 1.0);
}