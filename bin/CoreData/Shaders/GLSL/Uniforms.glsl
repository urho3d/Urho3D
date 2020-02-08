// Use of constant buffers on OpenGL 3 commented out for now as it seems to be slower in practice
//#define USE_CBUFFERS

#if !defined(GL3) || !defined(USE_CBUFFERS)

// OpenGL 2 uniforms (no constant buffers)

#ifdef COMPILEVS

// Vertex shader uniforms
uniform vec3 cAmbientStartColor;
uniform vec3 cAmbientEndColor;
uniform mat3 cBillboardRot;
uniform vec3 cCameraPos;
uniform float cNearClip;
uniform float cFarClip;
uniform vec4 cDepthMode;
uniform vec3 cFrustumSize;
uniform float cDeltaTime;
uniform float cElapsedTime;
uniform vec4 cGBufferOffsets;
uniform vec4 cLightPos;
uniform vec3 cLightDir;
uniform vec4 cNormalOffsetScale;
uniform mat4 cModel;
uniform mat4 cView;
uniform mat4 cViewInv;
uniform mat4 cViewProj;
uniform vec4 cUOffset;
uniform vec4 cVOffset;
uniform mat4 cZone;
#if !defined(GL_ES) || defined(WEBGL) || __VERSION__>=300
    uniform mat4 cLightMatrices[4];
#else
    uniform highp mat4 cLightMatrices[2];
#endif
#ifdef SKINNED
    uniform vec4 cSkinMatrices[MAXBONES*3];
#endif
#ifdef NUMVERTEXLIGHTS
    uniform vec4 cVertexLights[4*3];
#endif
#ifdef GL3
    uniform vec4 cClipPlane;
#endif
#endif

#ifdef COMPILEPS

// Fragment shader uniforms
#ifdef MOBILE_GRAPHICS
    precision mediump float;
#endif

uniform vec4 cAmbientColor;
uniform vec3 cCameraPosPS;
uniform float cDeltaTimePS;
uniform vec4 cDepthReconstruct;
uniform float cElapsedTimePS;
uniform vec4 cFogParams;
uniform vec3 cFogColor;
uniform vec2 cGBufferInvSize;
uniform vec4 cLightColor;
uniform vec4 cLightPosPS;
uniform vec3 cLightDirPS;
uniform vec4 cNormalOffsetScalePS;
uniform vec4 cMatDiffColor;
uniform vec3 cMatEmissiveColor;
uniform vec3 cMatEnvMapColor;
uniform vec4 cMatSpecColor;
#ifdef PBR
    uniform float cRoughness;
    uniform float cMetallic;
    uniform float cLightRad;
    uniform float cLightLength;
#endif
uniform vec3 cZoneMin;
uniform vec3 cZoneMax;
uniform float cNearClipPS;
uniform float cFarClipPS;
uniform vec4 cShadowCubeAdjust;
uniform vec4 cShadowDepthFade;
uniform vec2 cShadowIntensity;
uniform vec2 cShadowMapInvSize;
uniform vec4 cShadowSplits;
uniform mat4 cLightMatricesPS[4];
#ifdef VSM_SHADOW
uniform vec2 cVSMShadowParams;
#endif
#endif

#else

// OpenGL 3 uniforms (using constant buffers)

#ifdef COMPILEVS

uniform FrameVS
{
    float cDeltaTime;
    float cElapsedTime;
};

uniform CameraVS
{
    vec3 cCameraPos;
    float cNearClip;
    float cFarClip;
    vec4 cDepthMode;
    vec3 cFrustumSize;
    vec4 cGBufferOffsets;
    mat4 cView;
    mat4 cViewInv;
    mat4 cViewProj;
    vec4 cClipPlane;
};

uniform ZoneVS
{
    vec3 cAmbientStartColor;
    vec3 cAmbientEndColor;
    mat4 cZone;
};

uniform LightVS
{
    vec4 cLightPos;
    vec3 cLightDir;
    vec4 cNormalOffsetScale;
#ifdef NUMVERTEXLIGHTS
    vec4 cVertexLights[4 * 3];
#else
    mat4 cLightMatrices[4];
#endif
};

#ifndef CUSTOM_MATERIAL_CBUFFER
uniform MaterialVS
{
    vec4 cUOffset;
    vec4 cVOffset;
};
#endif

uniform ObjectVS
{
    mat4 cModel;
#ifdef BILLBOARD
    mat3 cBillboardRot;
#endif
#ifdef SKINNED
    uniform vec4 cSkinMatrices[MAXBONES*3];
#endif
};

#endif

#ifdef COMPILEPS

#ifdef MOBILE_GRAPHICS
    precision mediump float;
#endif

// Pixel shader uniforms
uniform FramePS
{
    float cDeltaTimePS;
    float cElapsedTimePS;
};

uniform CameraPS
{
    vec3 cCameraPosPS;
    vec4 cDepthReconstruct;
    vec2 cGBufferInvSize;
    float cNearClipPS;
    float cFarClipPS;
};

uniform ZonePS
{
    vec4 cAmbientColor;
    vec4 cFogParams;
    vec3 cFogColor;
    vec3 cZoneMin;
    vec3 cZoneMax;
};

uniform LightPS
{
    vec4 cLightColor;
    vec4 cLightPosPS;
    vec3 cLightDirPS;
    vec4 cNormalOffsetScalePS;
    vec4 cShadowCubeAdjust;
    vec4 cShadowDepthFade;
    vec2 cShadowIntensity;
    vec2 cShadowMapInvSize;
    vec4 cShadowSplits;
    mat4 cLightMatricesPS[4];
#ifdef VSM_SHADOW
    vec2 cVSMShadowParams;
#endif
#ifdef PBR
    float cLightRad;
    float cLightLength;
#endif
};

#ifndef CUSTOM_MATERIAL_CBUFFER
uniform MaterialPS
{
    vec4 cMatDiffColor;
    vec3 cMatEmissiveColor;
    vec3 cMatEnvMapColor;
    vec4 cMatSpecColor;
#ifdef PBR
    float cRoughness;
    float cMetallic;
#endif
};
#endif

#endif

#endif
