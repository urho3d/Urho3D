#ifdef COMPILEPS
uniform sampler2D sDiffMap;
uniform samplerCube sDiffCubeMap;
uniform sampler2D sNormalMap;
uniform sampler2D sSpecMap;
uniform sampler2D sEmissiveMap;
uniform sampler2D sEnvMap;
uniform samplerCube sEnvCubeMap;
uniform sampler2D sLightRampMap;
uniform sampler2D sLightSpotMap;
uniform samplerCube sLightCubeMap;
#if !defined(GL_ES) || __VERSION__ >= 300
    uniform highp sampler3D sVolumeMap;
    uniform sampler2D sAlbedoBuffer;
    uniform sampler2D sNormalBuffer;
    uniform sampler2D sDepthBuffer;
    uniform sampler2D sLightBuffer;
    #ifdef VSM_SHADOW
        uniform highp sampler2D sShadowMap;
    #else
        uniform highp sampler2DShadow sShadowMap;
    #endif
    uniform samplerCube sFaceSelectCubeMap;
    uniform samplerCube sIndirectionCubeMap;
    uniform samplerCube sZoneCubeMap;
    uniform highp sampler3D sZoneVolumeMap;
#else
    uniform highp sampler2D sShadowMap;
#endif

#ifdef GL3
#define texture2D texture
#define texture2DProj textureProj
#define texture3D texture
#define textureCube texture
#define texture2DLod textureLod
#define texture2DLodOffset textureLodOffset
#endif

vec3 DecodeNormal(vec4 normalInput)
{
    #ifdef PACKEDNORMAL
        vec3 normal;
        normal.xy = normalInput.ag * 2.0 - 1.0;
        normal.z = sqrt(max(1.0 - dot(normal.xy, normal.xy), 0.0));
        return normal;
    #else
        return normalize(normalInput.rgb * 2.0 - 1.0);
    #endif
}

vec3 EncodeDepth(float depth)
{
    #if !defined(GL3) || (defined(GL_ES) && __VERSION__ < 300)
        vec3 ret;
        depth *= 255.0;
        ret.x = floor(depth);
        depth = (depth - ret.x) * 255.0;
        ret.y = floor(depth);
        ret.z = (depth - ret.y);
        ret.xy *= 1.0 / 255.0;
        return ret;
    #else
        // OpenGL 3 can use different MRT formats, so no need for encoding
        return vec3(depth, 0.0, 0.0);
    #endif
}

float DecodeDepth(vec3 depth)
{
    #if !defined(GL3) || (defined(GL_ES) && __VERSION__ < 300)
        const vec3 dotValues = vec3(1.0, 1.0 / 255.0, 1.0 / (255.0 * 255.0));
        return dot(depth, dotValues);
    #else
        // OpenGL 3 can use different MRT formats, so no need for encoding
        return depth.r;
    #endif
}

float ReconstructDepth(float hwDepth)
{
    return dot(vec2(hwDepth, cDepthReconstruct.y / (hwDepth - cDepthReconstruct.x)), cDepthReconstruct.zw);
}
#endif
