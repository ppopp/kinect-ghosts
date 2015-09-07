#include "gl_texture.h"

#include "log.h"

#include <OpenGL/gl.h>
#include <stddef.h>

typedef struct _texture_option {
	struct _texture_unit unit;
	int claimed;
} texture_option;

static texture_option _texture_options[] = {
	{{0, GL_TEXTURE0}, 0},
	{{1, GL_TEXTURE1}, 0},
	{{2, GL_TEXTURE2}, 0},
	{{3, GL_TEXTURE3}, 0},
	{{4, GL_TEXTURE4}, 0},
	{{5, GL_TEXTURE5}, 0},
	{{6, GL_TEXTURE6}, 0},
	{{7, GL_TEXTURE7}, 0},
	{{8, GL_TEXTURE8}, 0},
	{{9, GL_TEXTURE9}, 0},
	{{10, GL_TEXTURE10}, 0},
	{{11, GL_TEXTURE11}, 0},
	{{12, GL_TEXTURE12}, 0},
	{{13, GL_TEXTURE13}, 0},
	{{14, GL_TEXTURE14}, 0},
	{{15, GL_TEXTURE15}, 0},
	{{16, GL_TEXTURE16}, 0},
	{{17, GL_TEXTURE17}, 0},
	{{18, GL_TEXTURE18}, 0},
	{{19, GL_TEXTURE19}, 0},
	{{20, GL_TEXTURE20}, 0},
	{{21, GL_TEXTURE21}, 0},
	{{22, GL_TEXTURE22}, 0},
	{{23, GL_TEXTURE23}, 0},
	{{24, GL_TEXTURE24}, 0},
	{{25, GL_TEXTURE25}, 0},
	{{26, GL_TEXTURE26}, 0},
	{{27, GL_TEXTURE27}, 0},
	{{28, GL_TEXTURE28}, 0},
	{{29, GL_TEXTURE29}, 0},
	/*
	{30, GL_TEXTURE30, 0},
	{31, GL_TEXTURE31, 0},
	{32, GL_TEXTURE32, 0},
	{33, GL_TEXTURE33, 0},
	{34, GL_TEXTURE34, 0},
	{35, GL_TEXTURE35, 0},
	{36, GL_TEXTURE36, 0},
	{37, GL_TEXTURE37, 0},
	{38, GL_TEXTURE38, 0},
	{39, GL_TEXTURE39, 0},
	{40, GL_TEXTURE40, 0},
	{41, GL_TEXTURE41, 0},
	{42, GL_TEXTURE42, 0},
	{43, GL_TEXTURE43, 0},
	{44, GL_TEXTURE44, 0},
	{45, GL_TEXTURE45, 0},
	{46, GL_TEXTURE46, 0},
	{47, GL_TEXTURE47, 0},
	{48, GL_TEXTURE48, 0},
	{49, GL_TEXTURE49, 0},
	{50, GL_TEXTURE50, 0},
	{51, GL_TEXTURE51, 0},
	{52, GL_TEXTURE52, 0},
	{53, GL_TEXTURE53, 0},
	{54, GL_TEXTURE54, 0},
	{55, GL_TEXTURE55, 0},
	{56, GL_TEXTURE56, 0},
	{57, GL_TEXTURE57, 0},
	{58, GL_TEXTURE58, 0},
	{59, GL_TEXTURE59, 0},
	{60, GL_TEXTURE60, 0},
	{61, GL_TEXTURE61, 0},
	{62, GL_TEXTURE62, 0},
	{63, GL_TEXTURE63, 0},
	{64, GL_TEXTURE64, 0},
	{65, GL_TEXTURE65, 0},
	{66, GL_TEXTURE66, 0},
	{67, GL_TEXTURE67, 0},
	{68, GL_TEXTURE68, 0},
	{69, GL_TEXTURE69, 0},
	{70, GL_TEXTURE70, 0},
	{71, GL_TEXTURE71, 0},
	{72, GL_TEXTURE72, 0},
	{73, GL_TEXTURE73, 0},
	{74, GL_TEXTURE74, 0},
	{75, GL_TEXTURE75, 0},
	{76, GL_TEXTURE76, 0},
	{77, GL_TEXTURE77, 0},
	{78, GL_TEXTURE78, 0},
	{79, GL_TEXTURE79, 0}
	*/
};

const size_t _num_texture_options = sizeof(_texture_options) / sizeof(texture_option);

const texture_unit* gl_texture_claim() {
	size_t i = 0;
	for (i = 0; i < _num_texture_options; i++) {
		if (_texture_options[i].claimed == 0) {
			_texture_options[i].claimed = 1;
			return (texture_unit*)&_texture_options[i];
		}
	}
	LOG_WARNING("failed to claim texture");
	return NULL;
}

void gl_texture_release(const texture_unit* tunit) {
	size_t i = 0;

	if (tunit == NULL) {
		LOG_WARNING("null texture unit released");
	}
	
	for (i = 0; i < _num_texture_options; i++) {
		if (_texture_options[i].unit.textureEnum == tunit->textureEnum) {
			_texture_options[i].claimed = 0;
			return;
		}
	}
	LOG_DEBUG("released texture never claimed");
}

