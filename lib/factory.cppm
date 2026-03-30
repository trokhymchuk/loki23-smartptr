module;

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

export module loki23.factory;
import loki23;

export template <typename T>
concept HasTypeName = requires {
    { T::type_name } -> std::convertible_to<std::string_view>;
};

export template <typename Policy, typename Base, typename Derived>
concept FactoryCreationPolicy = requires {
    { Policy::template create<Base, Derived>() } -> std::same_as<SmartPtr<Base, NoCopy>>;
};

export struct NewCreation {
    template <typename Base, typename Derived>
    static SmartPtr<Base, NoCopy> create() {
        return SmartPtr<Base, NoCopy>{new Derived()};
    }
};

export struct FactoryMethodCreation {
    template <typename Base, typename Derived>
    static SmartPtr<Base, NoCopy> create() {
        return SmartPtr<Base, NoCopy>{Derived::create().release()};
    }
};

export template <typename Base, typename TypeListT,
                 typename CreationPolicy = NewCreation>
class Factory;

export template <typename Base, typename... Types, typename CreationPolicy>
    requires(HasTypeName<Types> && ...) &&
            (FactoryCreationPolicy<CreationPolicy, Base, Types> && ...)
class Factory<Base, type_list<Types...>, CreationPolicy> {
    using Creator = std::function<SmartPtr<Base, NoCopy>()>;
    std::unordered_map<std::string_view, Creator> m_registry;

  public:
    Factory() {
        (m_registry.emplace(Types::type_name,
                            [] { return CreationPolicy::template create<Base, Types>(); }),
         ...);
    }

    SmartPtr<Base, NoCopy> create(std::string_view name) const {
        auto it = m_registry.find(name);
        if (it == m_registry.end())
            throw std::invalid_argument("Factory: unknown type '" +
                                        std::string(name) + "'");
        return it->second();
    }

    bool contains(std::string_view name) const noexcept {
        return m_registry.count(name) > 0;
    }
};


export template <typename Base, typename CreationPolicy>
struct MakeUnit {
    template <typename T>
    struct Unit {
        SmartPtr<Base, NoCopy> create() {
            return CreationPolicy::template create<Base, T>();
        }
    };
};

export template <typename Base, typename TypeListT,
                 typename CreationPolicy = NewCreation>
struct HierarchyFactory;

export template <typename Base, typename... Types, typename CreationPolicy>
    requires(FactoryCreationPolicy<CreationPolicy, Base, Types> && ...)
struct HierarchyFactory<Base, type_list<Types...>, CreationPolicy>
    : GenScatterHierarchy<MakeUnit<Base, CreationPolicy>::template Unit,
                          type_list<Types...>> {};
