Some quick notes about my shell program:

-I have implemented my solution in C.

-I did not add any additional files so the sample Makefile was sufficient. I have provided it in the tar archive.

-Background processes are assigned job numbers/ids.

		-These numbers can be from {0, 1, ..., N}

-If you wish to execute foo.exe in the background alone (without commands like cat etc.), make sure to give the full path:

		-something like: bg ./foo.exe

-I've included command line history in my shell, for ease of use.

-I have modeled my shell to notify the user of finished background processes upon hitting enter in the command line.

-Type exit if you wish to leave the shell program.  