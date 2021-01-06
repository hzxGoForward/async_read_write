#include <string>
#include <iostream>

int main() {
	char buff[1024] = {"hello world"};
	std::string str(buff);
	std::cout << str.size() << std::endl;
	return 0;
}
