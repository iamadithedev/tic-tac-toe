#include "glfw/platform_factory.hpp"
#include "glfw/platform.hpp"

#include "file.hpp"
#include "shader.hpp"
#include "vertex_array.hpp"
#include "importers/mesh_importer.hpp"
#include "buffer.hpp"
#include "material.hpp"
#include "render_pass.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "time.hpp"
#include "combine_geometry.hpp"
#include "physics_world.hpp"
#include "board.hpp"

//#define USE_EDITOR

#ifdef USE_EDITOR
#include "editor.hpp"
#include "components/camera_window.hpp"
#include "render_pass_window.hpp"
#endif

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

    if (!glfw::Platform::init_context())
    {
        window->destroy();
        platform->release();

        return -1;
    }

    platform->vsync();

    // ==================================================================================

    auto diffuse_vertex_source = File::read<char>("../Assets/glsl/diffuse.vert.glsl");
    auto diffuse_vertex_instance_source = File::read<char>("../Assets/glsl/diffuse_instance.vert.glsl");

    auto diffuse_fragment_source = File::read<char>("../Assets/glsl/diffuse.frag.glsl");

    ShaderStage diffuse_vertex_shader { "diffuse_vert.glsl", GL_VERTEX_SHADER };
    diffuse_vertex_shader.create();
    diffuse_vertex_shader.source(diffuse_vertex_source);

    ShaderStage diffuse_vertex_instance_shader { "diffuse_vert_instance.glsl", GL_VERTEX_SHADER };
    diffuse_vertex_instance_shader.create();
    diffuse_vertex_instance_shader.source(diffuse_vertex_instance_source);

    ShaderStage diffuse_fragment_shader {"diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_fragment_shader.create();
    diffuse_fragment_shader.source(diffuse_fragment_source);

    Shader diffuse_program;
    diffuse_program.create();
    diffuse_program.attach(&diffuse_vertex_shader);
    diffuse_program.attach(&diffuse_fragment_shader);
    diffuse_program.link();

    Shader diffuse_instance_program;
    diffuse_instance_program.create();
    diffuse_instance_program.attach(&diffuse_vertex_instance_shader);
    diffuse_instance_program.attach(&diffuse_fragment_shader);
    diffuse_instance_program.link();

    diffuse_program.detach(&diffuse_vertex_shader);
    diffuse_program.detach(&diffuse_fragment_shader);

    diffuse_instance_program.detach(&diffuse_vertex_instance_shader);
    diffuse_instance_program.detach(&diffuse_fragment_shader);

    // ==================================================================================

    auto tic_tac_toe_geometries = MeshImporter::load("../Assets/tic_tac_toe.obj");

    CombineGeometry scene_geometry;
    scene_geometry.combine(tic_tac_toe_geometries);

    auto x_mesh_part = scene_geometry[0];
    auto o_mesh_part = scene_geometry[1];

    auto frame_mesh_part = scene_geometry[2];
    auto cover_mesh_part = scene_geometry[3];

    // ==================================================================================

    vertex_attributes diffuse_vertex_attributes =
    {
        { 0, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, position) },
        { 1, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, normal) }
    };

    VertexArray scene_vao;
    scene_vao.create();
    scene_vao.bind();

    Buffer scene_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_vbo.create();
    scene_vbo.bind();
    scene_vbo.data(BufferData::make_data(scene_geometry.vertices()));

    Buffer scene_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_ibo.create();
    scene_ibo.bind();
    scene_ibo.data(BufferData::make_data(scene_geometry.faces()));

    scene_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    Material x_material  {{1.0f, 1.0f, 0.0f } };
    Material o_material  {{0.0f, 1.0f, 1.0f } };

    Material frame_material { { 0.0f, 0.0f, 1.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 10.0f, 10.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    Buffer matrices_buffer { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_buffer.create();
    matrices_buffer.bind_at_location(0);

    Buffer matrices_instance_buffer { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_instance_buffer.create();
    matrices_instance_buffer.bind_at_location(3);

    Buffer material_buffer { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    material_buffer.create();
    material_buffer.bind_at_location(1);

    Buffer light_buffer { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    light_buffer.create();
    light_buffer.bind_at_location(2);

    // ==================================================================================

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    rgb clear_color { 0.062, 0.403, 0.436 };
    render_pass.clear_color(clear_color);

    // ==================================================================================

    std::vector<glm::mat4> matrices          { 3 };
    std::vector<glm::mat4> matrices_instance { 9 };

    // ==================================================================================

    Camera perspective_camera { 60.0f };
    vec3   camera_position { 0.0f, 0.0f, -12.0f };

    perspective_camera.resize((float) width, (float) height);

    Transform camera_transform;
    camera_transform.translate(camera_position);

    // ==================================================================================

    Transform item_transform;
    Transform frame_transform;

    // ==================================================================================

    PhysicsWorld physics;
    physics.init();

    // ==================================================================================

    #ifdef USE_EDITOR

    Editor editor;
    editor.init(window.get(), &physics);

    CameraWindow camera_window;
    camera_window.set_camera(&perspective_camera);
    camera_window.set_transform(&camera_transform, camera_position);

    RenderPassWindow render_pass_window;
    render_pass_window.set_render_pass(&render_pass, clear_color);

    editor.add_window(&camera_window);
    editor.add_window(&render_pass_window);

    #endif

    // ==================================================================================

    Board board;
    board.init();

    auto    shape = new btBoxShape({ 1.3f, 1.3f, 0.2f });

    int32_t index = 0;
    float offset  = 3.0f;
    float y       = offset;

    for (int32_t row = 0; row < board.rows(); row++)
    {
        float x = -offset;

        for (int32_t column = 0; column < board.columns(); column++)
        {
            const auto& item = board.item_at(row, column);

            item_transform.translate({ x, y, 0.0f })
                          .scale({ 0.5f, 0.5f, 0.5f });
            matrices_instance[index] = item_transform.matrix();

            physics.add_collision(index, shape, item.position);

            index += 1;
                x += offset;
        }

        y -= offset;
    }

    matrices_instance_buffer.data(BufferData::make_data(matrices_instance));

    // ==================================================================================

    const Time time;

    bool x_turn  = true;
    bool is_over = false;

    while (!window->closed())
    {
        #ifdef USE_EDITOR

        physics.compute_debug_geometry();

        #endif

        const float total_time = time.total_time();

        // ==================================================================================

        const int32_t new_width  = window->size().width;
        const int32_t new_height = window->size().height;

        if (width != new_width || height != new_height)
        {
            width  = new_width;
            height = new_height;

            perspective_camera.resize((float) width, (float) height);
        }

        // ==================================================================================

        if (!is_over && input->mouse_pressed(window.get(), input::Button::Left))
        {
            vec2 mouse_position = input->mouse_position(window.get());

            auto ray = perspective_camera.screen_to_world(camera_transform.matrix(), mouse_position);
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

        #ifdef USE_EDITOR

        editor.begin(width, height, total_time);
        editor.end();

        #endif

        // ==================================================================================

        render_pass.viewport({ 0, 0 }, { width, height });
        render_pass.clear_buffers();

        // ==================================================================================

        matrices[0] = glm::mat4 { 1.0f };
        matrices[1] = camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&frame_material));
        light_buffer.data(BufferData::make_data(&directional_light));

        // ==================================================================================

        diffuse_instance_program.bind();

        material_buffer.sub_data(BufferData::make_data(&frame_material));

        scene_vao.bind();
        glDrawElementsInstanced(GL_TRIANGLES, cover_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cover_mesh_part.index), 9);

        diffuse_program.bind();

        for (int32_t row = 0; row < board.rows(); row++)
        {
            for (int32_t column = 0; column < board.columns(); column++)
            {
                const auto& item = board.item_at(row, column);

                item_transform.translate(item.position)
                               .scale({ 0.5f, 0.5f, 0.5f });

                matrices_buffer.sub_data(BufferData::make_data(&item_transform.matrix()));

                if (item.type == Item::Type::X)
                {
                    material_buffer.sub_data(BufferData::make_data(&x_material));
                    glDrawElements(GL_TRIANGLES, x_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(x_mesh_part.index));
                }
                else if (item.type == Item::Type::O)
                {
                    material_buffer.sub_data(BufferData::make_data(&o_material));
                    glDrawElements(GL_TRIANGLES, o_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(o_mesh_part.index));
                }
            }
        }

        // ==================================================================================

        frame_transform.translate({ 0.0f, 0.0f, 0.0f })
                       .scale({ 0.5f, 0.5f, 0.5f });

        matrices_buffer.sub_data(BufferData::make_data(&frame_transform.matrix()));
        material_buffer.sub_data(BufferData::make_data(&frame_material));

        glDrawElements(GL_TRIANGLES, frame_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(frame_mesh_part.index));

        // ==================================================================================

        #ifdef USE_EDITOR

        editor.draw(&matrices_buffer);

        #endif

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}