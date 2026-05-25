# Notes on files
- Alpha files must have some type corresponding to a "string -> integer" and declare the `entry` function with that type

# Notes on generating a running executable
- For codegen to work, both '-ir' and '-cg' flags need to be there when the compiler is invoked
- To get a runnable executable from the asm file, use the following command
  `gcc -no-pie [outfile].s lib/alpha_driver.s lib/alpha_lib_reg.s`
  (without no-pie it complains about "relocating against .rodata")

# Examples
- We've provided some sample alpha files under the folder "submission_tests"
## simpleFun.alpha
- Tests basic functionality including assignment, int expressions, function calling and returning, and calling standard library functions
## recTest.alpha
- Tests record assignment/reading
## arrTest.alpha
- Tests array assignment/reading, and while loops
## stringLiteralTest.alpha
- Test case involving string literals
## mergesort.alpha
- Nontrivial test case that involves recursive calls and stresses our get_reg more than the previous tests here
## chainFun.alpha
- Test case involving functions returning functions. To verify that it works, manually change 't.switcher' on line 46 to be either true or false, and observe how the output changes

# Things we unfortunately didn't get around to
- Arrays of records act weird due to missing extra reserve semantics
- Multidimensional arrays
