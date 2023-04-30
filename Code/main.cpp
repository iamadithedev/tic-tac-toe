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
    auto o_geometry = MeshImporter::load("../o.obj");

    auto frame_geometry = MeshImporter::load("../frame.obj");
    auto cover_geometry = MeshImporter::load("../cover.obj");

    // ==================================================================================

    std::vector<vertex_attribute> diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(vertex::diffuse, position) },
        { 1, 3, (int32_t)offsetof(vertex::diffuse, normal) }
    };

    // ==================================================================================

    VertexArray x_vao;
    x_vao.create();
    x_vao.bind();

    Buffer x_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_vbo.create();
    x_vbo.data(BufferData::make_data(x_geometry.vertices()));

    Buffer x_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_ibo.create();
    x_ibo.data(BufferData::make_data(x_geometry.indices()));

    x_vao.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray o_vao;
    o_vao.create();
    o_vao.bind();

    Buffer o_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    o_vbo.create();
    o_vbo.data(BufferData::make_data(o_geometry.vertices()));

    Buffer o_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    o_ibo.create();
    o_ibo.data(BufferData::make_data(o_geometry.indices()));

    o_vao.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray frame_vao;
    frame_vao.create();
    frame_vao.bind();

    Buffer frame_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    frame_vbo.create();
    frame_vbo.data(BufferData::make_data(frame_geometry.vertices()));

    Buffer frame_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    frame_ibo.create();
    frame_ibo.data(BufferData::make_data(frame_geometry.indices()));

    frame_vao.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray cover_vao;
    cover_vao.create();
    cover_vao.bind();

    Buffer cover_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    cover_vbo.create();
    cover_vbo.data(BufferData::make_data(cover_geometry.vertices()));

    Buffer cover_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    cover_ibo.create();
    cover_ibo.data(BufferData::make_data(cover_geometry.indices()));

    cover_vao.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    Material x_material { { 1.0f, 1.0f, 0.0f } };
    Material o_material { { 0.0f, 1.0f, 0.0f } };

    Material frame_material { { 0.0f, 0.0f, 1.0f } };
    Material cover_material { { 0.0f, 0.0f, 1.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 8.0f }, { 1.0f, 1.0f, 1.0f } };

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
    perspective_camera_transform.translate({0.0f, 0.0f, -10.0f });

    // ==================================================================================

    Transform x_transform;
    Transform o_transform;

    Transform frame_transform;
    Transform cover_transform;

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

        cover_transform.translate({ 0.0f, 0.0f, 0.0f })
                       .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = cover_transform.matrix();
        matrices[1] = perspective_camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&cover_material));
        light_buffer.data(BufferData::make_data(&directional_light));

        diffuse_program.bind();

        cover_vao.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)cover_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                   .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = x_transform.matrix();

        matrices_buffer.sub_data(BufferData::make_data(&matrices[0]));
        material_buffer.sub_data(BufferData::make_data(&x_material));

        x_vao.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)x_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        o_transform.translate({ 3.0f, 0.0f, 0.0f })
                   .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = o_transform.matrix();

        matrices_buffer.sub_data(BufferData::make_data(&matrices[0]));
        material_buffer.sub_data(BufferData::make_data(&o_material));

        o_vao.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)o_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        frame_transform.translate({ 0.0f, 0.0f, 0.0f })
                       .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = frame_transform.matrix();

        matrices_buffer.sub_data(BufferData::make_data(&matrices[0]));
        material_buffer.sub_data(BufferData::make_data(&frame_material));

        frame_vao.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)frame_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}