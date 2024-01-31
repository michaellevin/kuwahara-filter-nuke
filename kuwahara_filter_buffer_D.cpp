kernel AnisotropicKuwaharaFilter : ImageComputationKernel<ePixelWise>
{
    Image<eRead, eAccessRandom, eEdgeClamped> src; // Input image
    Image<eWrite> dst; // Output image

    param:
        int radius; // Radius of the filter
        float q;
        float alpha;
        float scale;

    local:
        float width;
        float height;

    void define() {
        defineParam(radius, "Radius", 15); 
        defineParam(q, "Q", 12.0f); 
        defineParam(alpha, "alpha", 5.0f); 
        defineParam(scale, "scale", 1.0f); 
    }

    void init() {
        width = src.bounds.x2;
        height = src.bounds.y2;
    }

    void process(int2 pos) {
        SampleType(src) input = src(pos.x, pos.y);
        float2 uv = float2(pos.x / float(width), pos.y / float(height));

        float4 m[4];
        float4 s[4];
        
        for (int k = 0; k < 4; ++k) {
            m[k] = float4(0.0f, 0.0f, 0.0f, 0.0f);
            s[k] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        float piN = 2.0f * PI / 4.0f;
        
        
        float tw = 1.0f; // place texture reading here Alpha channel
        float a = radius * clamp((alpha + tw) / alpha, 0.1, 2.0); 
        float b = radius * clamp(alpha / (alpha + tw), 0.1, 2.0);

        float tz = 0.0f;
        float cos_phi = cos(tz); // 1
        float sin_phi = sin(tz); // 0

        float4 SR = float4(cos_phi/a, -sin_phi/b, 
                            sin_phi/a, cos_phi/b); 

        int max_x = int(sqrt(a*a * cos_phi*cos_phi +
                            b*b * sin_phi*sin_phi));
        int max_y = int(sqrt(a*a * sin_phi*sin_phi +
                            b*b * cos_phi*cos_phi));
        float real_scale = clamp(pow(scale,2), 0.05, 1.0); // TEST
        for (int j = 0; j <= max_y; ++j) {
            for (int i = -max_x; i <= max_x; ++i) {
                if ((j != 0) || (i > 0)) {
                    // float2 v = float2(float(i), float(j));
                    float2 v = float2(
                        SR[0]*float(i) + SR[1]*float(j),
                        SR[2]*float(i) + SR[3]*float(j));
                    float dot_v = dot(v, v);
                    if (dot_v <= real_scale) {
                        int2 coord0 = int2(uv.x * width + i, uv.y * height + j);
                        int2 coord1 = int2(uv.x * width - i, uv.y * height - j);

                        float4 c0_fix = src(coord0.x, coord0.y);
                        float3 c0 = float3(c0_fix.x, c0_fix.y, c0_fix.z);
                        float4 c1_fix = src(coord1.x, coord1.y);
                        float3 c1 = float3(c1_fix.x, c1_fix.y, c1_fix.z);

                        float3 cc0 = c0 * c0;
                        float3 cc1 = c1 * c1;

                        float n = 0.0f;
                        float wx[4];
                        
                        float z;
                        float xx = 0.33f - 0.84f * v.x * v.x;
                        float yy = 0.33f - 0.84f * v.y * v.y;

                        z = max(0.0f, v.y + xx);
                        n += wx[0] = z * z;

                        z = max(0.0f, -v.x + yy);
                        n += wx[1] = z * z;

                        z = max(0.0f, -v.y + xx);
                        n += wx[2] = z * z;

                        z = max(0.0f, v.x + yy);
                        n += wx[3] = z * z;
                        

                        float g = exp(-3.125f * dot_v) / n;
                        for (int k = 0; k < 4; ++k) {
                            float w = wx[k] * g;
                            m[k] += float4(c0.x * w, c0.y * w, c0.z * w, w);
                            // Extend cc0 and cc1 to float4 by adding a default fourth component, here I use 0.0f
                            s[k] += float4(cc0.x * w, cc0.y * w, cc0.z * w, 0.0f);
                            m[(k + 2) & 3] += float4(c1.x * w, c1.y * w, c1.z * w, w);
                            s[(k + 2) & 3] += float4(cc1.x * w, cc1.y * w, cc1.z * w, 0.0f);
                        }
                    }
                }
            }
        }
        
        float4 o = float4(0.0f, 0.0f, 0.0f, 0.0f);
        for (int k = 0; k < 4; ++k) {
            m[k].x /= m[k].w;
            m[k].y /= m[k].w;
            m[k].z /= m[k].w;

            float4 temp = s[k] / m[k].w - float4(m[k].x * m[k].x, m[k].y * m[k].y, m[k].z * m[k].z, 0.0f);
            s[k] = float4(fabs(temp.x), fabs(temp.y), fabs(temp.z), fabs(temp.w));
            
            float sigma2 = sqrt(s[k].x) + sqrt(s[k].y) + sqrt(s[k].z);
            float w =1.0f / (1.0f + pow(255.0f * sigma2, q)); 

            // Accumulate the result in o
            o += float4(m[k].x * w, m[k].y * w, m[k].z * w, w);
        }

        dst() = float4(o.x / o.w, o.y / o.w, o.z / o.w, 1.0f);
        // dst() = input;
    }

};
