
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

- paths, when inside a character string. Ex: `list.files("|")` then TAB suggests the files in the current working directory.

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

The history is project specific. To find the location of the history, type `%path_history` (this is a special command, see the dedicated section). Just press up/down to navigate the history.

Start a command with `@` to navigate the history with the help of the autocomplete. Pressing `@` then TAB gives you the list of all previous entries. Simply start typing to refine the search.

## Options

There are multiple options. Any option can be set either: i) globally (`set_global`), ii) locally  (`set_local`), or iii) for the current session only (`set`). Global values modify entries in the config file located at `USERPROFILE/.sircon`, while local values modify entries in `working_dir/.sircon`. You can also `get` the current option value or `reset` it to factory default.

To set the options, from within the console, you need to use the special command `%options.` then autocomplete your way to the desired option.

When options are set using the console, their integrity is *always strongly checked*, ensuring bug free configurations.

The available options are:

- `R_path`: path to the R executable. Ex: `%options.R_path.set_local path/to/R.exe` sets the R executable to a specific version for the current project (which may be different from the global value).

- `color.`: this is a family of options, it contains more than 15 subvalues to customize syntax highlighting at will. Ex: `%options.color.fun.set light_coral` sets the color of the functions to the HTML color `light_coral`. To have a diplay of all the available colors, type `%list_colors`. You can also provide colors in the `#rrggbb` format.

- `ignore_comment`: whether comments should be automatically discared. By default this is true.

- `ignore_empty_lines`: whether, when copy pasting code, empty lines be automatically discared. By default this is true.

- `pretty_int`: if true (default), then the output of short integer vectors is automatically formatted to add commas to separate the thousands. Ex: `123456` ENTER will display `[1] 123,456`

- `prompt.`: family of three subvalues: `color`, `continue` and `main`. It controls how the display of the prompt. Ex: `%options.prompt.main.set "R> "` displays `"R> "` instead of `"> "` as main prompt.

- `reprex.`: family of two subvalues: `output_color` and `prompt`. When using the `%reprex_last` special command (to create reproducible examples), controls the color of the output and the prompt before each output. Ex: `%options.reprex.prompt.get` returns `"#> "`.

- `shortcut.`: a family of numerous subvalues: `alt+enter`, `enter`, `ctrl+a` to `ctrl+z`. Controls the behavior of shortcuts. These are highly customizable thanks to a macro language. See the dedicated shortcut section for more information. Ex: `%options.shortcut.ctrl+a.get` displays `"<select: context>"`.

- `tab_size`: for multiline commands, controls the tab size before each command after the first. Ex: `%options.tab_size.get` is 2.

## Special functions

Commands starting with `%` are special functions. They are not related to R but instead is a language specific to the console.

Just above we have seen the special command `%options`. When these function need arguments, you can pass them as for regular command line arguments. For example, `file_list` requires a path, then you can just type `file_list ./.git/` to list all files contained in the `.git` folder. Note that the autocomplete also works for the special commands. Optional arguments are followed by a question mark.

The special commands are:

- `clear_history`: clears the history cache for the current project.

- `copy_last_output`: sends the last output to the clipboard.

- `file_copy path_orig path_dest?`: Copies the file/folder at the origin path to the destination path (default is the working directory).

- `file_list path?`: lists all files from in a folder (default is the working directory).

- `file_peek path`: looks into the first lines of a text file and display them dynamically on the console.

- `list_colors text?`: list all available colors which can be used to customize the syntax highlighting. Add a text 

- `open_folder path?`: opens the folder at the current path (default is the working directory).

- `options`: to set the options, see the dedicated section.

- `path_executable`: path to sircon's executable.

- `path_history`: path to the history of the current project.

- `path_options`: path to the global option file.

- `pretty_int logical?`: whether to automatically add commas to long integers. By default, it switched the option `%options.pretty_int` between true and false.

- `reprex_last int?`: prints out the last `n` (default is 1) commands along with their outputs. The autocomplete also displays the values of the commands to make it easy to pick.

- `reprex_mode logical?`: if `true`, then the options `ignore_comment` and `ignore_emty_lines` are turned to false, so that code that is run from a script can be represented verbatim. By default it swithes between `true` and `false`.

- `step_into_last_output`: displays the last output in a stepwise fashion (useful for very long outputs). The first 6 lines of the output are shown then you enter a special mode where: ENTER shows the next line, `digit` displays the next `digit` lines, `q` quits.

- `time_all`: turns on time reporting. All commands report their execution time. Note that it includes the overheads of the console.

- `time_none`: turns off time reporting (see above).

- `width int`: sets the window width.


## Shortcuts



By default, sircon offers the following shortcuts:
- ctrl+a: selects the current 


## Limitations 

The two major limitation are:
- Windows only (it is built using the Windows C API)
- does not support wide characters (i.e. CJK)

## Acknowledgements

I want to thank the main source of inspiration for this project, namely [R-Studio](https://github.com/rstudio/rstudio) and [radian](https://github.com/randy3k/radian).
