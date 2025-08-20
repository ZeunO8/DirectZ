#include <DirectZ.hpp>

int main() {

    std::string guid;

    std::cin >> guid;

    dz::load_direct_registry_guid(guid);

    auto dr_ptr_guid = dz::get_direct_registry_guid();

    std::cout << dr_ptr_guid.to_string();

    std::string message;

    std::cin >> message;

    if (message == "Incorrect GUID!") {
        std::cout << "Failed to create Image!" << std::endl;
        return 1;
    }

    try {
        auto image = image_create({
            .width = 640,
            .height = 480,
            .create_shared = true
        });
        if (!image) {
            return 1;
        }
        std::cout << "Created Image with width(" <<
            std::to_string(image_get_width(image)) << ")" << std::endl;
        image_free(image);
    }
    catch (...) {
        std::cout << "Failed to create image" << std::endl;
    }


    return 0;
}