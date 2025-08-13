#include <dz/State.hpp>
#include <dz/ECS.hpp>
#include "Directz.cpp.hpp"

void dz::track_static_state(
    int sid,
    const std::function<bool(Serial&)>& restore,
    const std::function<bool(Serial&)>& backup
) {
    dr_ptr->stateHolder.static_restores[sid] = restore;
    dr_ptr->stateHolder.static_backups[sid] = backup;
}

void dz::set_state_file_path(const std::filesystem::path& path) {
    dr_ptr->stateHolder.path = path;
}

void dz::set_state_istream(std::istream& istream) {
    dr_ptr->stateHolder.istream_ptr = &istream;
    dr_ptr->stateHolder.use_istream = true;
}

void dz::set_state_ostream(std::ostream& ostream) {
    dr_ptr->stateHolder.ostream_ptr = &ostream;
    dr_ptr->stateHolder.use_ostream = true;
}

void dz::register_restorable_constructor(int c_id, const std::function<Restorable*(Serial&)>& constructor_fn) {
    StateHolder::c_id_fn_map[c_id] = constructor_fn;
}

std::tuple<Serial*, std::istream*, bool> get_owned_iserial() {
    Serial* serial_ptr = nullptr;
    bool own_local_istream = dr_ptr->stateHolder.use_istream;
    auto local_istream = dr_ptr->stateHolder.istream_ptr;
    if (!own_local_istream)
        local_istream = new std::ifstream(dr_ptr->stateHolder.path, std::ios::in | std::ios::binary);
    serial_ptr = new Serial(*local_istream);
    return {serial_ptr, local_istream, own_local_istream};
}

std::tuple<Serial*, std::ostream*, bool> get_owned_oserial() {
    Serial* serial_ptr = nullptr;
    bool own_local_ostream = dr_ptr->stateHolder.use_ostream;
    auto local_ostream = dr_ptr->stateHolder.ostream_ptr;
    if (!own_local_ostream)
        local_ostream = new std::ofstream(dr_ptr->stateHolder.path, std::ios::out | std::ios::binary | std::ios::trunc);
    serial_ptr = new Serial(*local_ostream);
    return {serial_ptr, local_ostream, own_local_ostream};
}

void disown_iserial(Serial* serial_ptr, std::istream* istream_ptr, bool owned) {
    serial_ptr->synchronize();
    delete serial_ptr;
    if (owned)
        delete istream_ptr;
}

void disown_oserial(Serial* serial_ptr, std::ostream* ostream_ptr, bool owned) {
    serial_ptr->synchronize();
    delete serial_ptr;
    if (owned)
        delete ostream_ptr;
}

bool dz::has_state() {
    auto [serial_ptr, istream_ptr, owned] = get_owned_iserial();
    auto& serial = *serial_ptr;
    auto state_has = serial.canRead() && serial.getReadLength() > 0;
    disown_iserial(serial_ptr, istream_ptr, owned);
    return state_has;
}

bool dz::is_tracking_state() {
    return !dr_ptr->stateHolder.path.empty();
}

bool dz::is_state_loaded() {
    return dr_ptr->stateHolder.loaded;
}

bool dz::save_state() {
    bool _saved = true;
    auto [serial_ptr, ostream_ptr, owned] = get_owned_oserial();
    auto& serial = *serial_ptr;
    auto& sh = dr_ptr->stateHolder;
    auto static_backups_size = sh.static_backups.size();
    serial << static_backups_size;
    for (auto& [sid, static_backup] : sh.static_backups) {
        serial << sid;
        if (!static_backup(serial))
            return false;
    }
    auto restorables_size = sh.restorables.size();
    serial << restorables_size;
    for (auto& restorer : sh.restorables) {
        serial << restorer.cid;
        auto& restorable = *restorer.restorable_ptr;
        if (!restorable.backup(serial)) {
            _saved = false;
            goto _return;
        }
    }
_return:
    disown_oserial(serial_ptr, ostream_ptr, owned);
    return _saved;
}

bool dz::load_state() {
    bool _loaded = true;
    auto [serial_ptr, istream_ptr, owned] = get_owned_iserial();
    auto& serial = *serial_ptr;
    auto& sh = dr_ptr->stateHolder;
    auto static_restores_size = sh.static_restores.size();
    serial >> static_restores_size;
    for (size_t restore_count = 1; restore_count <= static_restores_size; ++restore_count) {
        int sid = 0;
        serial >> sid;
        auto static_restore_it = sh.static_restores.find(sid);
        if (static_restore_it == sh.static_restores.end())
            return false;
        auto& static_restore = static_restore_it->second;
        if (!static_restore(serial))
            return false;
    }
    auto restorables_size = sh.restorables.size();
    serial >> restorables_size;
    for (size_t restorer_count = 1; restorer_count <= restorables_size; ++restorer_count) {
        int cid = 0;
        serial >> cid;
        auto c_fn_it = sh.c_id_fn_map.find(cid);
        if (c_fn_it == sh.c_id_fn_map.end()) {
            _loaded = false;
            goto _return;
        }
        auto restorable_ptr = c_fn_it->second(serial);
        StateHolder::Restorer restorer{
            restorable_ptr,
            cid == CID_WINDOW ? false : true,
            cid
        };
        sh.restorables.push_back(restorer);
    }
_return:
    disown_iserial(serial_ptr, istream_ptr, owned);
    dr_ptr->stateHolder.loaded = _loaded;
    return _loaded;
}

bool dz::free_state() {
    try {
        for (auto& restorer : dr_ptr->stateHolder.restorables) {
            if (restorer.owned_by_state_holder)
                delete restorer.restorable_ptr;
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

void dz::track_state(Restorable* restorable_ptr) {
    if (!restorable_ptr)
        return;
    StateHolder::Restorer restorer{
        restorable_ptr,
        false,
        restorable_ptr->getCID()
    };
    dr_ptr->stateHolder.restorables.push_back(restorer);
}

void dz::track_window_state(WINDOW* window_ptr) {
    StateHolder::Restorer restorer{
        window_ptr,
        false,
        WINDOW::CID
    };
    dr_ptr->stateHolder.restorables.push_back(restorer);
}

Restorable* dz::state_get_restorable_ptr(int cid, int index) {
    auto cid_index = 0;
    for (auto& restorer : dr_ptr->stateHolder.restorables) {
        if (restorer.cid == cid) {
            if (cid_index == index)
                return restorer.restorable_ptr;
            cid_index++;
        }
    }
    return nullptr;
}