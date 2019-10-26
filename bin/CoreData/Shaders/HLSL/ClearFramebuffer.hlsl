#urhover 1.8.0
#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#ifdef COMPILEVS
void VS(float4 iPos : POSITION,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
}
#endif
#ifdef COMPILEPS
void PS(out float4 oColor : OUTCOLOR0)
{
    oColor = cMatDiffColor;
}
#endif
