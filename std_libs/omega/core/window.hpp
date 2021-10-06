#include <GLFW/glfw3.h>
#include <glad.h>

class Window {

public:
	/* The major version of OpenGL to use, defaults to 3 */
	int gl_major_version = 3;

	/* Minor version of OpenGL to use, defaults to 3 */
	int gl_minor_version = 3;

	/* Width and hight of the window */
	int window_size = [400, 550];

public:
	/* Title of the window */
	string window_title = "GL-Window";

	Window() {

		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major_version);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_major_version);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* window = glfwCreateWindow(window_size[0], window_size[1], window_title, NULL, NULL);
		if (window == NULL) {
			return -1;

		};

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		};

		glfwMakeContextCurrent(window);
		glViewport(0, 0, window_size[0], window_size[1]);
		glfwSetFramebufferSizeCallback(window, win_resize_callback);

		while (!glfwWindowShouldClose(window))
		{
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		glfwTerminate();
		return 0;
	};

	void win_resize_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}
};
