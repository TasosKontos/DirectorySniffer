#include <map>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <csignal>

using namespace std;

int main(int argc, char* argv[]){
    int fifo_desc = open(argv[1], O_RDONLY);
    if(fifo_desc<0){
        perror("open at worker 1");
        return 1;
    }

    map<string, int> table;
    while(1) {
        char filename_ar[200];
        if(read(fifo_desc, filename_ar, 200)<0){
            perror("worker read error");
        };
        string filename(filename_ar);
        int file_desc;
        if(argc==2) file_desc = open(filename_ar, O_RDONLY, 0);
        else{
            char* path_filename= (char*) malloc(strlen(argv[2])+strlen(filename_ar));
            strcpy(path_filename, argv[2]);
            strcat(path_filename, filename_ar);
            file_desc=open(path_filename, O_RDONLY, 0);
        }
        if (file_desc < 0) {
            perror("open at worker 2");
            return 1;
        }

        int n = 100, bytes_read = 0;
        char buffer[n];
        string url = "";
        bool flag = true;

        int readable_bytes;
        if (ioctl(file_desc, FIONREAD, &readable_bytes) < 0) {
            perror("ioctl error at worker");
            return 1;
        }

        while (flag) {
            memset(buffer, 0, n);
            if (read(file_desc, buffer, n) < 0) {
                perror("read error at worker");
                return 1;
            }
            bytes_read += 100;
            for (int i = 0; i < n; i++) {
                if (buffer[i] != ' ' && (bytes_read - 100 + i) != readable_bytes && buffer[i]!='\n') {
                    url.push_back(buffer[i]);
                } else {
                    if (url.find("http://") != string::npos) {
                        url = url.substr(url.find("http://") + 7);
                        if (url.substr(0, 4) == "www.") url = url.substr(4);
                        if (url.find('/')) url = url.substr(0, url.find('/'));
                        if (table.count(url) != 0) {
                            table.find(url)->second++;
                        } else {
                            table.insert({url, 1});
                        }
                    }
                    url = "";
                }
                if (i == n - 1 && bytes_read >= readable_bytes) flag = false;
            }
        }

        filename = filename + ".out";
        string path = "output/" + filename;

        int lenght = path.length();
        char array_path[lenght + 1];
        strcpy(array_path, path.c_str());

        int write_desc;
        if ((write_desc = open(array_path, O_CREAT | O_WRONLY | O_APPEND, 0644)) < 0) {
            perror("open error at worker");
            return 1;
        }

        char *temp;
        string entry;
        for (auto i = table.begin(); i != table.end(); i++) {
            entry = i->first + " " + to_string(i->second) + "\n";
            temp = (char *) malloc((int) entry.length());
            strcpy(temp, entry.c_str());
            write(write_desc, temp, entry.length());
            free(temp);
        }
        table.clear();

        if (close(file_desc) < 0) {
            perror("close at worker");
            return 1;
        }
        if (close(write_desc) < 0) {
            perror("close at worker");
            return 1;
        }

        memset(filename_ar, 0, 200);
        kill(getpid(), SIGSTOP);
    }
}
