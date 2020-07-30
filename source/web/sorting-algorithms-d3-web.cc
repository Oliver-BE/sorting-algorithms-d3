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
#include "web/Input.h"
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

// levenshtein distance (swaps to finish graphic)
// some distribution graphic
// add set delay in transition.h

// add bar plot that takes the num of swaps for each sorting alg and generates avg num swaps taken

// TODO: get rid of slider initially showing up
// TODO: fix bubble sort being initially clickable 
// TOOD: fix stats initially showing up
// TODO: fix colors
// ideas: maybe just do a dummy bubble sort that doesn't actually do anything (but ensures no crashing) and then click bubble sort button normally

// set up divs
emp::web::Document stats_div("emp_stats");
emp::web::Document controls_div("emp_controls");

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
        // show slider
        controls_div.SetAttr("style", "display: block;");
        // sort data / update stats
        bubbleSort(data, data.size());
        bubbleSortUpdateStats();
        bubbleSortUpdateControls();
        // disable the bubble sort button
        EM_ASM({
          $("#bubble-sort-button").attr("disabled", true);
        });
      };
  size_t bubble_sort_id;

  // shuffle button function
  std::function<void()> ShuffleArrayButton =
      [this]() {
        // hide slider
        controls_div.SetAttr("style", "display: none;");
        ShuffleArray(data);
        // barColor = "#fc9723";
        UpdateViz(data);

        // enable the bubble sort button
        EM_ASM({
          $("#bubble-sort-button").attr("disabled", false);
        });
      };
  size_t shuffle_id;

  // bubble sort slider / input buttons
  emp::web::Input bs_slider;
  emp::web::Button bs_inc_down;
  emp::web::Button bs_inc_up;
  emp::web::Button bs_play;
  bool playing = false;
  size_t playing_id;

  // bubble sort meta data
  int bs_num_swaps = 0;
  int total_bs_num_swaps = 0;
  int bs_num_times_sorted = 0;
  int avg_bs_num_swaps = 0;
  emp::vector<emp::array<int, 25>> bs_swaps_vec{};

  ///////////////////////////////
  //        CONSTRUCTORS       //
  ///////////////////////////////
  BarPlot() {
    // add live counters to page
    stats_div << "Bubble Sort last took: " << emp::web::Live(bs_num_swaps) << " swaps." << "<hr>";
    stats_div << "Avg num Bubble Sort swaps: " << emp::web::Live(avg_bs_num_swaps) << "<hr>";

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

    playing_id = emp::JSWrap([this]() { playing = false; }, "SetPlayFalse");
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
    // we want to sort the data and initially draw the 0th step 
    bubbleSort(data, data.size());

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
    xScale.SetDomain(bs_swaps_vec[0])
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
        .Data(bs_swaps_vec[0], return_d)
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

    // create sliders
    bs_slider = emp::web::Input([this](std::string curr) { 
      int val = emp::from_string<int>(curr);
      int max_val = emp::from_string<int>(bs_slider.GetMax());

      if(val >= max_val) {
        EM_ASM({
          $("#play-button").attr("disabled", true);
        });
      }
      else if(val < max_val && !playing) { 
        EM_ASM({
          $("#play-button").attr("disabled", false);
        });
      }

      // update viz on change
      UpdateViz(bs_swaps_vec[val]);
      printArray(bs_swaps_vec[val], 25);

      std::cout << val << std::endl;
    }, "range", "", "bs_slider");
    bs_slider.Min(0);
    bs_slider.Max(bs_num_swaps);
    bs_slider.Value(0);
    bs_slider.SetAttr("style", "width: 85%; cursor: pointer;"); 

    // create increment buttons for slider
    // TODO: fix bs_inc button styles
    // TODO: add current slider value to screen 
    // TODO: make sure bubble sort isn't hooked up with play button
    bs_inc_down = emp::web::Button([this]() {
      int current_val = emp::from_string<int>(bs_slider.GetCurrValue());
      if(current_val != 0) {
        bs_slider.Value(current_val - 1);
      }
    }, "<i class='fa fa-minus'></i>", "bs-inc-down-button");
    bs_inc_down.SetAttr("class", "btn btn-outline-primary btn-sm");
    bs_inc_down.SetAttr("style", "width: 7.5%; min-width: 31px, cursor: pointer;");

    bs_inc_up = emp::web::Button([this]() {
      int current_val = emp::from_string<int>(bs_slider.GetCurrValue());
      int max_val = emp::from_string<int>(bs_slider.GetMax());
      if(current_val != max_val) {
        bs_slider.Value(current_val + 1);
      } 
      
    }, "<i class='fa fa-plus'></i>", "bs-inc-up-button");
    bs_inc_up.SetAttr("class", "btn btn-outline-primary btn-sm");
    bs_inc_up.SetAttr("style", "width: 7.5%; min-width: 31px; cursor: pointer;");



    // custom play event
    EM_ASM({
      $(document).on("bs_play_button_press", function(event) {

        var curr_val = parseInt($("#bs_slider").attr("value"));
        var max_val = parseInt($("#bs_slider").attr("max"));
        // if not at max slider value
        if(curr_val < max_val) {
          $("#bs_slider").attr("value", curr_val + 1);
          $("#bs_slider").trigger("change");

          setTimeout(function() {
            // trigger play button press again
            $(document).trigger("bs_play_button_press");
          }, 50); 
        }

        else {
          emp.SetPlayFalse();
        }
      });
    });

    bs_play = emp::web::Button([this]() {
      playing = true;
      EM_ASM({
        $("#play-button").attr("disabled", true);
        $(document).trigger("bs_play_button_press");
      }); 
    }, "<i class='fa fa-play'></i>", "play-button");

    bs_play.SetAttr("class", "btn btn-success btn-sm");
    bs_play.SetAttr("style", "width: 25%; margin-left: 37.5%; margin-right: 37.5%; cursor: pointer;");

    
    // add buttons to controls div
    controls_div << bs_inc_down;
    controls_div << bs_slider;
    controls_div << bs_inc_up;
    controls_div << "<br>" << bs_play;
    // controls_div << "<br>" << emp::web::Live(emp::from_string<int>(bs_slider.GetCurrValue()));

    // initially set slider display as none 
    controls_div.SetAttr("style", "display: block; text-align: center; vertical-align: middle;");

    bubbleSortUpdateStats();

    // TODO: disable slider on play
    // TODO: remove pointers on disable
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
        .Data(data, return_d)
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); }) 
        .SetAttr("width", xScale.GetBandwidth())
        .SetAttr("height", [this](int d, int i, int j) { return height - yScale.ApplyScale<int, int>(d); });
  }

  /// An update function that should be called when the data is changed (sorted or shuffled)
  void UpdateViz(emp::array<int, 25> newData) {
    // if data is fully sorted, change barColor
    if(bs_slider.GetCurrValue() == bs_slider.GetMax()) { barColor = "#69b3a2"; }
    else { barColor = "#fc9723"; }

    // update the scales and axes
    xScale.SetDomain(newData); 
    xAxis.Rescale(newData, xAxisSel);

    // update the bars 
    bars.SelectAll("rect")
        .Data(newData, return_d)
        .MakeTransition().SetDuration(50) 
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
    // reset bs_swaps_vec
    bs_swaps_vec.clear();
    // add initial, unsorted data
    bs_swaps_vec.emplace_back(empArr);

    // do actual sorting
    int i, j; 
    for (i = 0; i < size-1; i++) { 
      // Last i elements are already in place  
      for (j = 0; j < size-i-1; j++) {
        if (empArr[j] > empArr[j+1]) {  
          swap(&empArr[j], &empArr[j+1]);

          // add new step to vector
          bs_swaps_vec.emplace_back(empArr);
        }
      }
    }
    // keep track of num swaps for the emp::Live() input
    // remove 1 since we track initial state as a "step"
    bs_num_swaps = bs_swaps_vec.size() - 1;
  }  

  /// Updates values in the stats div
  void bubbleSortUpdateStats() {
    // if the array wasn't already sorted, update our stats
    if(bs_num_swaps != 0) {
      // increment how many times it's been sorted
      bs_num_times_sorted++;
      // update total number of swaps taken
      total_bs_num_swaps = total_bs_num_swaps + bs_num_swaps;
      // update avg num of swaps taken (total / number of times sorted)
      avg_bs_num_swaps = total_bs_num_swaps / bs_num_times_sorted; 
      // redraw stats div
      stats_div.Redraw();
    }
  }

  /// Updates values in the controls div
  void bubbleSortUpdateControls() {
    // if array wasn't already sorted, update our slider to reflect the new sorting
    if(bs_num_swaps != 0) {
      // update the bubble sort slider
      bs_slider.Max(bs_num_swaps);
      bs_slider.Value(0);
    }
    // if the array was already sorted
    else {
      // then leave our slider at the end
      // bs_slider.Value(bs_slider.GetMax());
    }

    controls_div.Redraw();
  }

  // Other sorting alg
  /// Implements other sorting alg
};

BarPlot barChart{};

int main() {
  barChart.Init();   
}
