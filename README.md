Kaileb Cole (kjc265), 
CS214 Assignment 3: My Shell

# MyShell
This program is a custom implementation of a terminal shell program written in C. This program is responsible for handling batch input via file arguments and user input via interactive terminal arguments.

Key responsibilities of this program are to read, parse, and execute commands submitted by the user to the terminal. Some of these include the following:

cd "Filepath": Change directory to the prvovided filepath

pwd: Print out the present working directory

which: Identify the location of executable files

exit: exit the program closing files properly and clearing memory

And other commands stored in the following directories:
/usr/local/bin
/usr/bin
/bin

In addition to this, this shell should handle wildcard declarations using the (*) character in a filename, it should handle the redirection of arguments, it should handle pipelining multiple arguments separated by the (|) character, and should allow the use of "then" and "else" arguments to handle conditional execution.

