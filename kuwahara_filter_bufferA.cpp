kernel AnisotropicKuwaharaFilterBufferA : ImageComputationKernel<ePixelWise>
{
    Image<eRead, eAccessRandom, eEdgeClamped> src; // Input image
    Image<eWrite> dst; // Output image

    local:
        float width;
        float height;

    void define() {
    }

    void init() {
        width = src.bounds.x2;
        height = src.bounds.y2;
    }

    float4 pick_color(float2 uv) {
        uv = clamp(uv, float2(0.0, 0.0), float2(1.0, 1.0));
        float2 coord = uv * float2(width - 1, height - 1);
        return src(coord.x, coord.y);
    }


    void process(int2 pos) {
        float2 uv = float2(pos.x / float(width), pos.y / float(height));
        float2 d = float2(1.0f / float(width), 1.0f / float(height)); 
        float4 u = (
            -1.0f * pick_color(uv + float2(-d.x, -d.y)) +
            -2.0f * pick_color(uv + float2(-d.x,  0.0)) +
            -1.0f * pick_color(uv + float2(-d.x,  d.y)) +
            +1.0f * pick_color(uv + float2( d.x, -d.y)) +
            +2.0f * pick_color(uv + float2( d.x,  0.0)) +
            +1.0f * pick_color(uv + float2( d.x,  d.y))
        ) / 4.0f;

        float4 v = (
         -1.0f * pick_color(uv + float2(-d.x, -d.y)) +
         -2.0f * pick_color(uv + float2( 0.0, -d.y)) +
         -1.0f * pick_color(uv + float2( d.x, -d.y)) +
         +1.0f * pick_color(uv + float2(-d.x,  d.y)) +
         +2.0f * pick_color(uv + float2( 0.0,  d.y)) +
         +1.0f * pick_color(uv + float2( d.x,  d.y))
         ) / 4.0f;


        float x = dot(float3(u.x, u.y, u.z), float3(u.x, u.y, u.z));
        float y = dot(float3(v.x, v.y, v.z), float3(v.x, v.y, v.z));
        float z = dot(float3(u.x, u.y, u.z), float3(v.x, v.y, v.z));

        dst() = float4(x, y, z , 1.0f);
    }
};
