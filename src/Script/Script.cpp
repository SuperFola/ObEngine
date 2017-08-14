#include <vili/Vili.hpp>

#include <Bindings/Bindings.hpp>
#include <Collision/PolygonalCollider.hpp>
#include <Debug/Console.hpp>
#include <Graphics/Canvas.hpp>
#include <Input/InputManager.hpp>
#include <Script/Script.hpp>
#include <System/Cursor.hpp>
#include <Triggers/TriggerGroup.hpp>
#include <Triggers/TriggerDatabase.hpp>
#include <Utils/TypeUtils.hpp>

namespace obe
{
    namespace Script
    {
        CoreHook hookCore;

        void CoreHook::getValue(kaguya::State* lua, std::string name)
        {
            if (containerMap.find(name) != containerMap.end())
            {
                std::string hookType = containerMap[name].first;
                if (hookType == Utils::Type::getClassType<Debug::Console*>())
                {
                    Bindings::Load(lua, "Core.Debug.Console");
                    (*lua)["Hook"][name] = containerMap[name].second->as<Debug::Console*>();
                }
                else if (hookType == Utils::Type::getClassType<System::Cursor*>())
                {
                    Bindings::Load(lua, "Core.System.Cursor");
                    (*lua)["Hook"][name] = containerMap[name].second->as<System::Cursor*>();
                }
                else if (hookType == Utils::Type::getClassType<Input::InputManager*>())
                {
                    Bindings::Load(lua, "Core.Input.InputManager");
                    (*lua)["Hook"][name] = containerMap[name].second->as<Input::InputManager*>();
                }
                else if (hookType == Utils::Type::getClassType<Triggers::TriggerDatabase*>())
                {
                    Bindings::Load(lua, "Core.Triggers.TriggerDatabase");
                    (*lua)["Hook"][name] = containerMap[name].second->as<Triggers::TriggerDatabase*>();
                }
                else if (hookType == Utils::Type::getClassType<Triggers::TriggerGroup*>())
                {
                    Bindings::Load(lua, "Core.Triggers.TriggerGroup");
                    (*lua)["Hook"][name] = containerMap[name].second->as<Triggers::TriggerGroup*>();
                }
                else
                    throw aube::ErrorHandler::Raise("ObEngine.Script.CoreHook.UnknownHookType", {{"type", hookType}, {"name", name}});
            }
            else
                throw aube::ErrorHandler::Raise("ObEngine.Script.CoreHook.UnknownHookName", {{"name", name}});
        }

        Types::Any* CoreHook::getPointer(std::string name)
        {
            if (containerMap.find(name) != containerMap.end())
            {
                return containerMap[name].second;
            }
            throw aube::ErrorHandler::Raise("ObEngine.Script.CoreHook.UnknownPointerName", {{"name", name}});
        }

        void loadHook(kaguya::State* lua, std::string hookname)
        {
            hookCore.getValue(lua, hookname);
        }
    }
}
