/* uses eglut from mesa/demos for context setup
 *   git clone git://git.freedesktop.org/git/mesa/demos
 *
 * build
 *   libtool --mode=link gcc -g -O -lEGL -lGLESv2 -lm  -lX11 -o sam-tex-ext sam-tex-ext.c /home/varad/devel/mesa/demos/src/egl/eglut/libeglut_x11.la */

#define W 128
#define H 32

#define EGL_EGLEXT_PROTOTYPES

#include "gl-util.h"

EGLImageKHR (EGLAPIENTRY *CreateImageKHR)(EGLDisplay, EGLContext,
                                              EGLenum, EGLClientBuffer,
                                              const EGLint *);
EGLBoolean (EGLAPIENTRY *DestroyImageKHR)(EGLDisplay, EGLImageKHR);
void (EGLAPIENTRY *EGLImageTargetTexture2DOES)(GLenum, GLeglImageOES);

static const char vs_src[] =
	"attribute vec4 aVertex;\n"
	"attribute vec4 aTexCoords;\n"
	"varying vec2 TexCoords;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	TexCoords = aTexCoords.xy;\n"
	"	gl_Position = aVertex;\n"
"}\n";

static const char fs_src[] =
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"uniform sampler2D sampler;\n"
	"varying vec2 TexCoords;\n"
	"\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(sampler, TexCoords);\n"
	"}\n";

void draw() {
	GLfloat vertices[12] = {
				-1, -1, 0.0,
				 1, -1, 0.0,
				-1,  1, 0.0,
				 1,  1, 0.0
	};

	GLfloat texcoords[] = {
			1.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			0.0f, 0.0f,
	};

	GLfloat quad_color[] =  {1.0, 0.0, 0.0, 1.0};
	GLuint tex, texext;
	EGLImage *eglimage = NULL;
	GLenum error;
	int i, j;

	GLubyte nv12_img[W][H][4] = {0};
	for(i = 0; i < W; i++) {
		for(j = 0; j < H; j++) {
			nv12_img[i][j][0] = i;
			nv12_img[i][j][1] = j;
			nv12_img[i][j][2] = 0;
			nv12_img[i][j][3] = 0;
		}
	}

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H , 0, GL_RGBA, GL_UNSIGNED_BYTE, nv12_img);

	error = glGetError();

	CreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	eglimage = CreateImageKHR(eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(intptr_t)tex, NULL);
	/* we have an eglimage from tex data */

	/* create external tex from eglimage */
	glGenTextures(1, &texext);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texext);
	EGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
	EGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimage);

	error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\nglEGLImageTargetTexture2DOES failed %x\n", error);
	}

	/* setup shaders */
	GLuint program = get_program(vs_src, fs_src);

	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texext);
	glUniform1i(glGetUniformLocation(program, "sampler"), 0);
	glBindAttribLocation(program, 0, "aVertex");
	glBindAttribLocation(program, 1, "aTexCoords");
	link_program(program);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	/* read and verify */
	GLubyte *data = malloc(4 * W * H);
	int t;
	glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data);
	for(t = 0; t < 4 * W * H; t += 4) {
		// just dump everything for now
		printf("px%d: y %d u %d v %d\n", t/4, data[t], data[t + 1], data[t + 2]);
	}
	exit(0);
	//  glFlush();
}

int
main(int argc, char *argv[])
{
	eglutInitWindowSize(W, H);
	eglutInitAPIMask(EGLUT_OPENGL_ES2_BIT);
	eglutInit(argc, argv);
	eglutCreateWindow("sampler2DExternal-test");

	eglutDisplayFunc(draw);

	//  init();

	eglutMainLoop();

	return 0;
}
