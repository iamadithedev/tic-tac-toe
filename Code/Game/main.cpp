#include "glfw/platform_factory.hpp"
#include "glfw/platform.hpp"

#include "shader.hpp"
#include "vertex_array.hpp"
#include "buffer.hpp"
#include "material.hpp"
#include "render_pass.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "time.hpp"
#include "physics_world.hpp"
#include "board.hpp"
#include "importers/mesh_importer.hpp"
#include "importers/texture_importer.hpp"
#include "geometries/combine_geometry.hpp"
#include "geometries/sprite_geometry.hpp"
#include "resource_manager.hpp"
#include "sampler.hpp"

//#define USE_EDITOR
#define USE_BLEND

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

    ResourceManager resources;
    resources.init("../Assets/");

    auto diffuse_shader          = resources.load<Shader>("diffuse_shader.asset");
    auto diffuse_instance_shader = resources.load<Shader>("diffuse_instance_shader.asset");
    auto sprite_shader           = resources.load<Shader>("sprite_shader.asset");

    // ==================================================================================

    auto tic_tac_toe_texture_data = TextureImporter::load("../Assets/tic-tac-toe.PNG");

    Texture tic_tac_toe_texture { GL_TEXTURE_2D };
    tic_tac_toe_texture.create();
    tic_tac_toe_texture.source(tic_tac_toe_texture_data);
    tic_tac_toe_texture_data.release();

    Sampler default_sampler;
    default_sampler.create();

    default_sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    default_sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    default_sampler.bind_at_location(0);

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

    Buffer matrices_ubo { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_ubo.create();
    matrices_ubo.bind_at_location(0);

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

    float logo_sprite_w = (float)tic_tac_toe_texture_data.width()  * 0.57f;
    float logo_sprite_h = (float)tic_tac_toe_texture_data.height() * 0.5f;

    float x_sprite_w = (float)tic_tac_toe_texture_data.width()  * 0.25f;
    float x_sprite_h = (float)tic_tac_toe_texture_data.height() * 0.18f;

    SpriteGeometry sprite_geometry;

    sprite_geometry.begin();
    sprite_geometry.add_sprite(logo_sprite_w / 2.0f,
                               logo_sprite_h / 2.0f, { 0.23f, 0.35f }, { 0.8f, 0.85f });

    sprite_geometry.add_sprite(x_sprite_w / 2.0f,
                               x_sprite_h / 2.0f, { 0.25f, 0.17f }, { 0.5f, 0.35f });
    sprite_geometry.end();

    auto logo_sprite_submesh = sprite_geometry[0];
    auto x_sprite_submesh    = sprite_geometry[1];

    // ==================================================================================

    vertex_attributes sprite_vertex_attributes =
    {
        { 0, 2, GL_FLOAT, (int32_t)offsetof(mesh_vertex::sprite, position) },
        { 1, 2, GL_FLOAT, (int32_t)offsetof(mesh_vertex::sprite, uv) }
    };

    VertexArray sprite_vao;
    sprite_vao.create();
    sprite_vao.bind();

    Buffer sprite_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    sprite_vbo.create();
    sprite_vbo.bind();
    sprite_vbo.data(BufferData::make_data(sprite_geometry.vertices()));

    Buffer sprite_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    sprite_ibo.create();
    sprite_ibo.bind();
    sprite_ibo.data(BufferData::make_data(sprite_geometry.faces()));

    sprite_vao.init_attributes_of_type<mesh_vertex::sprite>(sprite_vertex_attributes);

    // ==================================================================================

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    render_pass.enable(GL_BLEND);

    #ifdef USE_BLEND
    render_pass.blend();
    #endif

    rgb clear_color { 0.062, 0.403, 0.436 };
    render_pass.clear_color(clear_color);

    // ==================================================================================

    std::vector<glm::mat4> matrices          { 3 };
    std::vector<glm::mat4> matrices_instance { 9 };

    // ==================================================================================

    Camera ortho_camera;
    Camera scene_camera {60.0f };
    vec3   camera_position { 0.0f, 0.0f, -12.0f };

    scene_camera.resize((float) width, (float) height);
    ortho_camera.resize((float)width, (float)height);

    Transform scene_camera_transform;

    scene_camera_transform.translate(camera_position);

    // ==================================================================================

    Transform item_transform;
    Transform frame_transform;

    Transform logo_sprite_transform;
    Transform x_sprite_transform;

    logo_sprite_transform.translate({(float)width / 2.0f, (float)height / 2.0f })
                         .scale({ 0.8f, 0.8f, 0.8f });

    x_sprite_transform.translate({ 200.0f, (float)height / 2.0f })
                      .scale({ 0.8f, 0.8f, 0.8f });

    // ==================================================================================

    PhysicsWorld physics;
    physics.init();

    // ==================================================================================

    #ifdef USE_EDITOR

    Editor editor;
    editor.init(window.get(), &resources, &physics);

    CameraWindow camera_window;
    camera_window.set_camera(&scene_camera);
    camera_window.set_transform(&scene_camera_transform, camera_position);

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

    bool show_logo = true;

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

            scene_camera.resize((float) width, (float) height);
            ortho_camera.resize((float)width, (float)height);

            logo_sprite_transform.translate({(float)width / 2.0f, (float)height / 2.0f })
                    .scale({ 0.8f, 0.8f, 0.8f });
        }

        // ==================================================================================

        if (!is_over && input->mouse_pressed(window.get(), input::Button::Left))
        {
            vec2 mouse_position = input->mouse_position(window.get());

            auto ray    = scene_camera.screen_to_world(scene_camera_transform.matrix(), mouse_position);
            auto result = physics.cast(ray, 50.0f);

            if (result.hit())
            {
                const int32_t hit_index = result.index();

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
            x_turn    = true;
            is_over   = false;
            show_logo = false;

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

        if (!show_logo)
        {
            matrices[0] = glm::mat4{1.0f};
            matrices[1] = scene_camera_transform.matrix();
            matrices[2] = scene_camera.projection();

            matrices_ubo.data(BufferData::make_data(matrices));
            material_buffer.data(BufferData::make_data(&frame_material));
            light_buffer.data(BufferData::make_data(&directional_light));

            // ==================================================================================

            diffuse_instance_shader->bind();

            material_buffer.sub_data(BufferData::make_data(&frame_material));

            scene_vao.bind();
            glDrawElementsInstanced(GL_TRIANGLES, cover_mesh_part.count, GL_UNSIGNED_INT,
                                    reinterpret_cast<std::byte*>(cover_mesh_part.index), 9);
            diffuse_shader->bind();

            for (int32_t row = 0; row < board.rows(); row++) {
                for (int32_t column = 0; column < board.columns(); column++) {
                    const auto &item = board.item_at(row, column);

                    item_transform.translate(item.position)
                            .scale({0.5f, 0.5f, 0.5f});

                    matrices_ubo.sub_data(BufferData::make_data(&item_transform.matrix()));

                    if (item.type == Item::Type::X) {
                        material_buffer.sub_data(BufferData::make_data(&x_material));
                        glDrawElements(GL_TRIANGLES, x_mesh_part.count, GL_UNSIGNED_INT,
                                       reinterpret_cast<std::byte *>(x_mesh_part.index));
                    } else if (item.type == Item::Type::O) {
                        material_buffer.sub_data(BufferData::make_data(&o_material));
                        glDrawElements(GL_TRIANGLES, o_mesh_part.count, GL_UNSIGNED_INT,
                                       reinterpret_cast<std::byte*>(o_mesh_part.index));
                    }
                }
            }

            // ==================================================================================

            frame_transform.translate({0.0f, 0.0f, 0.0f})
                            //.rotate({ 0.0f, 1.0f, 0.0f }, total_time)
                    .scale({0.5f, 0.5f, 0.5f});

            matrices_ubo.sub_data(BufferData::make_data(&frame_transform.matrix()));
            material_buffer.sub_data(BufferData::make_data(&frame_material));

            glDrawElements(GL_TRIANGLES, frame_mesh_part.count, GL_UNSIGNED_INT,
                           reinterpret_cast<std::byte*>(frame_mesh_part.index));

            // ==================================================================================
        }
        else
        {
            matrices[0] = logo_sprite_transform.matrix();
            matrices[1] = glm::mat4 {1.0f };
            matrices[2] = ortho_camera.projection();

            matrices_ubo.data(BufferData::make_data(matrices));

            sprite_shader->bind();
            tic_tac_toe_texture.bind();

            sprite_vao.bind();
            glDrawElements(GL_TRIANGLES, logo_sprite_submesh.count, GL_UNSIGNED_INT,
                           reinterpret_cast<std::byte*>(logo_sprite_submesh.index));

            // ==================================================================================

            matrices_ubo.sub_data(BufferData::make_data(&x_sprite_transform.matrix()));

            glDrawElements(GL_TRIANGLES, x_sprite_submesh.count, GL_UNSIGNED_INT,
                           reinterpret_cast<std::byte*>(x_sprite_submesh.index));

            // ==================================================================================
        }

        #ifdef USE_EDITOR

        editor.draw(&matrices_ubo);

        #endif

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}