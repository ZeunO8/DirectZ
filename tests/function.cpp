#include <dz/function.hpp>
#include <iostream>

struct _12Byte {
    int x;
    double y;
    bool set(int _x, double _y) {
        x = _x;
        y = _y;
        return true;
    }
};

int main()
{
    using FN_Def = dz::function<bool(int, const double&)>;
    {
        FN_Def my_int_only_triple([=](int i, const double& o) -> bool {
            ((double&)o) *= 3;
            std::cout << "i: " << i << ", o: " << o << std::endl;
            return true;
        });
        std::cout << (my_int_only_triple(1, 2.4) ? "true" : "false") << std::endl;
    }
    {
        _12Byte _12byte{1, 1.3};
        FN_Def my_set(&_12byte, &_12Byte::set);
        std::cout << (my_set(1, 2.4) ? "true" : "false") << std::endl;

        auto _12byte2 = _12byte;
        FN_Def my_13_set(&_12byte2, &_12Byte::set);
        std::cout << (my_13_set(131, 42.) ? "true" : "false") << std::endl;

        {
            FN_Def my_int_second_triple([_12byte, _12byte2](int i, const double& o) -> bool {
                ((double&)o) *= 3;
                std::cout << "i: " << i << ", o: " << o << std::endl;
                return true;
            });
            std::cout << (my_int_second_triple(3, 84.) ? "true" : "false") << std::endl;
        }
    }
    return 0;
}