
# sircon: Simple R Console

This is a dependency-free, Windows-only, R console. It possesses most of the features we expect from a console, in particular: context-aware history navigation, (beefy) autocomplete, syntax highlighting, multi-lines editing, etc. It can be very easily customized, directly from within the console. It provides a macro language to empower the user with powerful custom shortcuts.

## Getting started

This program is a simple executable: `bin/sircon.exe`. Either you can download it directly from this project, either you can compile it on Windows, provided you have the appropriate toolchain installed ([mysys2](https://www.msys2.org/)). 

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

Several histories co-exist, but live in different houses. For example, when you run `browser()` from within a function, sircon creates an history specific to this function. Hence the commands from within this function will not mess your main history for the main commands. And when you return to the function that has been `browser()`'ed, you recover its previous history.

Note that commands longer than 10 lines do not appear in the history.

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

sircon's has a built-in macro language to create any kind of shortcut you want. Note that a few important shortcuts cannot be customized, they are detailed last.

### Creating shortcuts: Macro language principle

In its simplest form, a shortcut just inserts a piece of text. Let's create a shortcut that inserts "hello!":

```raw
%options.shortcut.ctrl+d.set hello!
```

Now if you press `ctrl+d`, `hello!` is inserted at the cursor position. To add spaces at the edges of the text, you need to use quotes: `%options.shortcut.ctrl+d.set "hello! "` would insert a space after `hello!`.

Shortcuts can contain commands, which control the console. A shortcut command is of the form `<command_name>` or `<command_name: value>`. For example, change `ctrl+d` with:

```raw
%options.shortcut.ctrl+d.set <move_x: rightmost> hello!
```

Now when we press `ctrl+d`, the cursors goes at the right of the current line, then inserts `hello!`. 

Shortcuts can contain any number of commands: these are applied in turn, left to right.

### Macro language: Commands reference

Here is the list of available commands (note that the autocomplete will show them to you):

- `clear_screen`: clears the screen
- `command: {stash, pop, clear}`: `stash` to stash the curent command, `pop` to pop it, and `clear` to clear it.
- `copy`: copies the current selection.
- `cut`: cuts the current selection.
- `debug`: only used internally -- do not use.
- `delete: {left, right, word_left, word_right, all_left, all_right}`: it deletes: one character `left`, or `right`; the current `line`; a word left (`word_left`) or right (`word_right`); everything left of the cursor (`all_left`) or ight of the cursor (`all_right`).
- `enter`: presses enter.
- `insert: ""`: inserts a verbatim the text in quotes.
- `move_x: {left, right, word_left, word_right, leftmost, rightmost}`: move the cursor, left, right, a word left, a word right, leftmost or rightmost.
- `move_y: {down, up, top, bottom}`: move the cursor up or down. Note that is the cursor is at the top and you move up, this will trigger history navigation. When moving with `top` or `bottom`, there is no history navigation.
- `newline`: inserts a new line at the cursor position. That means that the content right of the cursor will be in the line down.
- `paste`: pastes the content of the clipboard at the cursor's location.
- `redo`: redo the last undone change
- `run: ""`: runs the command in quotes, then gets back to the current command. The command that is run is displayed.
- `run_no_echo: ""`: runs the command in quotes, then gets back to the current command. The command that is run *is* **not** *displayed*.
- `select: {all, context}`: selects all the content of the current line (`all`), or the cursor's context (`context`).
- `selection: {stash, pop}`: deletes and statshes the current selection, or pops the selection that has been stashed.
- `undo`: undo the last change.

### Macro language: Conditions

You can apply shortcuts conditionnally thanks to if/else clauses. 

The syntax is:
```raw
<if: CONDITION> <command_sequence> <else> <command_sequence> <endif>
```

You must end an `<if>` with an `<enfif>`. There are three valid logical operators in conditions: `not`, `and` and `or`. Here is the list of all conditions:

- `any_selection`: true if some text is selected.

- `empty`: true if there is not a single character in the command.

- `is_letter_left`: true if the next character left of the cursor is a letter (spaces are ignored). Note that `'_'` and `'.'` in R are considered as letters.

- `is_letter_right`: true if the next character right of the cursor is a letter (spaces are ignored). Note that `'_'` and `'.'` in R are considered as letters.

- `is_punct_left`: true if the next character left of the cursor is a punctuation (spaces are ignored). Note that `'_'` and `'.'` in R are considered as letters.

- `is_punct_right`: true if the next character left of the cursor is a punctuation (spaces are ignored). Note that `'_'` and `'.'` in R are considered as letters.

- `line_empty`: true if the current line, in a multiline statement, is empty. If in a single line statement, this condition is the same as `empty`.

- `line_matches: ""`: true if the pattern in quotes is found in the current line. Note that this is an exact match and no regular expression can be used (so far). You can use the special value `_cursor_` to represent the cursor. For example, `line_matches: {_cursor_}` is true for the current line `f = function(){|}`, but false for `f| = function(){}` because the cursor is not at the location of the pattern.

- `one_liner`: true if the command is a one liner, i.e. not a multi-lines command.

- `x_leftmost`: true if the cursor is leftmost of the current line.

- `x_rightmost`: true if the cursor is rightmost of the current line.

- `y_bottom`: true if the cursor is at the bottom of the command (for multilines statements).

- `y_top`: true if the cursor is at the top of the command (for multilines statements).


Please see the two last examples of the default shortcuts for complex examples using conditions.

### Default shortcuts

By default, sircon implements the following shortcuts:

- `ctrl+a = <select: context>`: it selects the context around the cursor. Ex: `greeting("john| and mary", "wha'ts up?")` (cursor in on `john`), press `ctrl+a` first selects `john and mary`, pressing again selects `"john| and mary", "wha'ts up?"`, pressing again selects the full line. Further `ctrl+a` presses cycle across those selections. To always select the full line, set `ctrl+a` to `<select: all>` instead.

- `ctrl+c = <copy>`

- `ctrl+d = <delete: line>`: deletes the line and goes up.

- `ctrl+l = <clear_screen>`

- `ctrl+n = <newline>`

- `ctrl+v = <paste>`

- `ctrl+x = <cut>`

- `ctrl+y = <redo>`

- `ctrl+z = <undo>`

- `enter = <if: line_matches: "{_cursor_}"> <newline> <newline> <delete: all_left> <move_y: up> <move_x: rightmost> <endif>`: Ex: `f = function(){|}` then ENTER formats the code as line 1: `f = function(){`, line 2: `  |` (cursor is here), line 3: `}`.

- `alt+enter = <if: empty> <move_y: up> <endif> <move_y: bottom> <move_x: rightmost> <insert: " |>"> <newline>`: if there's something in the line, it inserts ` |>` at the end of the line and inserts a new line. If the line is empty, it first catches the previous entry in the history, goes at the bottom line (if it's a multi-lines statement), adds ` |>` to it, and finally inserts a new line.

### Shortcuts that cannot be customized

On top of the previous shortcuts, there are the cursor mobility shortcuts which, so far, cannot be customized.

- `shift+up` / `shift+down`: goes to the top/bottom of a multilines statement, **beware: it does not select across lines!**
- `ctrl+alt+left` / `ctrl+alt+right`: goes to the inside of the next parenthesis/bracket.

## Other features

There are many other features here and there, but I don't really remember them. For example: when a sequence of code is sent to the console, this sequence is stopped at the first error. 

## Current bugs

There are current known bugs that will be fixed:
- when downsizing the console width, lines may disappear above
- R's `.Last.value` cannot be used

## Limitations 

The two major limitation are:
- Windows only (it is built using the Windows C API)
- does not support wide characters (i.e. CJK)

## Acknowledgements

I want to thank the main source of inspiration for this project, namely [R-Studio](https://github.com/rstudio/rstudio) and [radian](https://github.com/randy3k/radian).
