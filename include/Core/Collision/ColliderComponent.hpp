#pragma once

#include <Collision/CapsuleCollider.hpp>
#include <Collision/CircleCollider.hpp>
#include <Collision/PolygonCollider.hpp>
#include <Collision/RectangleCollider.hpp>
#include <Collision/ComplexPolygonCollider.hpp>
#include <Component/Component.hpp>

#include <variant>

namespace obe::collision
{
    template <ColliderType type>
    struct collider_type_helper_t;
    template <>
    struct collider_type_helper_t<ColliderType::Collider>
    {
        static Collider* type;
    };
    template <>
    struct collider_type_helper_t<ColliderType::Capsule>
    {
        static CapsuleCollider* type;
    };
    template <>
    struct collider_type_helper_t<ColliderType::Circle>
    {
        static CircleCollider* type;
    };
    template <>
    struct collider_type_helper_t<ColliderType::Polygon>
    {
        static PolygonCollider* type;
    };
    template <>
    struct collider_type_helper_t<ColliderType::Rectangle>
    {
        static RectangleCollider* type;
    };
    template <>
    struct collider_type_helper_t<ColliderType::ComplexPolygon>
    {
        static ComplexPolygonCollider* type;
    };

    template <typename CheckType, typename Variant>
    struct is_variant_member;

    template <typename CheckType, typename... VariantTypes>
    struct is_variant_member<CheckType, std::variant<VariantTypes...>>
        : public std::disjunction<std::is_same<CheckType, VariantTypes>...>
    {
    };

    using ColliderTypes
        = std::variant<CapsuleCollider, CircleCollider, PolygonCollider, RectangleCollider, ComplexPolygonCollider>;
    template <typename ColliderClass>
    concept IsValidColliderClass = is_variant_member<ColliderClass, ColliderTypes>::value;

    class ColliderComponent : public component::Component<ColliderComponent>
    {
    private:
        ColliderTypes m_collider;
        transform::Units m_unit;

        void load_capsule(const vili::node& data);
        void load_circle(const vili::node& data);
        void load_polygon(const vili::node& data);
        void load_rectangle(const vili::node& data);
        void load_complex_polygon(const vili::node& data);

        [[nodiscard]] vili::node dump_capsule() const;
        [[nodiscard]] vili::node dump_circle() const;
        [[nodiscard]] vili::node dump_polygon() const;
        [[nodiscard]] vili::node dump_rectangle() const;
        [[nodiscard]] vili::node dump_complex_polygon() const;

    protected:
        [[nodiscard]] vili::node schema() const override;

    public:
        explicit ColliderComponent(const std::string& id);
        ColliderComponent(const ColliderComponent& other) = default;

        [[nodiscard]] vili::node dump() const override;
        void load(const vili::node& data) override;

        [[nodiscard]] ColliderType get_collider_type() const;
        /**
         * \nobind
         */
        static constexpr std::string_view ComponentType = "Collider";
        [[nodiscard]] std::string_view type() const override;

        [[nodiscard]] Collider* get_inner_collider();
        [[nodiscard]] const Collider* get_inner_collider() const;

        /**
         * \brief Retrieves casted collider
         * \thints
         * \thint{get_capsule_collider, ColliderClass=obe::collision::CapsuleCollider}
         * \thint{get_circle_collider, ColliderClass=obe::collision::CircleCollider}
         * \thint{get_polygon_collider, ColliderClass=obe::collision::PolygonCollider}
         * \thint{get_rectangle_collider, ColliderClass=obe::collision::RectangleCollider}
         * \thint{get_complex_polygon_collider, ColliderClass=obe::collision::ComplexPolygonCollider}
         * \endthints
         */
        template <IsValidColliderClass ColliderClass>
        ColliderClass* get_inner_collider();
    };

    template <IsValidColliderClass ColliderClass>
    ColliderClass* ColliderComponent::get_inner_collider()
    {
        return &std::get<ColliderClass>(m_collider);
    }
}
