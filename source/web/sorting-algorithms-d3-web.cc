//  This file is part of sorting-algorithms-d3
//  Copyright (C) Oliver Baldwin Edwards, 2020.
//  Released under MIT license; see LICENSE

// #include <iostream>

// #include "web/web.h"

// #include "../example.h"

// namespace UI = emp::web;

// UI::Document doc("emp_base");

// int main()
// {
//   doc << "<h1>Hello, browser!</h1>";
//   std::cout << "Hello, console!" << std::endl;
//   return example();
// }

#include <iostream>
#include <vector>
#include <string>

#include "web/init.h"
#include "web/web.h"
#include "web/Document.h"

#include "../example.h"

#include "web/js_utils.h"
#include "base/map.h"

#include "web/d3/d3_init.h"
#include "web/d3/selection.h"
#include "web/d3/transition.h"
#include "web/d3/scales.h"
#include "web/d3/utils.h"

namespace UI = emp::web;
UI::Document doc("emp_base");

void CreateDivScratch() {
  // D3::Selection test_select = D3::Select("#emp_d3_test").Append("svg").SetAttr("width", 400).SetAttr("height", 400);
  
  D3::LinearScale test;
  test.SetDomain(0, 1000).SetRange(0, 500);
  int result = test.ApplyScaleInt(500);

  std::cout << result << std::endl;
}

int main() {
  D3::internal::get_emp_d3();
  CreateDivScratch();
  doc << "<h1>Hello, browser!</h1>";
  std::cout << "Hello, console!" << std::endl;
  return example();
  return 0;
}
