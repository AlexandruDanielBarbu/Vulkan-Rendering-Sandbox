/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "PathUtils.h"
#include "Utils.h"

#include <vulkan/vulkan.h>

#include <vector>


#undef min
#undef max

constexpr char WELCOME_MSG[] = ":::::: WELCOME TO GCG 2025 ::::::";
constexpr char WINDOW_TITLE[] = "GCG 2025";

#pragma region Helper Function Declarations
/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices		A pointer which points to contiguous memory of #physical_device_count sequentially
                                        stored VkPhysicalDevice handles is expected. The handles can (or should) be those
 *										that are returned from vkEnumeratePhysicalDevices.
 *	@param		physical_device_count	The number of consecutive physical device handles there are at the memory location
 *										that is pointed to by the physical_devices parameter.
 *	@param		surface					A valid VkSurfaceKHR handle, which is used to determine if a certain
 *										physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface);

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices	A vector containing all available VkPhysicalDevice handles, like those
 *									that are returned from vkEnumeratePhysicalDevices.
 *	@param		surface				A valid VkSurfaceKHR handle, which is used to determine if a certain
 *									physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, select a queue family which supports both,
 *	graphics and presentation to the given surface. Return the INDEX of an appropriate queue family!
 *	@return		The index of a queue family which supports the required features shall be returned.
 */
uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a the physical device's surface capabilites are read and returned.
 *	@return		VkSurfaceCapabilitiesKHR data
 */
VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a supported surface image format
 *	which can be used for the framebuffer's attachment formats is searched and returned.
 *	@return		A supported format is returned.
 */
VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, return its surface transform flag.
 *	This can be used to set the swap chain to the same configuration as the surface's current transform.
 *	@return		The surface capabilities' currentTransform value is returned, which is suitable for swap chain config.
 */
VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface);


/*!
 *	This callback function gets invoked by GLFW whenever a GLFW error occured.
 */
void errorCallbackFromGlfw(int error, const char* description) { std::cout << "GLFW error " << error << ": " << description << std::endl; }
#pragma endregion

#pragma region alexd custom callbacks
bool reset_camera = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
            reset_camera = true;
    if (key == GLFW_KEY_F && action == GLFW_RELEASE)
            reset_camera = false;
}

bool left_mouse_btn_pressed = false;
bool right_mouse_btn_pressed = false;
float scroll_delta = {};

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    // left mouse press
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        VKL_LOG("Pressed left mouse button");
        left_mouse_btn_pressed = true;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        VKL_LOG("Released left mouse button");
        left_mouse_btn_pressed = false;
        scroll_delta = 0;
    }

    // right mouse press
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            VKL_LOG("Pressed right mouse button");
            right_mouse_btn_pressed = true;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
            VKL_LOG("Released right mouse button");
            right_mouse_btn_pressed = false;
            scroll_delta = 0;

    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    scroll_delta = yoffset;
}
#pragma endregion

#pragma region alexd custom helpers
// If a discrete gpu is found return it, else return the first device
VkPhysicalDevice trySelectFirstDiscreteGPU(const std::vector<VkPhysicalDevice>& devices) {

        for (const auto& device : devices) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);

                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                        return device;
        }

        VKL_LOG("Falling back to first device");
        return devices[0];
}

float xposPrev = 0;
float yPosPrev = 0;
void get_mouse_delta(GLFWwindow* window, double& deltax, double& deltay) {
    // get current mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // if i want to move camera compute deltas
    if (left_mouse_btn_pressed || right_mouse_btn_pressed) {
        // write deltas
        deltax = xpos - xposPrev;
        deltay = ypos - yPosPrev;

        std::cout << '(' << deltax << ' ' << deltay << ')' << std::endl;
        // update previous mouse positions
        xposPrev = xpos;
        yPosPrev = ypos;

        return;
    }
    
    // Otherwise do nothing
    xposPrev = xpos;
    yPosPrev = ypos;

    deltax = 0.0;
    deltay = 0.0;
}
#pragma endregion

#pragma region Camera class
class Camera {
public:
        Camera(const std::string& ini_file_path, const double window_width, const double window_height) {
                INIReader camera_reader(ini_file_path);
                camera_fov   = camera_reader.GetReal("camera", "fov",   10);    // not in radians
                camera_near  = camera_reader.GetReal("camera", "near",  10);
                camera_far   = camera_reader.GetReal("camera", "far",   20);
                camera_yaw   = camera_reader.GetReal("camera", "yaw",   10);    // radians
                camera_pitch = camera_reader.GetReal("camera", "pitch", 10);    // radians
                aspect_ratio = (double)window_width / (double)window_height;

                proj = gcgCreatePerspectiveProjectionMatrix(
                        glm::radians(camera_fov),
                        aspect_ratio,
                        camera_near,
                        camera_far
                );

                compute_camera_pos();
        }

        glm::mat4 get_proj_view_matrix(const double deltax = 0, const double deltay = 0) {
                // arcball
                if (left_mouse_btn_pressed) {
                        camera_yaw -= deltax * arcball_sensitivity;
                        camera_pitch += deltay * arcball_sensitivity;

                        const double pitchLimit = glm::radians(89.0);
                        camera_pitch = glm::clamp(camera_pitch, -pitchLimit, pitchLimit);

                        compute_camera_pos();
                        //force return, my job here is done
                        return proj * myLookAt();
                }

                // strafing
                if (right_mouse_btn_pressed) {
                        glm::vec3 up = { 0.0f, 1.0f, 0.0f };

                        glm::vec3 camera_forward = glm::normalize(target - camera_pos);
                        glm::vec3 camera_right = glm::normalize(glm::cross(camera_forward, up));
                        glm::vec3 camera_up = glm::cross(camera_right, camera_forward);

                        // move target and camera_pos along camera local axis
                        target -= (float)(strafe_sensitivity * deltax) * camera_right;
                        camera_pos -= (float)(strafe_sensitivity * deltax) * camera_right;
                        
                        target += (float)(strafe_sensitivity * deltay) * camera_up;
                        camera_pos += (float)(strafe_sensitivity * deltay) * camera_up;

                        return proj * myLookAt();
                }

                // else rerturrn what is there
                compute_camera_pos();
                return proj * myLookAt();
        }

        void reset_camera_state() {
                target = {};
                camera_zoom_level = 5;
                camera_pos = { 0, 0, camera_zoom_level };
                camera_pitch = 0;
                camera_yaw = 0;
        }

        void update_camera_zoom(const float delta) {
                camera_zoom_level -= delta * scroll_sensitivity;

                if (camera_zoom_level < 0) camera_zoom_level = 0;
        }
private:
        const double arcball_sensitivity = 0.01;
        const double strafe_sensitivity = 0.01;
        const double scroll_sensitivity = 0.07;

        double camera_fov = 0;
        double camera_near = 0;
        double camera_far = 0;
        double aspect_ratio = 0;

        double camera_yaw = 0;
        double camera_pitch = 0;
        
        float camera_zoom_level = 5.0;
        
        glm::mat4 proj = {};
        
        glm::vec3 target = {};
        glm::vec3 camera_pos = { 0.0f, 0.0f, -5.0f };
        
        void compute_camera_pos() {
                camera_pos = target + camera_zoom_level * glm::normalize(glm::vec3(
                        cos(camera_pitch) * sin(camera_yaw),
                        sin(camera_pitch),
                        cos(camera_pitch) * cos(camera_yaw)
                ));
        }
        glm::mat4 myLookAt() const {
                glm::vec3 up = { 0.0f, 1.0f, 0.0f };

                glm::vec3 camera_forward = glm::normalize(target - camera_pos);
                glm::vec3 camera_right = glm::normalize(glm::cross(camera_forward, up));
                glm::vec3 camera_up = glm::cross(camera_right, camera_forward);

                glm::mat4 orientation(
                        glm::vec4(camera_right.x, camera_up.x, -camera_forward.x, 0),
                        glm::vec4(camera_right.y, camera_up.y, -camera_forward.y, 0),
                        glm::vec4(camera_right.z, camera_up.z, -camera_forward.z, 0),
                        glm::vec4(0, 0, 0, 1)
                );

                glm::mat4 translation(
                        glm::vec4(1, 0, 0, 0),
                        glm::vec4(0, 1, 0, 0),
                        glm::vec4(0, 0, 1, 0),
                        glm::vec4(-camera_pos.x, -camera_pos.y, -camera_pos.z, 1)
                );
                
                return (orientation * translation);
                //return glm::lookAt(camera_pos, target, up);
        }
};
#pragma endregion

int main(int argc, char** argv) {
    VKL_LOG(WELCOME_MSG);

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

#pragma region load window settings
    INIReader window_reader("assets/settings/window.ini");
    int window_width = window_reader.GetInteger("window", "width", 800);
    int window_height = window_reader.GetInteger("window", "height", 800);
    bool fullscreen = false;
    std::string window_title = window_reader.Get("window", "title", "Awesome Vulkan Project");
#pragma endregion

#pragma region load camera settings
    // GCG framework stuff
    std::string init_camera_filepath = "assets/settings/camera_front.ini";
    if (cmdline_args.init_camera)
        init_camera_filepath = cmdline_args.init_camera_filepath;
    
    Camera main_camera(init_camera_filepath, window_width, window_height);
#pragma endregion

    
    glm::mat4 view_projection = main_camera.get_proj_view_matrix();

    // Install a callback function, which gets invoked whenever a GLFW error occurred.
    glfwSetErrorCallback(errorCallbackFromGlfw);

#pragma region GLFW window setup
    if (!glfwInit()) {
        VKL_EXIT_WITH_ERROR("Failed to init GLFW");
    }

    // below line deactivates some OpenGl/OpenGl SE stuff
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Use a monitor if we'd like to open the window in fullscreen mode:
    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
    }

    // Get a valid window handle and assign to window
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);
    if (!window) {
        VKL_EXIT_WITH_ERROR("No GLFW window created.");
    }
#pragma endregion

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

#pragma region my vulkan related variables
    VkResult result;
    VkInstance vk_instance = VK_NULL_HANDLE;              // To be set during Subtask 1.3
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;             // To be set during Subtask 1.4
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE; // To be set during Subtask 1.5
    VkDevice vk_device = VK_NULL_HANDLE;                  // To be set during Subtask 1.7
    VkQueue vk_queue = VK_NULL_HANDLE;                    // To be set during Subtask 1.7
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;         // To be set during Subtask 1.8
#pragma endregion

#pragma region vulkan instance

#pragma region vk application info
    VkApplicationInfo application_info = {};                     
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; 
    application_info.pEngineName = "GCG_VK_Library";             
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 2023, 9, 1);
    application_info.pApplicationName = "GCG_VK_Solution";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 2023, 9, 19);
    application_info.apiVersion = VK_API_VERSION_1_1;  
#pragma endregion

    VkInstanceCreateInfo instance_create_info = {};                      
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; 
    instance_create_info.pApplicationInfo = &application_info;

#pragma region GLFW required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (glfwExtensions == nullptr) {
        VKL_EXIT_WITH_ERROR("No GLFW extensions supported to display things around.");
    }

    std::vector<const char*> instance_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#pragma endregion
    
#pragma region supported extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
#pragma endregion

    // Query if i support all extensions
    for (const auto& glfwExtension : instance_extensions) {
        bool isFound = false;
        for (const auto& extension : extensions) {
            if (strcmp(glfwExtension, extension.extensionName) == 0) {
                isFound = true;
                break;
            }
        }
        if (!isFound)
            VKL_EXIT_WITH_ERROR("One extension was not found.");
    }

    // Hook in required_extensions using VkInstanceCreateInfo::enabledExtensionCount and VkInstanceCreateInfo::ppEnabledExtensionNames!
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

    // Hook in enabled_layers using VkInstanceCreateInfo::enabledLayerCount and VkInstanceCreateInfo::ppEnabledLayerNames!
    instance_create_info.enabledLayerCount = 1;
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    instance_create_info.ppEnabledLayerNames = layers;
    
    // Get supported Layers
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Query if i support all layers
    for (int i = 0; i < instance_create_info.enabledLayerCount; i++) {
        bool isFound = false;
        for (const auto& layer : availableLayers) {
            if (strcmp(instance_create_info.ppEnabledLayerNames[i], layer.layerName) == 0) {
                isFound = true;
                break;
            }
        }

        if (!isFound)
            VKL_EXIT_WITH_ERROR("One layer was not found.");
    }
    
    if (vkCreateInstance(&instance_create_info, nullptr, &vk_instance) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Function vkCreateInstance failed.");
    
    if (!vk_instance)
        VKL_EXIT_WITH_ERROR("No VkInstance created or handle not assigned.");
#pragma endregion

#pragma region vulkan window surface
    if (glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Function glfwCreateWindowSurface failed.");

    if (!vk_surface)
        VKL_EXIT_WITH_ERROR("No VkSurfaceKHR created or handle not assigned.");
#pragma endregion

#pragma region vulkan physical device
    // Get devices
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, nullptr);
    
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, physicalDevices.data());

    // Print all devices' name and type
    for (const auto& device : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VKL_LOG(std::string(properties.deviceName + properties.deviceType));

    }

    // let the framework decide the device
    uint32_t device_index = selectPhysicalDeviceIndex(physicalDevices, vk_surface);

    vk_physical_device = physicalDevices[device_index];
    // Just print what GPU got selected
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(vk_physical_device, &properties);
        VKL_LOG(std::string(properties.deviceName));
    }

    if (!vk_physical_device)
        VKL_EXIT_WITH_ERROR("No VkPhysicalDevice selected or handle not assigned.");
#pragma endregion

#pragma region vulkan queue family
    uint32_t selected_queue_family_index = selectQueueFamilyIndex(vk_physical_device, vk_surface);

    // Sanity check if we have selected a valid queue family index:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    if (selected_queue_family_index >= queue_family_count)
        VKL_EXIT_WITH_ERROR("Invalid queue family index selected.");
#pragma endregion

#pragma region logical device and get queue
    VkDeviceQueueCreateInfo vk_device_queue_create_info = {};
    vk_device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vk_device_queue_create_info.queueFamilyIndex = selected_queue_family_index;
    vk_device_queue_create_info.queueCount = 1;
    float queue_priority = 1.0f;
    vk_device_queue_create_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo vk_device_create_info = {};
    vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vk_device_create_info.queueCreateInfoCount = 1;
    vk_device_create_info.pQueueCreateInfos = &vk_device_queue_create_info;
    vk_device_create_info.enabledExtensionCount = 1;
    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    vk_device_create_info.ppEnabledExtensionNames = device_extensions;

    if (vkCreateDevice(vk_physical_device, &vk_device_create_info, nullptr, &vk_device) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Something bad happened to vk_device.");

    if (!vk_device)
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");

    vkGetDeviceQueue(vk_device, selected_queue_family_index, 0, &vk_queue);
    if (!vk_queue)
        VKL_EXIT_WITH_ERROR("No VkQueue selected or handle not assigned.");
#pragma endregion

#pragma region swapchain
    uint32_t queueFamilyIndexCount = 0u;
    std::vector<uint32_t> queueFamilyIndices;
    VkSurfaceCapabilitiesKHR surface_capabilities = getPhysicalDeviceSurfaceCapabilities(vk_physical_device, vk_surface);
    
    // Build the swapchain config struct:
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageArrayLayers = 1u;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    } else {
        std::cout << "Warning: Automatic Testing might fail, VK_IMAGE_USAGE_TRANSFER_SRC_BIT image usage is not supported" << std::endl;
    }
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;

    VkSurfaceFormatKHR surface_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    
    VkExtent2D window_dimensions;
    window_dimensions.width = window_width;
    window_dimensions.height = window_height;
    
    swapchain_create_info.imageExtent = window_dimensions;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Create the swapchain using vkCreateSwapchainKHR and assign its handle to vk_swapchain!
    if (vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &vk_swapchain) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("VkSwapchainKHR does not have VK_SUCCESS status.");
    
    if (!vk_swapchain)
        VKL_EXIT_WITH_ERROR("No VkSwapchainKHR created or handle not assigned.");

    uint32_t swapchainCount;
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchainCount, nullptr);
    
    std::vector< VkImage> vk_swapchain_images(swapchainCount);
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchainCount, vk_swapchain_images.data());
#pragma endregion

#pragma region GCG Framework init
    VkClearValue vk_clear_value = {};
    vk_clear_value.depthStencil.depth = 1.0f;
    vk_clear_value.depthStencil.stencil = 0;

    VkImage depth_buffer = vklCreateDeviceLocalImageWithBackingMemory(
        vk_physical_device,
        vk_device,
        window_dimensions.width,
        window_dimensions.height,
        VK_FORMAT_D32_SFLOAT, // no stencil !!! note for future self
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    // Gather swapchain config as required by the framework:
    VklSwapchainConfig swapchain_config = {};
    swapchain_config.swapchainHandle = vk_swapchain;
    swapchain_config.imageExtent = window_dimensions;
    swapchain_config.swapchainImages.resize(swapchainCount);
    for (uint32_t i = 0; i < swapchainCount; i++) {
        auto& fbuffComp = swapchain_config.swapchainImages[i];

        fbuffComp.colorAttachmentImageDetails.imageHandle = vk_swapchain_images[i];
        fbuffComp.colorAttachmentImageDetails.imageFormat = surface_format.format;
        fbuffComp.colorAttachmentImageDetails.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        fbuffComp.colorAttachmentImageDetails.clearValue = {};
        fbuffComp.colorAttachmentImageDetails.clearValue.color = { {0.14f, 0.4f, 0.37f, 1.0f} };
        
        fbuffComp.depthAttachmentImageDetails.imageHandle = depth_buffer;
        fbuffComp.depthAttachmentImageDetails.imageFormat = VK_FORMAT_D32_SFLOAT;
        fbuffComp.depthAttachmentImageDetails.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        fbuffComp.depthAttachmentImageDetails.clearValue = vk_clear_value;
    }

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config))
        VKL_EXIT_WITH_ERROR("Failed to init framework");
#pragma endregion

#pragma region custom graphics pipeline config
    VklGraphicsPipelineConfig config = {};
    std::string vertexPath = gcgLoadShaderFilePath("assets/shader/testVertex.vert");
    config.vertexShaderPath = vertexPath.c_str();
    
    std::string fragPath = gcgLoadShaderFilePath("assets/shader/testFrag.frag");
    config.fragmentShaderPath = fragPath.c_str();
    
    config.polygonDrawMode = VK_POLYGON_MODE_FILL;
    config.triangleCullingMode = VK_CULL_MODE_NONE;

    config.vertexInputBuffers.resize(1, {});
    config.vertexInputBuffers[0].binding = 0;
    config.vertexInputBuffers[0].stride = 3 * sizeof(float);
    config.vertexInputBuffers[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    config.inputAttributeDescriptions.resize(1, {});
    config.inputAttributeDescriptions[0].location = 0;
    config.inputAttributeDescriptions[0].binding = 0;
    config.inputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    config.inputAttributeDescriptions[0].offset = 0;

    config.descriptorLayout.resize(1, {});
    config.descriptorLayout[0].binding = 0;
    config.descriptorLayout[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    config.descriptorLayout[0].descriptorCount = 1;
    config.descriptorLayout[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    config.descriptorLayout[0].pImmutableSamplers = nullptr;
#pragma endregion

#pragma region uniform buffer struct
    struct UniformBufferObject {
        glm::vec4 color;
        glm::mat4 view_proj;
    };

    UniformBufferObject ubo_teapot1 = {};
    ubo_teapot1.color = { 0.49f, 0.06f, 0.22f, 1.0f };
    glm::mat4 rotate1 = glm::rotate(glm::mat4(1.0f), (float)glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 translate1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
    glm::mat4 model1 = translate1 * rotate1;
    ubo_teapot1.view_proj = view_projection * model1;

    UniformBufferObject ubo_teapot2 = {};
    ubo_teapot2.color = { 0.0f, 0.13f, 0.31f, 1.0f };
    glm::mat4 scale2 = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 1.0f));
    glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 1.0f, 0.0f));
    glm::mat4 model2 = translate2 * scale2;
    ubo_teapot2.view_proj = view_projection * model2;
    
    VkBuffer uniform_buffer1 = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(ubo_teapot1), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    vklCopyDataIntoHostCoherentBuffer(uniform_buffer1, &ubo_teapot1, sizeof(ubo_teapot1));

    VkBuffer uniform_buffer2 = vklCreateHostCoherentBufferWithBackingMemory(
        sizeof(ubo_teapot2), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    vklCopyDataIntoHostCoherentBuffer(uniform_buffer2, &ubo_teapot2, sizeof(ubo_teapot2));
#pragma endregion

#pragma region Uniform buffer
#pragma region descriptor pool
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 8;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 8;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(vk_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to create descriptor pool");
#pragma endregion
    
#pragma region descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(vk_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to create descriptor set layout");
#pragma endregion
    
#pragma region allocate the descriptor set for teapot1 and teapot 2
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet1 = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet1) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 1");

    VkDescriptorSet descriptorSet2 = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet2) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 2");
#pragma endregion

    // write uniform buffer into descriptor set
#pragma region write uniform buffer for teapot 1
    VkDescriptorBufferInfo bufferInfo1 = {};
    bufferInfo1.buffer = uniform_buffer1; 
    bufferInfo1.offset = 0;
    bufferInfo1.range = sizeof(ubo_teapot1);

    VkWriteDescriptorSet write1 = {};
    write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write1.dstSet = descriptorSet1;
    write1.dstBinding = 0;
    write1.dstArrayElement = 0;
    write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write1.descriptorCount = 1;
    write1.pBufferInfo = &bufferInfo1;

    vkUpdateDescriptorSets(vk_device, 1, &write1, 0, nullptr);
#pragma endregion

#pragma region write uniform buffer for teapot 2
    VkDescriptorBufferInfo bufferInfo2 = {};
    bufferInfo2.buffer = uniform_buffer2;
    bufferInfo2.offset = 0;
    bufferInfo2.range = sizeof(ubo_teapot2);

    VkWriteDescriptorSet write2 = {};
    write2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write2.dstSet = descriptorSet2;
    write2.dstBinding = 0;
    write2.dstArrayElement = 0;
    write2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write2.descriptorCount = 1;
    write2.pBufferInfo = &bufferInfo2;

    vkUpdateDescriptorSets(vk_device, 1, &write2, 0, nullptr);
#pragma endregion

    VkPipeline vk_pipeline = VK_NULL_HANDLE;
    vk_pipeline = vklCreateGraphicsPipeline(config);

    if (vk_pipeline == VK_NULL_HANDLE) {
        VKL_EXIT_WITH_ERROR("Failed to init the custom pipeline.");
    }

    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);
#pragma endregion

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        vklWaitForNextSwapchainImage();
        
        if (reset_camera) main_camera.reset_camera_state();
        main_camera.update_camera_zoom(scroll_delta); scroll_delta = 0;

        double deltax = {}; double deltay = {}; get_mouse_delta(window, deltax, deltay);
        glm::mat4 proj_view = main_camera.get_proj_view_matrix(deltax, deltay);
        
        ubo_teapot1.view_proj = proj_view * model1;
        vklCopyDataIntoHostCoherentBuffer(uniform_buffer1, &ubo_teapot1, sizeof(ubo_teapot1));

        ubo_teapot2.view_proj = proj_view * model2;
        vklCopyDataIntoHostCoherentBuffer(uniform_buffer2, &ubo_teapot2, sizeof(ubo_teapot2));
        
        vklStartRecordingCommands();
        
        gcgDrawTeapot(vk_pipeline, descriptorSet1);
        gcgDrawTeapot(vk_pipeline, descriptorSet2);
        
        vklEndRecordingCommands();
        vklPresentCurrentSwapchainImage();


        // GCG specific code
        if (cmdline_args.run_headless) {
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename) {
                screenshot_filename = cmdline_args.filename;
            }
            gcgSaveScreenshot(screenshot_filename, vk_swapchain_images[idx],
                window_width, window_height, surface_format.format, vk_device, vk_physical_device,
                vk_queue, selected_queue_family_index);
            break;
        }
    }

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);

#pragma region cleanup
    vkDeviceWaitIdle(vk_device);

    vklDestroyGraphicsPipeline(vk_pipeline);

    vkDestroyDescriptorPool(vk_device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vk_device, descriptorSetLayout, nullptr);
    
    vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer1);
    vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer2);
    vklDestroyDeviceLocalImageAndItsBackingMemory(depth_buffer);
    
    gcgDestroyFramework();
    
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
    glfwDestroyWindow(window);
#pragma endregion

    return EXIT_SUCCESS;
}

#pragma region GCG helpers body
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface) {
    // Iterate over all the physical devices and select one that satisfies all our requirements.
    // Our requirements are:
    //  - Must support a queue that must have both, graphics and presentation capabilities
    for (uint32_t physical_device_index = 0u; physical_device_index < physical_device_count; ++physical_device_index) {
        // Get the number of different queue families:
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, nullptr);

        // Get the queue families' data:
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, queue_families.data());

        for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index) {
            // If this physical device supports a queue family which supports both, graphics and presentation
            //  => select this physical device
            if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                // This queue supports graphics! Let's see if it also supports presentation:
                VkBool32 presentation_supported;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[physical_device_index], queue_family_index, surface, &presentation_supported);

                if (VK_TRUE == presentation_supported) {
                    // We've found a suitable physical device
                    return physical_device_index;
                }
            }
        }
    }
    VKL_EXIT_WITH_ERROR("Unable to find a suitable physical device that supports graphics and presentation on the same queue.");
}

uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface) {
    return selectPhysicalDeviceIndex(physical_devices.data(), static_cast<uint32_t>(physical_devices.size()), surface);
}

uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    // Get the number of different queue families for the given physical device:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    // Get the queue families' data:
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    // copy paste from function above
    for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index) {
        // If this physical device supports a queue family which supports both, graphics and presentation
        //  => select this physical device
        if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            // This queue supports graphics! Let's see if it also supports presentation:
            VkBool32 presentation_supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface, &presentation_supported);

            if (VK_TRUE == presentation_supported) {
                // We've found a suitable physical device
                return queue_family_index;
            }
        }
    }

    VKL_EXIT_WITH_ERROR("Unable to find a suitable queue family that supports graphics and presentation on the same queue.");
}

VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkResult result;

    uint32_t surface_format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());
    VKL_CHECK_VULKAN_ERROR(result);

    if (surface_formats.empty()) {
        VKL_EXIT_WITH_ERROR("Unable to find supported surface formats.");
    }

    // Prefer a RGB8/sRGB format; If we are unable to find such, just return any:
    for (const VkSurfaceFormatKHR& f : surface_formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB) && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }

    return surface_formats[0];
}

VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    VKL_CHECK_VULKAN_ERROR(result);
    return surface_capabilities;
}

VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    return getPhysicalDeviceSurfaceCapabilities(physical_device, surface).currentTransform;
}
#pragma endregion
