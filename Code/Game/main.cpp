#include "glfw/platform_factory.hpp"
#include "glfw/window.hpp"

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

#include "editor.hpp"
#include "physics.hpp"

#include "components/camera_window.hpp"

#include "board.hpp"

int main()
{
    glfw::PlatformFactory platform_factory;

    int32_t width  = 1024;
    int32_t height = 768;

    auto platform = platform_factory.create_platform();
    auto window   = platform_factory.create_window("Tic-Tac-Toe", { width, height });
    auto input    = platform_factory.create_input();

    if (!platform->init())
    {
        return -1;
    }

    if (!window->create())
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

    Shader diffuse_fragment_shader {"diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_fragment_shader.create();
    diffuse_fragment_shader.source(diffuse_fragment_source.data());

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

    vertex_attributes diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(mesh_vertex::diffuse, position) },
        { 1, 3, (int32_t)offsetof(mesh_vertex::diffuse, normal) }
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
    x_ibo.data(BufferData::make_data(x_geometry.faces()));

    x_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray o_vao;
    o_vao.create();
    o_vao.bind();

    Buffer o_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    o_vbo.create();
    o_vbo.data(BufferData::make_data(o_geometry.vertices()));

    Buffer o_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    o_ibo.create();
    o_ibo.data(BufferData::make_data(o_geometry.faces()));

    o_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray frame_vao;
    frame_vao.create();
    frame_vao.bind();

    Buffer frame_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    frame_vbo.create();
    frame_vbo.data(BufferData::make_data(frame_geometry.vertices()));

    Buffer frame_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    frame_ibo.create();
    frame_ibo.data(BufferData::make_data(frame_geometry.faces()));

    frame_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray cover_vao;
    cover_vao.create();
    cover_vao.bind();

    Buffer cover_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    cover_vbo.create();
    cover_vbo.data(BufferData::make_data(cover_geometry.vertices()));

    Buffer cover_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    cover_ibo.create();
    cover_ibo.data(BufferData::make_data(cover_geometry.faces()));

    cover_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    Material x_material  {{1.0f, 1.0f, 0.0f } };
    Material o_material  {{0.0f, 1.0f, 1.0f } };

    Material frame_material { { 0.0f, 0.0f, 1.0f } };

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

    Camera perspective_camera { 60.0f };

    vec3 camera_position { 0.0f, 0.0f, -12.0f };

    Transform camera_transform;
    camera_transform.translate(camera_position);

    // ==================================================================================

    Transform item_transform;

    Transform frame_transform;
    Transform cover_transform;

    // ==================================================================================

    Physics physics;
    physics.init();

    // ==================================================================================

    Editor editor;
    editor.init(window.get(), &physics);

    CameraWindow camera_window;
    camera_window.set_camera(&perspective_camera);
    camera_window.set_transform(&camera_transform, camera_position);

    editor.add_window(&camera_window);

    // ==================================================================================

    Board board;
    board.init(3.0f);

    auto    shape = new btBoxShape({ 1.4f, 1.4f, 0.5f });
    int32_t index = 0;

    for (int32_t row = 0; row < board.rows(); row++)
    {
        for (int32_t column = 0; column < board.columns(); column++)
        {
            physics.add_collision(index++, shape, board.item_at(row, column).position);
        }
    }

    // ==================================================================================

    const Time time;
    float fov = 60.0f;

    rgb clear_color { 0.45f, 0.55f, 0.60f };

    bool x_turn  = true;
    bool is_over = false;

    while (!window->closed())
    {
        physics.compute_debug_geometry();

        const float total_time = time.total_time();

        width  = window->size().width;
        height = window->size().height;

        perspective_camera.resize((float)width, (float)height);

        // ==================================================================================

        if (!is_over && input->mouse_pressed(window.get(), input::Button::Left))
        {
            double xpos, ypos;
            glfwGetCursorPos(((glfw::Window*)window.get())->handle(), &xpos, &ypos);

            auto ray = perspective_camera.screen_to_world(camera_transform.matrix(), {(float)xpos, (float)ypos });
            auto hit = physics.cast(ray, 50.0f);

            if (hit.hasHit())
            {
                const int32_t hit_index = hit.m_collisionObject->getUserIndex();

                const int32_t row    = hit_index / board.columns();
                const int32_t column = hit_index % board.columns();

                auto& item = board.item_at(row, column);

                if (item.none())
                {
                    item.type =  x_turn ? Item::Type::X : Item::Type::O;
                       x_turn = !x_turn;

                    is_over = board.check_win(row, column, item.type);
                }
            }
        }

        if (input->key_pressed(window.get(), input::Key::Space))
        {
            x_turn  = true;
            is_over = false;

            board.reset();
        }

        if (input->key_pressed(window.get(), input::Key::Escape))
        {
            window->close();
        }

        // ==================================================================================

        editor.begin(width, height, total_time);
        editor.end();

        // ==================================================================================

        render_pass.viewport(0, 0, width, height);
        render_pass.clear_color(clear_color.r, clear_color.g, clear_color.b);
        render_pass.clear_buffers();

        // ==================================================================================

        cover_transform.translate({ 0.0f, 0.0f, 0.0f })
                       .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = cover_transform.matrix();
        matrices[1] = camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&frame_material));
        light_buffer.data(BufferData::make_data(&directional_light));

        diffuse_program.bind();

        // ==================================================================================

        for (int32_t row = 0; row < board.rows(); row++)
        {
            for (int32_t column = 0; column < board.columns(); column++)
            {
                const auto& item = board.item_at(row, column);

                item_transform.translate(item.position)
                              .scale({ 0.5f, 0.5f, 0.5f });

                matrices[0] = item_transform.matrix();

                matrices_buffer.sub_data(BufferData::make_data(&matrices[0]));
                material_buffer.sub_data(BufferData::make_data(&frame_material));

                cover_vao.bind();
                glDrawElements(GL_TRIANGLES, (int32_t)cover_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);

                if (item.type == Item::Type::X)
                {
                    material_buffer.sub_data(BufferData::make_data(&x_material));
                    x_vao.bind();
                    glDrawElements(GL_TRIANGLES, (int32_t)x_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);
                }
                else if (item.type == Item::Type::O)
                {
                    material_buffer.sub_data(BufferData::make_data(&o_material));
                    o_vao.bind();
                    glDrawElements(GL_TRIANGLES, (int32_t)o_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);
                }
            }
        }

        // ==================================================================================

        frame_transform.translate({ 0.0f, 0.0f, 0.0f })
                       .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = frame_transform.matrix();

        matrices_buffer.sub_data(BufferData::make_data(&matrices[0]));
        material_buffer.sub_data(BufferData::make_data(&frame_material));

        frame_vao.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)frame_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);

        // ==================================================================================

        editor.draw(&matrices_buffer);

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}