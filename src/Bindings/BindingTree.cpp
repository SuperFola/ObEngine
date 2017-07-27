#include <Bindings/BindingTree.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/VectorUtils.hpp>

namespace obe
{
    namespace Bindings
    {
        std::function<void(kaguya::State*)> RegisterLib = [](kaguya::State* lua)
        {
        };

        bool checkIfLuaElementExists(kaguya::State* lua, const std::string& path)
        {
            bool exists = (*lua)["LuaCore"]["Exists"](path);
            return exists;
        }

        BindingTree::BindingTree(BindingTree* parent, const std::string& id, std::function<void(kaguya::State*)> lib) : Identifiable(id)
        {
            m_parent = parent;
            m_lib = lib;
            m_hasLib = true;
        }

        BindingTree::BindingTree(BindingTree* parent, const std::string& id) : Identifiable(id)
        {
            m_parent = parent;
            m_hasLib = false;
        }

        BindingTree::BindingTree(const std::string& id) : Identifiable(id)
        {
            m_parent = nullptr;
            m_hasLib = false;
        }

        std::string BindingTree::getNodePath() const
        {
            std::vector<std::string> parentChain;
            BindingTree* currentParent = m_parent;
            if (currentParent != nullptr)
            {
                while (currentParent != nullptr)
                {
                    parentChain.push_back(currentParent->getId());
                    currentParent = currentParent->m_parent;
                }
                parentChain.pop_back();
                reverse(parentChain.begin(), parentChain.end());
                parentChain.push_back(m_id);
                std::string retPath = Utils::Vector::join(parentChain, ".");
                return retPath;
            }
            else
            {
                return m_id;
            }
        }

        BindingTree& BindingTree::operator[](const std::string& id)
        {
            for (auto& tree : m_children)
            {
                if (tree->getId() == id)
                    return *tree.get();
            }
            // Add error <REVISION>
        }

        BindingTree& BindingTree::add(const std::string& id, std::function<void(kaguya::State*)> lib)
        {
            if (!m_hasLib)
            {
                m_children.push_back(std::make_unique<BindingTree>(this, id, lib));
                return *this;
            }
            else
            {
                throw aube::ErrorHandler::Raise("ObEngine.Bindings.BindingTree.TerminalNodeCantHaveChild");
            }
            
        }

        BindingTree& BindingTree::add(const std::string& id)
        {
            if (!m_hasLib)
            {
                m_children.push_back(std::make_unique<BindingTree>(this, id));
                return *this;
            }
            else
            {
                throw aube::ErrorHandler::Raise("ObEngine.Bindings.BindingTree.TerminalNodeCantHaveChild");
            }
        }

        BindingTree& BindingTree::walkTo(std::vector<std::string> path)
        {
            if (this->exists(path[0]))
            {
                if (path.size() > 1)
                {
                    std::vector<std::string> subPath = Utils::Vector::getSubVector(path, 1, 0);
                    return this->operator[](path[0]).walkTo(subPath);
                }
                if (path.size() == 1)
                {
                    return this->operator[](path[0]);
                }
            }
            else
            {
                throw aube::ErrorHandler::Raise("ObEngine.Bindings.BindingTree.ChildNotFound", { {"id", this->getNodePath() }, {"child", path[0] }
            });
                // Raise Error <REVISION>
            }
        }

        bool BindingTree::exists(const std::string& id)
        {
            for (auto& tree : m_children)
            {
                if (tree->getId() == id)
                    return true;
            }
            return false;
        }

        void BindingTree::operator()(kaguya::State* lua, bool spreads)
        {
            std::vector<BindingTree*> createNodePath;
            BindingTree* currentParent = m_parent;
            if (currentParent != nullptr)
            {
                while (currentParent != nullptr)
                {
                    createNodePath.push_back(currentParent);
                    currentParent = currentParent->m_parent;
                }
                createNodePath.pop_back();
                reverse(createNodePath.begin(), createNodePath.end());
                for (BindingTree* parentNode : createNodePath)
                {
                    (*parentNode)(lua, false);
                }
            }
            bool elementAlreadyExists = checkIfLuaElementExists(lua, this->getNodePath());
            if (!elementAlreadyExists && !m_hasLib)
            {
                std::cout << "Doesn't exists, create table : " << this->getNodePath() << std::endl;
                (*lua)(this->getNodePath() + " = {};");
            }
            else if (!elementAlreadyExists)
            {
                std::cout << "Doesn't exists, LoadLib : " << this->getNodePath() << std::endl;
                m_lib(lua);
            }
            if (spreads)
            {
                for (auto& tree : m_children)
                {
                    (*tree)(lua);
                }
            }
        }
    }
}