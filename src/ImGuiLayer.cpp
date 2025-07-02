std::unordered_map<size_t, float> ImGuiLayer::id_priority_map;
std::map<float, std::map<size_t, ImGuiLayer::ImmediateDrawPair>> ImGuiLayer::priority_immediate_draw_fn_map;

bool ImGuiLayer::Init(/*...*/)
{
    return true;
}

void ImGuiLayer::Render()
{
    return;
}

size_t ImGuiLayer::AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn)
{
    auto id = GlobalUID::GetNew();
    auto& map = priority_immediate_draw_fn_map[priority];
    ImmediateDrawPair pair{key, fn};
    map[id] = pair;
    id_priority_map[id] = priority;
    return id;
}

bool ImGuiLayer::RemoveImmediateDrawFunction(size_t id)
{
    auto priority_it = id_priority_map.find(id);
    if (priority_it == id_priority_map.end()) {
        return false;
    }
    auto& priority = priority_it->second;
    auto& map = priority_immediate_draw_fn_map[priority];
    auto id_it = map.find(id);
    if (id_it == map.end()) {
        return false;
    }
    map.erase(id_it);
    return true;
}
