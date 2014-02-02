#ifndef _glut_main_h_
#define _glut_main_h_

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _glut_callbacks {
		int (*init)(void* userData);
		void (*display)(void* userData);
		void (*reshape)(int width, int height, void* userData);
		void (*idle)(void* userData);
		void (*keyboard)(
			unsigned char key, 
			int mouseX, 
			int mouseY, 
			void* userData);
		void (*special)(int key, int x, int y, void* userData);
		void (*cleanup)(void* userData);
	} glut_callbacks;

	int glut_main(
		int argc, 
		const char* argv[], 
		const char* title,
		const glut_callbacks* callbacks, 
		void* userData);

#ifdef __cplusplus
}
#endif
#endif

