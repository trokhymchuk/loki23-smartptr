#include <iostream>
#include <memory>
#include <string>

import loki23;
import loki23.factory;

struct IShape {
    virtual std::string describe() const = 0;
    virtual std::unique_ptr<IShape> clone() const = 0;
    virtual ~IShape() = default;
};

struct Circle : IShape {
    static constexpr std::string_view type_name = "Circle";
    std::string describe() const override { return "Circle (r=1)"; }
    std::unique_ptr<IShape> clone()    const override { return std::make_unique<Circle>(*this); }
    static std::unique_ptr<IShape>  create() { return std::make_unique<Circle>(); }
};

struct Square : IShape {
    static constexpr std::string_view type_name = "Square";
    std::string describe() const override { return "Square (side=1)"; }
    std::unique_ptr<IShape> clone()    const override { return std::make_unique<Square>(*this); }
    static std::unique_ptr<IShape>  create() { return std::make_unique<Square>(); }
};

struct Triangle : IShape {
    static constexpr std::string_view type_name = "Triangle";
    std::string describe() const override { return "Triangle (equilateral)"; }
    std::unique_ptr<IShape> clone()    const override { return std::make_unique<Triangle>(*this); }
    static std::unique_ptr<IShape>  create() { return std::make_unique<Triangle>(); }
};

struct Pentagon : IShape {
    static constexpr std::string_view type_name = "Pentagon";
    std::string describe() const override { return "Pentagon (regular)"; }
    std::unique_ptr<IShape> clone()    const override { return std::make_unique<Pentagon>(*this); }
    static std::unique_ptr<IShape>  create() { return std::make_unique<Pentagon>(); }
};

using Shapes = type_list<Circle, Square, Triangle, Pentagon>;

int main() {
    std::cout << "=== Policy-based Factory ===\n\n";

    std::cout << "[1] Factory<NewCreation>  (default)\n";
    {
        Factory<IShape, Shapes> factory;

        for (std::string_view name : {"Circle", "Square", "Triangle", "Pentagon"}) {
            auto obj = factory.create(name);
            std::cout << "    " << obj->describe() << "\n";
        }

        try {
            factory.create("Hexagon");
        } catch (const std::invalid_argument &e) {
            std::cout << "    caught: " << e.what() << "\n";
        }
    }

    std::cout << "\n[2] Factory<FactoryMethodCreation>\n";
    {
        Factory<IShape, Shapes, FactoryMethodCreation> factory;

        for (std::string_view name : {"Circle", "Square", "Triangle", "Pentagon"}) {
            auto obj = factory.create(name);
            std::cout << "    " << obj->describe() << "\n";
        }
    }

    std::cout << "\n[3] HierarchyFactory  (compile-time, no map)\n";
    {
        HierarchyFactory<IShape, Shapes> factory;

        auto c = Field<Circle>  (factory).create();
        auto s = Field<Square>  (factory).create();
        auto t = Field<Triangle>(factory).create();
        auto p = Field<Pentagon>(factory).create();

        std::cout << "    " << c->describe() << "\n";
        std::cout << "    " << s->describe() << "\n";
        std::cout << "    " << t->describe() << "\n";
        std::cout << "    " << p->describe() << "\n";

        // Field<Hexagon>(factory) // error
    }

    std::cout << "\n=== Done ===\n";
}
