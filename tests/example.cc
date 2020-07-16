#define CATCH_CONFIG_MAIN

#include "Catch/single_include/catch2/catch.hpp"

#include "example.h"

TEST_CASE("Test example")
{
  REQUIRE( example() );
}

// #include "../third-party/Empirical/source/web/Document.h"
// #include "../third-party/Empirical/source/web/_MochaTestRunner.h"

// struct ExampleTest : emp::web::BaseTest {
//   int testValue;

//   ExampleTest() { Setup(); }

//   void Setup() {
//     testValue = 2;
//   }

//   void Describe() override {

//     EM_ASM({  
//       const testValue = $0;

//       describe("an example test", function() {
//         it("should pass in a test value correctly", function() {
//           chai.assert.equal(testValue, 2); 
//         });
//       });
//     }, testValue);
//   }
// };

// emp::web::MochaTestRunner test_runner;

// int main() {
//   emp::Initialize();

//   test_runner.AddTest<ExampleTest>("ExampleTest");
  
//   test_runner.Run();
// }
