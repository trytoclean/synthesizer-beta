#include <iostream>
int main(int argc, char* argv[]) {
  int x{};
  if (x == 5) {
    std::cout << "yes ";
  } else {
    std::cout << "no ";
  }

  for (int i = 0; i < 10; ++i) {
    x = 1 + x * i;
  }
  return 0;
}
