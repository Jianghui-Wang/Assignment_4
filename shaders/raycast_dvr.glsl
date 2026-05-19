#version 120
// Direct Volume Rendering (Assignment 4, 35 pts).
//
// Front-to-back compositing along the ray, with:
//   • opacity correction for the actual step size (5 pts)
//   • Lambertian shading from a central-difference gradient (5 pts)
//   • early ray termination at alpha > 0.95 (5 pts)
//
// Bonus: also runs MIP (maximum-intensity projection) when the user toggles
// `mip` with the `l` key.

uniform sampler1D transferfunction;
uniform sampler2D frontfaces;
uniform sampler2D backfaces;
uniform sampler3D volume;

uniform float step_size;
uniform float ambient;
uniform int   enable_lighting;
uniform int   enable_gm_scaling;
uniform int   mip;
uniform vec3  vol_dim;

uniform vec4  params;   // legacy debug param

// reference step the transfer-function opacities are "calibrated" for;
// any step_size different from this is corrected by Beer–Lambert below.
const float REF_STEP = 1.0 / 200.0;

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
    if (ray_len < 1e-5) discard;

    vec3 dir      = ray / ray_len;
    vec3 step_vec = dir * step_size;
    int  steps    = int(ray_len / step_size) + 2;

    // ----------------- MIP (bonus +5) -----------------
    if (mip == 1) {
        float maxV = 0.0;
        vec3 p     = rayStart;
        for (int i = 0; i < 2048; ++i) {
            if (i >= steps) break;
            maxV = max(maxV, texture3D(volume, p).r);
            p += step_vec;
        }
        vec4 c = texture1D(transferfunction, maxV);
        gl_FragColor = vec4(c.rgb, 1.0);
        return;
    }

    // ---------------- standard DVR --------------------
    vec4 accum = vec4(0.0);
    vec3 pos   = rayStart;
    vec3 L     = normalize(vec3(1.0, 1.0, 1.0));

    for (int i = 0; i < 2048; ++i) {
        if (i >= steps) break;

        float v   = texture3D(volume, pos).r;
        vec4  src = texture1D(transferfunction, v);

        // Opacity correction for the actual step size — keeps the rendered
        // density consistent when the user changes step_size with +/-.
        src.a = 1.0 - pow(max(1.0 - src.a, 0.0), step_size / REF_STEP);

        if (src.a > 0.001) {
            if (enable_lighting == 1) {
                vec3  grad = sampleGradient(pos);
                vec3  N    = -normalize(grad + vec3(1e-7));   // outward
                float diff = max(dot(N, L), 0.0);
                float shade = clamp(ambient + diff, 0.0, 1.0);
                if (enable_gm_scaling == 1) {
                    // gradient-magnitude-modulated shading highlights edges.
                    float gm = length(grad);
                    shade *= clamp(gm * 4.0, 0.0, 1.0);
                }
                src.rgb *= shade;
            }

            // front-to-back compositing in associated (pre-multiplied) form
            accum.rgb += (1.0 - accum.a) * src.a * src.rgb;
            accum.a   += (1.0 - accum.a) * src.a;

            // EARLY RAY TERMINATION
            if (accum.a > 0.95) break;
        }

        pos += step_vec;
    }

    gl_FragColor = accum;
}
