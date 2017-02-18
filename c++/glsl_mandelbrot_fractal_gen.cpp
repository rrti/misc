#include <GL/glew.h>
//#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <GL/gl.h>
#include <SDL/SDL.h>

#include <cassert>
#include <fstream>


#define USE_MOUSELOOK  false
#define WINDOW_SIZE_X   1200
#define WINDOW_SIZE_Y    750
#define STRINGIFY(s)      #s

// "glXGetProcAddressARB was not declared in this scope"
#define glXGetProcAddr(x) (*glXGetProcAddress) ((const GLubyte*) x)



static const char* mandelbrot_vert_shader_text =
	STRINGIFY(void main(void) {)
	STRINGIFY(
		 // we must replace this part of the fixed pipeline, so
		 // just copy the coors set via the glTexCoord() calls
	)
	STRINGIFY(	gl_TexCoord[0] = gl_MultiTexCoord0;)
	STRINGIFY(	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;)
	STRINGIFY(});

static const char* mandelbrot_frag_shader_text =
	STRINGIFY(uniform   int max_iter;)
	STRINGIFY(uniform float zoom_lvl;)
	STRINGIFY(uniform float xcenter;)
	STRINGIFY(uniform float ycenter;)

	STRINGIFY(
		// region of the complex-plane we visit
	)
	STRINGIFY(const float w = 4.0;)
	STRINGIFY(const float h = 2.0;)

	STRINGIFY(const vec3 black = vec3(0.0, 0.0, 0.0);)

	STRINGIFY(void main(void) {)
	STRINGIFY(	vec2 pixel = gl_TexCoord[0].xy;)
	STRINGIFY(	vec3 color;)

	STRINGIFY(	float yrel = pixel.y;)
	STRINGIFY(	float yabs = (yrel * h) - 1.0;)
	STRINGIFY(	float xrel = pixel.x;)
	STRINGIFY(	float xabs = (xrel * w) - 2.5;)
	STRINGIFY(	float yabsz = yabs * zoom_lvl;)
	STRINGIFY(	float xabsz = xabs * zoom_lvl;)

	STRINGIFY(	float real = xabsz + xcenter;)
	STRINGIFY(	float imag = yabsz + ycenter;)
	STRINGIFY(	float creal = real;)
	STRINGIFY(	float cimag = imag;)

	STRINGIFY(	float rsq = 0.0;)
	STRINGIFY(	float p = sqrt(((real - 0.25) * (real - 0.25)) + (imag * imag));)

	STRINGIFY(	int iter = 0;)

	STRINGIFY(	if (real <= (p - (2.0 * p * p) + 0.25)) {)
	STRINGIFY(		color = black;)
	STRINGIFY(	} else if (((real + 1.0) * (real + 1.0) + (imag * imag)) <= 0.0625) {)
	STRINGIFY(		color = black;)
	STRINGIFY(	} else {)
	STRINGIFY(		for (iter = max_iter; iter > 0 && rsq < 4.0; --iter) {)
	STRINGIFY(			float t = real;)

	STRINGIFY(			real = (t * t) - (imag * imag) + creal;)
	STRINGIFY(			imag = 2.0 * t * imag + cimag;)
	STRINGIFY(			rsq  = (real * real) + (imag * imag);)
	STRINGIFY(		})

	STRINGIFY(		if (rsq < 4.0) {)
	STRINGIFY(			color = black;)
	STRINGIFY(		} else {)
	STRINGIFY(
						// {x,y}rel are in [0.0, 1.0]
						// {x,y}rel * c are in [0.0, max_iter * 1.0]
						//
						// c is in [0, max_iter]
						// d is in [0.0, max_iter * 0.01]
						//
						// pixels that need more iterations to escape should be brighter
	)
	STRINGIFY(			float c = float(max_iter - iter);       )
	STRINGIFY(			float d = float(max_iter       ) * 0.01;)

	STRINGIFY(			color.r = min(1.0, (xrel * c) / d);)
	STRINGIFY(			color.b = min(1.0, (yrel * c) / d);)
	STRINGIFY(			color.g = 0.0;)

	STRINGIFY(
						// color.r = min(1.0, log((xrel * c) * (xrel * c)) / log(float(max_iter * max_iter)));
						// color.b = min(1.0, log((yrel * c) * (yrel * c)) / log(float(max_iter * max_iter)));
	)
	STRINGIFY(		})
	STRINGIFY(	})

	STRINGIFY(	gl_FragColor = vec4(color, 1.0);)
	STRINGIFY(});





/*
// ARB functions
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC			glCompileShaderARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB = NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObjectARB = NULL;
PFNGLDETACHOBJECTARBPROC			glDetachObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB = NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLogARB = NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB = NULL;
PFNGLUNIFORM1FARBPROC				glUniform1fARB = NULL;
PFNGLUNIFORM1IARBPROC				glUniform1iARB = NULL;

// OpenGL 2.0 core functions
PFNGLCREATEPROGRAMPROC				glCreateProgram = NULL;
PFNGLCREATESHADERPROC				glCreateShader = NULL;
PFNGLSHADERSOURCEPROC				glShaderSource = NULL;
PFNGLCOMPILESHADERPROC				glCompileShader = NULL;
// PFNGLGETOBJECTPARAMETERIVPROC	glGetObjectParameteriv = NULL;
PFNGLGETSHADERIVPROC				glGetShaderiv = NULL;
PFNGLGETPROGRAMIVPROC				glGetProgramiv = NULL;
PFNGLATTACHSHADERPROC				glAttachShader = NULL;
PFNGLDETACHSHADERPROC				glDetachShader = NULL;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMINFOLOGPROC			glGetProgramInfoLog = NULL;
PFNGLLINKPROGRAMPROC				glLinkProgram = NULL;
PFNGLUSEPROGRAMPROC					glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC			glValidateProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC			glGetUniformLocation = NULL;
PFNGLGETATTRIBLOCATIONPROC			glGetAttribLocation = NULL;
PFNGLUNIFORM1FPROC					glUniform1f = NULL;
PFNGLUNIFORM1IPROC					glUniform1i = NULL;
PFNGLVERTEXATTRIB1FPROC				glVertexAttrib1f = NULL;
PFNGLDELETESHADERPROC				glDeleteShader = NULL;
PFNGLDELETEPROGRAMPROC				glDeleteProgram = NULL;
*/






struct t_runtime_data {
	bool is_running;
	bool have_opengl_2;
};

struct t_timing_data {
	unsigned int frames_rendered;
	unsigned int secs_elapsed;
	unsigned int msecs_elapsed;
	unsigned int last_key_press_time;
};

struct t_shader_data {
	// shader handle and uniform indices
	GLuint id;
	GLint uniform_locs[4];
};
struct t_uniform_data {
	// shader uniform values
	int max_iter;
	float zoom_lvl;
	float xcenter;
	float ycenter;
};

struct t_render_data {
	float xmin, xmax, xoff;
	float ymin, ymax, yoff;

	float zdist;
	float scale;
};



static t_runtime_data runtime_data;
static t_timing_data timing_data;
static t_shader_data shader_data;
static t_uniform_data uniform_data;
static t_render_data render_data;



class t_extension_loader {
public:
	static bool load_extensions() {
		const char* gl_version  = ((const char*) glGetString(GL_VERSION));
		const char* gl_vendor   = ((const char*) glGetString(GL_VENDOR));
		const char* gl_renderer = ((const char*) glGetString(GL_RENDERER));

		printf("[%s]\n", __FUNCTION__);
		printf("\tGL version:  %s\n", gl_version);
		printf("\tGL vendor:   %s\n", gl_vendor);
		printf("\tGL renderer: %s\n", gl_renderer);

		// back when this code was written, OGL2 was still a novelty
		// and driver support for it very limited --> all extensions
		// (which are now core functions) had to be loaded manually
		// and (ideally) a fallback render path provided
		// these days it is totally unnecessary and we can use GLEW
		return true;

		if ((runtime_data.have_opengl_2 = (gl_version[0] >= '2'))) {
			glCreateProgram = (PFNGLCREATEPROGRAMPROC) glXGetProcAddr("glCreateProgram");
			glCreateShader = (PFNGLCREATESHADERPROC) glXGetProcAddr("glCreateShader");
			glShaderSource = (PFNGLSHADERSOURCEPROC) glXGetProcAddr("glShaderSource");
			glCompileShader = (PFNGLCOMPILESHADERPROC) glXGetProcAddr("glCompileShader");
			glGetShaderiv = (PFNGLGETSHADERIVPROC) glXGetProcAddr("glGetShaderiv");
			glGetProgramiv = (PFNGLGETPROGRAMIVPROC) glXGetProcAddr("glGetProgramiv");
			glAttachShader = (PFNGLATTACHSHADERPROC) glXGetProcAddr("glAttachShader");
			glDetachShader = (PFNGLDETACHSHADERPROC) glXGetProcAddr("glDetachShader");
			glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) glXGetProcAddr("glGetShaderInfoLog");
			glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) glXGetProcAddr("glGetProgramInfoLog");
			glLinkProgram = (PFNGLLINKPROGRAMPROC) glXGetProcAddr("glLinkProgram");
			glUseProgram = (PFNGLUSEPROGRAMPROC) glXGetProcAddr("glUseProgram");
			glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) glXGetProcAddr("glValidateProgram");
			glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) glXGetProcAddr("glGetUniformLocation");
			glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) glXGetProcAddr("glGetAttribLocation");
			glUniform1f = (PFNGLUNIFORM1FPROC) glXGetProcAddr("glUniform1f");
			glUniform1i = (PFNGLUNIFORM1IPROC) glXGetProcAddr("glUniform1i");
			glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) glXGetProcAddr("glVertexAttrib1f");
			glDeleteShader = (PFNGLDELETESHADERPROC) glXGetProcAddr("glDeleteShader");
			glDeleteProgram = (PFNGLDELETEPROGRAMPROC) glXGetProcAddr("glDeleteProgram");
		} else {
			glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) glXGetProcAddr("glCreateProgramObjectARB");
			glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) glXGetProcAddr("glCreateShaderObjectARB");
			glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) glXGetProcAddr("glShaderSourceARB");
			glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) glXGetProcAddr("glCompileShaderARB");
			glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) glXGetProcAddr("glGetObjectParameterivARB");
			glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) glXGetProcAddr("glAttachObjectARB");
			glDetachObjectARB = (PFNGLDETACHOBJECTARBPROC) glXGetProcAddr("glDetachObjectARB");
			glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) glXGetProcAddr("glDeleteObjectARB");
			glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) glXGetProcAddr("glGetInfoLogARB");
			glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) glXGetProcAddr("glLinkProgramARB");
			glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) glXGetProcAddr("glUseProgramObjectARB");
			glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) glXGetProcAddr("glGetUniformLocationARB");
			glUniform1fARB = (PFNGLUNIFORM1FARBPROC) glXGetProcAddr("glUniform1fARB");
			glUniform1iARB = (PFNGLUNIFORM1IARBPROC) glXGetProcAddr("glUniform1iARB");
		}

		return (runtime_data.have_opengl_2);
	}
};

class t_shader_loader {
public:
	static int create_program_object() { return (glCreateProgram()); }
	static void delete_program_object(int program_object) { glDeleteShader(program_object); }

	static void finalize_program_object(int program_object) {
		char program_log[65536] = {0};
		int program_log_len = 0;

		GLint program_object_linked = -1;
		GLint program_object_valid = -1;

		glLinkProgram(program_object);
		glValidateProgram(program_object);

		glGetProgramiv(program_object, GL_LINK_STATUS, &program_object_linked);
		glGetProgramiv(program_object, GL_VALIDATE_STATUS, &program_object_valid);
		glGetProgramInfoLog(program_object, 65536, &program_log_len, program_log);
		// ARB-style
		//   glGetObjectParameterivARB(program_object, GL_OBJECT_LINK_STATUS_ARB, &program_object_linked);
		//   glUseProgramObjectARB(program_object);

		printf("[%s]\n", __FUNCTION__);
		printf("\tprogram object:        %d\n", program_object);
		printf("\tprogram-object linked: %d\n", program_object_linked);
		printf("\tprogram-object valid:  %d\n", program_object_valid);

		show_log(program_log, program_log_len, "program object");
	}

	static std::string load_shader_program_from_file(const std::string& fname) {
		std::ifstream stream(fname.c_str(), std::ios::in);
		std::string line, ptext;

		while (!stream.eof()) {
			std::getline(stream, line);
			ptext += line;
			ptext += '\n';
		}

		stream.close();
		return ptext;
	}

	static int get_shader_program_type(const std::string& fname) {
		int type = -1;

		// if file extension is ".glsl", user has to specify this
		if (fname.find(".fp") != std::string::npos) { type = GL_FRAGMENT_SHADER; }
		if (fname.find(".vp") != std::string::npos) { type = GL_VERTEX_SHADER; }

		return type;
	}

	static int prepare_shader(const std::string& str, int program_object, int program_type, bool from_file) {
		if (str.empty())
			return 0;

		std::string shader_name = ((from_file)? str: "[UNNAMED]");
		std::string shader_text;

		if (from_file) {
			// interpret <str> as the shader's filename
			shader_text = load_shader_program_from_file(str);
	
			if (shader_text.empty()) {
				return 0;
			}
		} else {
			// interpret <str> as the shader's sourcecode
			shader_text = str;
		}

		const char* shader_src = shader_text.c_str();
		char shader_log[65536] = {0};
		int shader_log_len = 0;

		GLint shader_compiled = -1;
		GLuint shader_object = glCreateShader(program_type);

		glShaderSource(shader_object, 1, &shader_src, NULL);
		glCompileShader(shader_object);
		glAttachShader(program_object, shader_object);
		glGetShaderiv(shader_object, GL_COMPILE_STATUS, &shader_compiled);
		glGetShaderInfoLog(shader_object, 65536, &shader_log_len, shader_log);
		// ARB-style
		//   glGetObjectParameterivARB(shader_object, GL_OBJECT_COMPILE_STATUS_ARB, &shader_compiled);
		//   glUseProgramObjectARB(shader_object);

		printf("[%s]\n", __FUNCTION__);
		printf("\tshader name:     %s\n", shader_name.c_str());
		printf("\tshader object:   %d\n", shader_object);
		printf("\tshader compiled: %d\n", shader_compiled);

		show_log(shader_log, shader_log_len, shader_name);
		return shader_object;
	}

	static void show_log(const char* log, int len, const std::string& sname) {
		if (len <= 0)
			return;

		printf("[%s] compile-log for %s (length %d):\n", __FUNCTION__, sname.c_str(), len);
		printf("%s\n", log);
	}
};







void prepare_shader_data(t_shader_data& sd, t_uniform_data& ud) {
	const GLuint program_object = t_shader_loader::create_program_object();

	#if 1
	t_shader_loader::prepare_shader(mandelbrot_vert_shader_text, program_object, GL_VERTEX_SHADER, false);
	t_shader_loader::prepare_shader(mandelbrot_frag_shader_text, program_object, GL_FRAGMENT_SHADER, false);
	#else
	t_shader_loader::prepare_shader("./shaders/mandelbrot_vert_shader_glsl", program_object, GL_VERTEX_SHADER, true);
	t_shader_loader::prepare_shader("./shaders/mandelbrot_frag_shader.glsl", program_object, GL_FRAGMENT_SHADER, true);
	#endif

	t_shader_loader::finalize_program_object(program_object);

	sd.id = program_object;
	sd.uniform_locs[0] = glGetUniformLocation(program_object, "max_iter");
	sd.uniform_locs[1] = glGetUniformLocation(program_object, "zoom_lvl");
	sd.uniform_locs[2] = glGetUniformLocation(program_object, "xcenter");
	sd.uniform_locs[3] = glGetUniformLocation(program_object, "ycenter");

	ud.max_iter = 2048;
	ud.zoom_lvl = 1.0f;
}

void prepare_render_data(t_render_data& rd) {
	rd.xmin  =  -2.5f; // vertex x-coors: [-2.5*scale, 1.5*scale]
	rd.xmax  =   1.5f; // vertex x-coors: [-2.5*scale, 1.5*scale]
	rd.xoff  =   0.5f;
	rd.ymin  =  -1.0f; // vertex y-coors: [-1.0*scale, 1.0*scale]
	rd.ymax  =   1.0f; // vertex y-coors: [-1.0*scale, 1.0*scale]
	rd.yoff  =   0.0f;

	rd.zdist =  25.0f;
	rd.scale =  10.0f;
}



void draw_shader_quad() {
	glUseProgram(shader_data.id);
		glUniform1i(shader_data.uniform_locs[0], uniform_data.max_iter);
		glUniform1f(shader_data.uniform_locs[1], uniform_data.zoom_lvl);
		glUniform1f(shader_data.uniform_locs[2], uniform_data.xcenter);
		glUniform1f(shader_data.uniform_locs[3], uniform_data.ycenter);

		glPushMatrix();
		glBegin(GL_QUADS);
			glNormal3f(0.0f, 0.0f, 1.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f((render_data.xoff + render_data.xmin) * render_data.scale, (render_data.yoff + render_data.ymin) * render_data.scale, 0.0f);
			glNormal3f(0.0f, 0.0f, 1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f((render_data.xoff + render_data.xmin) * render_data.scale, (render_data.yoff + render_data.ymax) * render_data.scale, 0.0f);
			glNormal3f(0.0f, 0.0f, 1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f((render_data.xoff + render_data.xmax) * render_data.scale, (render_data.yoff + render_data.ymax) * render_data.scale, 0.0f);
			glNormal3f(0.0f, 0.0f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f((render_data.xoff + render_data.xmax) * render_data.scale, (render_data.yoff + render_data.ymin) * render_data.scale, 0.0f);
		glEnd();
		glPopMatrix();
	glUseProgram(0);
}

void render_frame() {
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	gluLookAt(
		0.0f, 0.0f, render_data.zdist,
		0.0f, 0.0f,              0.0f,
		0.0f, 1.0f,              0.0f
	);

	// glRotatef(((msecs_elapsed / 25) * 1) % 360, 0.0f, 1.0f, 0.0f);
	// draw the quad in world-space at z=0
	draw_shader_quad();

	glFlush();
	SDL_GL_SwapBuffers();

	timing_data.msecs_elapsed = SDL_GetTicks();
	timing_data.frames_rendered += 1;
}

void update_fps() {
	if ((SDL_GetTicks() / 1000) - timing_data.secs_elapsed < 5)
		return;

	printf("[%s] frames per second: %d\n", __FUNCTION__, (timing_data.frames_rendered / 5));

	timing_data.secs_elapsed = SDL_GetTicks() / 1000;
	timing_data.frames_rendered = 0;
}

bool handle_input(const Uint8* keys) {
	SDL_PumpEvents();

	bool key_pressed = false;
	bool ignore_input = ((timing_data.msecs_elapsed - timing_data.last_key_press_time) < 250);

	if (keys[SDLK_q])
		return false;

	if (keys[SDLK_LCTRL] && ignore_input)
		return true;
	if (keys[SDLK_RCTRL] && ignore_input)
		return true;

	// scale the arrow-key sensitivity by the zoom-factor
	if (keys[SDLK_LSHIFT]) { uniform_data.zoom_lvl += (0.01f * uniform_data.zoom_lvl); key_pressed = true; }
	if (keys[SDLK_RSHIFT]) { uniform_data.zoom_lvl -= (0.01f * uniform_data.zoom_lvl); key_pressed = true; }
	if (keys[SDLK_LCTRL ]) { uniform_data.max_iter = std::max(32, std::min(32768, int(uniform_data.max_iter >> 1))); key_pressed = true; }
	if (keys[SDLK_RCTRL ]) { uniform_data.max_iter = std::max(32, std::min(32768, int(uniform_data.max_iter << 1))); key_pressed = true; }
	if (keys[SDLK_UP    ]) { uniform_data.ycenter += (0.01f * uniform_data.zoom_lvl); key_pressed = true; }
	if (keys[SDLK_DOWN  ]) { uniform_data.ycenter -= (0.01f * uniform_data.zoom_lvl); key_pressed = true; }
	if (keys[SDLK_LEFT  ]) { uniform_data.xcenter -= (0.01f * uniform_data.zoom_lvl); key_pressed = true; }
	if (keys[SDLK_RIGHT ]) { uniform_data.xcenter += (0.01f * uniform_data.zoom_lvl); key_pressed = true; }

	if (key_pressed) {
		timing_data.last_key_press_time = timing_data.msecs_elapsed;
	}

	return true;
}






void init_ffp_lighting() {
	GLfloat ambient_light[] = {0.1f, 0.1f, 0.1f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);

	GLfloat diffuse_light[] = {0.8f, 0.8f, 0.8f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);

	GLfloat specular_light[] = {0.5f, 0.5f, 0.5f, 1.0f};
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular_light);

	// last parameter is 0.0: directional light
	GLfloat light_pos[] = {0.0f, 30.0f, 60.0f, 0.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

	// GLfloat specularReflection[] = {1.0f, 1.0f, 1.0f, 1.0f};
	// glMaterialfv(GL_FRONT, GL_SPECULAR, specularReflection);
	// glMateriali(GL_FRONT, GL_SHININESS, 128);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_light);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}



// initialize SDL surface and OpenGL
SDL_Surface* init_sdl(int w, int h) {
	SDL_Surface* sdl_surface = NULL;

	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		exit(1);
		return sdl_surface;
	}

	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if ((sdl_surface = SDL_SetVideoMode(w, h, 0, SDL_OPENGL)) == NULL) {
		exit(1);
		return sdl_surface;
	}

	// delay before repeating, repeat interval
	// SDL_EnableMouseRepeat(1, 5);
	SDL_EnableKeyRepeat(1, 5);

	if (USE_MOUSELOOK) {
		SDL_ShowCursor(SDL_DISABLE);
	}

	SDL_WM_SetCaption("Mandelbrot Shader Renderer", NULL);
	// SDL_WM_GrabInput(SDL_GRAB_ON);

	return sdl_surface;
}

void init_ogl(const SDL_Surface* sdl_surface) {
	// must come after SDL but before the rest
	if (glewInit() != GLEW_OK)
		exit(1);

	glViewport(0, 0, sdl_surface->w, sdl_surface->h);

	glMatrixMode(GL_PROJECTION);
	gluPerspective(60, (sdl_surface->w * 1.0) / sdl_surface->h, 0.1, 10000.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.25f, 0.25f, 0.25f, 0.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearDepth(1.0);
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glEnable(GL_NORMALIZE);

	// default winding is CCW
	// glFrontFace(GL_CCW);

	// can be skipped
	// init_ffp_lighting();
	t_extension_loader::load_extensions();

	printf("[%s][%dx%dx%dbpp]\n", __FUNCTION__, sdl_surface->w, sdl_surface->h, sdl_surface->format->BitsPerPixel);
}

void main_loop() {
	memset(&runtime_data, 0, sizeof(t_runtime_data));
	memset(&timing_data, 0, sizeof(t_timing_data));
	memset(&shader_data, 0, sizeof(t_shader_data));
	memset(&uniform_data, 0, sizeof(t_uniform_data));
	memset(&render_data, 0, sizeof(t_render_data));

	prepare_shader_data(shader_data, uniform_data);
	prepare_render_data(render_data);

	while ((runtime_data.is_running = handle_input(SDL_GetKeyState(NULL)))) {
		render_frame();
		update_fps();
	}

	t_shader_loader::delete_program_object(shader_data.id);
}



int main(int, char**) {
	init_ogl(init_sdl(WINDOW_SIZE_X, WINDOW_SIZE_Y));
	main_loop();

	SDL_Quit();
	return 0;
}

