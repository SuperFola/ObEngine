#pragma once

#include <Exception.hpp>

/**
 * \nobind
 */
namespace obe::component::exceptions
{
    class ComponentIdAlreadyTaken : public Exception<ComponentIdAlreadyTaken>
    {
    public:
        using Exception::Exception;
        ComponentIdAlreadyTaken(std::string_view id, DebugInfo info)
            : Exception(info)
        {
            this->error("Component with id '{}' already exists", id);
        }
    };

    class UnknownComponentType : public Exception<UnknownComponentType>
    {
    public:
        using Exception::Exception;
        UnknownComponentType(std::string_view component_type, DebugInfo info)
            : Exception(info)
        {
            this->error("Component with type '{}' does not exists or has not been registered",
                component_type);
        }
    };
} // namespace obe::component::exceptions
