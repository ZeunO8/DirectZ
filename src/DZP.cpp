#include <DirectZ.hpp>

void print_help();

int main(int argc, char** argv)
{
    ProgramArgs args(argc, argv);
    auto o_iter = args.options.find("o");
    if (o_iter == args.options.end())
    {
        print_help();
        return 0;
    }
    auto& o = o_iter->second;
    FileHandle asset_handle{FileHandle::PATH, o};
    auto asset_pack = create_asset_pack(asset_handle);
    for (auto& input_file_name : args.arguments)
    {
        FileHandle file_handle{FileHandle::PATH, input_file_name};
        add_asset(asset_pack, file_handle);
    }
    free_asset_pack(asset_pack);
    return 0;
}

void print_help()
{
    std::cout << "DZP Usage: \"dzp -o outpack.bin infile.txt ifile2.txt\"" << std::endl;
}