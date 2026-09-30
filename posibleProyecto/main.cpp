#include <soil2/SOIL2.h>
#include <iostream>

int main()
{
    std::cout << "SOIL2 version (cabecera): "
        << SOIL_MAJOR_VERSION << "." << SOIL_MINOR_VERSION << "." << SOIL_PATCH_LEVEL << "\n";
    std::cout << "SOIL2 version (libreria): " << SOIL_version() << "\n";
    return 0;
}