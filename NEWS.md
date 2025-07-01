

### sircon 0.2.0

### new features

- AC: suggest non exported functions of the current package when the working directory is found to be a package
- special functions %path_(executable|history|options): improve path format
- improved ctrl+(backspace|delete) within paths

### Bug fixes

- fix major bug causing the program to go through an infinite loop: when AC was triggered and the line started with an open paren
- fix major bug in history collection leading to some commands to disappear
- fix display bug of the AC inside control functions
- fix bug preventing the setup of global shortcuts
- option setting: paths in global options now are always absolute

### Internal 

- add the special function %debug_to_file to send internal debug messages to a file

### sircon 0.1.0

- first release of the beta version


