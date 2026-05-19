#version 120
// Iso-surface raycasting (Assignment 4, 20 pts).
//
// Pipeline:
//   ray_start = texture2D(frontfaces, uv) — RGB stores cube-tex entry point
//   ray_end   = texture2D(backfaces,  uv) — RGB stores cube-tex exit  point
//   step from start toward end with `step_size`; on every step sample the
//   3D volume and test whether the iso-value is crossed since the last step.
//   On the first crossing: refine to the exact iso position by linear
//   interpolation, compute a central-difference normal, shade, then EXIT
//   (early ray termination).

// textures
uniform sampler1D transferfunction;
uniform sampler2D frontfaces;
uniform sampler2D backfaces;
uniform sampler3D volume;

// scalars / vectors
uniform float step_size;
uniform float iso_value;
uniform float ambient;
uniform int   enable_lighting;
uniform vec3  vol_dim;

// legacy debug param kept so glGetUniformLocation never returns -1 for it
uniform vec4 params;

// Central-difference gradient in texture space. One voxel offset per axis.
vec3 sampleGradient(vec3 p) {
    vec3 inv_dim = 1.0 / vol_dim;
    float gx = texture3D(volume, p + vec3(inv_dim.x, 0.0, 0.0)).r -
               texture3D(volume, p - vec3(inv_dim.x, 0.0, 0.0)).r;
    float gy = texture3D(volume, p + vec3(0.0, inv_dim.y, 0.0)).r -
               texture3D(volume, p - vec3(0.0, inv_dim.y, 0.0)).r;
    float gz = texture3D(volume, p + vec3(0.0, 0.0, inv_dim.z)).r -
               texture3D(volume, p - vec3(0.0, 0.0, inv_dim.z)).r;
    return vec3(gx, gy, gz);
}

void main()
{
    vec2 uv       = gl_TexCoord[0].xy;
    vec3 rayStart = texture2D(frontfaces, uv).xyz;
    vec3 rayEnd   = texture2D(backfaces,  uv).xyz;

    vec3  ray     = rayEnd - rayStart;
    float ray_len = length(ray);
    if (ray_len < 1e-5) discard;   // pixel outside the bounding cube

    vec3 dir      = ray / ray_len;
    vec3 step_vec = dir * step_size;
    int  steps    = int(ray_len / step_size) + 2;

    vec3  pos    = rayStart;
    float prev_v = texture3D(volume, pos).r;

    // Loop bound is a fixed compile-time constant for legacy GLSL; the
    // dynamic `steps` is enforced with an early break.
    for (int i = 1; i < 2048; ++i) {
        if (i >= steps) break;
        pos += step_vec;

        float v = texture3D(volume, pos).r;

        // crossing on this segment?
        bool crossed = (prev_v < iso_value && v >= iso_value) ||
                       (prev_v > iso_value && v <= iso_value);
        if (crossed) {
            // 1-D linear refinement between (prev_v, v) along the segment.
            float t       = (iso_value - prev_v) / (v - prev_v + 1e-7);
            vec3  hitPos  = pos - step_vec + t * step_vec;

            vec3 color = vec3(0.85, 0.9, 1.0);   // bluish-white iso surface
            if (enable_lighting == 1) {
                vec3 grad = sampleGradient(hitPos);
                vec3 N    = -normalize(grad + vec3(1e-7));    // outward
                vec3 L    = normalize(vec3(1.0, 1.0, 1.0));
                float diff = max(dot(N, L), 0.0);
                color *= clamp(ambient + diff, 0.0, 1.0);
            }
            gl_FragColor = vec4(color, 1.0);
            return;                              // EARLY RAY TERMINATION
        }
        prev_v = v;
    }

    // ray exited the volume without ever crossing the iso value
    discard;
}
