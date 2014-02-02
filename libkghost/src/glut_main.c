#include "glut_main.h"
#include "log.h"
#include <GLUT/glut.h>
#include <stddef.h>

static const int _escape_key = 27;
static const int _full_screen_key = 6;
static const int _default_window_height = 480;
static const int _default_window_width = 640;

static glut_callbacks _callbacks = {0};
static void* _user_data = NULL;

static void _display();
static void _reshape(int width, int height);
static void _idle();
static void _keyboard(unsigned char key, int mouseX, int mouseY);
static void _special(int key, int x, int y);

int glut_main(
	int argc,
	const char* argv[],
	const char* title,
	const glut_callbacks* callbacks,
	void* userData)
{
	_callbacks = *callbacks;
	_user_data = userData;

	glutInit(&argc, (char **)argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(_default_window_width, _default_window_height);
	glutCreateWindow(title);

	glutDisplayFunc(&_display);
	glutIdleFunc(&_idle);
	glutReshapeFunc(&_reshape);
	glutKeyboardFunc(&_keyboard);
	glutSpecialFunc(&_special);

	if (_callbacks.init) {
		int initResult = _callbacks.init(_user_data);
		if (initResult) {
			LOG_WARNING("glut", NULL, "failed user init callback");
			return initResult;
		}
	}

	glutMainLoop();

	if (_callbacks.cleanup) {
		_callbacks.cleanup(_user_data);
	}
	 	
	return 0;
}

void _display() {
	if (_callbacks.display) {
		_callbacks.display(_user_data);
	}
}

void _reshape(int width, int height) {
	if (_callbacks.reshape) {
		_callbacks.reshape(width, height, _user_data);
	}
}

void _idle() {
	if (_callbacks.idle) {
		_callbacks.idle(_user_data);
	}
}

void _keyboard(unsigned char key, int mouseX, int mouseY) {
	if (key == _escape_key) {
		int initialWidth = glutGet(GLUT_INIT_WINDOW_WIDTH);
		int initialHeight = glutGet(GLUT_INIT_WINDOW_HEIGHT);
		if ((initialWidth < 1) || (initialHeight < 1)) {
			initialWidth = _default_window_width;
			initialHeight = _default_window_height;
		}
		glutReshapeWindow(initialWidth, initialHeight);
	}
	else if (key == _full_screen_key) {
		glutFullScreen();
	}
	if (_callbacks.keyboard) {
		_callbacks.keyboard(key, mouseX, mouseY, _user_data);
	}
}

void _special(int key, int x, int y) {
	if (_callbacks.special) {
		_callbacks.special(key, x, y, _user_data);
	}
}

