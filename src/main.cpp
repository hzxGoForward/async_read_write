#include "boost_read_hzx.h"
#include <iostream>

int main()
{
    try
    {
        rpw_test();
    }
    catch (std::exception &e)
    {
        std::cout << "find a exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "unknown exception" << std::endl;
    }


    return 0;
}
