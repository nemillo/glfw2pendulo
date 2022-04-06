#include <GL/glew.h>		// include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "sphere.hpp"
#include "plane.hpp"
#include "line.hpp"

#define GL_LOG_FILE "gl.log"
#define PH_LOG_FILE "ph.log"

const float gravity = 9.80665f;
const float R = 0.5f;
const glm::vec3 puntofijo = glm::vec3(0.0f,0.0f,4.0f);
const float L = 2.0f;
const float theta0 = glm::pi<float>()/4;

std::ofstream log_file;
std::ofstream ph_log_file;

std::ostream& operator<<(std::ostream& stream, const std::chrono::system_clock::time_point& point)
{
    auto time = std::chrono::system_clock::to_time_t(point);
    std::tm* tm = std::localtime(&time);
    char buffer[26];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S.", tm);
    stream << buffer;
    auto duration = point.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);
    // remove microsecond cast line if you would like a higher resolution sub second time representation, then just stream remaining.count()
    auto micro = std::chrono::duration_cast<std::chrono::microseconds>(remaining);
    return stream << micro.count();
}

/* start a new log file. put the time and date at the top */
bool restart_gl_log() {
	log_file.open(GL_LOG_FILE);
	auto now = std::chrono::high_resolution_clock::now();
    log_file << GL_LOG_FILE << " Local time : " << now << std::endl;
	log_file.close();
	return true;
}

/* we will tell GLFW to run this function whenever it finds an error */
void glfw_error_callback( int error, const char *description ) {
	
}

// keep track of window size for things like the viewport and the mouse cursor
float g_gl_width = 1024.0;
float g_gl_height = 800.0;

/* we will tell GLFW to run this function whenever the framebuffer size is changed */
void glfw_framebuffer_size_callback( GLFWwindow *window, int width, int height ) {
	g_gl_width = float(width);
	g_gl_height = float(height);
	/* update any perspective matrices used here */
}

void updateAcceleration (Sphere &sphere){
	glm::vec3 totalForce;
	glm::vec3 r = glm::vec3(sphere.getPosition().x-puntofijo.x,sphere.getPosition().y-puntofijo.y,sphere.getPosition().z-puntofijo.z);
	float theta = glm::atan((sphere.getPosition().x-puntofijo.x)/(puntofijo.z-sphere.getPosition().z));
	//glm::vec3 thetavel = glm::cross(r,sphere.getVelocity());
	glm::vec3 thetavel = glm::vec3(0.0f,glm::length(sphere.getVelocity())/L,0.0f);
	float T = sphere.getMass()*gravity*glm::cos(theta)+sphere.getMass()*L*thetavel.y*thetavel.y;
	float Ft = -sphere.getMass()*gravity*glm::sin(theta);
	float Fn = -T + sphere.getMass()*gravity*glm::cos(theta);

	totalForce.x = Fn*glm::sin(theta)+Ft*glm::cos(theta);
	totalForce.y = 0.0f;
	totalForce.z = -Fn*glm::cos(theta)+Ft*glm::sin(theta);
	sphere.setAcceleration(totalForce/(sphere.getMass()));

	ph_log_file.open(PH_LOG_FILE,std::ios::app);
	ph_log_file <<"Position: " << sphere.getPosition().x << "  " << sphere.getPosition().y << "  " << sphere.getPosition().z << "  "
				<< "r: " << r.x << "  " << r.y << "  " << r.z << "   " << "L: " << glm::length(r) << "   "
				<< "T: " << T << "   "
	            << "Velocity: " << sphere.getVelocity().x << "  " << sphere.getVelocity().y << "  " << sphere.getVelocity().z << "   "
				<< "Acceleration: " << sphere.getAcceleration().x << "  " << sphere.getAcceleration().y << "  " << sphere.getAcceleration().z << "  "
				<< "Theta:  " << theta*180.0f/glm::pi<float>() << "  Thetavel:  " << thetavel.x << "  "<< thetavel.y << "  "<< thetavel.z << std::endl;
	
	ph_log_file.close();
}

void IntegrateEuler(Sphere &sphere, float DT){
		sphere.setVelocity(sphere.getAcceleration()*DT + sphere.getVelocity());
		sphere.setPosition(sphere.getVelocity()*DT + sphere.getPosition());
		updateAcceleration(sphere);
}

void IntegrateRK4(Sphere &bola, float DT)
{
	glm::vec3 Pos;
	glm::vec3 Vel;

	glm::vec3 Kv1,Kv2,Kv3,Kv4; //Son aceleraciones
	glm::vec3 Kx1,Kx2,Kx3,Kx4; //Son velocidades
	glm::vec3 xK2,xK3,xK4; //Son las posiciones estimadas para evaluar la aceleracion

	Kv1 = bola.getAcceleration();
	Kx1 = bola.getVelocity();

	xK2 = bola.getPosition() + Kx1*DT/2.0f;
	updateAcceleration(bola);
	Kv2 = bola.getAcceleration();
	Kx2 = bola.getVelocity() + Kv1 * DT/2.0f;

	xK3 = bola.getPosition() + Kx2*DT/2.0f;
	updateAcceleration(bola);
	Kv3 = bola.getAcceleration();
	Kx3 = bola.getVelocity() + Kv2 * DT/2.0f;

	xK4 = bola.getPosition() + Kx3*DT;
	updateAcceleration(bola);
	Kv4 = bola.getAcceleration();
	Kx4 = bola.getVelocity() + Kv3 * DT;

    Vel = bola.getVelocity() + (Kv1+Kv2*2.0f+Kv3*2.0f+Kv4)/6.0f*DT;
    Pos = bola.getPosition() + (Kx1+Kx2*2.0f+Kx3*2.0f+Kx4)/6.0f*DT;

	bola.setVelocity(Vel); // Update object's velocity
	bola.setPosition(Pos); // Update object's position
}

void IntegrateVerlet (Sphere &sphere, float DT){
        sphere.setPosition(sphere.getPosition() + sphere.getVelocity()*DT + 1.0f/2.0f*sphere.getAcceleration()*DT*DT);
        glm::vec3 oldAcceleration=sphere.getAcceleration();
        updateAcceleration(sphere);
        sphere.setVelocity(sphere.getVelocity() + 1.0f/2.0f*(oldAcceleration*DT+sphere.getAcceleration()*DT));
}

void CheckBC(Sphere &sphere) {
	if (sphere.getPosition().z <= R){
			glm::vec3 oldVelocity;
			glm::vec3 newVelocity;
			oldVelocity = sphere.getVelocity();
			newVelocity = oldVelocity;
			newVelocity.z = -oldVelocity.z;
			sphere.setVelocity(newVelocity);

			glm::vec3 oldPosition;
			glm::vec3 newPosition;
			oldPosition = sphere.getPosition();
			newPosition = oldPosition;
			newPosition.z = R;
			sphere.setPosition(newPosition);
			
		}

		if (sphere.getPosition().x <= -2+R){
			glm::vec3 oldVelocity;
			glm::vec3 newVelocity;
			oldVelocity = sphere.getVelocity();
			newVelocity = oldVelocity;
			newVelocity.x = -oldVelocity.x;
			sphere.setVelocity(newVelocity);

			glm::vec3 oldPosition;
			glm::vec3 newPosition;
			oldPosition = sphere.getPosition();
			newPosition = oldPosition;
			newPosition.x = -2+R;
			sphere.setPosition(newPosition);
			
		}

		if (sphere.getPosition().x >= 2-R){
			glm::vec3 oldVelocity;
			glm::vec3 newVelocity;
			oldVelocity = sphere.getVelocity();
			newVelocity = oldVelocity;
			newVelocity.x = -oldVelocity.x;
			sphere.setVelocity(newVelocity);

			glm::vec3 oldPosition;
			glm::vec3 newPosition;
			oldPosition = sphere.getPosition();
			newPosition = oldPosition;
			newPosition.x = 2-R;
			sphere.setPosition(newPosition);
			
		}

		if (sphere.getPosition().y <= -2+R){
			glm::vec3 oldVelocity;
			glm::vec3 newVelocity;
			oldVelocity = sphere.getVelocity();
			newVelocity = oldVelocity;
			newVelocity.y = -oldVelocity.y;
			sphere.setVelocity(newVelocity);

			glm::vec3 oldPosition;
			glm::vec3 newPosition;
			oldPosition = sphere.getPosition();
			newPosition = oldPosition;
			newPosition.y = -2+R;
			sphere.setPosition(newPosition);
			
		}

		if (sphere.getPosition().y >= 2-R){
			glm::vec3 oldVelocity;
			glm::vec3 newVelocity;
			oldVelocity = sphere.getVelocity();
			newVelocity = oldVelocity;
			newVelocity.y = -oldVelocity.y;
			sphere.setVelocity(newVelocity);

			glm::vec3 oldPosition;
			glm::vec3 newPosition;
			oldPosition = sphere.getPosition();
			newPosition = oldPosition;
			newPosition.y = 2-R;
			sphere.setPosition(newPosition);
			
		}
		
}

void SphereCollision (Sphere &sph1, Sphere &sph2){
	if (glm::distance(sph1.getPosition(),sph2.getPosition()) <= 2*R){
			glm::vec3 oldPosition1;
			glm::vec3 oldPosition2;
			glm::vec3 oldVelocity1;
			glm::vec3 oldVelocity2;
			glm::vec3 newPosition1;
			glm::vec3 newPosition2;
			glm::vec3 newVelocity1;
			glm::vec3 newVelocity2;

			oldPosition1 = sph1.getPosition();
			oldPosition2 = sph2.getPosition();
			oldVelocity1 = sph1.getVelocity();
			oldVelocity2 = sph2.getVelocity();

			/*newVelocity1 = oldVelocity1 + 
			     glm::length(oldPosition2 - oldPosition1)*
				 glm::dot(oldVelocity2,(oldPosition2 - oldPosition1))/
				 glm::dot((oldPosition2 - oldPosition1),(oldPosition2 - oldPosition1)) -
				 glm::length(oldPosition1 - oldPosition2)*
				 glm::dot(oldVelocity1,(oldPosition1 - oldPosition2))/
				 glm::dot((oldPosition1 - oldPosition2),(oldPosition1 - oldPosition2));

			newVelocity2 = oldVelocity2 + 
			     glm::length(oldPosition2 - oldPosition1)*
				 glm::dot(oldVelocity1,(oldPosition2 - oldPosition1))/
				 glm::dot((oldPosition2 - oldPosition1),(oldPosition2 - oldPosition1)) -
				 glm::length(oldPosition1 - oldPosition2)*
				 glm::dot(oldVelocity2,(oldPosition1 - oldPosition2))/
				 glm::dot((oldPosition1 - oldPosition2),(oldPosition1 - oldPosition2));*/

			glm::vec3 vecx = oldPosition1 - oldPosition2;
			glm::normalize(vecx);
			float x1 = glm::dot(vecx,oldVelocity1);
			glm::vec3 vecv1x = vecx * x1;
			glm::vec3 vecv1y = oldVelocity1 - vecv1x;
			float m1 = sph1.getMass();

			vecx = -vecx;
			float x2 = glm::dot(vecx,oldVelocity2);
			glm::vec3 vecv2x = vecx * x2;
			glm::vec3 vecv2y = oldVelocity2 - vecv2x;
			float m2 = sph2.getMass();

			newVelocity1 = vecv1x*(m1-m2)/(m1+m2)+vecv2x*(2*m2)/(m1+m2) + vecv1y;
			newVelocity2 = vecv1x*(2*m1)/(m1+m2)+vecv2x*(m2-m1)/(m1+m2) + vecv2y;


			sph1.setVelocity(newVelocity1);
			sph2.setVelocity(newVelocity2);
		}
}

int main() {
	GLFWwindow *window;
	const GLubyte *renderer;
	const GLubyte *version;
	int sectorCount = 10;
	int stackCount = 10;
	float fps = 0.0f;
	float frame_time = 0.0f;
	float frame_time_cummulated = 0.0f;
	
	Sphere sphere1;
	Plane plane1;
	Line line1;

	restart_gl_log();
	auto t_start = std::chrono::high_resolution_clock::now();
	
	log_file.open(GL_LOG_FILE,std::ios::app);
	log_file << "t_start: " << t_start << std::endl;
	log_file.close();
	
	GLuint vbo;
	GLuint vao;
	const char *vertex_shader = "#version 410\n"
		"in vec3 vp;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 proj;"
		"void main() {"
		"  gl_PointSize = 10.0;"
		"  gl_Position = proj * view * model * vec4( vp, 1.0 );"
		"}";

	const char *fragment_shader = "#version 410\n"
		"out vec4 frag_colour;"
		"void main() {"
		"  frag_colour = vec4( 0.5, 0.5, 0.5, 1.0 );"
		"}";
	GLuint shader_programme, vs, fs;

	// start GL context and O/S window using the GLFW helper library
	glfwSetErrorCallback( glfw_error_callback );
	if ( !glfwInit() ) {
		return 1;
	}
	// set anti-aliasing factor to make diagonal edges appear less jagged
	glfwWindowHint( GLFW_SAMPLES, 4 );

	/* we can run a full-screen window here */

	/*GLFWmonitor* mon = glfwGetPrimaryMonitor ();
	const GLFWvidmode* vmode = glfwGetVideoMode (mon);
	GLFWwindow* window = glfwCreateWindow (
		vmode->width, vmode->height, "Extended GL Init", mon, NULL
	);*/

	window = glfwCreateWindow( g_gl_width, g_gl_height, "Extended Init", NULL, NULL );
	if ( !window ) {
		glfwTerminate();
		return 1;
	}
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
	//
	glfwMakeContextCurrent( window );

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	renderer = glGetString( GL_RENDERER ); // get renderer string
	version = glGetString( GL_VERSION );	 // version as a string
	
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable( GL_DEPTH_TEST ); // enable depth-testing
	glEnable(GL_PROGRAM_POINT_SIZE);
	glDepthFunc( GL_LESS );		 // depth-testing interprets a smaller value as "closer"
	
	vs = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vs, 1, &vertex_shader, NULL );
	glCompileShader( vs );
	fs = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fs, 1, &fragment_shader, NULL );
	glCompileShader( fs );
	shader_programme = glCreateProgram();
	glAttachShader( shader_programme, fs );
	glAttachShader( shader_programme, vs );
	glLinkProgram( shader_programme );
	glUseProgram( shader_programme );
	
	GLuint vp = glGetAttribLocation(shader_programme, "vp");
	sphere1.init(vp,R);
	sphere1.setMass(1.0f);
	sphere1.setPosition(glm::vec3(puntofijo.x + L*glm::sin(theta0),0.0f,puntofijo.z - L*glm::cos(theta0)));
	sphere1.setVelocity(glm::vec3(0.0f,0.0f,0.0f));
	updateAcceleration(sphere1);

	plane1.init(vp,0.0f);

	line1.init(vp,puntofijo,sphere1.getPosition());
	
	GLint uniModel = glGetUniformLocation(shader_programme, "model");

    // Set up projection
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, -10.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    GLint uniView = glGetUniformLocation(shader_programme, "view");
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 proj = glm::perspective(
		glm::radians(45.0f), 
		g_gl_width / g_gl_height, 
		1.0f, 
		20.0f
	);

    GLint uniProj = glGetUniformLocation(shader_programme, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	
	while ( !glfwWindowShouldClose( window ) ) {
		auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
		// wipe the drawing surface clear
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glViewport( 0, 0, g_gl_width, g_gl_height );

		glm::mat4 model = glm::mat4(1.0f);
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //sets the uniform matrix model in shader
		plane1.draw();
		line1.init(vp,puntofijo,sphere1.getPosition());
		line1.draw();

		IntegrateRK4(sphere1,frame_time);
		//CheckBC(sphere1);
				
		glm::mat4 model1 = glm::mat4(1.0f);
		model1 = glm::translate(
            model1,
            sphere1.getPosition()
        );

		
		
    	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model1)); //sets the uniform matrix model in shader
		sphere1.draw();

		// update other events like input handling
		glfwPollEvents();
		if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) {
			glfwSetWindowShouldClose( window, 1 );
		}
		
		// put the stuff we've been drawing onto the display
		glfwSwapBuffers( window );
		auto t_after_frame_display = std::chrono::high_resolution_clock::now();
		frame_time = std::chrono::duration_cast<std::chrono::duration<float>>(t_after_frame_display - t_now).count();
		fps = 1/frame_time;
		frame_time_cummulated += frame_time;

		log_file.open(GL_LOG_FILE,std::ios::app);
		log_file << "t_now: " << t_now << std::endl;
		log_file << "t_after_frame_display: " << t_after_frame_display << std::endl;
		log_file << "frame_time: " << frame_time << std::endl;
		log_file << "fps: " << fps << std::endl;
		log_file << "time: " << time <<  std::endl;
		log_file.close();
		if (frame_time_cummulated >= 1.0f){
			std::string fps_title[] = {"OpenGL @ FPS: "};
			fps_title[0].append(std::to_string(fps));
			glfwSetWindowTitle( window,  fps_title[0].c_str() );
			frame_time_cummulated = 0.0f;
		}
		
	}

	// close GL context and any other GLFW resources
	glfwTerminate();
	sphere1.cleanup();
	plane1.cleanup();
	line1.cleanup();
	return 0;
}