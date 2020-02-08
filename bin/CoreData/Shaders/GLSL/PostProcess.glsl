#ifdef COMPILEPS
const float PI = 3.14159265;

vec2 Noise(vec2 coord)
{
    float noiseX = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0);
    float noiseY = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0);
    return vec2(noiseX, noiseY);
}

// Adapted: http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
vec4 GaussianBlur(int blurKernelSize, vec2 blurDir, vec2 blurRadius, float sigma, sampler2D texSampler, vec2 texCoord)
{
    int blurKernelSizeHalfSize = blurKernelSize / 2;

    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    vec3 gaussCoeff;
    gaussCoeff.x = 1.0 / (sqrt(2.0 * PI) * sigma);
    gaussCoeff.y = exp(-0.5 / (sigma * sigma));
    gaussCoeff.z = gaussCoeff.y * gaussCoeff.y;

    vec2 blurVec = blurRadius * blurDir;
    vec4 avgValue = vec4(0.0);
    float gaussCoeffSum = 0.0;

    avgValue += texture2D(texSampler, texCoord) * gaussCoeff.x;
    gaussCoeffSum += gaussCoeff.x;
    gaussCoeff.xy *= gaussCoeff.yz;

    for (int i = 1; i <= blurKernelSizeHalfSize; i++)
    {
        avgValue += texture2D(texSampler, texCoord - float(i) * blurVec) * gaussCoeff.x;
        avgValue += texture2D(texSampler, texCoord + float(i) * blurVec) * gaussCoeff.x;

        gaussCoeffSum += 2.0 * gaussCoeff.x;
        gaussCoeff.xy *= gaussCoeff.yz;
    }

    return avgValue / gaussCoeffSum;
}

const vec3 LumWeights = vec3(0.2126, 0.7152, 0.0722);

vec3 ReinhardEq3Tonemap(vec3 x)
{
    return x / (1.0 + x);
}

vec3 ReinhardEq4Tonemap(vec3 x, float white)
{
    return x * (1.0 + x / white) / (1.0 + x);
}

// Unchared2 tone mapping (See http://filmicgames.com)
const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;

vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

#if defined(GL_ES) && __VERSION__ >= 300
#define defprec mediump
#else
#define defprec
#endif

#if !defined(GL_ES) || __VERSION__ >= 300
vec3 ColorCorrection(vec3 color, defprec sampler3D lut)
{
    float lutSize = 16.0;
    float scale = (lutSize - 1.0) / lutSize;
    float offset = 1.0 / (2.0 * lutSize);
    return texture3D(lut, clamp(color, 0.0, 1.0) * scale + offset).rgb;
}
#endif

const float Gamma = 2.2;
const float InverseGamma = 1.0 / 2.2;

vec3 ToGamma(vec3 color)
{
    return vec3(pow(color.r, Gamma), pow(color.g, Gamma), pow(color.b, Gamma));
}

vec3 ToInverseGamma(vec3 color)
{
    return vec3(pow(color.r, InverseGamma), pow(color.g, InverseGamma), pow(color.b, InverseGamma));
}
#endif
