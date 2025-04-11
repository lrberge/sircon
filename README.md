
# sircon: Simple R Console

This is a dependency-free, Windows-only, R console. It possesses most of the features we expect from a console, in particular: history navigation, autocomplete, syntax highlighting, etc. It can be very easily customized, directly from within the console.

## Getting started

This program is a simple executable: `sircon.exe`. Either you can download it directly from this project, either you can compile it on Windows, provided you have the appropriate toolchain installed ([mysys2](https://www.msys2.org/)). 

### Compiling on Windows

Clone the repo, go to `./src/`, then run `make`.

### Launching sircon for the first time

sircon must be attached to an R executable. By default, the first time it is run it tries to find the executable using the variables in your PATH. Hence, if R is in your PATH, it should work directly.

If the path to R is not found, you need to provide it manually in the config file. The config file is located at `USERPROFILE/.sircon`. If there's no bug, the program tells you how to do and where this file is exactly located.

Then you're ready to code in R!

## Autocomplete

### General informations

- To run the autocomplete, press TAB. 

- The autocomplete is multi word. This means that you if you type `req nam` then TAB, the function `requireNamespace` will be suggested. 

- The autocomplete allows for spelling mistakes but only sparingly to avoid false positives.

- The autocomplete provides suggestions even without text input, depending on the context.

- Once in the autocomplete, and when it makes sense, you can constrain the scope of the suggestions using dedicated shortcuts. Press ctrl-K to display only pac**k**ages, ctrl-N for fu**n**ctions, ctrl-V for **v**ariables, ctrl-A for **a**rguments, ctrl-D for the **d**efault suggestions (I was constrained in the choice of letters due to existing VScode shortcuts that I didn't want to change -- later these will be configurable).

- The autocomplete is never silent: when there is no match, it tells you why.

### Available autocompletions

(The pipe, `|`, represents the cursor.)

The autocmplete provides suggestions for the following contexts:

- variables in the current envionment (i.e. when in `browser`, only the variables in the current environment are suggested). Ex: `f = function(x = 32){z = 21 ; browser()} ; f()` ENTER then TAB suggests `x` and `z`.

- variables from a `data.table`, when inside a `data.table`. `Ex: library(data.table)` ENTER ` dt = as.data.table(airquality)` ENTER ` dt[, .(O|)]` then TAB will suggest `"Ozone"`.

- names from lists/data.frames. Ex: `airquality$M` then TAB suggests `Month`. It supports chaining: `l = list(base = list(airquality))` ENTER `l$base[[1]]$M` then TAB will also suggest `Month`.

- argument names, with S3 method dispatch. Ex: `x = lm(Ozone ~ Solar.R, airquality) ; summary(x, |)` then TAB will suggest the arguments `object`, `correlation` and `symbolic.cor`.

- function names from loaded packages. Ex: `req na` then TAB will suggest the function `requireNamespace()`.

- functions in a given namespace. Ex: `fixest::` then TAB will suggest the functions exported from the package `fixest`. `fixest:::` then TAB will also include the non exported functions.

- data sets from the package `datasets`. Ex: `airq` then TAB will suggest the variable `airquality` (which is in fact a data set).

- `data(|)` then TAB: suggest the available data sets from all installed packages. Ex: `data(did|)` then TAB suggests the following completion `base_did, package = "fixest"`

- `library(|)` then TAB: suggests all the installed packages

- `install.packages(|)` then TAB: suggests the packages currently available on CRAN

- in a formula within a function, and if a data.frame is present as another argument, it suggests the variables from this data.frame. Ex: `lm(|~, airquality)` then TAB (note that the cursor is in position 3) suggests the variables `Ozone`, `Solar.R`, `Wind`, `Temp`, `Month` and `Day`.

- use `>>` to introspect a variable and replace that variable with one of its value, as provided by the suggestion. Ex: `iris$Species>>` then TAB suggests `setosa <50>`, `versicolor <50>` and `virginica <50>`, the value in `<>` is the count and will not be inserted. Assume we accept the first suggestion, then the full string `iris$Species>>` is replaced with `"setosa"`.

## History navigation

## Options

## Special functions

## Syntax highlighting



## Limitations 

The two major limitation are:
- Windows only (it is built using the Windows C API)
- does not support wide characters (i.e. CJK)

## Acknowledgements

I want to thank the main source of inspiration for this project, namely [R-Studio](https://github.com/rstudio/rstudio) and [radian](https://github.com/randy3k/radian).
