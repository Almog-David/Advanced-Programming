# Custome-Linux-Shell
This repository is an assignment for Operating Systems Course in Ariel university. The assignment is to create a simple linux shell as per the assignment instrutions which requiered to implement some of my own builtin CL commands for Linux using fork() and exec().  
The assingment is written in C programming language

## How to run:
 1. __Clone repository__ on a Linux system or download the ZIP folder of the code.
 2. __Navigate__ to the root directory of the repository.
 3. __Build__ executeable object [.o file] using the command: ` make myshell ` or ` make `.
 4. __Type__ ` ./myshell ` in order to run the program.
 5. __Execute__ commands.
 
 
## List of Commands initially made:
  - `>` : Redirect the output of the command to a file. 
  - `>>` : append\add the output of the command to a file. 
  - `2>` : Redirect the stderr output of the command to a file.  
  **NOTE:** in `>` \ `>>` \ `2>` if the file doesn't exist, it will create it.
  - `prompt` : allow you the change the prompt name. In order to activate the command write `prompt = ` and the new name you want to give to the prompt.
  - `echo` : prints the arguments given from the user. Just write `echo` and what you want to print.
  - `echo $?` : print the status of the last command that was activated.
  - `cd` : **C**hange **D**irectory command [identical to the bash cd command].
  - `!!` : Executes the most recent command executed.
  - `quit` : Exits the shell.
  - `Control-C` : if the user is pressing Control-C, the program will continue and print "you typed Control-C!". If the SHELL is running another process, the process will thrown by the system (defualt behavior).
  - `|` : allow you to pipe several commands. 
  - `Adding parameters` : in order to add parameter to the shell you need to type `$` before the new variable, then type = and after that, the value you would like to give to the variable.  
For example: `$person = Almog`. when you will execute `echo person`- it will print `person`. but when you will execute `echo $person` - it will print `Almog`.
  - `read command` : in order to define a new variable you will have to do the following:
    1. type `echo Enter a string`.
    2. type  `read` and then the name of the new variable you would like to define.
    3. type the value of the new variable.  
    **NOTE:** you can only add one variable every time. Even if you will write more then variable, the value you will give will be saved only for the first variable. 
  - `history` : Displays the last 20 executed commands by the user starting form the most-recently executed one. In order to start going through the command's history, press the up arrow or the down arrow and then press enter. If you want to execute the command you are standing on, just press enter. In order to exit from the history display, press `Q`.  
**NOTE:** when you execute a command from the command history, you will go straight to the most recent command (the one you just executed).
  - `if\else` : you can run an if command and according to the command's status it will execute the following commands. If the status is 0 (command executed successfully) it will execute the commands in the then section. Otherwise, it will execute the commands in the else section.  
   you can activate the if statement under the following conditions:
    1. the command must start with if and after that a command you would like to check.
    2. after you enter the if statement, there must be `then`.
    3. you dint have to enter an `else` statement.
    4. in order to finish with the if statement, you must write `fi`.  
    **NOTE:** the if command executed line by line, which means that every output will be display under the command in the right section (according to the if statement).

 
 
  
   
   
 
