//  This file is part of sorting-algorithms-d3
//  Copyright (C) Oliver Baldwin Edwards, 2020.
//  Released under MIT license; see LICENSE

#include <iostream>
#include <vector>
#include <string>

#include "web/init.h"
#include "web/web.h"
#include "web/Document.h"

#include "web/js_utils.h"
#include "base/map.h"

#include "web/d3/d3_init.h"
#include "web/d3/selection.h"
#include "web/d3/transition.h"
#include "web/d3/scales.h"
#include "web/d3/utils.h"

// namespace UI = emp::web;
emp::web::Document div_d3viz("emp_d3");
emp::web::Document div_d3viz_wrapper("emp_d3_wrapper");

void CreateD3Viz() {

  // div_d3viz.Freeze();

  EM_ASM({
    // set the dimensions and margins of the graph
    var margin = 30;
    var width = 450;
    var height = 400;

    var svg = d3.select("#emp_d3")
                  .append("svg")
                  .attr("width", width)
                  .attr("height", height);

    // Create data
    var data = ([ {x : 10, y : 20}, {x : 40, y : 90}, {x : 80, y : 50} ]);

    // X scale and Axis
    var x = d3.scaleLinear()
                .domain([ 0, 100 ])
                .range([ margin, width - margin ]);

    svg.append('g')
        .attr("transform", "translate(0," + (height - margin) + ")")
        .call(d3.axisBottom(x));

    // X scale and Axis
    var y = d3.scaleLinear()
                .domain([ 0, 100 ])
                .range([ height - margin, margin ]);

    svg.append('g')
        .attr("transform", "translate(" + margin + ",0)")
        .call(d3.axisLeft(y));

    // Add 3 dots for 0, 50 and 100%
    svg.selectAll("circle")
        .data(data)
        .enter()
        .append("circle")
        .attr("cx", function(d){return x(d.x)})
        .attr("cy", function(d){return y(d.y)})
        .attr("r", 7);

    console.log("in macro");
  });
}

void CreateD3Viz2() {

  // int margin = 30;
  int width = 450;
  int height = 400;

  D3::Selection svg = D3::Select("#emp_d3_wrapper")
                          .Append("svg")
                          .SetAttr("width", width)
                          .SetAttr("height", height);
  
  emp::vector<int> data = {10, 100, 300, 234, 53};
  std::function<int(int, int, int)> return_d =
      [](int d, int i, int j) { return d; };

  svg.SelectAll("circle")
      .Data(data)
      .EnterAppend("circle")
      .SetAttr("fill", "blue")
      .SetAttr("cx", 100)
      .SetAttr("cy", 200)
      .SetAttr("r", 1)
      .MakeTransition()
      .SetDuration(2000)
      .SetAttr("cx", return_d)
      .SetAttr("cy", return_d)
      .SetAttr("r", 7)
      .SetAttr("fill", "red");

  // EM_ASM({
  //   $("body").append('<div id="body_emp_test_container"></div>')
  // });
  // D3::Selection svg = D3::Select("#emp_base").Append("svg");

  // D3::Selection test_select = D3::Select("#test_test").Append("svg").SetAttr("width", 400).SetAttr("height", 400);

  D3::LinearScale test;
  test.SetDomain(0, 1000).SetRange(0, 500);
  int result = test.ApplyScale<int>(500);

  std::cout << result << std::endl;
}

void CreateTestDiv() {
  EM_ASM({
    $("body").append('<div id="body_emp_test_container"></div>');
  });

  // int margin = 30;
  int width = 450;
  int height = 400;

  D3::Selection svg = D3::Select("#body_emp_test_container")
                          .Append("svg")
                          .SetAttr("width", width)
                          .SetAttr("height", height);
  
  emp::vector<int> data = {10, 100, 300, 234, 53};
  std::function<int(int, int, int)> return_d =
      [](int d, int i, int j) { return d; };

  svg.SelectAll("circle")
      .Data(data)
      .EnterAppend("circle")
      .SetAttr("fill", "blue")
      .SetAttr("cx", 100)
      .SetAttr("cy", 200)
      .SetAttr("r", 1)
      .MakeTransition()
      .SetDuration(2000)
      .SetAttr("cx", return_d)
      .SetAttr("cy", return_d)
      .SetAttr("r", 7)
      .SetAttr("fill", "red");
}

int main() {
  D3::internal::get_emp_d3();

  CreateD3Viz();
  CreateD3Viz2();
  CreateTestDiv();

  std::cout << "Hello, console!" << std::endl;
}
