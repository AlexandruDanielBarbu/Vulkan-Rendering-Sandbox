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

const float PI = glm::pi<float>();

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
struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
};

bool reset_camera = false;

bool wireframe_mode = false;
std::vector<size_t> cullModes = {
    VK_CULL_MODE_NONE,
    VK_CULL_MODE_FRONT_BIT,
    VK_CULL_MODE_BACK_BIT
};
size_t selectedCullMode = 0;

bool normalMode = false;
bool fresnelMode = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
            reset_camera = true;
    if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
            reset_camera = false;

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            wireframe_mode = !wireframe_mode;
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
            selectedCullMode++;
            selectedCullMode %= cullModes.size();
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
            normalMode = !normalMode;

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
            fresnelMode = !fresnelMode;
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

        //std::cout << '(' << deltax << ' ' << deltay << ')' << std::endl;
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

void alexd_drawTeapot(const VkPipeline vk_pipeline, const VkDescriptorSet& descriptorSet) {
        // command buffer and pipeline layout
        VkCommandBuffer commandBuffer = vklGetCurrentCommandBuffer();
        VkPipelineLayout pipelineLayout = vklGetLayoutForPipeline(vk_pipeline);

        // bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // bind pipeline
        vklCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        // bind vertex buffer
        VkBuffer vbuff = gcgGetTeapotPositionsBuffer();
        VkDeviceSize  offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbuff, offsets);

        // bind vertex index buffer
        VkBuffer ibuff = gcgGetTeapotIndicesBuffer();
        vkCmdBindIndexBuffer(commandBuffer, ibuff, 0, VK_INDEX_TYPE_UINT32);

        // draw
        uint32_t numTeapotIndices = gcgGetNumTeapotIndices();
        vkCmdDrawIndexed(commandBuffer, numTeapotIndices, 1, 0, 0, 0);
}

void alexd_drawObject(const VkPipeline vk_pipeline, const VkDescriptorSet& descriptorSet, const VkBuffer object_vbuff, const VkBuffer object_ibuff, const uint32_t numIndices) {
        // command buffer and pipeline layout
        VkCommandBuffer commandBuffer = vklGetCurrentCommandBuffer();
        VkPipelineLayout pipelineLayout = vklGetLayoutForPipeline(vk_pipeline);

        // bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // bind pipeline
        vklCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        // bind vertex buffer
        VkDeviceSize  offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_vbuff, offsets);

        // bind vertex index buffer
        vkCmdBindIndexBuffer(commandBuffer, object_ibuff, 0, VK_INDEX_TYPE_UINT32);

        // draw
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
}

void populate_pipeline_configs(
        VklGraphicsPipelineConfig& config,

        const std::string& vertexShader,
        const std::string& fragmentShader,

        const VkPolygonMode drawMode,
        const VkCullModeFlagBits cullMode)
{
        config = {};

        config.vertexShaderPath = vertexShader.c_str();
        config.fragmentShaderPath = fragmentShader.c_str();

        // wireframe or not
        config.polygonDrawMode = drawMode;

        // cull mode
        config.triangleCullingMode = cullMode;

        // layout of my vertex buffer
        config.vertexInputBuffers.resize(1, {});
        config.vertexInputBuffers[0] = {
                0,                              // .binding 
                sizeof(Vertex),                 // .stride 
                VK_VERTEX_INPUT_RATE_VERTEX     // .inputRate 
        };

        // on location 0 in my vertex buffer
        config.inputAttributeDescriptions.resize(3, {});
        config.inputAttributeDescriptions[0] = {
                0,                                   // .location 
                0,                                   // .binding 
                VK_FORMAT_R32G32B32_SFLOAT,          // .format 
                offsetof(Vertex, pos)                // .offset 
        };

        config.inputAttributeDescriptions[1] = {
                1,                                   // .location 
                0,                                   // .binding 
                VK_FORMAT_R32G32B32_SFLOAT,          // .format 
                offsetof(Vertex, color)              // .offset 
        };

        config.inputAttributeDescriptions[2] = {
                2,                                   // .location 
                0,                                   // .binding 
                VK_FORMAT_R32G32B32_SFLOAT,          // .format 
                offsetof(Vertex, normal)             // .offset 
        };

        // ubo setup
        config.descriptorLayout.resize(1, {});
        config.descriptorLayout[0] = {
                0,                                                              // .binding 
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                              // .descriptorType 
                1,                                                              // .descriptorCount 
                VK_SHADER_STAGE_VERTEX_BIT,  // only vertex shader visible      // .stageFlags 
                nullptr                                                         // .pImmutableSamplers 
        };
}

std::vector<VkPipeline> pipeline_factory(
        const std::string& vertexShader,
        const std::string& fragmentShader)
{
        std::vector<VkPolygonMode> drawMode = {
                VK_POLYGON_MODE_FILL,
                VK_POLYGON_MODE_LINE
        };

        std::vector<VkCullModeFlagBits> cullMode = {
                VK_CULL_MODE_NONE,
                VK_CULL_MODE_FRONT_BIT,
                VK_CULL_MODE_BACK_BIT
        };

        std::vector<VkPipeline> pipelines;

        for (auto dmode : drawMode) {
                for (auto cmode : cullMode) {
                        VklGraphicsPipelineConfig config = {};
                        populate_pipeline_configs(config, vertexShader, fragmentShader, dmode, cmode);

                        VkPipeline pipeline = vklCreateGraphicsPipeline(config);

                        pipelines.push_back(pipeline);
                }
        }

        return pipelines;
}

VkPipeline choose_pipeline(const std::vector<VkPipeline>& pipelines) {
        if (!wireframe_mode) {
                if (cullModes[selectedCullMode] == VK_CULL_MODE_NONE) return pipelines[0];
                if (cullModes[selectedCullMode] == VK_CULL_MODE_FRONT_BIT) return pipelines[1];
                if (cullModes[selectedCullMode] == VK_CULL_MODE_BACK_BIT) return pipelines[2];
                
        }

        if (cullModes[selectedCullMode] == VK_CULL_MODE_NONE) return pipelines[3];
        if (cullModes[selectedCullMode] == VK_CULL_MODE_FRONT_BIT) return pipelines[4];
        if (cullModes[selectedCullMode] == VK_CULL_MODE_BACK_BIT) return pipelines[5];

        return pipelines[0];
}

void destroy_pipelines(std::vector<VkPipeline>& pipelines) {
        for (auto pipeline : pipelines) {
                vklDestroyGraphicsPipeline(pipeline);
        }
}

#pragma endregion

#pragma region alexd defined classes
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
                        camera_yaw += deltax * arcball_sensitivity;
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
                        cos(camera_pitch) * sin(-camera_yaw),
                        sin(camera_pitch),
                        cos(camera_pitch) * cos(-camera_yaw)
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
        }
};

class Object {
public:
        const void* get_vbuff() const {
                return vbuff.data();
        }
        size_t get_vbuff_size() const {
                return vbuff.size();
        }

        const void* get_ibuff() const {
                return ibuff.data();
        }
        size_t get_ibuff_size() const {
                return ibuff.size();
        }

        VkBuffer get_vk_vbuff() {
                return vk_vbuff;
        };
        VkBuffer get_vk_ibuff() {
                return vk_ibuff;
        };

        void destroyVkBuffers() {
                vklDestroyHostCoherentBufferAndItsBackingMemory(vk_vbuff);
                vklDestroyHostCoherentBufferAndItsBackingMemory(vk_ibuff);
        }
protected:
        std::vector<Vertex> vbuff;
        std::vector<uint32_t> ibuff;

        VkBuffer vk_vbuff;
        VkBuffer vk_ibuff;

        void populate_VkBuffers() {
                vk_vbuff = vklCreateHostCoherentBufferAndUploadData(
                        get_vbuff(), get_vbuff_size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                );

                vk_ibuff = vklCreateHostCoherentBufferAndUploadData(
                        get_ibuff(), get_ibuff_size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                );
        }
        //glm::mat4 object_matrix = glm::mat4(1.0f);
};

class Cube : public Object {
public:
        Cube(const float width = 1, const float height = 1, const float depth = 1, const glm::vec3& color = { 0, 0, 0 }, const glm::vec3& origin = { 0, 0, 0 }) {
                vbuff = {
                        // top face verts
                        {origin + glm::vec3(-width / 2, height / 2, -depth / 2), color, glm::vec3(0, 1, 0)},  // A
                        {origin + glm::vec3( width / 2, height / 2, -depth / 2), color, glm::vec3(0, 1, 0)},  // B
                        {origin + glm::vec3( width / 2, height / 2,  depth / 2), color, glm::vec3(0, 1, 0)},  // C
                        {origin + glm::vec3(-width / 2, height / 2,  depth / 2), color, glm::vec3(0, 1, 0)},  // D

                        // front face (one facing +z axis)
                        {origin + glm::vec3(-width / 2, -height / 2, depth / 2), color, glm::vec3(0, 0, 1)},  // P
                        {origin + glm::vec3( width / 2, -height / 2, depth / 2), color, glm::vec3(0, 0, 1)},  // Q
                        {origin + glm::vec3( width / 2,  height / 2, depth / 2), color, glm::vec3(0, 0, 1)},  // D
                        {origin + glm::vec3(-width / 2,  height / 2, depth / 2), color, glm::vec3(0, 0, 1)},  // C

                        // left face
                        {origin + glm::vec3(width / 2, -height / 2, -depth / 2), color, glm::vec3(1, 0, 0)},  // N
                        {origin + glm::vec3(width / 2, -height / 2,  depth / 2), color, glm::vec3(1, 0, 0)},  // P
                        {origin + glm::vec3(width / 2,  height / 2,  depth / 2), color, glm::vec3(1, 0, 0)},  // C
                        {origin + glm::vec3(width / 2,  height / 2, -depth / 2), color, glm::vec3(1, 0, 0)},  // B

                        // back face (one facing away from +z axis)
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), color, glm::vec3(0, 0, -1)},  // M
                        {origin + glm::vec3( width / 2, -height / 2, -depth / 2), color, glm::vec3(0, 0, -1)},  // N
                        {origin + glm::vec3( width / 2,  height / 2, -depth / 2), color, glm::vec3(0, 0, -1)},  // B
                        {origin + glm::vec3(-width / 2,  height / 2, -depth / 2), color, glm::vec3(0, 0, -1)},  // A

                        // right face
                        {origin + glm::vec3(-width / 2, -height / 2,  depth / 2), color, glm::vec3(-1, 0, 0)},  // Q
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), color, glm::vec3(-1, 0, 0)},  // M
                        {origin + glm::vec3(-width / 2,  height / 2, -depth / 2), color, glm::vec3(-1, 0, 0)},  // A
                        {origin + glm::vec3(-width / 2,  height / 2,  depth / 2), color, glm::vec3(-1, 0, 0)},  // D

                        // bottom face verts
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), color, glm::vec3(0, -1, 0)},  // M
                        {origin + glm::vec3( width / 2, -height / 2, -depth / 2), color, glm::vec3(0, -1, 0)},  // N
                        {origin + glm::vec3( width / 2, -height / 2,  depth / 2), color, glm::vec3(0, -1, 0)},  // P
                        {origin + glm::vec3(-width / 2, -height / 2,  depth / 2), color, glm::vec3(0, -1, 0)}   // Q
                };

                ibuff = {
                        // top
                        3, 2, 1,
                        3, 1, 0,

                        // front
                        4, 5, 6,
                        4, 6, 7,

                        // left
                        11, 10, 9,
                        11, 9, 8,

                        // back
                        15, 14, 13,
                        15, 13, 12,

                        // right
                        19, 18, 17,
                        19, 17, 16,

                        // bottom
                        20, 21, 23,
                        21, 22, 23
                };
        
                populate_VkBuffers();
        }
};

class CornellBox : public Object {
public:
        CornellBox(const float width = 1, const float height = 1, const float depth = 1, const glm::vec3& origin = { 0, 0, 0 }) {
                // TODO i might need to flip the normals here
                vbuff = {
                        // top verts
                        {origin + glm::vec3(-width / 2, height / 2, -depth / 2), topFaceColor, glm::vec3(0, -1, 0)},
                        {origin + glm::vec3( width / 2, height / 2, -depth / 2), topFaceColor, glm::vec3(0, -1, 0)},
                        {origin + glm::vec3( width / 2, height / 2,  depth / 2), topFaceColor, glm::vec3(0, -1, 0)},
                        {origin + glm::vec3(-width / 2, height / 2,  depth / 2), topFaceColor, glm::vec3(0, -1, 0)},

                        // bottom verts
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), bottomFaceColor, glm::vec3(0, 1, 0)},
                        {origin + glm::vec3( width / 2, -height / 2, -depth / 2), bottomFaceColor, glm::vec3(0, 1, 0)},
                        {origin + glm::vec3( width / 2, -height / 2,  depth / 2), bottomFaceColor, glm::vec3(0, 1, 0)},
                        {origin + glm::vec3(-width / 2, -height / 2,  depth / 2), bottomFaceColor, glm::vec3(0, 1, 0)},

                        // left verts
                        {origin + glm::vec3(-width / 2, -height / 2,  depth / 2), leftFaceColor, glm::vec3(1, 0, 0)},
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), leftFaceColor, glm::vec3(1, 0, 0)},
                        {origin + glm::vec3(-width / 2,  height / 2, -depth / 2), leftFaceColor, glm::vec3(1, 0, 0)},
                        {origin + glm::vec3(-width / 2,  height / 2,  depth / 2), leftFaceColor, glm::vec3(1, 0, 0)},

                        // right verts
                        {origin + glm::vec3(width / 2, -height / 2, -depth / 2), rightFaceColor, glm::vec3(-1, 0, 0)},
                        {origin + glm::vec3(width / 2, -height / 2,  depth / 2), rightFaceColor, glm::vec3(-1, 0, 0)},
                        {origin + glm::vec3(width / 2,  height / 2,  depth / 2), rightFaceColor, glm::vec3(-1, 0, 0)},
                        {origin + glm::vec3(width / 2,  height / 2, -depth / 2), rightFaceColor, glm::vec3(-1, 0, 0)},

                        // back verts
                        {origin + glm::vec3( width / 2, -height / 2, -depth / 2), backFaceColor, glm::vec3(0, 0, 1)},
                        {origin + glm::vec3(-width / 2, -height / 2, -depth / 2), backFaceColor, glm::vec3(0, 0, 1)},
                        {origin + glm::vec3(-width / 2,  height / 2, -depth / 2), backFaceColor, glm::vec3(0, 0, 1)},
                        {origin + glm::vec3( width / 2,  height / 2, -depth / 2), backFaceColor, glm::vec3(0, 0, 1)}
                };

                ibuff = {
                        // TOP (0,1,2,3)
                        1, 3, 0,
                        1, 2, 3,

                        // BOTTOM (4,5,6,7)
                        4, 6, 5,
                        4, 7, 6,

                        // LEFT (12,13,14,15)
                        12, 13, 14,
                        12, 14, 15,

                        // RIGHT (8,9,10,11)
                        9, 11, 8,
                        9, 10, 11,

                        // BACK (16,17,18,19)
                        16, 18, 17,
                        16, 19, 18
                };

                populate_VkBuffers();
        }

private:
        glm::vec3 topFaceColor    = glm::vec3(0.96f, 0.93f, 0.85f);
        glm::vec3 bottomFaceColor = glm::vec3(0.64f, 0.64f, 0.64f);
        glm::vec3 leftFaceColor   = glm::vec3(0.49f, 0.06f, 0.22f);
        glm::vec3 rightFaceColor  = glm::vec3(0.00f, 0.13f, 0.31f);
        glm::vec3 backFaceColor   = glm::vec3(0.76f, 0.74f, 0.68f);
};

class Cylinder : public Object {
public:
        Cylinder(const float radius = 1.0f, const float height = 1.0f, const int subdivisions = 10, const glm::vec3& color = { 0, 0, 0 }, const glm::vec3& origin = {0, 0, 0}) {
                float step = (2 * PI) / subdivisions;
                
                // Top vert
                vbuff.push_back({ origin + glm::vec3(0, height / 2, 0), color, glm::vec3(0, 1, 0)});

                // Top ring
                for (int i = 0; i < subdivisions; i++) {
                        float phi = i * step;
                        float x = radius * cos(phi);
                        float z = radius * sin(phi);
                        
                        vbuff.push_back({ origin + glm::vec3(x, height / 2, z), color, glm::vec3(0, 1, 0)});
                }

                // 2nd Top ring
                for (int i = 0; i < subdivisions; i++) {
                        float phi = i * step;
                        float x = radius * cos(phi);
                        float z = radius * sin(phi);

                        vbuff.push_back({ origin + glm::vec3(x, height / 2, z), color, glm::normalize(glm::vec3(x, height / 2, z) - glm::vec3(0, height / 2, 0)) });
                }

                // 2nd Bottom ring
                for (int i = 0; i < subdivisions; i++) {
                        float phi = i * step;
                        float x = radius * cos(phi);
                        float z = radius * sin(phi);

                        vbuff.push_back({ origin + glm::vec3(x, -height / 2, z), color, glm::normalize(glm::vec3(x, -height / 2, z) - glm::vec3(0, -height / 2, 0)) });
                }

                // Bottom ring
                for (int i = 0; i < subdivisions; i++) {
                        float phi = i * step;
                        float x = radius * cos(phi);
                        float z = radius * sin(phi);

                        vbuff.push_back({ origin + glm::vec3(x, -height / 2, z), color, glm::vec3(0, -1, 0) });
                }

                // Bottom vert
                vbuff.push_back({ origin + glm::vec3(0, -height / 2, 0), color, glm::vec3(0, -1, 0) });


                // top face
                for (int i = 0; i < subdivisions; i++) {
                        int iNext = (i + 1) % subdivisions;
                        ibuff.push_back(0);
                        ibuff.push_back(1 + iNext);
                        ibuff.push_back(1 + i);
                }

                // Lateral faces
                int top2Start = 1 + subdivisions;
                int bottom2Start = top2Start + subdivisions;

                for (int j = 0; j < subdivisions; j++) {
                        int jNext = (j + 1) % subdivisions;

                        int top0 = top2Start + j;
                        int top1 = top2Start + jNext;
                        int bot0 = bottom2Start + j;
                        int bot1 = bottom2Start + jNext;

                        // triangle 1
                        ibuff.push_back(top0);
                        ibuff.push_back(top1);
                        ibuff.push_back(bot0);

                        // triangle 2
                        ibuff.push_back(top1);
                        ibuff.push_back(bot1);
                        ibuff.push_back(bot0);
                }

                // bottom face
                int last_vert = vbuff.size() - 1;
                int last_ring_start = last_vert - subdivisions;

                for (int j = 0; j < subdivisions; j++) {
                        int jNext = (j + 1) % subdivisions;
                        ibuff.push_back(last_vert);
                        ibuff.push_back(last_ring_start + j);
                        ibuff.push_back(last_ring_start + jNext);
                }

                populate_VkBuffers();
        }
};

class Sphere : public Object {
public:
        Sphere(const float radius = 1.0f, const int latitude_subdivisions = 10, const int longitude_subdivisions = 10, const glm::vec3& color = { 0, 0, 0 }, const glm::vec3& origin = {}) {
                float latitude_step = PI / latitude_subdivisions;
                float longitude_step = (2 * PI) / longitude_subdivisions;

                // bottom vert
                vbuff.push_back({ origin + glm::vec3(0, radius, 0), color, glm::vec3(0, -1, 0)});

                // ring verts
                for (int i = 1; i < latitude_subdivisions; i++) {
                        float theta = i * latitude_step;

                        for (int j = 0; j < longitude_subdivisions; j++) {
                                float phi = j * longitude_step;

                                vbuff.push_back({ origin + glm::vec3(
                                        radius * sin(theta) * cos(phi),
                                        radius * cos(theta),
                                        radius * sin(theta) * sin(phi)),
                                        color,
                                        glm::normalize(glm::vec3(
                                        radius * sin(theta) * cos(phi),
                                        radius * cos(theta),
                                        radius * sin(theta) * sin(phi)) - origin)});
                        }
                }

                // top vert
                vbuff.push_back({ origin + glm::vec3(0, -radius, 0), color, glm::vec3(0, 1, 0) });


                // bottom cap
                for (int j = 0; j < longitude_subdivisions; j++) {
                        int jNext = (j + 1) % longitude_subdivisions;
                        ibuff.push_back(0);
                        ibuff.push_back(1 + jNext);
                        ibuff.push_back(1 + j);
                }

                // lateral faces
                // nasty mesh bug if i let this loop run up to  `< latitude_subdivisions - 1`
                for (int i = 0; i < latitude_subdivisions - 2; i++) {
                        int ringStart = 1 + i * longitude_subdivisions;
                        int nextRingStart = ringStart + longitude_subdivisions;

                        for (int j = 0; j < longitude_subdivisions; j++) {
                                int jNext = (j + 1) % longitude_subdivisions;

                                // Triangle 1
                                ibuff.push_back(ringStart + j);
                                ibuff.push_back(ringStart + jNext);
                                ibuff.push_back(nextRingStart + j);

                                // Triangle 2
                                ibuff.push_back(ringStart + jNext);
                                ibuff.push_back(nextRingStart + jNext);
                                ibuff.push_back(nextRingStart + j);
                        }
                }


                // top cap
                int top_vert = vbuff.size() - 1;
                int top_ring_start = top_vert - longitude_subdivisions;

                for (int j = 0; j < longitude_subdivisions; j++) {
                        int jNext = top_ring_start + (j + 1) % longitude_subdivisions;
                        ibuff.push_back(top_vert);
                        ibuff.push_back(top_ring_start + j);
                        ibuff.push_back(jNext);
                }

                populate_VkBuffers();
        }
};

class Bezier : public Object {
public:
        Bezier(std::vector<glm::vec3>& controlPoints, const float radius, const int segments, const int subdivisions, const glm::vec3& color = { 0, 0, 0 }) {
                // Top verts facing normally away from the mesh
                {
                        glm::vec3 topTangent = glm::normalize(derivateBezierCurve(controlPoints, 0));
                        glm::vec3 up, forward, right;

                        up = topTangent;
                        forward = glm::vec3(0, 0, 1);
                        right = glm::normalize(glm::cross(up, forward));

                        vbuff.push_back({ controlPoints[0], color, -up });


                        float vertStep = (2 * PI) / subdivisions;
                        for (int vertCount = 0; vertCount < subdivisions; vertCount++) {
                                float phi = vertCount * vertStep;

                                glm::vec3 point(
                                        radius * cos(phi),
                                        0,
                                        radius * sin(phi)
                                );

                                glm::mat3 orientation(right, up, forward);

                                point = orientation * point;

                                point += controlPoints[0];

                                vbuff.push_back({ point, color,  glm::normalize(point - controlPoints[0]) });
                        }
                }

                
                // generate rings
                float step = 1.0f / segments;
                for (int i = 0; i <= segments; i++) {
                        float t = i * step;

                        // generate point on curve
                        glm::vec3 circle_origin = DeCasteljau(controlPoints, t);
                        glm::vec3 tangent = glm::normalize(derivateBezierCurve(controlPoints, t));
                        
                        glm::vec3 up, forward, right;

                        up = tangent;
                        forward = glm::vec3(0, 0, 1);
                        right = glm::normalize(glm::cross(up, forward));
                        
                        // generate circle in the new x'o'z' plane
                        float vertStep = (2 * PI) / subdivisions;
                        for (int vertCount = 0; vertCount < subdivisions; vertCount++) {
                                float phi = vertCount * vertStep;
                                
                                glm::vec3 point(
                                        radius * cos(phi),
                                        0,
                                        radius * sin(phi)
                                );

                                glm::mat3 orientation(right, up, forward);

                                point = orientation * point;

                                point += circle_origin;

                                vbuff.push_back({ point, color,  glm::normalize(point - circle_origin) });
                        }
                }


                {
                        glm::vec3 topTangent = glm::normalize(derivateBezierCurve(controlPoints, 1));
                        glm::vec3 up, forward, right;

                        up = topTangent;
                        forward = glm::vec3(0, 0, 1);
                        right = glm::normalize(glm::cross(up, forward));

                        float vertStep = (2 * PI) / subdivisions;
                        for (int vertCount = 0; vertCount < subdivisions; vertCount++) {
                                float phi = vertCount * vertStep;

                                glm::vec3 point(
                                        radius * cos(phi),
                                        0,
                                        radius * sin(phi)
                                );

                                glm::mat3 orientation(right, up, forward);

                                point = orientation * point;

                                point += controlPoints[controlPoints.size() - 1];

                                vbuff.push_back({ point, color,  glm::normalize(point - controlPoints[controlPoints.size()-1])});
                        }

                        vbuff.push_back({ controlPoints[controlPoints.size() - 1], color, up });
                }

        
                // top face - unchanged
                for (int i = 0; i < subdivisions; i++) {
                        int iNext = (i + 1) % subdivisions;
                        ibuff.push_back(0);
                        ibuff.push_back(1 + i);
                        ibuff.push_back(1 + iNext);
                }

                // TODO this changes just like the cylinder
                // lateral faces
                // nasty mesh bug if i let this loop run up to  `< latitude_subdivisions - 1`
                for (int i = 1; i < segments + 1; i++) {
                        int ringStart = 1 + i * subdivisions;
                        int nextRingStart = 1 + (i + 1) * subdivisions;

                        for (int j = 0; j < subdivisions; j++) {
                                int jNext = (j + 1) % subdivisions;

                                // Triangle 1
                                ibuff.push_back(ringStart + j);
                                ibuff.push_back(nextRingStart + j);
                                ibuff.push_back(ringStart + jNext);

                                // Triangle 2
                                ibuff.push_back(ringStart + jNext);
                                ibuff.push_back(nextRingStart + j);
                                ibuff.push_back(nextRingStart + jNext);
                        }
                }

                // bottom face - unchanged
                int last_vert = vbuff.size() - 1;
                int last_ring_start = last_vert - subdivisions;

                for (int j = 0; j < subdivisions; j++) {
                        int jNext = last_ring_start + (j + 1) % subdivisions;
                        ibuff.push_back(last_vert);
                        ibuff.push_back(jNext);
                        ibuff.push_back(last_ring_start + j);
                }

                populate_VkBuffers();
        }
private:
        // DeCasteljau(t) -- does the alg for a nominated t
        glm::vec3 DeCasteljau(std::vector<glm::vec3>& og_points, const float t) {
                std::vector<glm::vec3> points = og_points;

                int n = og_points.size();
                for (int k = n - 1; k > 0; k--) {
                        for (int i = 0; i < k; i++) {
                                points[i] = points[i] * (1.0f - t) + points[i + 1] * t;
                        }
                }

                return points[0];
        }

        glm::vec3 derivateBezierCurve(std::vector<glm::vec3>& og_points, const float t) {
                int n = og_points.size() - 1;
                std::vector<glm::vec3> d_points(n);
                
                for (int i = 0; i < n; i++) {
                        d_points[i] = static_cast<float>(n) * (og_points[i + 1] - og_points[i]);
                }

                return DeCasteljau(d_points, t);
        }
};

class Torus : public Object {
public:
        Torus(float bigRadius, float smallRadius, int s, int n, glm::vec3 origin = { 0,0,0 }, glm::vec3 color = { 0,0,0 }) {
                // generate vbuff
                float r = smallRadius;
                float R = bigRadius;

                float torusSectionStep = (2 * PI) / s;
                float vertStep = (2 * PI) / n;

                for (int circleSegment = 0; circleSegment <= s; circleSegment++) {
                        float alfa = circleSegment * torusSectionStep;
                        if (circleSegment == s) alfa = 0.0f;

                        glm::vec3 forward(0, 0, 1);

                        glm::vec3 circleOrigin(
                                R * cos(alfa),
                                R * sin(alfa),
                                0
                        );

                        glm::vec3 right = glm::normalize(circleOrigin);

                        // the normal might point backwards here
                        glm::vec3 up = glm::normalize(glm::cross(forward, right));

                        for (int vertIndex = 0; vertIndex < n; vertIndex++) {
                                float beta = vertIndex * vertStep;

                                glm::vec3 point(
                                        r * cos(beta),
                                        0,
                                        r * sin(beta)
                                );

                                glm::mat3 orientation(right, up, forward);

                                point = orientation * point + circleOrigin;

                                //glm::vec3 point = r * cos(beta) * up + r * sin(beta) * right;
                                //point += circleOrigin;

                                vbuff.push_back({ point, color, glm::normalize(point - circleOrigin)});
                        }
                }

                // generate ibuff
                for (int i = 0; i < s; i++) {
                        int ringStart = i * n;
                        int nextRingStart = (i + 1) * n;

                        for (int j = 0; j < n; j++) {
                                int jNext = (j + 1) % n;

                                // Triangle 1
                                ibuff.push_back(ringStart + j);
                                ibuff.push_back(nextRingStart + j);
                                ibuff.push_back(ringStart + jNext);

                                // Triangle 2
                                ibuff.push_back(ringStart + jNext);
                                ibuff.push_back(nextRingStart + j);
                                ibuff.push_back(nextRingStart + jNext);
                        }
                }

                // create VkBuffers
                populate_VkBuffers();
        }
};

struct DirectionalLight_UniformBufferObject
{
        glm::vec4 color;
        glm::vec4 direction;
};

struct UniformBufferObject {
        glm::vec4 color;
        glm::mat4 object_matrix;
        glm::mat4 normalMatrix;
        glm::ivec4 drawModes;
};

class ObjectSettings
{
public:
        ObjectSettings() {
                ubo.color = glm::vec4(1.0f);
                ubo.object_matrix = glm::mat4(1.0f);
                ubo.normalMatrix = glm::mat4(1.0f);
        }

        ObjectSettings& set_color(const glm::vec4& color) {
                ubo.color = color;
                return *this;
        }

        ObjectSettings& apply_scale(const glm::vec3& scale) {
                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
                ubo.object_matrix = scaleMatrix * ubo.object_matrix;
                return *this;
        }
        ObjectSettings& apply_rotation(float rads, const glm::vec3& rot) {
                glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rads, rot);
                ubo.object_matrix = rotationMatrix * ubo.object_matrix;
                return *this;
        }
        ObjectSettings& apply_translation(const glm::vec3& tr) {
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), tr);
                ubo.object_matrix = translationMatrix * ubo.object_matrix;
                return *this;
        }
        
        ObjectSettings& compute_normals_matrix() {
                ubo.normalMatrix = glm::transpose(glm::inverse(ubo.object_matrix));
                return *this;
        }

        ObjectSettings& uploadDrawModes(int normals, int fresnel) {
                ubo.drawModes = glm::ivec4(normals, fresnel, 0, 0);
                return *this;
        }

        glm::mat4 get_object_matrix() {
                return ubo.object_matrix;
        }

        UniformBufferObject get_ubo() {
                return ubo;
        }
        ObjectSettings& reset() {
                ubo.object_matrix = glm::mat4(1.0f);
                return *this;
        }

        static VkBuffer makeVkBufferfromUBOandUpload(UniformBufferObject& ubo_object) {
                VkBuffer buffer = vklCreateHostCoherentBufferWithBackingMemory(
                        sizeof(ubo_object), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                );
                vklCopyDataIntoHostCoherentBuffer(buffer, &ubo_object, sizeof(ubo_object));

                return buffer;
        }

        static void updateDescriptorSets(VkBuffer& uniform_buffer1, VkDescriptorSet& descriptorSet1, VkDevice vk_device) {
                VkDescriptorBufferInfo bufferInfo1 = {};
                bufferInfo1.buffer = uniform_buffer1;
                bufferInfo1.offset = 0;
                bufferInfo1.range = sizeof(UniformBufferObject);

                VkWriteDescriptorSet write1 = {};
                write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write1.dstSet = descriptorSet1;
                write1.dstBinding = 0;
                write1.dstArrayElement = 0;
                write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write1.descriptorCount = 1;
                write1.pBufferInfo = &bufferInfo1;

                vkUpdateDescriptorSets(vk_device, 1, &write1, 0, nullptr);
        }

        static void updateDescriptorSets_DirectionalLight(VkBuffer& uniform_buffer1, VkDescriptorSet& descriptorSet1, VkDevice vk_device) {
                VkDescriptorBufferInfo bufferInfo1 = {};
                bufferInfo1.buffer = uniform_buffer1;
                bufferInfo1.offset = 0;
                bufferInfo1.range = sizeof(DirectionalLight_UniformBufferObject);

                VkWriteDescriptorSet write1 = {};
                write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write1.dstSet = descriptorSet1;
                write1.dstBinding = 0;
                write1.dstArrayElement = 0;
                write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write1.descriptorCount = 1;
                write1.pBufferInfo = &bufferInfo1;

                vkUpdateDescriptorSets(vk_device, 1, &write1, 0, nullptr);
        }

private:
        UniformBufferObject ubo;
};
#pragma endregion

int main(int argc, char** argv) {
        VKL_LOG(WELCOME_MSG);

        CMDLineArgs cmdline_args;
        gcgParseArgs(cmdline_args, argc, argv);

#pragma region App settings

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
        //std::string init_camera_filepath = "assets/settings/camera_front_right.ini";
        if (cmdline_args.init_camera)
                init_camera_filepath = cmdline_args.init_camera_filepath;

        Camera main_camera(init_camera_filepath, window_width, window_height);
#pragma endregion
#pragma region load render settings
        std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
        if (cmdline_args.init_renderer) {
                init_renderer_filepath = cmdline_args.init_renderer_filepath;
        }

        INIReader renderer_reader(init_renderer_filepath);
        wireframe_mode = renderer_reader.GetBoolean("renderer", "wireframe", false);
        bool with_backface_culling = renderer_reader.GetBoolean("renderer",
                "backface_culling", false);
        if (with_backface_culling) selectedCullMode = 2;
#pragma endregion
#pragma region load user preferred view mode
        init_renderer_filepath = "assets/settings/renderer_standard.ini";
        INIReader renderer_reader2(init_renderer_filepath);
        normalMode = renderer_reader2.GetBoolean("renderer", "normals", false);
        fresnelMode = renderer_reader2.GetBoolean("renderer", "fresnel", true);
#pragma endregion

#pragma endregion

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
        VkPhysicalDeviceFeatures features = {};
        features.fillModeNonSolid = VK_TRUE;

        // Get devices
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, physicalDevices.data());

        // remove physical devices that do not support wireframe mode
        for (size_t i = 0; i < physicalDevices.size(); i++)
        {
                VkPhysicalDeviceFeatures physDeviceFeatures = {};
                vkGetPhysicalDeviceFeatures(physicalDevices[i], &physDeviceFeatures);

                if (physDeviceFeatures.fillModeNonSolid == VK_FALSE) {
                        VKL_LOG("Removing a physical device not supporting fillModeNonSolid");
                        physicalDevices.erase(physicalDevices.begin() + i);
                }
        }

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
        vk_device_create_info.pEnabledFeatures = &features;
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
        }
        else {
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
        std::string cube_vertexShader_path = gcgLoadShaderFilePath("assets/shader/vertex/testVertex.vert");
        std::string cube_fragmentShader_path = gcgLoadShaderFilePath("assets/shader/fragment/testFrag.frag");

        std::string cornellBox_vertexShader_path = gcgLoadShaderFilePath("assets/shader/vertex/cornellBoxVert.vert");
        std::string cornellBox_fragmentShader_path = gcgLoadShaderFilePath("assets/shader/fragment/cornellBoxFrag.frag");
#pragma endregion

#pragma region uniform buffer struct
        ObjectSettings ubo_builder;
        
        // CornellBox settings
        UniformBufferObject ubo_cornellBox = ubo_builder
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_cornell = ubo_cornellBox.object_matrix;
        ubo_cornellBox.object_matrix = main_camera.get_proj_view_matrix() * model_cornell;
        VkBuffer cornell_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_cornellBox);


        ubo_builder.reset();


        // Cube settings
        UniformBufferObject ubo_cube = ubo_builder
                .apply_rotation((float)glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f))
                .apply_translation(0.6f * glm::vec3(-1, 0, 0))
                .apply_translation(0.9f * glm::vec3(0, -1, 0))
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_cube = ubo_cube.object_matrix;
        ubo_cube.object_matrix = main_camera.get_proj_view_matrix() * model_cube;
        VkBuffer cube_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_cube);


        ubo_builder.reset();


        // Cylinder settings
        UniformBufferObject ubo_cylinder = ubo_builder
                .apply_translation(0.6f * glm::vec3(1, 0, 0))
                .apply_translation(0.3f * glm::vec3(0, 1, 0))
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_cylinder = ubo_cylinder.object_matrix;
        ubo_cylinder.object_matrix = main_camera.get_proj_view_matrix() * model_cylinder;
        VkBuffer cylinder_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_cylinder);


        ubo_builder.reset();


        // Bezier Cylinder settings
        UniformBufferObject ubo_bezier_cyl = ubo_builder
                .apply_translation(0.6f * glm::vec3(-1, 0, 0))
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_bezier_cyl= ubo_bezier_cyl.object_matrix;
        ubo_bezier_cyl.object_matrix = main_camera.get_proj_view_matrix() * model_bezier_cyl;
        VkBuffer bezier_cylinder_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_bezier_cyl);


        ubo_builder.reset();


        // Sphere settings
        UniformBufferObject ubo_sphere = ubo_builder
                .apply_translation(0.6f * glm::vec3(1, 0, 0))
                .apply_translation(0.9f * glm::vec3(0, -1, 0))
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_shpere = ubo_sphere.object_matrix;
        ubo_sphere.object_matrix = main_camera.get_proj_view_matrix() * model_shpere;
        VkBuffer sphere_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_sphere);

        
        ubo_builder.reset();
        
        
        // Torus
        UniformBufferObject ubo_torus = ubo_builder
                .apply_rotation(glm::radians(45.0f), glm::vec3(1,0,0))
                .apply_scale(glm::vec3(1, 1.5f, 1))
                .compute_normals_matrix()
                .uploadDrawModes(normalMode, fresnelMode)
                .get_ubo();
        glm::mat4 model_torus = ubo_torus.object_matrix;
        ubo_torus.object_matrix = main_camera.get_proj_view_matrix() * model_torus;
        VkBuffer torus_uniform_buffer = ObjectSettings::makeVkBufferfromUBOandUpload(ubo_torus);

        DirectionalLight_UniformBufferObject dirLight;
        dirLight.color = glm::vec4(0, 0, 0, 0);
        dirLight.direction = glm::vec4(0, 0, 0, 0);
        VkBuffer dirLight_vk_buffer = vklCreateHostCoherentBufferAndUploadData(&dirLight, sizeof(DirectionalLight_UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

#pragma endregion

        CornellBox cornellBox = CornellBox(3, 3, 3);
        Cube cube = Cube(0.34f, 0.34f, 0.34f, { 0.0, 0.21, 0.16 });
        Cylinder cylinder = Cylinder(0.21f, 1.6, 20, { 0.75, 0.25, 0.01 });

        std::vector<glm::vec3> controlPoints{
                glm::vec3(-0.3f, 0.6f, 0.0f),
                glm::vec3(0.0f, 1.6f, 0.0f),
                glm::vec3(1.4f, 0.3f, 0.0f),
                glm::vec3(0.0f, 0.3f, 0.0f),
                glm::vec3(0.0f, -0.5f, 0.0f)
        };
        Bezier curve(controlPoints, 0.21f, 36, 20, { 0.75, 0.25, 0.01 });
        Sphere sphere = Sphere(0.26f, 18, 36, { 0.12, 0.12, 0.12 });
        Torus torus = Torus(1.1f, 0.1f, 32, 8, { 0,0,0 }, { 0.38, 0.67, 0.84 });

#pragma region Uniform buffer
#pragma region descriptor pool
    uint32_t poolCount = 32;
    
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = poolCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = poolCount;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(vk_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to create descriptor pool");
#pragma endregion
    
#pragma region descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(vk_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to create descriptor set layout");
#pragma endregion
    
#pragma region allocate the descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet_cornell = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_cornell) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 1");

    VkDescriptorSet descriptorSet_cube = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_cube) != VK_SUCCESS)
        VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 2");

    VkDescriptorSet descriptorSet_cyl = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_cyl) != VK_SUCCESS)
            VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 3");

    VkDescriptorSet descriptorSet_bez_cyl = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_bez_cyl) != VK_SUCCESS)
            VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 4");

    VkDescriptorSet descriptorSet_sphere = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_sphere) != VK_SUCCESS)
            VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 5");

    VkDescriptorSet descriptorSet_torus = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_torus) != VK_SUCCESS)
            VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 6");

    VkDescriptorSet descriptorSet_directionaLight = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(vk_device, &allocInfo, &descriptorSet_directionaLight) != VK_SUCCESS)
            VKL_EXIT_WITH_ERROR("Failed to allocate descriptor set 6");

    // note i have 32 total descriptors reserved!

#pragma endregion

    ObjectSettings::updateDescriptorSets(cornell_uniform_buffer, descriptorSet_cornell, vk_device);
    ObjectSettings::updateDescriptorSets(cube_uniform_buffer, descriptorSet_cube, vk_device);
    ObjectSettings::updateDescriptorSets(cylinder_uniform_buffer, descriptorSet_cyl, vk_device);
    ObjectSettings::updateDescriptorSets(bezier_cylinder_uniform_buffer, descriptorSet_bez_cyl, vk_device);
    ObjectSettings::updateDescriptorSets(sphere_uniform_buffer, descriptorSet_sphere, vk_device);
    ObjectSettings::updateDescriptorSets(torus_uniform_buffer, descriptorSet_torus, vk_device);
    ObjectSettings::updateDescriptorSets_DirectionalLight(dirLight_vk_buffer, descriptorSet_directionaLight, vk_device);

    auto cornellPipelines = pipeline_factory(cornellBox_vertexShader_path, cornellBox_fragmentShader_path);
    auto objectsPipeline = pipeline_factory(cube_vertexShader_path, cube_fragmentShader_path);
    
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);
#pragma endregion

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        vklWaitForNextSwapchainImage();
        
        // reset camera to original pos and zoom it if needed
        if (reset_camera) main_camera.reset_camera_state();
        main_camera.update_camera_zoom(scroll_delta); scroll_delta = 0;

        // move camera logic
        double deltax = {}; double deltay = {}; get_mouse_delta(window, deltax, deltay);
        glm::mat4 proj_view = main_camera.get_proj_view_matrix(deltax, deltay);
        
        // update ubo
        ubo_cornellBox.object_matrix = proj_view * model_cornell;
        ubo_cornellBox.drawModes = glm::ivec4(normalMode, fresnelMode, 0, 0);
        vklCopyDataIntoHostCoherentBuffer(cornell_uniform_buffer, &ubo_cornellBox, sizeof(ubo_cornellBox));
        
        ubo_cube.object_matrix = proj_view * model_cube;
        ubo_cube.drawModes = glm::ivec4(normalMode, fresnelMode, 0, 0);
        vklCopyDataIntoHostCoherentBuffer(cube_uniform_buffer, &ubo_cube, sizeof(ubo_cube));

        ubo_cylinder.object_matrix = proj_view * model_cylinder;
        ubo_cylinder.drawModes = glm::ivec4(normalMode, fresnelMode, 0, 0);
        vklCopyDataIntoHostCoherentBuffer(cylinder_uniform_buffer, &ubo_cylinder, sizeof(ubo_cylinder));

        ubo_bezier_cyl.object_matrix = proj_view * model_bezier_cyl;
        ubo_bezier_cyl.drawModes = glm::ivec4(normalMode, fresnelMode, 0, 0);
        vklCopyDataIntoHostCoherentBuffer(bezier_cylinder_uniform_buffer, &ubo_bezier_cyl, sizeof(ubo_bezier_cyl));

        ubo_sphere.object_matrix = proj_view * model_shpere;
        ubo_sphere.drawModes = glm::ivec4(normalMode, fresnelMode, 0, 0);
        vklCopyDataIntoHostCoherentBuffer(sphere_uniform_buffer, &ubo_sphere, sizeof(ubo_sphere));

        ubo_torus.object_matrix = proj_view * model_torus;
        vklCopyDataIntoHostCoherentBuffer(torus_uniform_buffer, &ubo_torus, sizeof(ubo_torus));

        vklStartRecordingCommands();
        
        VkPipeline cornellPipeline = choose_pipeline(cornellPipelines);
        alexd_drawObject(cornellPipeline, descriptorSet_cornell, cornellBox.get_vk_vbuff(), cornellBox.get_vk_ibuff(), static_cast<uint32_t>(cornellBox.get_ibuff_size()));
        
        VkPipeline vk_pipeline = choose_pipeline(objectsPipeline);
        alexd_drawObject(vk_pipeline, descriptorSet_cube, cube.get_vk_vbuff(), cube.get_vk_ibuff(), static_cast<uint32_t>(cube.get_ibuff_size()));
        alexd_drawObject(vk_pipeline, descriptorSet_cyl, cylinder.get_vk_vbuff(), cylinder.get_vk_ibuff(), static_cast<uint32_t>(cylinder.get_ibuff_size()));
        alexd_drawObject(vk_pipeline, descriptorSet_bez_cyl, curve.get_vk_vbuff(), curve.get_vk_ibuff(), static_cast<uint32_t>(curve.get_ibuff_size()));
        alexd_drawObject(vk_pipeline, descriptorSet_sphere, sphere.get_vk_vbuff(), sphere.get_vk_ibuff(), static_cast<uint32_t>(sphere.get_ibuff_size()));
        //alexd_drawObject(vk_pipeline, descriptorSet_torus, torus.get_vk_vbuff(), torus.get_vk_ibuff(), static_cast<uint32_t>(torus.get_ibuff_size()));
        
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

    destroy_pipelines(cornellPipelines);
    destroy_pipelines(objectsPipeline);

    vkDestroyDescriptorPool(vk_device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vk_device, descriptorSetLayout, nullptr);
    
    cube.destroyVkBuffers();
    cornellBox.destroyVkBuffers();
    cylinder.destroyVkBuffers();
    sphere.destroyVkBuffers();
    curve.destroyVkBuffers();
    torus.destroyVkBuffers();

    vklDestroyHostCoherentBufferAndItsBackingMemory(cornell_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cube_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cylinder_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(bezier_cylinder_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(sphere_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(torus_uniform_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(dirLight_vk_buffer);

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
