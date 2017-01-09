#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef TEXT_EFFECT_SHADOW
uniform vec2 cShadowOffset;
uniform vec4 cShadowColor;
#endif

#ifdef TEXT_EFFECT_STROKE
uniform vec4 cStrokeColor;
#endif
#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    
    vTexCoord = iTexCoord;
    vColor = iColor;
}
#endif
#ifdef COMPILEPS
void main()
{
#ifdef SIGNED_DISTANCE_FIELD
    gl_FragColor.rgb = vColor.rgb;

    float distance = texture2D(sDiffMap, vTexCoord).a;
    if (distance < 0.5)
    {
    #ifdef TEXT_EFFECT_SHADOW
        if (texture2D(sDiffMap, vTexCoord - cShadowOffset).a > 0.5)
            gl_FragColor = cShadowColor;
        else
    #endif
        gl_FragColor.a = 0.0;
    }
    else
    {
    #ifdef TEXT_EFFECT_STROKE
        if (distance < 0.525)
            gl_FragColor.rgb = cStrokeColor.rgb;
    #endif

    #ifdef TEXT_EFFECT_SHADOW
        if (texture2D(sDiffMap, vTexCoord + cShadowOffset).a < 0.5)
            gl_FragColor.a = vColor.a;
        else
    #endif
        gl_FragColor.a = vColor.a * smoothstep(0.5, 0.505, distance);
    }
#else
    #ifdef ALPHAMAP
        gl_FragColor.rgb = vColor.rgb;
        #ifdef GL3
            gl_FragColor.a = vColor.a * texture2D(sDiffMap, vTexCoord).r;
        #else
            gl_FragColor.a = vColor.a * texture2D(sDiffMap, vTexCoord).a;
        #endif
    #else
        gl_FragColor = vColor * texture2D(sDiffMap, vTexCoord);
    #endif
#endif
}
#endif
