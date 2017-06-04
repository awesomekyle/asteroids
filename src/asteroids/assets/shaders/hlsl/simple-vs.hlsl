float4 main(uint vertexId : SV_VertexID) : SV_POSITION
{
    float2 pos[3] = {
        float2(-0.7,  0.7),
        float2( 0.7,  0.7),
        float2( 0.0, -0.7),
    };
    return float4(pos[vertexId], 0.0f, 1.0f);
}
