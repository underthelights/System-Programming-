[CSE4100] System Programming
# Project 2

20191603 심규환

## 1. How to compile
- put "Makefile, myshell.h, myshell.c, csapp.h, csapp.c" in same directory
- compile with 'make' command
- after compile, "myshell" file was created. Run with the command of './myshell'

## 2. Introduction
- Create child process with 'fork()' function
- Execute with Linux Basic Command 
- Pipeline command 
	- parent process create a new child process with pipe for each instruction
	- pipe the command : after distinguishing with '|' symbol,
	  previous command output is handled as the input of the latter command

## 3. Code
csapp.{c.h}
	CS:APP 3rd Edition 

myshell.c, myshell.h
	shell code

Makefile

