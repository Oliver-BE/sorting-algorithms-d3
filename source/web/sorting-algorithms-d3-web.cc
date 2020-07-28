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
#include "web/Animate.h"

#include "web/js_utils.h"
#include "base/array.h"
#include "base/map.h"

#include "web/d3/d3_init.h"
#include "web/d3/selection.h"
#include "web/d3/transition.h"
#include "web/d3/scales.h"
#include "web/d3/axis.h"
#include "web/d3/utils.h" 

// levenshtein distance (steps to finish graphic)
// some distribution graphic
// add info button / link to blogpost
// add set delay in transition.h

// slidey bar to go through steps
// keep track of each step in an array of arrays (each index holds one iteration of sorting alg)
// then slide through the iterations
// display num steps taken

// add bar plot that takes the num of steps for each sorting alg and generates avg num steps taken

emp::web::Document emp_stats("emp_stats");

struct BarPlot {
  ///////////////////////////////
  //     MEMBER VARIABLES      //
  ///////////////////////////////
  // basic barplot layout vars
  emp::map<std::string, int> margin = {{"top", 25}, {"right", 25}, {"bottom", 25}, {"left", 25}};
  int width = 0;
  int height = 0;
  std::string barColor = "#fc9723";

  // D3 selections / scales / axis
  D3::Selection svg;
  D3::Selection barplot;
  D3::Selection bars;
  D3::Selection xAxisSel;
  D3::BandScale xScale;
  D3::LinearScale yScale;
  D3::Axis<D3::BandScale> xAxis;

  // data and functions for data values
  emp::array<int, 25> data;
  std::function<int(int, int, int)> return_d =
      [](int d, int i, int j) { return d; };
  
  // bubble sort button function
  /// Runs bubble sort on the data and updates the visualization appropriately
  std::function<void()> BubbleSortButton =
      [this]() { 
        bubbleSort(data, data.size());
        barColor = "#69b3a2";
        UpdateViz();   
      };
  size_t bubble_sort_id;

  // shuffle button function
  std::function<void()> ShuffleArrayButton =
      [this]() {
        ShuffleArray(data);
        barColor = "#fc9723";
        UpdateViz();
      };
  size_t shuffle_id;

  // bubble sort meta data
  int bs_num_steps = 0;
  int total_bs_num_steps = 0;
  int bs_num_times_sorted = 0;
  int avg_bs_num_steps = 0;
  emp::vector<emp::array<int, 25>> bs_steps_vec;

  ///////////////////////////////
  //        CONSTRUCTORS       //
  ///////////////////////////////
  BarPlot() {
    // add live counters to page
    emp_stats << "<hr>" << "Bubble Sort last took: " << emp::web::Live(bs_num_steps) << " steps." << "<hr>";
    emp_stats << "Avg num Bubble Sort steps: " << emp::web::Live(avg_bs_num_steps);

    // init data and shuffle it
    for (int i = 0; i < data.size(); i++) { 
      data[i] = i + 1;
    }
    ShuffleArray(data);

    // on window resize, redraw everything
    emp::web::OnDocumentReady([this]() {
      emp::OnResize([this]() {
        ResizeUpdate();
      });
    });

    // create button functions
    bubble_sort_id = emp::JSWrap(BubbleSortButton, "BubbleSortButton"); 
    shuffle_id = emp::JSWrap(ShuffleArrayButton, "ShuffleArrayButton");
  }

  ~BarPlot() {
    // cleanup button functions  
    emp::JSDelete(bubble_sort_id);  
    emp::JSDelete(shuffle_id);  
  }


  ///////////////////////////////
  //       MAIN FUNCTIONS      //
  ///////////////////////////////
  /// Initializes the general HTML layout, scales, and axes
  void Init() {
    // init height and width (based on parent div width)
    width = GetParentWidth() - margin["right"] - margin["left"];
    height = 450 - margin["top"] - margin["bottom"];

    // initialize svg object with proper dimensions
    svg = D3::Select("#emp_d3_wrapper")
              .Append("svg")
              .SetAttr("width", width + margin["right"] + margin["left"])
              .SetAttr("height", height + margin["top"] + margin["bottom"]);
    
    // create group to hold all barplot elements
    barplot = svg.Append("g")                 
                  .SetAttr("id", "barplot") 
                  .Move(margin["left"], margin["top"]);

    // initialize scales
    xScale.SetDomain(data)
          .SetRange(0, width);
    // note that this can't be chained above (need to use a curiously recursive template pattern)
    xScale.SetPadding(0.15);

    yScale.SetDomain(0, data.size())
          .SetRange(height, 0);

    // initialize and draw x axis
    xAxisSel = barplot.Append("g")
                      .SetAttr("id", "x-axis")
                      .Move(0, height);

    xAxis = D3::Axis<D3::BandScale>("bottom", "", 0)
              .SetScale(xScale)
              .Draw(xAxisSel); 

    // initialize and draw bars
    bars = barplot.Append("g")
                  .SetAttr("id", "bars"); 

    bars.SelectAll("rect")
        .Data(data)
        .EnterAppend("rect")
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("width", xScale.GetBandwidth())
        // init y value and height to zero to create animation on page load
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(0); })
        .SetAttr("height", 0)
        .MakeTransition().SetDuration(2000)
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); })
        .SetAttr("height", [this](int d, int i, int j) { return height - yScale.ApplyScale<int, int>(d); })
        .SetAttr("fill", barColor);
  }

  /// A special update that should only be called when the window is getting resized
  /// Keeps everything intact while accounting for change in window size
  void ResizeUpdate() {
    // update the SVG if the window has changed size 
    width = GetParentWidth() - margin["right"] - margin["left"];
    height = 450 - margin["top"] - margin["bottom"];

    svg.SetAttr("width", width + margin["right"] + margin["left"])
       .SetAttr("height", height + margin["top"] + margin["bottom"]);

    // update the scales and axes based on new window size
    xScale.SetDomain(data)
          .SetRange(0, width);

    xAxis.Rescale(data, xAxisSel);

    yScale.SetDomain(0, data.size())
          .SetRange(height, 0);

    // update the bars based on new window size
    bars.SelectAll("rect")
        .Data(data)
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); }) 
        .SetAttr("width", xScale.GetBandwidth())
        .SetAttr("height", [this](int d, int i, int j) { return height - yScale.ApplyScale<int, int>(d); });
  }

  /// An update function that should be called when the data is changed (sorted or shuffled)
  void UpdateViz() {
    // update the scales and axes
    xScale.SetDomain(data); 
    xAxis.Rescale(data, xAxisSel);

    // update the bars 
    bars.SelectAll("rect")
        .Data(data)
        .MakeTransition().SetDuration(1000) 
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); }) 
        .SetAttr("height", [this](int d, int i, int j) { return height - yScale.ApplyScale<int, int>(d); })
        .SetAttr("fill", barColor);
  }


  ///////////////////////////////
  //     HELPER FUNCTIONS      //
  ///////////////////////////////
  /// Gets the width of the emp_d3_wrapper div
  double GetParentWidth() { 
    return EM_ASM_DOUBLE({ return $("#emp_d3_wrapper").width(); }); 
  }

  /// Randomly shuffles an array
  void ShuffleArray(emp::array<int, 25> & arr) {
    // obtain a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(arr.begin(), arr.end(), std::default_random_engine(seed)); 
  }

  /// Swaps two elements
  void swap(int *xp, int *yp)  {  
    int temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
  }  

  /// Prints an array
  template <size_t SIZE> 
  void printArray(emp::array<int, SIZE> empArr, int size) {  
    int i;  
    for (i = 0; i < size; i++)  
      std::cout << empArr[i] << " ";  
    std::cout << std::endl;  
  }  

  // BUBBLE SORT 
  /// Implements bubble sort (source: https://www.geeksforgeeks.org/bubble-sort/)
  template <size_t SIZE> 
  void bubbleSort(emp::array<int, SIZE> & empArr, int size)  {
    // reset num of steps taken
    bs_num_steps = 0;
    emp_stats.Redraw();
    int i, j; 

    for (i = 0; i < size-1; i++) { 
      // Last i elements are already in place  
      for (j = 0; j < size-i-1; j++) {
        if (empArr[j] > empArr[j+1]) {  
          swap(&empArr[j], &empArr[j+1]);

          // add new step to vector
          bs_steps_vec.emplace_back(empArr);
          // increment total num steps and redraw the number on webpage
          bs_num_steps++;
          emp_stats.Redraw();
        }
      }
    }

    // if the array wasn't already sorted, update our metadata
    if(bs_num_steps != 0) {
      // increment how many times it's been sorted
      bs_num_times_sorted++;
      // update total number of steps taken
      total_bs_num_steps = total_bs_num_steps + bs_num_steps;
      // update avg num of steps taken (total / number of times sorted)
      avg_bs_num_steps = total_bs_num_steps / bs_num_times_sorted;
      emp_stats.Redraw();
    }
  }  

  // Other sorting alg
  /// Implements other sorting alg
};

BarPlot barChart{};

int main() {
  barChart.Init();   
}
