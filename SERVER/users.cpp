#include "auth/UsersFile.hpp"
#include <iostream>

int main()
{
    UsersFile &uf = UsersFile::getInstance();

    if (uf.addUser("patricia", "parola123"))
        std::cout << "User creat cu succes!\n";
    else
        std::cout << "Userul exista deja!\n";

    if (uf.authenticate("patricia", "parola123"))
        std::cout << "Autentificare reusita!\n";
    else
        std::cout << "Autentificare esuata!\n";

    UsersFile::destroyInstance();
    return 0;
}