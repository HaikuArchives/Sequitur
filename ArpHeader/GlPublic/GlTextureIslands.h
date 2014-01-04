#ifndef GLPUBLIC_GLTEXTUREISLANDS_H
#define GLPUBLIC_GLTEXTUREISLANDS_H

#include <support/SupportDefs.h>
class GlImage;
class _GlIslandData;

/*******************************************************
 * GL-TEXTURE-ISLANDS
 * Store a series of islands, which are essentially
 * little images a client can piece together like a
 * jigsaw.
 *******************************************************/
class GlTextureIslands
{
public:
	GlTextureIslands();
	GlTextureIslands(	GlImage* image, int32 initRange, int32 connectedRange,
						int32 separatedRange, int32 minW, int32 minH);
	virtual ~GlTextureIslands();

	status_t			InitCheck() const;
	uint32				Size() const;
	/* Answer the island limits -- the x from the island with
	 * the smallest width, the y from the island with the smallest height,
	 * and the same values from the largest islands.
	 */
	void				GetIslandLimits(int32* minX, int32* minY,
										int32* maxX, int32* maxY) const;
	const uint8*		GetIsland(uint32 index, int32* w, int32* h) const;
	/* I do not take ownership of the island. nor do I copy it.
	 */
	virtual status_t	AddIsland(const uint8* island, int32 w, int32 h);

protected:
	status_t			mStatus;
	_GlIslandData*		mData;
};

/*******************************************************
 * Predefined textures
 *******************************************************/
GlTextureIslands*		gl_new_fern_moss_texture();
GlTextureIslands*		gl_new_hair_cap_moss_texture();
GlTextureIslands*		gl_new_saucerman_texture();

#endif
