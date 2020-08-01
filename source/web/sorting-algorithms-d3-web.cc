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
#include "web/Element.h"
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

// TODO: clean up overall code structure
// TODO: add more sorting algorithms (if code structure becomes better, this should become easier)
// TODO: add bar plot that takes the num of swaps for each sorting alg and generates avg num swaps taken 

// set up divs
emp::web::Document stats_div("emp_stats");
emp::web::Document controls_div("emp_controls");
emp::web::Document slider_div("emp_slider");
emp::web::Document slider_value_div("emp_slider_val");
emp::web::Document slider_controls_div("emp_slider_controls");

struct BarPlot {
  ///////////////////////////////
  //     MEMBER VARIABLES      //
  ///////////////////////////////
  // barplot layout vars
  emp::map<std::string, int> margin = {{"top", 25}, {"right", 25}, {"bottom", 25}, {"left", 25}};
  int width = 0;
  int height = 0;
  std::string barColor = "#fc9723";
  int transitionDuration = 750; 

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
        // show slider and stats
        controls_div.SetAttr("style", "display: block;");
        stats_div.SetAttr("style", "display: block;");
        
        // TODO: fix the tool tip (it sticks on slider click/change)
        // AddSliderToolTip();

        // sort data / update stats and controls
        bubbleSort(data, data.size());
        bubbleSortUpdateStats();
        bubbleSortUpdateSlider();
        // disable the bubble sort button
        EM_ASM({
          $("#bubble-sort-button").attr("disabled", true);
        });
      };
  size_t bubble_sort_id;

  // shuffle button function
  std::function<void()> ShuffleArrayButton =
      [this]() {
        // hide bubble sort slider
        controls_div.SetAttr("style", "display: none;");
        // shuffle the data
        ShuffleArray(data);
        // reset slider to 0
        bs_slider.Value(0);
        // update the barchart based on the new data
        UpdateViz(data);

        // after shuffle, re-enable the bubble sort button
        EM_ASM({
          $("#bubble-sort-button").attr("disabled", false);
        });
      };
  size_t shuffle_id;

  // bubble sort slider / input buttons
  emp::web::Input bs_slider;
  int current_slider_value = 0;
  emp::web::Button bs_inc_down;
  emp::web::Button bs_inc_up;
  emp::web::Button bs_play;
  // keep track of whether or not the play button is active
  bool isPlaying = false;
  size_t set_isPlaying_false_id;

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
    // add title to stats card
    stats_div <<  emp::web::Div("stats-title").SetAttr("style", "display: flex; justify-content: space-around;")
              << "<h5 class='card-title mt-2 mb-0'> Bubble Sort Statistics </h5>";
    // add last number of swaps to card
    stats_div << "<hr>"
              << emp::web::Div("num-swaps-div").SetAttr("style", "display: flex; justify-content: space-between;")
              << "Bubble Sort last used:"
              << emp::web::Element("span", "num-swaps-badge").SetAttr("class", "badge badge-info ml-1 d-inline-flex align-items-center");
    stats_div.Div("num-swaps-badge") 
        << emp::web::Live(bs_num_swaps) << " swaps";
    // add avg number of swaps to card
    stats_div << "<hr>" 
              << emp::web::Div("avg-num-swaps-div").SetAttr("style", "display: flex; justify-content: space-between;")
              << "Bubble Sort on average has used:"
              << emp::web::Element("span", "avg-num-swaps-badge").SetAttr("class", "badge badge-info ml-1 d-inline-flex align-items-center");
    stats_div.Div("avg-num-swaps-badge") 
        << emp::web::Live(avg_bs_num_swaps) << " swaps";

    // initially hide the stats
    stats_div.SetAttr("style", "display: none;");

    // init data and shuffle it
    for (int i = 0; i < data.size(); i++) { 
      data[i] = i + 1;
    }
    ShuffleArray(data); 

    
  }

  ~BarPlot() {
    // cleanup JSWrap functions  
    emp::JSDelete(bubble_sort_id);  
    emp::JSDelete(shuffle_id); 
    emp::JSDelete(set_isPlaying_false_id); 
  }


  ///////////////////////////////
  //       MAIN FUNCTIONS      //
  ///////////////////////////////
  /// Initializes the general HTML layout, scales, and axes
  void Init() {
    // draw initial barplot
    DrawInitialViz();
    // create sliders and buttons
    CreateSlider();
    CreateButtons();
     
    // set div display properties
    controls_div.SetAttr("style", "display: none;");
    slider_div.SetAttr("style", "display: flex; justify-content: space-around").SetAttr("class", "mb-1");
    slider_controls_div.SetAttr("style", "display: flex; justify-content: space-between");

    // add buttons to controls div
    controls_div << slider_div; 
    slider_div << bs_slider;
    slider_div << slider_value_div
                << emp::web::Element("span", "slider-value-badge").SetAttr("class", "badge badge-primary d-inline-flex align-items-center");   
    slider_div.Div("slider-value-badge") 
        << emp::web::Live(current_slider_value);

    controls_div << slider_controls_div;
    slider_controls_div << bs_inc_down;
    slider_controls_div << bs_play;
    slider_controls_div << bs_inc_up;

    // set data to be equal to first index of bs_swaps_vec so it's not sorted 
    // when the user goes to click bubble sort for the first time (this will mess up the slider)
    data = bs_swaps_vec[0];
  }

  void DrawInitialViz() {
    // we want to sort the data and initially draw the 0th step 
    bubbleSort(data, data.size());
    emp::array<int, 25> newData = bs_swaps_vec[0];

    // init height and width (based on parent div width) 
    width = 725;
    height = 450;

    // initialize svg object with proper dimensions
    svg = D3::Select("#emp_d3_wrapper")
              .Append("svg")
              .SetAttr("viewBox", "0 0 " + std::to_string(width)+ " " + std::to_string(height)); 
    
    // create group to hold all barplot elements
    barplot = svg.Append("g")                 
                  .SetAttr("id", "barplot");  

    // initialize scales
    xScale.SetDomain(newData) 
          .SetRange(margin["left"], width - margin["right"]); 
    // note that this can't be chained above (need to use a curiously recursive template pattern)
    xScale.SetPadding(0.15);

    yScale.SetDomain(0, newData.size())
          .SetRange(height - margin["bottom"], margin["top"]); 

    // initialize and draw x axis
    xAxisSel = barplot.Append("g")
                      .SetAttr("id", "x-axis")
                      .Move(0, height - margin["bottom"]);

    xAxis = D3::Axis<D3::BandScale>("bottom", "", 0)
              .SetScale(xScale)
              .Draw(xAxisSel); 

    // initialize and draw bars
    bars = barplot.Append("g")
                  .SetAttr("id", "bars"); 

    bars.SelectAll("rect")
        .Data(newData, return_d)
        .EnterAppend("rect")
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("width", xScale.GetBandwidth())
        // initialize the y value and height to zero to create a transition on page load
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(0); })
        .SetAttr("height", 0)
        .MakeTransition().SetDuration(1750)
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); })
        .SetAttr("height", [this](int d, int i, int j) { return height - margin["top"] - yScale.ApplyScale<int, int>(d); })
        .SetAttr("fill", barColor);
  }

  void CreateSlider() { 
    // create slider
    bs_slider = emp::web::Input([this](std::string curr) {  
      // read in current slider value and update value 
      int val = emp::from_string<int>(curr);
      current_slider_value = val;
      bs_slider.Value(val);

      // redraw slider value
      slider_value_div.Redraw();

      // update viz on slider change
      UpdateViz(bs_swaps_vec[val]); 
    }, "range", "", "bs_slider");
    bs_slider.Min(0);
    bs_slider.Max(bs_num_swaps);  
    bs_slider.SetAttr("style", "width: 92%;");  
  }

  void CreateButtons() {
    // create JSWrap button functions
    bubble_sort_id = emp::JSWrap(BubbleSortButton, "BubbleSortButton"); 
    shuffle_id = emp::JSWrap(ShuffleArrayButton, "ShuffleArrayButton");
    set_isPlaying_false_id = emp::JSWrap([this]() { isPlaying = false; }, "SetIsPlayingFalse");

    // add increment down button
    bs_inc_down = emp::web::Button([this]() {
      int current_val = emp::from_string<int>(bs_slider.GetCurrValue());
       // if slider isn't at max value, decrement it 
      if(current_val > 0) {
        bs_slider.Value(current_val - 1);
        // redraw slider_value_div to show new value
        slider_value_div.Redraw();
      }
    }, "<i class='fa fa-minus'></i>", "bs-inc-down-button");
    bs_inc_down.SetAttr("class", "btn btn-outline-primary btn-sm");
    bs_inc_down.SetAttr("style", "width: 31px;");

    // add increment up button
    bs_inc_up = emp::web::Button([this]() {
      int current_val = emp::from_string<int>(bs_slider.GetCurrValue());
      int max_val = emp::from_string<int>(bs_slider.GetMax());
      // if slider isn't at max value, increment it 
      if(current_val < max_val) {
        bs_slider.Value(current_val + 1);
        // redraw slider_value_div to show new value
        slider_value_div.Redraw();
      }   
    }, "<i class='fa fa-plus'></i>", "bs-inc-up-button");
    bs_inc_up.SetAttr("class", "btn btn-outline-primary btn-sm");
    bs_inc_up.SetAttr("style", "width: 31px;");

    // Custom event for the play button that gets triggered on press
    // NOTE: this is necessary because of the asychronus nature of JS 
    // and C++ code will run before JS code even when it seems that it shouldn't (Timeouts are hard!!)
    EM_ASM({
      $(document).on("bs_play_button_press", function(event) { 
        // get current slider values
        var curr_val = parseInt($("#bs_slider").attr("value"));
        var max_val = parseInt($("#bs_slider").attr("max"));
        // if not at max slider value
        if(curr_val < max_val) { 
          // increment value by one and manually trigger change
          $("#bs_slider").attr("value", curr_val + 1);
          $("#bs_slider").trigger("change");

          setTimeout(function() {
            // trigger play button press again
            $(document).trigger("bs_play_button_press");
          }, 25); 
        }
        // otherwise the playing is done, so re-enable all buttons (and set isPlaying to false)
        else {
          emp.SetIsPlayingFalse();
          $("#play-button").attr("disabled", false);
          $("#shuffle-button").attr("disabled", false); 
          $("#bs-inc-up-button").attr("disabled", false); 
          $("#bs-inc-down-button").attr("disabled", false); 
        }
      });
    });

    // add play button
    bs_play = emp::web::Button([this]() {
      isPlaying = true;
      EM_ASM({
        // on play, set proper buttons to disabled
        $("#play-button").attr("disabled", true);
        $("#shuffle-button").attr("disabled", true); 
        $("#bs-inc-up-button").attr("disabled", true); 
        $("#bs-inc-down-button").attr("disabled", true); 
        // trigger play button action 
        $(document).trigger("bs_play_button_press");
      }); 
    }, "<i class='fa fa-play'></i>", "play-button");
    bs_play.SetAttr("class", "btn btn-success btn-sm");
    bs_play.SetAttr("style", "width: 35%;");
  }

  /// An update function that should be called when the data is changed (sorted or shuffled)
  void UpdateViz(emp::array<int, 25> newData) {
    // if we're playing, make the transition faster
    if(isPlaying) {
      transitionDuration = 50;
    }
    else {
      transitionDuration = 750;
    } 

    // if data is fully sorted, change barColor
    if(bs_slider.GetCurrValue() == bs_slider.GetMax()) { barColor = "#69b3a2"; }
    else { barColor = "#fc9723"; }

    // update the scales and axes
    xScale.SetDomain(newData); 
    xAxis.Rescale(newData, xAxisSel);

    // update the bars 
    bars.SelectAll("rect")
        .Data(newData, return_d)
        .MakeTransition().SetDuration(transitionDuration) 
        .SetAttr("x", [this](int d, int i, int j) { return xScale.ApplyScale<int, int>(d); })
        .SetAttr("y", [this](int d, int i, int j) { return yScale.ApplyScale<int, int>(d); }) 
        .SetAttr("height", [this](int d, int i, int j) { return height - margin["top"] - yScale.ApplyScale<int, int>(d); })
        .SetAttr("fill", barColor); 
  }


  ///////////////////////////////
  //     HELPER FUNCTIONS      //
  ///////////////////////////////
  /// Adds a tooltip to the slider
  void AddSliderToolTip() {
    EM_ASM({ 
      $(function() {
        $(document).ready(function() {
          var el = document.getElementById("bs_slider");
          el["title"] = "Slide to iterate over the swaps performed from start to finish";
          el.setAttribute("data-toggle", "tooltip"); 
          $('[data-toggle="tooltip"]').tooltip({ 
            trigger : 'hover'
          }); 
        });
      });
    });
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

  /// Updates/redraws values in the controls div
  void bubbleSortUpdateSlider() {
    // update our slider to reflect the new sorting 
    bs_slider.Max(bs_num_swaps);
    bs_slider.Value(0);
    controls_div.Redraw();
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
    // remove 1 since we track initial state as a "swap"
    bs_num_swaps = bs_swaps_vec.size() - 1;
  }  
};

BarPlot barChart{};

int main() {
  barChart.Init();   
}
