#include <Graphics/DrawUtils.hpp>
#include <Scene/Scene.hpp>
#include <Tiles/Exceptions.hpp>
#include <Tiles/Layer.hpp>

constexpr std::string_view TILE_FRAGMENT_SHADER_SOURCE = "\
#version 130\n\
uniform sampler2D m;\n\
uniform sampler2D t;\n\
uniform vec2 w;\n\
uniform vec2 z;\n\
void main()\n\
{\n\
    vec4 c = texture2D(m, gl_TexCoord[0].xy / 32.0);\n\
    float i = floor(c.r * 255.0 + c.g * 65280);\n\
    vec2 a = vec2(mod(i, w.x), floor(i / w.x))/w;\n\
    vec2 b = mod((gl_TexCoord[0].xy * z) / 32.0, 1.0);\n\
    gl_FragColor = texture2D(t, a + b / w);\n\
    gl_FragColor.a *= c.a;\n\
}";
constexpr std::string_view TILE_VERTEX_SHADER_SOURCE = "\
#version 130\n\
void main()\n\
{\n\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n\
    gl_FrontColor = gl_Color;\n\
}";

namespace obe::tiles
{
    void TileLayer::build_tile(uint32_t x, uint32_t y, uint32_t tile_id)
    {
        if (!tile_id)
            return;

        const uint32_t tile_index = y + x * m_height;
        const uint32_t tile_data_index = x + y * m_width;

        const TileInfo tile_info = get_tile_info(tile_id);

        const Tileset& tileset = m_scene.get_tilesets().tileset_from_tile_id(tile_info.tile_id);
        const uint32_t first_tile_id = tileset.get_first_tile_id();
        sfe::SpannableVertexArray& vertices = m_sublayers_by_tileset[tileset.get_first_tile_id()];
        sf::Vertex* quad = &vertices[tile_index * 4];
        for (auto& animation : m_scene.get_animated_tiles())
        {
            if (animation->get_id() == tile_info.tile_id)
            {
                animation->attach_quad(quad, tile_info);
                break;
            }
        }
        for (auto& collider : m_scene.get_collider_models())
        {
            if (collider->get_id() == std::to_string(tile_info.tile_id))
            {
                m_colliders[tile_data_index] = &m_scene.get_scene().create_collider();
                (*m_colliders[tile_data_index]) = *collider;
                // m_colliders[tile_data_index]->set_parent_id("tile_" + std::to_string(tile_info.tile_id));
                // TODO: Fix this horrible code
                // TODO: I mean, really, fix this
                auto camera_size_backup = m_scene.get_scene().get_camera().get_size().y / 2;
                m_scene.get_scene().get_camera().set_size(1);
                transform::UnitVector collider_offset
                    = collider->get_inner_collider()->get_position().to<transform::Units::ScenePixels>();
                m_colliders.at(tile_data_index)
                    ->get_inner_collider()
                    ->set_position(
                        transform::UnitVector(x * tileset.get_tile_width() + collider_offset.x,
                            y * tileset.get_tile_height() + collider_offset.y,
                            transform::Units::ScenePixels));
                // Backup camera size
                m_scene.get_scene().get_camera().set_size(camera_size_backup);
            }
        }
        for (const auto& game_object : m_scene.get_game_objects_models())
        {
            if (game_object.at("tileId").as<vili::integer>() == tile_info.tile_id)
            {
                std::string game_object_id = utils::string::replace(game_object.at("id"), "{index}",
                    std::to_string(m_scene.get_scene().get_game_object_amount()));
                vili::node requirements = game_object.at("Requires");
                transform::UnitVector game_object_position(x * tileset.get_tile_width(),
                    y * tileset.get_tile_height(), transform::Units::ScenePixels);
                requirements["x"] = requirements["x"].as_number() + game_object_position.x;
                requirements["y"] = requirements["y"].as_number() + game_object_position.y;
                m_scene.get_scene()
                    .create_game_object(game_object.at("type"), game_object_id)
                    .init_from_vili(requirements);
            }
        }
        m_positions[tile_index] = quad;

        const uint32_t tile_width = tileset.get_tile_width();
        const uint32_t tile_height = tileset.get_tile_height();

        const int texture_x
            = (tile_info.tile_id - first_tile_id) % (tileset.get_image_width() / tile_width);
        const int texture_y
            = (tile_info.tile_id - first_tile_id) / (tileset.get_image_width() / tile_width);

        TextureQuadsIndex quads;
        quads.transform(tile_info);

        quad[0].position = sf::Vector2f(x * tile_width, y * tile_height);
        quad[1].position = sf::Vector2f((x + 1) * tile_width, y * tile_height);
        quad[2].position = sf::Vector2f((x + 1) * tile_width, (y + 1) * tile_height);
        quad[3].position = sf::Vector2f(x * tile_width, (y + 1) * tile_height);

        quad[quads.q0].texCoords = sf::Vector2f(texture_x * tile_width, texture_y * tile_height);
        quad[quads.q1].texCoords
            = sf::Vector2f((texture_x + 1) * tile_width, texture_y * tile_height);
        quad[quads.q2].texCoords
            = sf::Vector2f((texture_x + 1) * tile_width, (texture_y + 1) * tile_height);
        quad[quads.q3].texCoords
            = sf::Vector2f(texture_x * tile_width, (texture_y + 1) * tile_height);
    }

    void TileLayer::clear_tile(uint32_t x, uint32_t y)
    {
        const uint32_t tile_index = y + x * m_height;
        const uint32_t tile_data_index = x + y * m_width;
        sf::Vertex* quad = m_positions[tile_index];
        const uint32_t old_tile_id = m_data[tile_data_index];
        for (const auto& animation : m_scene.get_animated_tiles())
        {
            if (animation->get_id() == old_tile_id)
            {
                animation->detach_quad(quad);
                break;
            }
        }
        if (const auto tile_collision = m_colliders.find(tile_data_index);
            tile_collision != m_colliders.end())
        {
            m_scene.get_scene().remove_collider(tile_collision->second->get_id());
            m_colliders.erase(tile_collision);
        }

        // TODO: Clear GameObjects when necessary
        this->clear_quad(quad);
        m_positions.erase(tile_index);
    }

    void TileLayer::update_quad(sf::Vertex* quad, uint32_t tile_id) const
    {
        if (!tile_id)
            return;

        const TileInfo tile_info = get_tile_info(tile_id);
        const Tileset& tileset = m_scene.get_tilesets().tileset_from_tile_id(tile_info.tile_id);

        const uint32_t first_tile_id = tileset.get_first_tile_id();

        const uint32_t tile_width = tileset.get_tile_width();
        const uint32_t tile_height = tileset.get_tile_height();

        const int texture_x = (tile_id - first_tile_id) % (tileset.get_image_width() / tile_width);
        const int texture_y = (tile_id - first_tile_id) / (tileset.get_image_width() / tile_width);

        TextureQuadsIndex quads;
        quads.transform(tile_info);

        quad[quads.q0].texCoords = sf::Vector2f(texture_x * tile_width, texture_y * tile_height);
        quad[quads.q1].texCoords
            = sf::Vector2f((texture_x + 1) * tile_width, texture_y * tile_height);
        quad[quads.q2].texCoords
            = sf::Vector2f((texture_x + 1) * tile_width, (texture_y + 1) * tile_height);
        quad[quads.q3].texCoords
            = sf::Vector2f(texture_x * tile_width, (texture_y + 1) * tile_height);
    }

    void TileLayer::clear_quad(sf::Vertex* quad)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            quad[i].position = sf::Vector2f(0, 0);
            quad[i].texCoords = sf::Vector2f(0, 0);
        }
    }

    TileLayer::TileLayer(const TileScene& scene, const std::string& id, int32_t layer,
        int32_t sublayer, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        std::vector<uint32_t> data, bool visible)
        : Renderable(layer, sublayer)
        , m_scene(scene)
        , m_id(id)
        , m_x(x)
        , m_y(y)
        , m_width(width)
        , m_height(height)
        , m_data(std::move(data))
        , m_visible(visible)
    {
    }

    std::string TileLayer::get_id() const
    {
        return m_id;
    }

    void TileLayer::build()
    {
        m_sublayers_by_tileset.clear();
        for (const uint32_t first_tile_id : m_scene.get_tilesets().get_tilesets_first_tiles_ids())
        {
            m_sublayers_by_tileset[first_tile_id] = sfe::SpannableVertexArray {};
            m_sublayers_by_tileset[first_tile_id].setPrimitiveType(sf::Quads);
            m_sublayers_by_tileset[first_tile_id].resize(m_width * m_height * 4);
        }

        for (unsigned int x = 0; x < m_width; ++x)
        {
            for (unsigned int y = 0; y < m_height; ++y)
            {
                const uint32_t tile_data_index = x + y * m_width;
                const uint32_t tile_id = m_data[tile_data_index];
                build_tile(x, y, tile_id);
            }
        }
    }

    void TileLayer::draw(graphics::RenderTarget& surface, const scene::Camera& camera)
    {
        if (!m_visible)
        {
            return;
        }

        sf::RenderStates states;
        states.transform = sf::Transform::Identity;

        const transform::UnitVector middle_camera
            = camera.get_position(transform::Referential::Center)
                  .to<transform::Units::SceneUnits>();
        const transform::UnitVector camera_size = camera.get_size();

        const float middle_x = transform::UnitVector::Screen.w / 2.0;
        const float middle_y = transform::UnitVector::Screen.h / 2.0;

        // Scale layers based on camera size
        const double camera_scale = 1.0 / (camera_size.y / 2.0);
        states.transform.scale(camera_scale, camera_scale, middle_x, middle_y);

        float translate_x = -(middle_camera.x * (transform::UnitVector::Screen.h / 2.f))
            + (transform::UnitVector::Screen.w / 2);
        float translate_y = -(middle_camera.y * (transform::UnitVector::Screen.h / 2.f))
            + (transform::UnitVector::Screen.h / 2);

        // Translate layers based on camera position
        if (!m_scene.is_anti_aliased())
        {
            translate_x = std::round(translate_x);
            translate_y = std::round(translate_y);
        }
        states.transform.translate(translate_x, translate_y);

        const transform::UnitVector camera_position = camera.get_position();
        const int64_t camera_x = (camera_position.x * transform::UnitVector::Screen.h / 2.f);
        const int64_t camera_width = std::ceil(camera_size.y * transform::UnitVector::Screen.w / 2);

        uint32_t vertex_count = 0;

        for (auto& [first_tile_id, layer] : m_sublayers_by_tileset)
        {
            const Tileset& tileset = m_scene.get_tilesets().tileset_from_tile_id(first_tile_id);
            states.texture = &tileset.get_texture().operator const sf::Texture&();

            // Finding out VertexArray rendering span based on camera size and position
            const uint32_t tileset_tile_width = tileset.get_tile_width();
            const uint32_t tileset_tile_height = tileset.get_tile_width();
            const size_t span_start
                = std::max(0ll, static_cast<long long int>(((camera_x / tileset_tile_width)) * 4 * m_height));
            const size_t span_end = std::min(layer.getVertexCount(),
                static_cast<size_t>((((camera_x + camera_width) / tileset_tile_width) + 1) * 4 * m_height));
            layer.setSpan(span_start, span_end);

            // Rendering entire VertexArray
            surface.draw(layer, states);
        }
    }

    void TileLayer::set_tile(uint32_t x, uint32_t y, uint32_t tile_id)
    {
        if (x >= m_width || y >= m_height || x < 0 || y < 0)
        {
            throw exceptions::TilePositionOutsideLayer(x, y, m_width, m_height, EXC_INFO);
        }
        const uint32_t tile_data_index = x + y * m_width;
        const uint32_t old_tile_id = m_data[tile_data_index];
        if (old_tile_id)
        {
            this->clear_tile(x, y);
        }
        m_data[tile_data_index] = tile_id;
        this->build_tile(x, y, tile_id);
    }

    uint32_t TileLayer::get_tile(uint32_t x, uint32_t y) const
    {
        if (x >= m_width || y >= m_height || x < 0 || y < 0)
        {
            throw exceptions::TilePositionOutsideLayer(x, y, m_width, m_height, EXC_INFO);
        }
        const uint32_t tile_data_index = x + y * m_width;
        return m_data[tile_data_index];
    }
}
