#version 450

// CFD Compute Shader - Real-time fluid dynamics simulation
// Simplified lattice Boltzmann method for airflow visualization

// Input parameters
layout(local_size_x = 64) in;

// Uniform data
struct Uniforms {
    mat4 projection;
    mat4 view;
    mat4 model;
    
    // Car parameters
    vec3 carPosition;
    float carSpeed;
    float slipAngle;
    
    // Wind tunnel parameters
    vec3 windDirection;
    float windSpeed;
    float airDensity;
    
    // Simulation settings
    int gridResolution;
    float dt;
    float viscosity;
    
    // Tire influence
    vec4 tirePositions[4];  // FL, FR, RL, RR
    float tireSlip[4];      // lateral slip angle per tire
    float tireForce[4];     // force magnitude per tire
};

// Output: velocity and pressure at each grid cell
layout(location = 0) out vec3 v_outVelocity;  // velocity field
layout(location = 1) out float v_outPressure; // pressure field

// Grid constants
const int MAX_GRID = 128;
const vec3 GLOBAL_MIN = vec3(-50.0, -5.0, -50.0);
const vec3 GLOBAL_MAX = vec3(50.0, 20.0, 50.0);
const vec3 GRID_SIZE = (GLOBAL_MAX - GLOBAL_MIN) / float(MAX_GRID - 1);

// Shared memory for velocity field
shared vec3 velField[MAX_GRID * MAX_GRID * MAX_GRID];
shared float presField[MAX_GRID * MAX_GRID * MAX_GRID];

void main() {
    // Calculate grid coordinates
    uint gid = gl_GlobalInvocationID.x;
    uint gid2 = gl_GlobalInvocationID.y;
    uint gid3 = gl_GlobalInvocationID.z;
    
    if (gid >= MAX_GRID || gid2 >= MAX_GRID || gid3 >= MAX_GRID) return;
    
    // Convert grid index to world position
    vec3 pos = GLOBAL_MIN + vec3(float(gid), float(gid2), float(gid3)) * GRID_SIZE;
    
    // Initialize fields
    velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] = vec3(0.0);
    presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] = 1.0f;
    
    // --- Car body influence ---
    // Model the car as a simple box for now
    vec3 carHalfSize = vec3(2.0, 1.0, 4.0);  // width, height, length
    vec3 toCar = pos - carPosition;
    
    // Check if point is inside car volume
    bool insideCar = abs(toCar.x) < carHalfSize.x && 
                     abs(toCar.y) < carHalfSize.y && 
                     abs(toCar.z) < carHalfSize.z;
    
    if (insideCar) {
        // No flow inside car - mark as obstacle
        velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] = vec3(0.0);
        presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] = 1.5f;  // high pressure
        return;
    }
    
    // --- Tire influence ---
    // Simplified tire wake model
    for (int i = 0; i < 4; i++) {
        vec3 tirePos = tirePositions[i];
        float slip = tireSlip[i];
        float force = tireForce[i];
        
        vec3 toTire = pos - tirePos;
        float dist = length(toTire);
        
        // Wake region behind tire
        if (dot(toTire, normalize(windDirection)) > 0.0 && dist < 15.0) {
            // High turbulence region
            float turbulence = 0.5 + 0.5 * sin(gid + gid2 + gid3 + i * 100.0);
            float lowPressure = 0.8 - 0.2 * turbulence;
            
            presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] *= lowPressure;
            
            // Add velocity perturbation (vortices)
            vec3 vortexDir = cross(normalize(windDirection), normalize(toTire));
            velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] += vortexDir * force * 0.1 * turbulence;
        }
    }
    
    // --- Background wind flow ---
    // Simple uniform wind plus car disturbance
    vec3 baseWind = windDirection * windSpeed;
    
    // Car-induced flow (simplified)
    float carDist = length(toCar);
    if (carDist > 0.0 && carDist < 20.0) {
        // Car creates high pressure at front, low pressure at rear (wake)
        float frontFactor = smoothstep(0.0, 10.0, carDist);  // front third
        float rearFactor = smoothstep(10.0, 20.0, carDist);  // rear third
        
        // Front stagnation point - high pressure, low velocity
        if (dot(toCar, windDirection) > 0.0) {
            float pressureBoost = 2.0 * frontFactor;
            presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] += pressureBoost;
            // Slow down the wind at front
            velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] -= normalize(toCar) * windSpeed * 0.3 * frontFactor;
        }
        // Rear wake - low pressure, swirling velocity
        else {
            presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] -= 1.5 * rearFactor;
            // Add swirling vortex
            vec3 axis = cross(vec3(0.0, 1.0, 0.0), normalize(toCar));
            float spin = 3.0 * rearFactor / max(carDist, 0.1);
            mat3 rotation = mat3(
                vec3(cos(spin), 0.0, sin(spin)),
                vec3(0.0, 1.0, 0.0),
                vec3(-sin(spin), 0.0, cos(spin))
            );
            velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3] += rotation * baseWind * 0.2 * rearFactor;
        }
    }
    
    // Apply viscosity diffusion (simple Jacobi iteration hint)
    // In a real implementation, this would be a separate pass
    
    // Store outputs
    v_outVelocity = velField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3];
    v_outPressure = presField[gid * MAX_GRID * MAX_GRID + gid2 * MAX_GRID + gid3];
}