System Programming 2021-2022 1st project
Kontos Anastasios 1115201800080

Compilation:
    -Makefile is included in the project file, to compile the program just type "make all"

    -A "make clean" macro is included to the makefile, it removes all binary files previously produced
with "make all" and also removes all the .out files produced in the ./output directory

    -Note: Inside the project directory there must be a "output/" directory for the output files to be put inside
also there needs to be a "n_pipes/" directory for the named pipes produced, during the execution of the program, tobe stored in.

Execution:
    The makefile produces a "sniffer" executional which is the one that needs to be executed for the program to work correctly.
The execution command format is:
            1) ./sniffer
                    { with this execution command the listener process and subsequently the inotifywait command executed looks for
                    changes in files and creation of new file inside the main project directory }

            2) ./sniffer -p <directory>
                    { with this execution command the listener process and subsequently the inotifywait command executed looks for
                    changes in files and creation of new files inside the <directory> directory. NOTE the <directory> used in the execution command
                    needs to already exist in the system and needs to be written in format such as <  grandparent_dir/parent_dir/dir/  >

    For the bash script execution:
The execution command format is:
            ./finder <tld1> <tld2> ...
                   { for example the command "./finder com gr" prints in the terminal the sum of occurances of urls (taken from the /output directory as previously described)
                   that end with the tld "com" and the tld "gr", the result will look something like:
                       com --> <times of occurances of urls in the /output directory that end with the com tld>
                       gr --> <times of occurances of urls in the /output directory that end with the gr tld>
                     NOTE: at least 1 tld needs to be given as argument in the script execution command in order for the script to work properly
                   }

Functionality:
    After the execution command actions are taken as follows:
        1) manager forks the listener process and waits to retrieve results
        2) listener process executes the inotifywait command and puts the results in the listener-manager pipe
        3) once results are retrieved the manager process checks if there are already constructed available workers
            3.1) if there are available worker processes, the manager assigns the file parsing to that worker, communicating the file name via the manager worker named pipe
            3.2) if there are not any available workers, the manager forks a new worker and then assigns the file parsing
        4) the worker process after being forked and after connecting to its named pipe, waits for the manager to assing a file-parsing job, when that happens it produced the
        described .out file and then stops itself and awaits a new assignment

        NOTE 1) if at any point of the program execution the manager get a SIGCHLD signal (meaning a child process / worker changed status to stopped) it handles it by pushing the
        newly stopped pid into a queue and then continues its execution normally

        NOTE 2) if at any point of the program execution the manager gets a SIGINT signal (meaning the user gave a CNTL+C input) it handles it by closing all the previously opened
        pipes and named pipes and then sending a SIGKILL (ending the execution) to all its child processes (listener + all constructed workers) and then finishes its execution
