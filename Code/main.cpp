#include "GLFW/platform_factory.hpp"
#include "GLFW/window.hpp"

#include "file.hpp"
#include "program.hpp"
#include "vertex_array.hpp"
#include "mesh_importer.hpp"
#include "buffer.hpp"
#include "material.hpp"
#include "render_pass.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "time.hpp"

int main()
{
    glfw::PlatformFactory platform_factory;

    auto platform = platform_factory.create_platform();
    auto window   = platform_factory.create_window(1024, 768);

    if (!platform->init())
    {
        return -1;
    }

    if (!window->create("Tic-Tac-Toe"))
    {
        platform->release();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        window->destroy();
        platform->release();

        return -1;
    }

    platform->vsync();

    // ==================================================================================

    auto diffuse_vertex_source = File::read("../diffuse_vert.glsl");
    auto diffuse_fragment_source = File::read("../diffuse_frag.glsl");

    Shader diffuse_vertex_shader {"diffuse_vert.glsl", GL_VERTEX_SHADER };
    diffuse_vertex_shader.create();
    diffuse_vertex_shader.source(diffuse_vertex_source.data());
    diffuse_vertex_shader.compile();

    Shader diffuse_fragment_shader {"diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_fragment_shader.create();
    diffuse_fragment_shader.source(diffuse_fragment_source.data());
    diffuse_fragment_shader.compile();

    Program diffuse_program;
    diffuse_program.create();
    diffuse_program.attach(&diffuse_vertex_shader);
    diffuse_program.attach(&diffuse_fragment_shader);
    diffuse_program.link();

    diffuse_program.detach(&diffuse_vertex_shader);
    diffuse_program.detach(&diffuse_fragment_shader);

    // ==================================================================================

    auto x_geometry = MeshImporter::load("../x.obj");

    // ==================================================================================

    std::vector<vertex_attribute> diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(vertex::diffuse, position) },
        { 1, 3, (int32_t)offsetof(vertex::diffuse, normal) }
    };

    // ==================================================================================

    VertexArray x_vertex_array;
    x_vertex_array.create();
    x_vertex_array.bind();

    Buffer x_vertex_buffer {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_vertex_buffer.create();
    x_vertex_buffer.data(BufferData::make_data(x_geometry.vertices()));

    Buffer x_indices_buffer {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_indices_buffer.create();
    x_indices_buffer.data(BufferData::make_data(x_geometry.indices()));

    x_vertex_array.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    Material x_material { { 1.0f, 1.0f, 0.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    Buffer matrices_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    matrices_buffer.create();
    matrices_buffer.bind_at_location(0);

    Buffer material_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    material_buffer.create();
    material_buffer.bind_at_location(1);

    Buffer light_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    light_buffer.create();
    light_buffer.bind_at_location(2);

    // ==================================================================================

    const RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    // ==================================================================================

    std::vector<glm::mat4> matrices { 3 };

    // ==================================================================================

    Camera perspective_camera;

    Transform perspective_camera_transform;
    perspective_camera_transform.translate({0.0f, 0.0f, -5.0f });

    // ==================================================================================

    Transform x_transform;

    // ==================================================================================

    const Time time;
    float fov = 60.0f;

    rgb clear_color { 0.45f, 0.55f, 0.60f };

    while (!window->closed())
    {
        const float total_time = time.total_time();

        int32_t width  = window->width();
        int32_t height = window->height();

        const float ratio = (float) width / (float) height;

        perspective_camera.perspective(fov, ratio);

        // ==================================================================================

        render_pass.viewport(0, 0, width, height);
        render_pass.clear_color(clear_color.r, clear_color.g, clear_color.b);
        render_pass.clear_buffers();

        // ==================================================================================

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                .rotate({ 0.0f, 1.0f, 0.0f }, total_time)
                .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = x_transform.matrix();
        matrices[1] = perspective_camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&x_material));
        light_buffer.data(BufferData::make_data(&directional_light));

        diffuse_program.bind();

        x_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)x_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}