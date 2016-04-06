#ifdef COMPILEPS
    float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
    {
        float a = Roughness * Roughness;
        float Phi = 2.0 * 3.14159 * Xi.x;
        float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
        float SinTheta = sqrt(1.0 - CosTheta * CosTheta);
        float3 H = 0;
        H.x = SinTheta * cos(Phi);
        H.y = SinTheta * sin(Phi);
        H.z = CosTheta;

        float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
        float3 TangentX = normalize(cross(UpVector, N));
        float3 TangentY = cross(N, TangentX);
        // Tangent to world space
        return TangentX * H.x + TangentY * H.y + N * H.z;
    }

    float3 GetSpecularDominantDir(float3 normal, float3 reflection, float roughness)
    {
        const float smoothness = 1.0 - roughness;
        const float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
        return lerp(normal, reflection, lerpFactor);
    }

    #define IMPORTANCE_SAMPLES 16
    static const float2 IMPORTANCE_KERNEL[IMPORTANCE_SAMPLES] =
    {
        float2(-0.0780436, 0.0558389),
        float2(0.034318, -0.0635879),
        float2(0.00230821, 0.0807279),
        float2(0.0124638, 0.117585),
        float2(0.093943, -0.0944602),
        float2(0.139348, -0.109816),
        float2(-0.181872, -0.129649),
        float2(0.240066, -0.0494057),
        float2(0.115965, -0.0374714),
        float2(-0.294819, -0.100726),
        float2(-0.149652, 0.37459),
        float2(0.261695, -0.292813),
        float2(-0.37944, -0.425145),
        float2(0.628994, -0.189387),
        float2(-0.331257, -0.646864),
        float2(-0.467004, 0.439687),
    };


/*            const float3 Hn = normalize(V + wsNormal);
            const float ndv = saturate(dot(V, wsNormal));
            const float vdh = saturate(dot(V, Hn));
            const float ndh = saturate(dot(wsNormal, Hn));*/

    float3 ImportanceSampling(in float3 reflectVec, in float3 wsNormal, in float3 toCamera, in float3 specular, in float roughness, out float3 reflectionCubeColor)
    {
        reflectionCubeColor = float3(1,1,1);
        const float3 V = -toCamera;
        const float3 N = wsNormal;
        const float ndv = saturate(dot(N, V));

        float smoothness = 1.0 - roughness;
        //float mipLevel = (1.0 - smoothness * smoothness) * 10.0;

        float3 accumulatedColor = float3(0,0,0);
        for (int i = 0; i < IMPORTANCE_SAMPLES; i++)
        {
            float3 H = ImportanceSampleGGX(IMPORTANCE_KERNEL[i], roughness, N);
            float3 L = normalize(-reflect(V, H));

            const float vdh = saturate(dot(V, H));
            const float ndh = saturate(dot(N, V + N));
            float ndl = saturate(dot(N, L));

            if (ndl > 0.0)
            {
              float G = SchlickVisibility(ndl, ndv, roughness);
              float Fc = pow(1.0 - vdh, 5.0);
              float3 F = (1.0 - Fc) * specular + Fc;

              float D = (smoothness * smoothness) * 2.0;//DFactor(roughness,NdotH);
              float pdf = (D * ndh / (4 * vdh)) + 0.0001f  ;
              float saTexel = 4.0f * 3.14159 / (6.0f * 128 * 128);
              float saSample = 1.0f / (IMPORTANCE_SAMPLES * pdf)  ;
              float mipLevel = roughness == 0.0f ? 0.0f :  0.5f * log2( saSample / saTexel )	;

              float3 sampledColor = SampleCubeLOD(ZoneCubeMap, float4(L, mipLevel));

              accumulatedColor += sampledColor * F * G * vdh / (ndh * ndv);
            }
        }

        return accumulatedColor / IMPORTANCE_SAMPLES;
    }

    float3 EnvBRDFApprox (float3 SpecularColor, float Roughness, float NoV)
    {
        const float4 c0 = float4(-1, -0.0275, -0.572, 0.022 );
        const float4 c1 = float4(1, 0.0425, 1.0, -0.04 );
        float4 r = Roughness * c0 + c1;
        float a004 = min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
        float2 AB = float2( -1.04, 1.04 ) * a004 + r.zw;
        return SpecularColor * AB.x + AB.y;
    }

    float EnvBRDFApproxNonmetal( float Roughness, float NoV )
    {
        // Same as EnvBRDFApprox( 0.04, Roughness, NoV )
        const float2 c0 = { -1, -0.0275 };
        const float2 c1 = { 1, 0.0425 };
        float2 r = Roughness * c0 + c1;
        return min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
    }

    float3 ImageBasedLighting(in float3 reflectVec, in float3 wsNormal, in float3 toCamera, in float3 specular, in float roughness, in float metallic, out float3 reflectionCubeColor)
    {
        // reflectionCubeColor = SampleCubeLOD(ZoneCubeMap, float4(wsNormal, 9.6)).rgb;

        // return ImportanceSampling(reflectVec, wsNormal, toCamera, specular, roughness, reflectionCubeColor);

        reflectVec = GetSpecularDominantDir(wsNormal, reflectVec, roughness);
        const float3 Hn = normalize(-toCamera + wsNormal);
        const float vdh = saturate(dot(-toCamera, Hn));
        const float ndv = saturate(dot(-toCamera, wsNormal));

        float smoothness = 1.0 - roughness;
        float mipLevel = (1.0 - smoothness * smoothness) * 10.0;

        float3 cube =  ImportanceSampling(reflectVec, wsNormal, toCamera, specular, roughness, reflectionCubeColor);
        //reflectionCubeColor = SampleCubeLOD(ZoneCubeMap, float4(wsNormal, 9.6)).rgb;

       // float3 environmentSpecular = float3(0.1,0.1,0.1);

        //float3 environmentSpecular = EnvBRDFApprox(specular, roughness, ndv);

        return cube;
    }
#endif
