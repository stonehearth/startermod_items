
local pipelines = {}

function setup()
   pipeline:setup([[
      <Setup>
         <RenderTarget id="GBUFFER" depthBuf="true" numColBufs="4" format="RGBA16F" scale="1.0"/>
         <RenderTarget id="MBUFF" depthBuf="true" numColBufs="1" format="RGBA16F" scale="1.0"/>
         <RenderTarget id="BLURBUF1" depthBuf="false" numColBufs="1" format="RGBA8" scale="0.5"/>
         <RenderTarget id="BLURBUF2" depthBuf="false" numColBufs="1" format="RGBA8" scale="0.5"/>
         <RenderTarget id="FINALIMAGE" depthBuf="false" numColBufs="1" format="RGBA16F" scale="1.0"/>
         <RenderTarget id="OUTLINE" depthBuf="false" numColBufs="1" format="RGBA8" scale="1.0"/>
         <RenderTarget id="SSAOBUF" depthBuf="false" numColBufs="1" format="RGBA8" scale="0.5" />
         <RenderTarget id="SSAOBLUR" depthBuf="false" numColBufs="1" format="RGBA8" scale="1.0" />
         <RenderTarget id="TEMPSSAOBLUR" depthBuf="false" numColBufs="1" format="RGBA8" scale="1.0" />
      </Setup>
   ]])

   render_gbuff_attributes = pipeline:compile_stage([[
      <Stage id="Attribpass">
         <SwitchTarget target="GBUFFER" />
         <ClearTarget depthBuf="true" colBuf0="true" />
         <DrawGeometry context="ATTRIBPASS" class="~Translucent"/>
      </Stage>
   ]])
   
   deferred_light_loop = pipeline:compile_stage([[
      <Stage id="Lighting" link="pipelines/globalSettings.material.xml">
         <SwitchTarget target="MBUFF" />

         <!-- Copy depth buffer to allow occlusion culling of lights
              For some reason, we need to clear the depth buffer of the MBUFF before copying
              the contents of the GBUFFER depth texture.  Setting ZEnable to false in the
              COPY_DEPTH lighting material doesn't help.  Ideally, we would just clober the
              MBUFF depth buffer unconditionally. -->
              
         <ClearTarget depthBuf="true"/>
         <BindBuffer sampler="depthBuf" sourceRT="GBUFFER" bufIndex="32" />
         <DrawQuad material="materials/light.material.xml" context="COPY_DEPTH" />
         <UnbindBuffers />

         <BindBuffer sampler="gbuf0" sourceRT="GBUFFER" bufIndex="0" />
         <BindBuffer sampler="gbuf1" sourceRT="GBUFFER" bufIndex="1" />
         <BindBuffer sampler="gbuf2" sourceRT="GBUFFER" bufIndex="2" />
         <BindBuffer sampler="gbuf3" sourceRT="GBUFFER" bufIndex="3" />

         <DrawQuad material="materials/light.material.xml" context="AMBIENT" />
         <DoDeferredLightLoop class="~Translucent"/>

         <UnbindBuffers />
      </Stage>
   ]])
   
   render_translucent = pipeline:compile_stage([[
      <Stage id="Translucent">
         <SwitchTarget target="MBUFF" />
         <DrawGeometry context="TRANSLUCENT"/>
         <DrawGeometry context="DEBUG_SHAPES" />
      </Stage>
   ]])
   
   render_selected = pipeline:compile_stage([[
      <Stage id="Selected">
         <DrawSelected context="SELECTED" />
      </Stage>   
   ]])
   
   ssao = pipeline:compile_stage([[
      <Stage id="SSAO">
         <SwitchTarget target="SSAOBUF" />
         <BindBuffer sampler="gbuf0" sourceRT="GBUFFER" bufIndex="0" />
         <BindBuffer sampler="gbuf1" sourceRT="GBUFFER" bufIndex="1" />
         <DrawQuad material="pipelines/ssao.material.xml" context="SSAO"/>
         <UnbindBuffers />
      </Stage>
   ]])
   
   blur_ssao = pipeline:compile_stage([[
      <Stage id="blurSSAO" >
         <SwitchTarget target="TEMPSSAOBLUR" />
         <BindBuffer sampler="buf0" sourceRT="SSAOBUF" bufIndex="0" />
         <DrawQuad material="pipelines/ssao.material.xml" context="VERTICALBLUR" />
         <UnbindBuffers />
         <SwitchTarget target="SSAOBLUR" />
         <BindBuffer sampler="buf0" sourceRT="TEMPSSAOBLUR" bufIndex="0" />
         <DrawQuad material="pipelines/ssao.material.xml" context="HORIZONTALBLUR" />
         <UnbindBuffers />
      </Stage>
   ]])
   
   brightpass = pipeline:compile_stage([[
      <Stage id="BrightPass">
         <SwitchTarget target="BLURBUF1" />
         <BindBuffer sampler="buf0" sourceRT="MBUFF" bufIndex="0" />
         <DrawQuad material="pipelines/postHDR.material.xml" context="BRIGHTPASS" />
         <UnbindBuffers />
      </Stage>
   ]])

   bloom = pipeline:compile_stage([[
      <Stage id="Bloom">
         <SwitchTarget target="BLURBUF2" />
         <BindBuffer sampler="buf0" sourceRT="BLURBUF1" bufIndex="0" />
         <SetUniform material="pipelines/postHDR.material.xml" uniform="blurParams" a="0.0" />
         <DrawQuad material="pipelines/postHDR.material.xml" context="BLUR" />
         
         <SwitchTarget target="BLURBUF1" />
         <BindBuffer sampler="buf0" sourceRT="BLURBUF2" bufIndex="0" />
         <SetUniform material="pipelines/postHDR.material.xml" uniform="blurParams" a="1.0" />
         <DrawQuad material="pipelines/postHDR.material.xml" context="BLUR" />
         
         <SwitchTarget target="BLURBUF2" />
         <BindBuffer sampler="buf0" sourceRT="BLURBUF1" bufIndex="0" />
         <SetUniform material="pipelines/postHDR.material.xml" uniform="blurParams" a="2.0" />
         <DrawQuad material="pipelines/postHDR.material.xml" context="BLUR" />
         
         <SwitchTarget target="BLURBUF1" />
         <BindBuffer sampler="buf0" sourceRT="BLURBUF2" bufIndex="0" />
         <SetUniform material="pipelines/postHDR.material.xml" uniform="blurParams" a="3.0" />
         <DrawQuad material="pipelines/postHDR.material.xml" context="BLUR" />
         <UnbindBuffers />
      </Stage>
   ]])

   combination = pipeline:compile_stage([[
      <Stage id="Combination">
         <SwitchTarget target="FINALIMAGE" />
         <ClearTarget colBuf0="true" />

         <BindBuffer sampler="depthBuf" sourceRT="GBUFFER" bufIndex="32" />
         <BindBuffer sampler="buf0" sourceRT="MBUFF" bufIndex="0" />
         <BindBuffer sampler="buf1" sourceRT="BLURBUF1" bufIndex="0" />
         <BindBuffer sampler="buf2" sourceRT="OUTLINE" bufIndex="0" />
         <BindBuffer sampler="buf3" sourceRT="SSAOBLUR" bufIndex="0" />
         <DrawQuad material="pipelines/outline.material.xml" context="FINALPASS" />
         <UnbindBuffers />
      </Stage>
   ]])
   
   fxaa = pipeline:compile_stage([[
      <Stage id="FXAA">
         <SwitchTarget target=""/>
         <BindBuffer sampler="buf0" sourceRT="FINALIMAGE" bufIndex="0" />
         <DrawQuad material="pipelines/outline.material.xml" context="FXAA" />
         <UnbindBuffers />
      </Stage>
   ]])
   
   render_overlays = pipeline:compile_stage([[
      <Stage id="Overlays">
         <DrawOverlays context="OVERLAY" />
      </Stage>   
   ]])
   
   debug_render_shadowmap = pipeline:compile_stage([[
      <Stage id="RenderShadowmap">
         <SwitchTarget target=""/>
         <BindBuffer sampler="buf0" sourceRT="shadowMap" bufIndex="32" />
         <DrawQuad material="pipelines/outline.material.xml" context="DRAW_SHADOWMAP" />
         <UnbindBuffers />
      </Stage>
   ]])
   
   debug_render_ssao = pipeline:compile_stage([[
      <Stage id="RenderSSAO">
         <SwitchTarget target=""/>
         <BindBuffer sampler="buf0" sourceRT="SSAOBUF" bufIndex="0" />
         <DrawQuad material="pipelines/outline.material.xml" context="DRAW_COLORBUF" />
         <UnbindBuffers />
      </Stage>
   ]])
   

   debug_clear_gbuffer = pipeline:compile_stage([[
      <Stage id="ClearGbuffer">
         <SwitchTarget target="GBUFFER" />
         <ClearTarget depthBuf="true" colBuf0="true" colBuf1="true" colBuf2="true" colBuf3="true" />
      </Stage>
   ]])
   
   
   debug_render_normals = pipeline:compile_stage([[
      <Stage id="RenderNormals">
         <SwitchTarget target=""/>
         <BindBuffer sampler="gbuf0" sourceRT="GBUFFER" bufIndex="0" />
         <BindBuffer sampler="gbuf1" sourceRT="GBUFFER" bufIndex="1" />
         <BindBuffer sampler="gbuf2" sourceRT="GBUFFER" bufIndex="2" />
         <BindBuffer sampler="gbuf3" sourceRT="GBUFFER" bufIndex="3" />
         <DrawQuad material="pipelines/outline.material.xml" context="DRAW_NORMALS" />
         <UnbindBuffers />
      </Stage>
   ]])
   

   pipelines.debug_render_last_shadowmap =  {
      render_gbuff_attributes,
      deferred_light_loop,
      debug_render_shadowmap
   }

   pipelines.debug_render_ssao_buff = {
      render_gbuff_attributes,
      ssao,
      debug_render_ssao,
      render_overlays      
   }

   pipelines.debug_render_normals = {
      debug_clear_gbuffer,
      render_gbuff_attributes,
      debug_render_normals,
      render_overlays      
   }

   pipelines.render_deferred = {
      render_gbuff_attributes,
      deferred_light_loop,
      render_translucent,
      render_selected,
      --brightpass,
      --bloom,
      ssao,
      blur_ssao,
      combination,
      fxaa,
      render_overlays
   }
   
end

function get_stages()
   return pipelines.render_deferred -- the game pipeline
   
   --return pipelines.debug_render_last_shadowmap -- useful for shadomap debugging of the LAST light to get rendered
   --return pipelines.debug_render_ssao_buff
   --return pipelines.debug_render_normals
end
print("exiting !")
