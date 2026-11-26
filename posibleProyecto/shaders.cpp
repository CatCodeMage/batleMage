#include "shaders.h"

// Incluye adaptaciones a GLSL 330 core de los shaders procedurales de:
// - GLACILUX Lite — Copyright (c) 2026 gennai (MIT License)
// - Stylized Projectiles VFX (Fire: sp_orb, sp_add, sp_mix, sp_trail_add, sp_decal) — Copyright (c) 2026 REEBOO Labs

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in mat4 iModel;     // ocupa las locations 3, 4, 5 y 6
layout (location = 7) in vec4 iColor;
layout (location = 8) in float iKind;
uniform mat4 uViewProj;
out vec3 vWorldPos;
out vec3 vLocalPos;
out vec3 vLocalPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;
flat out float vKind;
void main() {
    vec4 world = iModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vLocalPos = aPos;
    vLocalPos = aPos;
    vNormal = mat3(iModel) * aNormal;
    vUV = aUV;
    vColor = iColor;
    vKind = iKind;
    gl_Position = uViewProj * world;
})";

const char* fragmentSrc = R"(
#version 330 core
#define MAX_LIGHTS 16
in vec3 vWorldPos;
in vec3 vLocalPos;
in vec3 vNormal;
in vec4 vColor;
in vec2 vUV;
flat in float vKind;
uniform sampler2D uAlbedo;
uniform vec3 uEmission;
uniform vec3 uCamPos;
uniform int  uNumLights;
uniform vec4 uLights[2 * MAX_LIGHTS];   // [2*i] = pos.xyz + alcance ; [2*i+1] = color.rgb + intensidad

//Uniforms de sombras
uniform int uShadowsEnabled;
uniform int uShadowSlotForLight[MAX_LIGHTS];
uniform float uShadowFarPlanes[4];
uniform samplerCube uShadowMap0;
uniform samplerCube uShadowMap1;
uniform samplerCube uShadowMap2;
uniform samplerCube uShadowMap3;

out vec4 FragColor;

// --- Ruido procedural GLACILUX (Copyright (c) 2026 gennai, MIT License) ---
float gx_hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.x, p.y, p.x) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float gx_hash13(vec3 p) {
    vec3 p3 = fract(p * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 gx_hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.x, p.y, p.x) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float gx_vnoise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = gx_hash12(i);
    float b = gx_hash12(i + vec2(1.0, 0.0));
    float c = gx_hash12(i + vec2(0.0, 1.0));
    float d = gx_hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float gx_fbm2(vec2 p, int octaves) {
    float sum = 0.0;
    float amp = 0.5;
    float norm = 0.0;
    for (int o = 0; o < 5; o++) {
        if (o >= octaves) break;
        sum += gx_vnoise2(p) * amp;
        norm += amp;
        p *= 2.02;
        amp *= 0.5;
    }
    return sum / max(norm, 0.0001);
}

float gx_ridged2(vec2 p, int octaves) {
    float sum = 0.0;
    float amp = 0.5;
    float norm = 0.0;
    for (int o = 0; o < 5; o++) {
        if (o >= octaves) break;
        float n = 1.0 - abs(gx_vnoise2(p) * 2.0 - 1.0);
        sum += n * n * amp;
        norm += amp;
        p *= 2.13;
        amp *= 0.5;
    }
    return sum / max(norm, 0.0001);
}

vec2 gx_voronoi2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = p - i;
    float f1 = 8.0;
    float f2 = 8.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 g = vec2(float(x), float(y));
            vec2 o = gx_hash22(i + g);
            float d = length(g + o - f);
            if (d < f1) {
                f2 = f1;
                f1 = d;
            } else if (d < f2) {
                f2 = d;
            }
        }
    }
    return vec2(f1, f2 - f1);
}

// Leer la profundidad del cubemap correspondiente
float readShadowDepth(int slot, vec3 direction) {
    if (slot == 0) return texture(uShadowMap0, direction).r;
    if (slot == 1) return texture(uShadowMap1, direction).r;
    if (slot == 2) return texture(uShadowMap2, direction).r;
    if (slot == 3) return texture(uShadowMap3, direction).r;
    return 1.0;
}

// Calcular la visibilidad de una luz
float shadowVisibility(int lightIndex, vec3 lightPos, vec3 normal) {
    if (uShadowsEnabled == 0) return 1.0;

    int slot = uShadowSlotForLight[lightIndex];
    if (slot < 0 || slot >= 4) return 1.0;

    float farPlane = uShadowFarPlanes[slot];
    if (farPlane <= 0.0) return 1.0;

    vec3 lightToFrag = vWorldPos - lightPos;
    float currentDepth = length(lightToFrag);
    if (currentDepth >= farPlane) return 1.0;

    vec3 directionToLight = -lightToFrag / max(currentDepth, 0.0001);
    float angle = max(dot(normal, directionToLight), 0.0);
    float bias = max(0.01 * (1.0 - angle), 0.005);

	float storedDistance = readShadowDepth(slot, lightToFrag) * farPlane;

	if (currentDepth - bias > storedDistance)
		return 0.0;

	return 1.0;
}

void main() {
    const vec3 ICE_BODY  = vec3(0.21, 0.47, 0.76);
    const vec3 ICE_FROST = vec3(0.85, 0.94, 1.00);
    const vec3 ICE_GLOW  = vec3(0.55, 0.85, 1.00);
    const vec3 ICE_MIST  = vec3(0.74, 0.83, 0.92);

    // kinds 15..17: agua (water_jet / Undalux Water Bolt). Procedural, siguiendo el aspecto de
    // undalux_bolt.gdshader y undalux_splash.gdshader de gennai (MIT): agua aireada con burbujas,
    // fresnel rim, glint, gotas y bruma translúcida.
    if (vKind > 14.5) {
        if (vColor.a < 0.004) discard;
        const vec3 WATER_BODY = vec3(0.22, 0.52, 0.62);
        const vec3 WATER_FOAM = vec3(0.85, 0.95, 0.97);
        if (vKind < 15.5) {
            // 15: masa de agua aireada (orbe/bolt)
            vec3 nrm = normalize(vNormal);
            vec3 viewDir = normalize(uCamPos - vWorldPos);
            float ndv = clamp(abs(dot(nrm, viewDir)), 0.0, 1.0);
            float fres = pow(1.0 - ndv, 2.0);
            vec3 lp = normalize(vLocalPos);
            float t = (vWorldPos.x + vWorldPos.z) * 0.35;
            float b1 = gx_fbm2(vec2(atan(lp.z, lp.x) * 1.5 + t, lp.y * 5.0 - t * 1.4), 3);
            float b2 = gx_fbm2(vec2(lp.x * 4.0 - t * 0.8, lp.z * 4.0 + lp.y * 4.0), 2);
            float bubbles = smoothstep(0.48, 0.80, b1 * 0.65 + b2 * 0.45);
            float glint = pow(max(dot(reflect(-viewDir, nrm), normalize(vec3(0.4, 0.8, 0.3))), 0.0), 18.0);
            vec3 col = mix(WATER_BODY * vColor.rgb, WATER_FOAM, clamp(fres * 0.75 + bubbles * 0.55 + glint * 0.85, 0.0, 1.0));
            float alpha = clamp((0.38 + fres * 0.52 + bubbles * 0.30 + glint * 0.25) * vColor.a, 0.0, 0.94);
            FragColor = vec4(col, alpha);
            return;
        }
        if (vKind < 16.5) {
            // 16: gota de agua / salpicadura
            vec2 c = (vUV - 0.5) * 2.0;
            float r = length(c);
            if (r > 1.0) discard;
            float core = 1.0 - smoothstep(0.15, 0.95, r);
            float rim = smoothstep(0.35, 0.75, r) * (1.0 - smoothstep(0.75, 1.0, r));
            vec3 col = mix(WATER_BODY * vColor.rgb, WATER_FOAM, clamp(core * 0.45 + rim * 0.75, 0.0, 1.0));
            FragColor = vec4(col, clamp((core * 0.85 + rim * 0.35) * vColor.a, 0.0, 0.95));
            return;
        }
        // 17: bruma / velo de agua
        vec2 c = (vUV - 0.5) * 2.0;
        float r = length(c);
        if (r > 1.0) discard;
        float n = gx_fbm2(vUV * 3.5 + vec2(vWorldPos.x * 0.25, -vWorldPos.z * 0.25), 3);
        float mask = (1.0 - smoothstep(0.15, 1.0, r)) * smoothstep(0.25, 0.75, n);
        vec3 col = mix(WATER_BODY, WATER_FOAM, 0.65);
        FragColor = vec4(col, clamp(mask * vColor.a, 0.0, 0.85));
        return;
    }

    // kinds 12..14: viento (wind_blade). Procedural, escrito para BattleMage; la idea visual (esfera de aire con
    // corrientes en espiral, rafaga con anillo y envolvente) sigue "Spell Orbs - wind" de Himmelblau Studios.
    if (vKind > 11.5) {
        if (vColor.a < 0.004) discard;
        const vec3 AIR_DEEP  = vec3(0.60, 0.77, 0.90);
        const vec3 AIR_LIGHT = vec3(0.97, 1.00, 1.00);
        if (vKind < 12.5) {
            // 12: orbe/envolvente de aire. r = -, g = tiempo, b = 0 orbe | 1 solo envolvente, a = opacidad
            vec3 nrm = normalize(vNormal);
            vec3 viewDir = normalize(uCamPos - vWorldPos);
            float ndv = clamp(abs(dot(nrm, viewDir)), 0.0, 1.0);
            float fres = pow(1.0 - ndv, 1.7);
            float tt = vColor.g;
            vec3 lp = normalize(vLocalPos);
            // corrientes que rodean el eje de vuelo (+Y local), retorcidas segun la latitud (sin costura)
            float la = atan(lp.z, lp.x);
            float a1 = la + tt * 1.35 + lp.y * 2.4;
            float a2 = la - tt * 2.05 - lp.y * 3.6;
            float s1 = gx_fbm2(vec2(cos(a1), sin(a1)) * 1.6 + vec2(0.0, lp.y * 9.0), 3);
            float s2 = gx_fbm2(vec2(cos(a2), sin(a2)) * 2.3 + vec2(3.7, lp.y * 15.0 + 1.3), 3);
            float streak = smoothstep(0.44, 0.78, s1 * 0.62 + s2 * 0.5);
            float body = pow(ndv, 2.0) * 0.20;
            float a = mix(fres * 0.80 + streak * 0.55 + body, fres * 0.95 + streak * 0.12, vColor.b);
            vec3 col = mix(AIR_DEEP, AIR_LIGHT, clamp(streak * 0.7 + fres * 0.6, 0.0, 1.0));
            FragColor = vec4(col, clamp(a * vColor.a, 0.0, 0.92));
            return;
        }
        if (vKind < 13.5) {
            // 13: linea curva de viento, a lo largo de UV.y. r = semilla, g = curvatura 0..1, b = brillo, a = opacidad
            float seed = vColor.r;
            float t = vUV.y;
            float across = vUV.x - 0.5;
            float taper = pow(max(sin(3.14159265 * t), 0.0), 0.85);
            float wob = (gx_fbm2(vec2(t * 3.0 + seed * 7.0, seed * 3.1), 2) - 0.5) * 0.18;
            float xc = ((vColor.g - 0.5) * 0.85 + wob) * sin(3.14159265 * t);
            float w = 0.03 + 0.07 * taper;
            float d = abs(across - xc * 0.5);
            float core = 1.0 - smoothstep(w * 0.25, w * 0.85, d);
            float glow = exp(-d / (w * 2.4)) * 0.32;
            float gap = smoothstep(0.22, 0.58, gx_fbm2(vec2(t * 5.0 - seed * 9.0, seed * 4.0), 2) + 0.22);
            float m = (core + glow) * taper * gap;
            float a = clamp(m * vColor.a, 0.0, 1.0);
            if (a < 0.004) discard;
            FragColor = vec4(mix(AIR_DEEP, AIR_LIGHT, clamp(vColor.b * 0.6 + core * 0.4, 0.0, 1.0)), a);
            return;
        }
        // 14: onda de choque / remolino plano sobre la superficie. r = progreso (radio), g = semilla, b = 0 anillo | 1 remolino
        vec2 p = vUV * 2.0 - 1.0;
        float r = length(p);
        if (r >= 1.0) discard;
        float ang = atan(p.y, p.x);
        float seed = vColor.g;
        float n = gx_fbm2(vec2(cos(ang), sin(ang)) * 2.0 + vec2(seed * 7.0, vColor.r * 3.0), 3);
        float m;
        if (vColor.b < 0.5) {
            float rad = clamp(vColor.r, 0.10, 0.95);
            float th = 0.11;
            m = (1.0 - smoothstep(th * 0.3, th, abs(r - rad))) * (0.45 + 0.75 * n)
                + exp(-abs(r - rad) / (th * 1.7)) * 0.20;
        } else {
            float a2 = ang + r * 5.0 - vColor.r * 6.0;
            float arms = pow(0.5 + 0.5 * cos(a2 * 3.0), 3.0);
            m = arms * (1.0 - smoothstep(0.25, 0.95, r)) * smoothstep(0.0, 0.12, r) * (0.45 + 0.7 * n);
        }
        m *= 1.0 - smoothstep(0.90, 1.0, r);
        float a = clamp(m * vColor.a, 0.0, 1.0);
        if (a < 0.004) discard;
        FragColor = vec4(mix(AIR_DEEP, AIR_LIGHT, clamp(m * 0.8, 0.0, 1.0)), a);
        return;
    }

    // kind == 11: REEBOO Lightning Bolt Arc (sp_bolt.gdshader)
    if (vKind > 10.5) {
        if (vColor.a < 0.004) discard;
        float s = vColor.r;
        float jaggedness = vColor.g;
        float branchAmount = vColor.b;
        float y = vUV.y;
        float x = vUV.x - 0.5;

        // sp_arc_path(y, s)
        float off = 0.0;
        float amp = 1.0;
        float freq = 1.2;
        for (int i = 0; i < 5; i++) {
            off += (gx_vnoise2(vec2(y * freq, s * 0.173 + float(i) * 0.31)) - 0.5) * amp;
            freq *= 2.1;
            amp *= 0.55;
        }
        off *= sin(3.14159265 * y) * jaggedness;

        float d = abs(x - off);
        float core = 1.0 - smoothstep(0.025, 0.060, d);
        float glow = exp(-d / 0.095) * 0.55;

        // Rama secundaria (fork)
        float t = clamp((y - 0.25) / 0.40, 0.0, 1.0);
        float sideSign = fract(s * 0.618) > 0.5 ? 1.0 : -1.0;
        float bOff = 0.0;
        float bAmp = 1.0;
        float bFreq = 1.2;
        for (int i = 0; i < 5; i++) {
            bOff += (gx_vnoise2(vec2(y * 1.6 * bFreq, (s + 17.0) * 0.173 + float(i) * 0.31)) - 0.5) * bAmp;
            bFreq *= 2.1;
            bAmp *= 0.55;
        }
        bOff = off + (bOff * sin(3.14159265 * y) * 0.5 + sideSign * 0.2) * t * jaggedness * 1.8;
        float bd = abs(x - bOff);
        float fork = ((1.0 - smoothstep(0.015, 0.036, bd)) + exp(-bd / 0.055) * 0.32)
            * step(0.25, y) * (1.0 - smoothstep(0.50, 0.70, y)) * branchAmount;

        float ends = smoothstep(0.0, 0.05, y) * smoothstep(0.0, 0.05, 1.0 - y);
        float strobe = mix(0.45, 1.0, step(0.30, gx_hash12(vec2(s * 1.91, s * 0.73))));
        float m = (core + glow + fork) * ends * strobe;
        float a = clamp(m * vColor.a, 0.0, 1.0);
        if (a < 0.004) discard;
        vec3 col = mix(vec3(0.45, 0.62, 1.0), vec3(0.95, 0.98, 1.0), clamp(core + fork * 0.5, 0.0, 1.0)) * 3.8;
        FragColor = vec4(col, a);
        return;
    }

    // kind == 10: REEBOO Fire Surface Decals (sp_decal.gdshaderinc: Scorch, Glow Pool, Ring Wave)
    if (vKind > 9.5) {
        if (vColor.a < 0.004) discard;
        vec2 p = vUV * 2.0 - 1.0;
        float r = length(p);
        if (r >= 1.0) discard;
        float seed = vColor.g;
        float n = gx_fbm2(vUV * 2.2 + vec2(seed * 5.1, seed * 2.7), 3);
        float n2 = gx_fbm2(vUV * 4.8 + vec2(seed * 9.3, seed * 4.1), 3);
        float borderFade = 1.0 - smoothstep(0.92, 1.0, r);
        vec3 col;
        float a;
        if (vColor.b < 0.25) {
            // Style 2: Scorch (sp_decal_mix) — marca quemada oscura con centro incandescente inicial
            float edge = r + (n - 0.5) * 0.46 + (n2 - 0.5) * 0.15;
            float m = (1.0 - smoothstep(0.42, 0.82, edge)) * (0.75 + 0.25 * n2);
            float hot = (1.0 - smoothstep(0.0, 0.42, edge)) * vColor.r;
            col = mix(vec3(0.05, 0.035, 0.03), vec3(0.85, 0.22, 0.03), clamp(hot, 0.0, 1.0));
            a = m * borderFade * vColor.a;
        } else if (vColor.b < 0.65) {
            // Style 1: Glow Pool (sp_decal_add) — resplandor calido en el suelo
            float m = pow(clamp(1.0 - r, 0.0, 1.0), 1.6) * (0.7 + 0.3 * n);
            float hot = pow(clamp(1.0 - r * 1.5, 0.0, 1.0), 2.0);
            col = mix(vec3(1.0, 0.30, 0.04), vec3(1.0, 0.72, 0.25), hot) * 2.1;
            a = m * borderFade * vColor.a;
        } else {
            // Style 0: Ring Wave (sp_decal_add / sp_add shape 3) — anillo expansivo de fuego
            float rad = clamp(vColor.r, 0.12, 0.92);
            float th = 0.095 * mix(1.5, 0.55, rad);
            float m = (1.0 - smoothstep(th * 0.3, th, abs(r - rad))) * (0.65 + 0.35 * n);
            m += exp(-abs(r - rad) / max(th * 1.5, 0.01)) * 0.22;
            float hot = 1.0 - smoothstep(0.15, 0.85, rad);
            col = mix(vec3(1.0, 0.40, 0.08), vec3(1.0, 0.86, 0.45), hot) * 2.6;
            a = m * borderFade * vColor.a;
        }
        if (a < 0.004) discard;
        FragColor = vec4(col, clamp(a, 0.0, 1.0));
        return;
    }

    // kind == 9: REEBOO Fire Smoke (sp_mix.gdshader, shape = 4, puff_shading)
    if (vKind > 8.5) {
        if (vColor.a < 0.004) discard;
        vec2 p = vUV * 2.0 - 1.0;
        float r = length(p);
        float seed = fract(vColor.g * 37.3 + vColor.b * 71.7);
        float age = clamp(1.0 - vColor.r * 2.3, 0.0, 1.0);
        float n = gx_fbm2(vUV * 2.8 + vec2(seed * 7.31, seed * 3.17), 3);
        float edge = r + (n - 0.5) * 0.46;
        float m = 1.0 - smoothstep(0.05, 1.0, edge + 0.14);
        float value = m * (0.35 + 0.65 * n);
        float threshold = age * 0.44;
        float cut = smoothstep(threshold, threshold + 0.30, value);
        m = cut * mix(1.0, m, 0.35);
        float a = m * vColor.a;
        if (a < 0.004) discard;
        float up = 0.5 - vUV.y;
        float light = mix(0.65, 1.22, up + 0.5) * (0.85 + 0.30 * n);
        vec3 col = vColor.rgb * mix(1.0, light, 0.58);
        FragColor = vec4(col, clamp(a, 0.0, 1.0));
        return;
    }

    // kind == 8: REEBOO Fire Flash & Embers (sp_add.gdshader, shape = 1 Flare / shape = 2 Streak)
    if (vKind > 7.5) {
        if (vColor.a < 0.004) discard;
        vec2 p = vUV * 2.0 - 1.0;
        float r = length(p);
        float core = exp(-r * r * 8.0);
        float rays = max(
            exp(-abs(p.x) * 22.0) * max(1.0 - abs(p.y), 0.0),
            exp(-abs(p.y) * 22.0) * max(1.0 - abs(p.x), 0.0)
        );
        float m = clamp(core + rays * 0.85, 0.0, 1.0);
        float a = m * vColor.a;
        if (a < 0.004) discard;
        vec3 col = mix(vColor.rgb, vec3(1.0, 0.96, 0.82), core * 0.75) * 2.8;
        FragColor = vec4(col, clamp(a, 0.0, 1.0));
        return;
    }

    // kind == 7: REEBOO Fire Flames & Trail (sp_add.gdshader shape = 4 Puff & sp_trail_add.gdshader)
    if (vKind > 6.5) {
        if (vColor.a < 0.004) discard;
        vec2 p = vUV * 2.0 - 1.0;
        float r = length(p);
        float seed = fract(vColor.g * 29.3 + vColor.b * 61.7);
        float age = clamp(1.0 - vColor.g * 1.05, 0.0, 1.0);
        float n = gx_fbm2(vUV * 3.0 + vec2(seed * 7.31, -age * 2.6 + seed * 3.17), 3);
        float edge = r + (n - 0.5) * 0.45;
        float m = 1.0 - smoothstep(0.15, 1.0, edge + 0.14);
        float value = m * (0.35 + 0.65 * n);
        float threshold = age * 0.58;
        float cut = smoothstep(threshold, threshold + 0.22, value);
        float band = cut * (1.0 - smoothstep(threshold + 0.22, threshold + 0.58, value));
        m = cut * mix(1.0, m, 0.38);
        float a = m * vColor.a;
        if (a < 0.004) discard;
        float hotCenter = 1.0 - smoothstep(0.0, 0.58, r);
        vec3 col = mix(vColor.rgb, vec3(1.0, 0.95, 0.72), hotCenter * 0.55);
        col = mix(col, vec3(0.75, 0.12, 0.01), band * 0.55) * 2.15;
        FragColor = vec4(col, clamp(a, 0.0, 1.0));
        return;
    }

    // kind == 6: REEBOO Fire Projectile Orb Core (sp_orb.gdshader, style = 1 Flame)
    if (vKind > 5.5) {
        if (vColor.a < 0.004) discard;
        vec3 nrm = normalize(vNormal);
        vec3 viewDir = normalize(uCamPos - vWorldPos);
        float ndv = clamp(abs(dot(nrm, viewDir)), 0.0, 1.0);
        float fres = 1.0 - ndv;
        float tScroll = vColor.g;
        vec2 nuv = vUV * vec2(4.0, 2.0);
        float n  = gx_fbm2(nuv + vec2(tScroll * 0.31, -tScroll), 3);
        float n2 = gx_fbm2(nuv * 1.7 + vec2(-tScroll * 0.53, tScroll * 0.4) + vec2(0.5), 3);
        float churn = smoothstep(0.28, 0.78, n * 0.65 + n2 * 0.55);
        float core = pow(ndv, 2.2) * 0.85;
        float m = (fres * 0.85 + churn * 0.95) * (0.55 + 0.85 * n) + core * 0.85;
        float hot = clamp(core * 0.92 + churn * 0.30, 0.0, 1.0);
        vec3 col = mix(vec3(1.0, 0.38, 0.05), vec3(1.0, 0.86, 0.42), hot) * 2.35;
        FragColor = vec4(col, clamp(m * vColor.a, 0.0, 1.0));
        return;
    }

    // kind == 5: Parche de escarcha sobre la superficie (glacilux_ground.gdshader)
    if (vKind > 4.5) {
        if (vColor.a < 0.004 || vColor.r < 0.004) discard;
        vec2 d = (vUV - 0.5) * 2.0;
        float r = length(d);
        float v_rand = vColor.g;
        float ang = atan(d.y, d.x);
        float edge = 0.74 + 0.24 * gx_fbm2(vec2(cos(ang), sin(ang)) * 2.4 + v_rand * 11.0, 3);
        float inside = clamp(r / max(edge, 0.01), 0.0, 2.0);
        vec2 pp = d * 2.2;
        float ridge = gx_ridged2(pp * 2.6 + v_rand * 13.0, 4);
        float blotch = gx_fbm2(pp * 0.95 + v_rand * 7.0, 3);
        float field = ridge * 0.72 + blotch * 0.28;
        float lim = mix(0.16, 0.88, smoothstep(0.22, 1.0, inside));
        float cov = smoothstep(lim - 0.08, lim + 0.11, field);
        float dendrite = smoothstep(0.28, 0.58, ridge);
        float grain = smoothstep(0.82, 0.96, gx_vnoise2(pp * 9.5 + v_rand));
        vec3 blue = clamp(ICE_BODY * 1.7, vec3(0.0), vec3(1.0));
        vec3 col = mix(mix(blue, ICE_FROST, 0.42 + 0.56 * dendrite), ICE_FROST, grain * 0.5);
        float a = cov * (0.28 + 0.62 * dendrite) * vColor.a;
        if (a < 0.004) discard;
        FragColor = vec4(col + ICE_GLOW * 0.16, clamp(a, 0.0, 1.0));
        return;
    }

    // kind == 4: Destellos / Sparkles (glacilux_sparkle.gdshader / glacilux_trail.gdshader part 1)
    if (vKind > 3.5) {
        if (vColor.a < 0.004) discard;
        vec2 d = (vUV - 0.5) * 2.0;
        float r = length(d);
        float core = 1.0 - smoothstep(0.0, 0.55, r);
        float star = max(0.0, 1.0 - abs(d.x) * 7.0) * max(0.0, 1.0 - abs(d.y) * 1.3);
        star += max(0.0, 1.0 - abs(d.y) * 7.0) * max(0.0, 1.0 - abs(d.x) * 1.3);
        float s = clamp(core + star * 0.65, 0.0, 1.0) * vColor.a;
        if (s < 0.004) discard;
        vec3 col = mix(ICE_GLOW, ICE_FROST, 0.58) * 1.35 + uEmission;
        FragColor = vec4(col, clamp(s, 0.0, 1.0));
        return;
    }

    // kind == 3: Niebla fria / Cold Mist (glacilux_mist.gdshader / glacilux_trail.gdshader part 0)
    if (vKind > 2.5) {
        if (vColor.a < 0.004) discard;
        vec2 d = (vUV - 0.5) * 2.0;
        float seedf = vColor.g * 13.0;
        float ang = seedf * 2.17;
        float cs = cos(ang);
        float sn = sin(ang);
        vec2 dr = vec2(d.x * cs - d.y * sn, d.x * sn + d.y * cs);
        dr.y *= mix(1.0, 1.45, fract(seedf * 0.41));
        float r = length(dr);
        float n = gx_fbm2(vUV * 3.4 + seedf, 3);
        r *= 1.0 + n * 0.5;
        float soft = 1.0 - smoothstep(0.0, 1.0, r);
        soft = pow(clamp(soft, 0.0, 1.0), 1.3);
        float wisp = mix(0.45, 1.05, gx_fbm2(vUV * 6.2 + seedf * 1.7, 3));
        float a = soft * wisp * vColor.a;
        if (a < 0.004) discard;
        vec3 col = mix(ICE_MIST * 0.78, ICE_FROST, smoothstep(0.08, 0.9, soft) * 0.68) + uEmission * 0.15;
        FragColor = vec4(col, clamp(a, 0.0, 1.0));
        return;
    }

    vec3 n = normalize(vNormal);
    // Luces ambiente
    vec3 sky    = vec3(0.45, 0.55, 0.75); 
    vec3 ground = vec3(0.12, 0.10, 0.08);
    vec3 lit    = mix(ground, sky, n.y * 0.5 + 0.5) * 0.6;

    for (int i = 0; i < uNumLights; ++i) {
        vec4  pr      = uLights[2 * i];
        vec4  ci      = uLights[2 * i + 1];
        vec3  toLight = pr.xyz - vWorldPos;
        float dist    = length(toLight);
        float diff    = max(dot(n, toLight / max(dist, 0.0001)), 0.0);
        float atten   = pow(clamp(1.0 - dist / pr.w, 0.0, 1.0), 2.0);

        float visibility = 1.0;
        if (diff > 0.0 && atten > 0.0)
            visibility = shadowVisibility(i, pr.xyz, n);

        lit += ci.rgb * ci.a * diff * atten * visibility;
    }

    // kind == 2: Cristal de hielo / Corona / Shards (glacilux_spike.gdshader & glacilux_shards.gdshader)
    if (vKind > 1.5) {
        if (vColor.a < 0.004) discard;
        if (vColor.a < 0.98) {
            float d = gx_hash13(floor(vLocalPos * 26.0) + floor(vColor.g * 37.0));
            if (d > vColor.a * 1.3 + 0.04) discard;
        }
        vec3 viewDir = normalize(uCamPos - vWorldPos);
        float fres = pow(1.0 - clamp(abs(dot(n, viewDir)), 0.0, 1.0), 2.3);
        float v_tip = clamp(vUV.y, 0.0, 1.0);
        float v_crack = clamp(vUV.x, 0.0, 1.0);
        vec2 cr = gx_voronoi2(vLocalPos.xz * 4.2 + vec2(vLocalPos.y * 2.4) + vColor.g * 19.0);
        float crack = (1.0 - smoothstep(0.0, 0.11, cr.y)) * v_crack;
        vec3 bodyTint = mix(ICE_BODY, vColor.rgb, 0.35);
        vec3 baseIce = mix(bodyTint, ICE_FROST, 0.38 + 0.36 * v_tip);
        vec3 albedo = mix(baseIce, ICE_FROST, clamp(fres * 0.65, 0.0, 1.0)) * (1.0 - crack * 0.42);
        float keyShape = 0.68 + 0.42 * clamp(dot(n, normalize(vec3(0.38, 0.84, 0.39))), -1.0, 1.0);
        vec3 halfVec = normalize(viewDir + normalize(vec3(0.35, 0.85, 0.40)));
        float spec = pow(max(dot(n, halfVec), 0.0), 28.0) * (1.0 - crack * 0.5) * 0.55;
        vec3 glow = (ICE_GLOW + uEmission) * (0.32 + fres * 0.65 + v_tip * 0.38);
        FragColor = vec4(albedo * (lit * 0.65 + keyShape * 0.75) + glow + ICE_FROST * spec, 1.0);
        return;
    }

    vec3 base;
    if (vKind > 0.5) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        base = vec3(mix(0.25, 0.6, c));
     } else {
        base = texture(uAlbedo, vUV).rgb * vColor.rgb;
    }
    FragColor = vec4(base * lit + uEmission, 1.0);
})";


const char* depthFragmentSrc = R"GLSL(#version 330 core
in vec3 vWorldPos;
uniform vec3 uLightPosition;
uniform float uFarPlane;

void main() {
    float distanceToLight = length(vWorldPos - uLightPosition);
    gl_FragDepth = clamp(distanceToLight / uFarPlane, 0.0, 1.0);
}
)GLSL";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length + 1, '\0');
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Error de shader: ") + log.c_str());
    }
    return shader;
}

GLuint createShaderProgram(const char* vertex, const char* fragment) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex);
    GLuint fs;
    try {
        fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length + 1, '\0');
        glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Error enlazando shaders: ") + log.c_str());
    }
    return program;
}

GLuint setShaders() {
    const GLuint program = createShaderProgram(vertexSrc, fragmentSrc);
    glEnable(GL_DEPTH_TEST); // Z-Buffer

    glUseProgram(program);

    return program;
}
