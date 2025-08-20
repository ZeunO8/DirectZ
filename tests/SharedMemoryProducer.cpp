#include <DirectZ.hpp>

int main() {
    Process consumer_process("DZ_SharedMemoryConsumer.exe");

    auto dr_ptr_guid = get_direct_registry_guid();
    auto dr_ptr_guid_str = dr_ptr_guid.to_string();

    std::cout << "DirectRegistry GUID = " << dr_ptr_guid_str << std::endl;

    consumer_process.in << dr_ptr_guid_str << "\n";

    std::string return_dr_ptr_guid;

    consumer_process.out >> return_dr_ptr_guid;

    auto correct_dr_ptr = (return_dr_ptr_guid == dr_ptr_guid_str);

    std::cout << "Consumer Process Returned DirectRegistry GUID = " << return_dr_ptr_guid <<
                 ", Correct(" << (correct_dr_ptr ? "true" : "false") << ")" <<  std::endl;

    if (correct_dr_ptr) {
        consumer_process.in << "Correct GUID!\n";
    }
    else {
        consumer_process.in << "Incorrect GUID!\n";
    }

    std::string consumer_response;

    consumer_process.out >> consumer_response;

    std::cout << "consumer response: " << consumer_response << std::endl;

    std::string consumer_error;

    consumer_process.err >> consumer_error;

    std::cout << "consumer error: " << consumer_error << std::endl;

    // std::string image_width_str;

    // consumer_process >> image_width_str;

    // std::cout << "Consumer Image Width: " << image_width_str << std::endl;

    return 0;
}