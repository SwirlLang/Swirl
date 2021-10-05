class Window {
    public:
        int Window_size = 500, 600;
        string title = "Hello world";
        bool allow_resize = true;

    Window(){
        GLFWwindow* window = glfwCreateWindow(Window_size, title, NULL, NULL);
    }
}
