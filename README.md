# ECE 485/585 Cache Simulation Project
### Team 4
- Chris Kane-Pardy
- Ameer Melli
- Caleb Monti

## Description of main.c 
The purpose of this code is to simulate a Lowest Level Cache (LLC) in a multi-level cache system with multiple processors. This simulation does not move data, but simply presents where and how data would be gathered, stored, moved, and deleted from the cache in response to various operations. By default, the simulated cache has a capacity of 16 MB, is 16-way set associative, and uses 64-byte lines. More specific details can be found in the [defines.c](https://github.com/WhelpImAGhost/ECE485_Project_Team4/blob/main/defines.c).

## Usage of main.c
- This program was compiled using GCC via command-line tools within a terminal. Nothing special is needed to compile.
- At compile time, the argument -DDEBUG can be added for a high verbosity output for debugging and testing/reporting
- The program can run with no arguments. The default options will set the output to "Silent Mode" and use the file [Default.din](https://github.com/WhelpImAGhost/ECE485_Project_Team4/blob/main/Other_dins/Default.din)
- The argument "-f <filename>" can be used to redirect the code to a different trace file than Default
- The argument "-m 1" can be used to change from "Silent Mode" to a more verbose "Normal Mode"


## Changing Default Parameters
If you wish to use this program with non-default values, such as a larger cache size or a different set associative value, the easiest way is to change the values within  [defines.c](https://github.com/WhelpImAGhost/ECE485_Project_Team4/blob/main/defines.c). 
For rapid testing and switching parameters, the main.c function can be changed to accept more runtime arguments. 

## Test Cases
Our .din trace files used for testing can all be found in the [TestCases Folder](https://github.com/WhelpImAGhost/ECE485_Project_Team4/tree/main/TestCases).
From there they're organized in more folders, which correspond to the aspect of the cache we were testing.

## Documentation and Final report
We used overleaf (online LaTeX editor) rather than google docs to format our report, test plan, and some intermediary works. The links to those live documents are in the 
in the [documentation](https://github.com/WhelpImAGhost/ECE485_Project_Team4/tree/main/documentation) folder in the ([README(documentation)](https://github.com/WhelpImAGhost/ECE485_Project_Team4/blob/main/documentation/README(Documentation).md)) markdown.
The PDFs of each also resides in there.