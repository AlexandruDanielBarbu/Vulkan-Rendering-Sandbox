//#include "PathUtils.h"
//#include "Utils.h"
//
//#include <vulkan/vulkan.h>
//
//namespace alex {
//
//        inline void CreateGLFWWindow(const bool fullscreen, GLFWmonitor*& monitor, GLFWwindow*& window) {
//                if (!glfwInit()) {
//                        throw std::runtime_error("Failed to init GLFW");
//                        //VKL_EXIT_WITH_ERROR("");
//                }
//
//                // deactivate OpenGl
//                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//                glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//
//                // Use a monitor if we'd like to open the window in fullscreen mode:
//                /*GLFWmonitor* */monitor = nullptr;
//                if (fullscreen) {
//                        monitor = glfwGetPrimaryMonitor();
//                }
//
//                // Get a valid window handle and assign to window
//                // Note that i used the above mentioned variable called monitor.
//                /*GLFWwindow* */window = glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);
//                if (!window) {
//                        VKL_LOG("If your program reaches this point, that means two things:");
//                        VKL_LOG("1) Project setup was successful. Everything is working fine.");
//                        VKL_LOG("2) You haven't implemented Subtask 1.2, which is creating a window with GLFW.");
//                        VKL_EXIT_WITH_ERROR("No GLFW window created.");
//                }
//                VKL_LOG("Subtask 1.2 done.");
//        }
//
//}