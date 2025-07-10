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
    virtual std::vector<std::string>& GetPropertyList() = 0;
    virtual void* GetVoidPropertyByIndex(int prop_index) = 0;
    virtual void* GetVoidPropertyByName(const std::string& prop_name) = 0;
    // virtual std::vector<std::typeinfo> GetPropertyTypeinfos() = 0;
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
    std::vector<std::string>& GetPropertyList() override {
        static std::vector<std::string> property_list = {
            "x",
            "y",
            "z"
        };
        return property_list;
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
        static std::unordered_map<std::string, int> prop_name_indexes = {
            {"x", 0},
            {"y", 1},
            {"z", 2}
        };
        auto it = prop_name_indexes.find(prop_name);
        if (it == prop_name_indexes.end()) {
            return 0;
        }
        return GetVoidPropertyByIndex(it->second);
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
    PositionComponent* e1_pos_com = 0, *e2_pos_com = 0;
    if (e1_ptr) {
        auto& e1 = *e1_ptr;
        e1_pos_com = &ecs.ConstructComponent<PositionComponent>(e1.id, PositionComponent(1.f, 1.f, 1.f));
    }
    auto e2_ptr = ecs.GetEntity(e2_id);
    if (e2_ptr) {
        auto& e2 = *e2_ptr;
        e2_pos_com = &ecs.ConstructComponent<PositionComponent>(e2.id, PositionComponent(2.f, 2.f, 2.f));
    }


    while (window_poll_events(window)) {
        window_render(window);
    }
}