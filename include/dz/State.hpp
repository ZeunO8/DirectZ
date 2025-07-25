#pragma once

#include <filesystem>
#include <ostream>
#include <iostreams/Serial.hpp>

#define CID_WINDOW 1
#define CID_MIN 2

namespace dz {
    struct WINDOW;

    void track_static_state(
        int sid,
        const std::function<bool(Serial&)>& restore,
        const std::function<bool(Serial&)>& backup
    );

    struct StaticRestorable {

        template <typename T>
        inline static int GetSID() {
            if constexpr (requires { T::SID; })
                return T::SID;
            return 0;
        }

        template <typename T>
        inline static void InitStaticRestorable() {
            if constexpr (requires { T::RestoreFunction; } && requires { T::BackupFunction; })
                track_static_state(GetSID<T>(), T::RestoreFunction, T::BackupFunction);
        }
    };

    struct Restorable {
        virtual ~Restorable() = default;

        /**
        * @brief Returns the Constructor ID for this Restorable
        *
        * @note typename T should implement a unique constant value CID
        */
        template <typename T>
        inline static int GetCID() {
            if constexpr (requires { T::CID; }) {
                return T::CID;
            }
            else {
                return 0;
            }
        }

        /**
        * @brief Returns the Constructor ID at runtime
        *
        *
        */
        virtual int getCID() { return 0; }

        /**
        * @brief virtual method for writing the Restorable data
        */
        virtual bool backup(Serial& serial) { return true; }

        /**
        * @brief virtual method for reading the Restorable data
        */
        virtual bool restore(Serial& serial) { return true; }
    };

    /**
    * @brief Sets the State file path
    */
    void set_state_file_path(const std::filesystem::path& path);

    /**
    * @brief Overrides the States istream (Deserializing) (reading data)
    */
    void set_state_istream(std::istream& istream);

    /**
    * @brief Overrides the States ostream (Serializing) (writing data)
    */
    void set_state_ostream(std::ostream& ostream);

    /**
    * @brief Registers a constructor function such that State can accurately restore
    */
    void register_restorable_constructor(int c_id, const std::function<Restorable*(Serial&)>& constructor_fn);

    /**
    * @brief Returns a bool value indicating whether the state has date
    *
    * @note will return true only when a state has been saved and is available in path or istream
    */
    bool has_state();

    /**
    * @brief Returns a bool value indicating tracking state
    *
    * @note will return true only when either set_state_file_path or both (_state_istream & _state_ostream) have been called
    */
    bool is_tracking_state();

    /**
    * @brief Returns a bool value indicating state loaded
    *
    * @note will return true only when load_state() has been called
    */
    bool is_state_loaded();

    /**
    * @brief Saves the state to file or ostream
    *
    * @note Typically you would call this inside a window_register_free_callback (root window) 
    */
    bool save_state();

    /**
    * @brief Loads the State from file or istream
    *
    * @note This should be called after checking has_state()
    * @note if (has_state()) { load_state(); } else { your_init(); }
    */
    bool load_state();

    /**
    * @brief Frees any Restorables that are owned by the StateHolder
    */
    bool free_state();

    /**
    * @brief Tracks the state of a restorable by pointer
    *
    * @note Only needs to be called the first run of a stateful program, future loads do not need to call
    */
    void track_state(Restorable* restorable_ptr);

    /**
    * @brief Tracks a Windows state (size, xy, title)
    *
    * @note windows created by ImGui will be automatically tracked 
    */
    void track_window_state(WINDOW* window_ptr);
    
    /**
    * @brief returns a restorable at the index given with Constructor ID
    */
    Restorable* state_get_restorable_ptr(int cid, int index = 0);

    /**
    * @brief casts a restorable at the index given with Constructor ID
    */
    template <typename T>
    T* state_get_ptr(int cid, int index = 0) {
        return (T*)(state_get_restorable_ptr(cid, index));
    }
}