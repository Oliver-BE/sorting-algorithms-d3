//  This file is part of sorting-algorithms-d3
//  Copyright (C) Oliver Baldwin Edwards, 2020.
//  Released under MIT license; see LICENSE

#include <iostream>

#include "base/vector.h"
#include "config/command_line.h"

#include "../example.h"

// This is the main function for the NATIVE version of sorting-algorithms-d3.

int main(int argc, char* argv[])
{
  emp::vector<std::string> args = emp::cl::args_to_strings(argc, argv);

  std::cout << "Hello, world!" << std::endl;

  return example();
}
