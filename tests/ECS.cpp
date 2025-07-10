#include <DirectZ.hpp>

#include <typeinfo>

struct Entity {
    int id;
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Entity {
    int id;
};
)";
    }
};

struct Component {
    int id;
    virtual int GetPropertyIndexByName(const std::string& prop_name) = 0;
    virtual const std::vector<std::string>& GetPropertyNames() = 0;
    virtual void* GetVoidPropertyByIndex(int prop_index) = 0;
    virtual void* GetVoidPropertyByName(const std::string& prop_name) = 0;
    virtual const std::vector<const std::type_info*>& GetPropertyTypeinfos() = 0;
    template <typename T>
    T& GetPropertyByIndex(int prop_index) {
        return *(T*)GetVoidPropertyByIndex(prop_index);
    }
    template <typename T>
    T& GetPropertyByName(const std::string& prop_name) {
        return *(T*)GetVoidPropertyByName(prop_name);
    }
    virtual ~Component() = default;
};

struct PositionComponent : Component {
    float x = 0;
    float y = 0;
    float z = 0;
    PositionComponent(float x, float y, float z): 
        x(x),
        y(y),
        z(z)
    {};
    PositionComponent(const PositionComponent& other):
        x(other.x),
        y(other.y),
        z(other.z)
    {};
    PositionComponent& operator=(const PositionComponent& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
    int GetPropertyIndexByName(const std::string& prop_name) override {
        static std::unordered_map<std::string, int> prop_name_indexes = {
            {"x", 0},
            {"y", 1},
            {"z", 2}
        };
        auto it = prop_name_indexes.find(prop_name);
        if (it == prop_name_indexes.end()) {
            return -1;
        }
        return it->second;
    }
    const std::vector<std::string>& GetPropertyNames() override {
        static std::vector<std::string> prop_names = {
            "x",
            "y",
            "z"
        };
        return prop_names;
    }
    void* GetVoidPropertyByIndex(int prop_index) override {
        switch (prop_index) {
        case 0:
            return &x;
        case 1:
            return &y;
        case 2:
            return &z;
        }
        return 0;
    }
    void* GetVoidPropertyByName(const std::string& prop_name) override {
        auto prop_index = GetPropertyIndexByName(prop_name);
        if (prop_index == -1) {
            return 0;
        }
        return GetVoidPropertyByIndex(prop_index);
    }
    const std::vector<const std::type_info*>& GetPropertyTypeinfos() override {
        static const std::vector<const std::type_info*> typeinfos = {
            &typeid(float),
            &typeid(float),
            &typeid(float)
        };
        return typeinfos;
    }
};

// struct RotationComponent : Component {
//     float yaw;
//     float pitch;
//     float roll;
    
// };

struct System {
    int id;
    virtual ~System() = default;
};

int main() {
    auto window = window_create({
        .title = "ECS Test",
        .x = 0,
        .y = 240,
        .width = 1280,
        .height = 768,
        .borderless = false,
        .vsync = true
    });
    
    ECS<Entity, Component, System> ecs;

    auto e1_id = ecs.AddEntity({0});
    auto e2_id = ecs.AddEntity({0});

    auto e1_ptr = ecs.GetEntity(e1_id);
    assert(e1_ptr);
    auto& e1 = *e1_ptr;
    auto& e1_position_component = ecs.ConstructComponent<PositionComponent>(e1.id, PositionComponent(1.f, 1.f, 1.f));

    const auto& position_component_typeinfos = e1_position_component.GetPropertyTypeinfos();
    const auto& position_component_prop_names = e1_position_component.GetPropertyNames();
    for (auto& prop_name : position_component_prop_names) {
        auto prop_index = e1_position_component.GetPropertyIndexByName(prop_name);
        auto type_info = position_component_typeinfos[prop_index];
        if (*type_info == typeid(float)) {
            auto& value = e1_position_component.GetPropertyByIndex<float>(prop_index);
            std::cout << "Property " << prop_name << "<float>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(int)) {
            auto& value = e1_position_component.GetPropertyByIndex<int>(prop_index);
            std::cout << "Property " << prop_name << "<int>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(std::string)) {
            auto& value = e1_position_component.GetPropertyByIndex<std::string>(prop_index);
            std::cout << "Property " << prop_name << "<std::string>(" << value << ")" << std::endl;
        } else {
            // Handle unknown type
            std::cout << "Property " << prop_name << " has unknown type" << std::endl;
        }
    }

    auto e2_ptr = ecs.GetEntity(e2_id);
    assert(e2_ptr);
    auto& e2 = *e2_ptr;
    auto& e2_position_component = ecs.ConstructComponent<PositionComponent>(e2.id, PositionComponent(2.f, 2.f, 2.f));

    while (window_poll_events(window)) {
        window_render(window);
    }
}