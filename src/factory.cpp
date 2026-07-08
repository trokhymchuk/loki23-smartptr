#include <iostream>
#include <memory>
#include <string>

import loki23;
import loki23.factory;

struct Shape {
    virtual std::string describe() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual ~Shape() = default;
};

struct Circle : Shape {
    static constexpr std::string_view type_name = "Circle";
    std::string describe() const override { return "Circle (r=1)"; }
    std::unique_ptr<Shape> clone()    const override { return std::make_unique<Circle>(*this); }
    static std::unique_ptr<Shape>  create() { return std::make_unique<Circle>(); }
};

struct Square : Shape {
    static constexpr std::string_view type_name = "Square";
    std::string describe() const override { return "Square (side=1)"; }
    std::unique_ptr<Shape> clone()    const override { return std::make_unique<Square>(*this); }
    static std::unique_ptr<Shape>  create() { return std::make_unique<Square>(); }
};

struct Triangle : Shape {
    static constexpr std::string_view type_name = "Triangle";
    std::string describe() const override { return "Triangle (equilateral)"; }
    std::unique_ptr<Shape> clone()    const override { return std::make_unique<Triangle>(*this); }
    static std::unique_ptr<Shape>  create() { return std::make_unique<Triangle>(); }
};

struct Pentagon : Shape {
    static constexpr std::string_view type_name = "Pentagon";
    std::string describe() const override { return "Pentagon (regular)"; }
    std::unique_ptr<Shape> clone()    const override { return std::make_unique<Pentagon>(*this); }
    static std::unique_ptr<Shape>  create() { return std::make_unique<Pentagon>(); }
};

using Shapes = type_list<Circle, Square, Triangle, Pentagon>;

int main() {
    std::cout << "=== Policy-based Factory ===\n\n";

    std::cout << "[1] Factory<NewCreation>  (default)\n";
    {
        Factory<Shape, Shapes> factory;

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
        Factory<Shape, Shapes, FactoryMethodCreation> factory;

        for (std::string_view name : {"Circle", "Square", "Triangle", "Pentagon"}) {
            auto obj = factory.create(name);
            std::cout << "    " << obj->describe() << "\n";
        }
    }

    std::cout << "\n[3] HierarchyFactory  (compile-time, no map)\n";
    {
        HierarchyFactory<Shape, Shapes> factory;

        auto c = Field<Circle>  (factory).create();
        auto s = Field<Square>  (factory).create();
        auto t = Field<Triangle>(factory).create();
        auto p = Field<Pentagon>(factory).create();

        std::cout << "    " << c->describe() << "\n";
        std::cout << "    " << s->describe() << "\n";
        std::cout << "    " << t->describe() << "\n";
        std::cout << "    " << p->describe() << "\n";
    }

    std::cout << "\n=== Done ===\n";
}
