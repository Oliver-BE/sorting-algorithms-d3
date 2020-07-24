//  This file is part of sorting-algorithms-d3
//  Copyright (C) Oliver Baldwin Edwards, 2020.
//  Released under MIT license; see LICENSE

#include <iostream>
#include <string>
#include <algorithm>
#include <random>  
#include <chrono> 

#include "web/init.h"
#include "web/web.h"
#include "web/Document.h"

#include "web/js_utils.h"
#include "base/map.h"

#include "web/d3/d3_init.h"
#include "web/d3/selection.h"
#include "web/d3/transition.h"
#include "web/d3/scales.h"
#include "web/d3/axis.h"
#include "web/d3/utils.h" 


struct BarPlot {
  ///////////////////////////////
  //     MEMBER VARIABLES      //
  ///////////////////////////////
  // margin and height
  emp::map<std::string, int> margin = {{"top", 50}, {"right", 25}, {"bottom", 90}, {"left", 100}};
  int width = 0;
  int height = 0;

  // D3 selections / scales / axis
  D3::Selection svg;
  D3::Selection bar;
  D3::BandScale xScale;
  D3::LinearScale yScale;
  D3::Axis<D3::BandScale> xAxis = D3::Axis<D3::BandScale>("bottom", "Bottom Axis", 0);
  D3::Selection xAxisSel;
  D3::Axis<D3::LinearScale> yAxis = D3::Axis<D3::LinearScale>("left", "Left Axis", 0);
  D3::Selection yAxisSel;

  // data and functions for data values
  emp::array<int, 25> data;
  std::function<int(int, int, int)> return_d =
      [](int d, int i, int j) { return d; };
  // a way to cheat and sort the array
  std::function<void()> SortArrayRedraw =
      [this]() { 
        for (int i = 0; i < data.size(); i++) { data[i] = i + 1; } 
        Redraw();    
      };
  size_t sort_arr_id;

  // shuffle the array
  std::function<void()> ShuffleArrayRedraw =
      [this]() {
        ShuffleArray(data);
        Redraw();
      };
  size_t shuffle_arr_id;


  ///////////////////////////////
  //        CONSTRUCTORS       //
  ///////////////////////////////
  BarPlot() {
    // init data and shuffle it
    for (int i = 0; i < data.size(); i++) { 
      data[i] = i + 1;
    }
    ShuffleArray(data);

    // on window resize, redraw everything
    emp::web::OnDocumentReady([this]() {
      emp::OnResize([this]() {
        Redraw();
      });
    });

    // create button functions
    sort_arr_id = emp::JSWrap(SortArrayRedraw, "SortArrayRedraw"); 
    sort_arr_id = emp::JSWrap(ShuffleArrayRedraw, "ShuffleArrayRedraw"); 
  }

  ~BarPlot() {
    // cleanup button functions  
    emp::JSDelete(sort_arr_id);  
    emp::JSDelete(shuffle_arr_id); 
  }


  ///////////////////////////////
  //       MAIN FUNCTIONS      //
  ///////////////////////////////
  /// Initializes the general HTML layout, scales, and axes
  void Init() {
    // init height and width (based on parent div width)
    width = GetParentWidth() - margin["right"] - margin["left"];
    height = 500 - margin["top"] - margin["bottom"];
    
    // initialize svg object with proper dimensions
    svg = D3::Select("#emp_d3_wrapper")
              .Append("svg")
              .SetAttr("id", "barplot")
              .SetAttr("width", width + margin["right"] + margin["left"])
              .SetAttr("height", height + margin["top"] + margin["bottom"])
              .Move(margin["left"], margin["top"]);

    // initialize barplot selection
    bar = svg.Append("g")
             .SetAttr("id", "bars"); 

    // initialize scales
    xScale.SetDomain(data)
          .SetRange(0, width);
    xScale.SetPadding(0.1);

    yScale.SetDomain(0, data.size())
          .SetRange(height, 0);

    // init axis
    xAxisSel = svg.Append("g")
                   .SetAttr("id", "x-axis")
                   .Move(0, height);

    yAxisSel = svg.Append("g")
                  .SetAttr("id", "y-axis");

    xAxis.SetScale(xScale);
    yAxis.SetScale(yScale);
  }

  /// Draws all the necessary components for the viz
  void DrawViz() {
    // draw axes
    xAxis.Draw(xAxisSel);
    yAxis.Draw(yAxisSel);

    // draw bars
    bar.SelectAll("rect")
        .Data(data)
        .EnterAppend("rect")
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(return_d(d, i, j)); })
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(return_d(d, i, j)); })
        .SetAttr("width", xScale.GetBandwidth())
        .SetAttr("height", [this](int d, int i, int j) { return height - yScale.ApplyScale<int, int>(return_d(d, i, j)); })
        .SetAttr("fill", "#69b3a2");
  }

  /// Initializes everything and then draws it
  void RunApp() {
    Init();
    DrawViz();
  }


  ///////////////////////////////
  //     HELPER FUNCTIONS     //
  ///////////////////////////////
  void ClearBarPlot() {
    EM_ASM({
      d3.select("#emp_d3_wrapper")
        .select("#barplot")
        .remove();
    });
  }

  void Redraw() {
    ClearBarPlot();
    RunApp();
  }

  double GetParentWidth() { 
    return EM_ASM_DOUBLE({ return $("#emp_d3_wrapper").width(); }); 
  }

  void ShuffleArray(emp::array<int, 25> & arr) {
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::shuffle(arr.begin(), arr.end(), std::default_random_engine(seed)); 
  }
};

BarPlot test{};

int main() {
  test.RunApp();  
}
