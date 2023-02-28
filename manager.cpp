#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <cstring>
#include <queue>
#include <map>
#include <sys/wait.h>
#include <iostream>

#define PIPE_PATH "./n_pipes/";

using namespace std;


//declare flags for signal handlers
static volatile int keepRunning=1;
static volatile int chldFlag=1;
static volatile int chldreadFlag=1;
static volatile int writeFlag=1;

//queue to store stopped worker_processes
queue<int> stopped_pids;

void intHandler(int sig){
    keepRunning = 0;
}

void chldHandler(int sig){
    chldFlag = 0;
    chldreadFlag=1;
    writeFlag = 1;
}

int main(int argc ,char* argv[]){
    //check command line arguments
    if(argc!=1 && argc!=3){
        cout<<"Wrong arguments given in command!\n";
    }
    if(argc==3) {
        string str(argv[2]);
        if (str == "-p") cout << "Flag given in execution command doesn't exist!\n";
    }

    map<int ,int> pid_fd; //stores pid to file descriptor relation
    map<int, string> fd_path; //stores file descriptor to named pipe path relation

    //open listener pipe
    int listener_pipe[2];
    if(pipe(listener_pipe)<0){
        perror("pipe error in manager");
        return 1;
    }
    int listener;
    if((listener = fork())==0){ //fork listener process
        //we are in the listener process
        close(listener_pipe[0]);
        dup2(listener_pipe[1], STDOUT_FILENO); //redirect output
        //execute the correct inotifywait command depending on where specific directory was given during input
        if(argc==3){
            char* inotifywait[]={"inotifywait", "-m", "-q" ,"--format", "%f",  "-e",  "moved_to", "-e", "create", argv[2], nullptr};
            execvp(inotifywait[0], inotifywait);
        }
        else{
            char* inotifywait[]={"inotifywait", "-m", "-q" ,"--format", "%f",  "-e",  "moved_to", "-e", "create", ".",nullptr};
            execvp(inotifywait[0], inotifywait);
        }
    }
    else{
        close(listener_pipe[1]);
        //continue manager things
        char* pipe_path = PIPE_PATH;
        int worker_count=0;
        char buffer;
        int fd;

        //setup signal handling
        struct sigaction si_action;
        memset(&si_action, 0, sizeof(si_action));
        si_action.sa_handler = intHandler;
        si_action.sa_flags = 0;
        sigaction(SIGINT, &si_action, nullptr);

        struct sigaction sc_action;
        memset(&sc_action, 0, sizeof(sc_action));
        sc_action.sa_handler = chldHandler;
        si_action.sa_flags =0;
        sigaction(SIGCHLD, &sc_action, nullptr);

        //while we havent got sigint
        while(keepRunning){
            string filenm;
            //get file name from the listener pipe
            while(true){
                while(chldreadFlag){
                    chldreadFlag=0;
                    read(listener_pipe[0],&buffer,1);
                    if(chldreadFlag==0) break;
                }
                chldreadFlag=1;
                if(buffer=='\n'){
                    break;
                }
                filenm.push_back(buffer);
            }
            cout<<filenm<<endl;
            char* filenm_array = (char*)malloc(strlen(filenm.c_str()));
            strcpy(filenm_array, filenm.c_str());

            //sigchild handling
            //catching all the child processes that stopped during the sigchild signal and pushing them in the stopped-processes queue
            if(chldFlag==0){
                while(true){
                    int pid = waitpid(-1, nullptr, WNOHANG|WUNTRACED);
                    if(pid<=0) break;
                    stopped_pids.push(pid);
                }
                chldFlag=1;
            }
            if(keepRunning==0) break; //exit the manager loop if we got sigint at this point of execution
            if(stopped_pids.empty()){ //no workers available = need to make new worker
                //create worker pipe
                int worker_pid;
                char named_pipe[50];
                sprintf(named_pipe, "%d", worker_count);
                char* path = (char*)malloc(strlen(pipe_path)+strlen(named_pipe));
                strcpy(path, pipe_path);
                strcat(path ,named_pipe);
                mkfifo(path, 0666);
                //fork worker process
                if((worker_pid=fork())>0){
                    worker_count++;
                    //open the pipe from manager process
                    while(writeFlag) {
                        writeFlag=0;
                        fd = open(path, O_WRONLY);
                        if(writeFlag==0)break;
                    }
                    writeFlag=1;
                    if(fd<0)perror("open failed in manager");
                    //write file name in the pipe
                    if(write(fd, filenm_array, strlen(filenm_array))<0){
                        perror("write error in manager");
                    }
                    string str(path);
                    //store usefull info in maps
                    fd_path.insert({fd, str});
                    pid_fd.insert({worker_pid, fd});
                }
                else{
                    //execute worker code with execvp and give the specified path if it was given during input
                    if(argc==1){
                        char* temp[]={"./worker", path, nullptr};
                        execvp(temp[0], temp);
                    }
                    else{
                        char* temp[]={"./worker", path, argv[2], nullptr};
                        execvp(temp[0], temp);
                    }
                }
                //free allocated memory and clean used buffers
                free(path);
                memset(named_pipe, 0, 50);
            }
            else{
                //available worker in queue
                //get the top of the queue
                //1) put the file name in its named pipe
                //2) send sigcont to the process
                int cont_pid = stopped_pids.front();
                int cont_fd = pid_fd.find(cont_pid)->second;
                if(write(cont_fd, filenm_array, strlen(filenm_array))<0){
                    perror("write error in manager");
                }
                stopped_pids.pop();
                kill(cont_pid, SIGCONT);
            }
            free(filenm_array);
        }
        //sigint handling

        //send sigkill to all the processes that are currently running and unlink their named pipe
        int pid;
        while((pid=waitpid(-1, nullptr, WNOHANG|WUNTRACED))>0){
            int fd_to_stop = pid_fd.find(pid)->second;
            close(fd_to_stop);
            char* unlink_path = (char*)malloc(fd_path.find(fd_to_stop)->second.length());
            strcpy(unlink_path, fd_path.find(fd_to_stop)->second.c_str());
            unlink(unlink_path);
            free(unlink_path);
            kill(pid, SIGKILL);
        }

        //send sigkill to all the processes that are currently stopped and in the stopped_pid queue and unlink their named pipe
        while(!stopped_pids.empty()){
            int to_stop = stopped_pids.front();
            int fd_to_stop = pid_fd.find(to_stop)->second;
            close(fd_to_stop);
            char* unlink_path = (char*)malloc(fd_path.find(fd_to_stop)->second.length());
            strcpy(unlink_path, fd_path.find(fd_to_stop)->second.c_str());
            unlink(unlink_path);
            free(unlink_path);
            kill(to_stop, SIGKILL);
            stopped_pids.pop();
        }

        //send sigkill to the listener process
        kill(listener, SIGKILL);
    }
}