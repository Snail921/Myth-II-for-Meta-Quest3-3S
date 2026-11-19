#pragma once
static bool terrainDetailLod=false;
static constexpr float terrainDetailNear=20.f,terrainDetailFar=40.f;
static void loadTerrainOptions(const char*root){
 terrainDetailLod=false;char path[512];snprintf(path,sizeof(path),"%s/terrain-detail-lod.enabled",root);
 if(FILE*f=fopen(path,"rb")){terrainDetailLod=fgetc(f)=='1';fclose(f);}
 LOG("Terrain options detail_lod=%d fade=%.1f..%.1f backface_cull=1 reflected_winding=CW",terrainDetailLod,terrainDetailNear,terrainDetailFar);
}
// Terrain triangles have upward CCW normals after mythWorldX reverses X.
// The water reflection matrix reverses Y, hence its front winding is CW.
// drawScene enters with culling disabled; restore that state for two-sided art.
struct TerrainCullScope{
 explicit TerrainCullScope(bool reflected){glFrontFace(reflected?GL_CW:GL_CCW);glCullFace(GL_BACK);glEnable(GL_CULL_FACE);}
 ~TerrainCullScope(){glDisable(GL_CULL_FACE);glFrontFace(GL_CCW);}
};
