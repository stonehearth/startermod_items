//-------------------------------------------------------------------------
float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total, sampler2D depthbuff, sampler2D ssaobuff)
{
    float c = texture2D(ssaobuff, uv).r;
    float d = texture2D(depthbuff, uv).r;
    float g_BlurFalloff = 0.2;
    float g_Sharpness = 0.0;

    float ddiff = d - center_d;
    float w = exp(-r*r*g_BlurFalloff - ddiff*ddiff*g_Sharpness);
    w_total += w;

    return w * c; 
}
