#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <iostream>
#include <sys/mman.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <signal.h>
#define MAX_INPUT 30000

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

char arr[30001];
int expp[30001];
int fd[3000][2], cnt = -1, t;
bool ne = false;
int bashcnt = 0;
int pbs[2];

void sig_catch(int sig){
    if(sig == SIGCHLD){
        read(pbs[0],&bashcnt,sizeof(int));
        if(bashcnt == 0) exit(0);
    }
    return;
}

int toexeclp(int hh){
    int ret = 0;
    char *argv[hh + 2];
    for (int i = 0; i <= hh; i++) {
        argv[i] = arr + expp[i];
    }
    argv[hh + 1] = NULL;
    ret = execvp(argv[0], argv);
    if(ret == -1){
        printf("shell : command not found:%s",arr + expp[0]);
        for(int i = 1; i <= hh ;i++)
            printf(" %s",arr + expp[i]);
        printf("\n");
        return -1;
    }
    return 0;
}
int main()
{
    pipe(pbs);
    int ret = fork();
    if (ret > 0){
        //printf("I'm father\n");
        sigset_t set;
        sigaddset(&set,SIGINT);
        sigprocmask(SIG_BLOCK,&set,NULL);
        close(pbs[1]);
        struct sigaction act;
        act.sa_handler = sig_catch;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGCHLD,&act,NULL);
        while (1);
    }
    else if (ret == 0){
        sigset_t set;
        sigaddset(&set,SIGINT);
        sigprocmask(SIG_BLOCK,&set,NULL);
        bashcnt++;
        close(pbs[0]);
        int chre = chdir("/home/jianger"); // 回家
        if (chre == -1)
            sys_err("chdir"); // 判断错误返回值
        char *path = NULL;
        while (1){
            if(path!=NULL) free(path);
            path = NULL;
            path = getcwd(NULL, 0);
            printf("\033[;34m%s\033[;32m>\033[0m", path); // 输出当前工作目录
            fgets(arr, MAX_INPUT, stdin);                 // 输入命令
            int sp;
            for(sp = 0;arr[sp]!='\0';sp++)
                if(arr[sp]!=' '&&arr[sp]!='\n')
                    break;
            if(arr[sp]=='\0') continue;
            int l = 0, r = 0, len;
            //获取命令长度
            for (len = 0; len < MAX_INPUT && arr[len] != '\n' && arr[len] != '\0'; len++);
            //预处理输入
            std::string str;
            str.clear();
            for(int i=0;i<len;i++) str.push_back(arr[i]);
            while(str.size()>0 && str[0]==' ') str.erase(str.begin(),str.begin()+1);
            for(int i=0;i<str.size();i++){
                if(str[i]=='|'){
                        str.insert(str.begin()+i,' ');
                        i++;
                    }
            }
            for(int i=0;i<str.size();i++) arr[i] = str[i];
            arr[str.size()] = '\0';
            len = str.size();
            //
            if(arr[0]=='c'&&arr[1]=='d'){
                char *pa = NULL;
                pa = getcwd(NULL, 0);
                char path2[3000];
                int i = 2;
                while(arr[i]==' ') {
                    arr[i] = '\0';
                    i++;
                    if(i == len) break;
                }
                if(i == len){
                    int chre = chdir("/home/jianger"); // 回家
                    if (chre == -1)
                        perror("cd"); // 判断错误返回值
                    free(pa);
                    continue;
                }
                for(int n = i;n <= len;n++) if(arr[n]==' '||arr[n]=='\n') arr[n]='\0';
                if(strcmp(arr+i,"-")==0||strcmp(arr+i,"~")==0){
                    int chre = chdir("/home/jianger"); // 回家
                    if (chre == -1)
                        perror("cd"); // 判断错误返回值
                    free(pa);
                    continue;
                }
                else{
                    if(arr[i]!='/')
                        sprintf(path2,("%s/%s"),pa,arr+i);
                    else strcpy(path2,arr+i);
                    int chre = chdir(path2); // 回家
                    if (chre == -1)
                        perror("cd"); // 判断错误返回值
                    free(pa);
                    continue;
                }
            }
            else if(strcmp(arr,"exit") == 0){
                bashcnt--;
                write(pbs[1],&bashcnt,sizeof(int));
                exit(0);
            }
            else if(arr[0] == '\n') continue;
            //预处理命令
            cnt = 0;
            for(int i = 0; i < len; i++){
                if(arr[i]==' ') arr[i]='\0';
                if(arr[i]=='|') cnt++;//记录管道数量
            }    
            arr[len] = '\0';
            //开cnt个管道
            for(int i=0;i<cnt;i++) if(pipe(fd[i])==-1) sys_err("pipe");
            //遍历处理命令
            int pid;
            for(t = 0;t <= cnt; t++){
                pid = fork();
                if(pid == 0) break;
            }
            if(t == cnt+1){//父进程
                for(int j = 0;j < cnt ; j++) close(fd[j][0]);
                for(int j = 0;j < cnt ; j++) close(fd[j][1]);
                for(int son = 0; son <= cnt;son++)
                    if(wait(NULL)==-1)
                        sys_err("wait");
            }
            //使管道单向流动
            for(int i = 0;i <= cnt;i++){//i为当前第i个进程，
                if(i == t){//找到对应进程
                    if(t == 0){
                        //关闭所有读端
                        for(int j = 0;j < cnt ; j++) close(fd[j][0]);
                        //关闭非0外所有写端
                        for(int j = 1;j < cnt ; j++) close(fd[j][1]);
                        if(0 != cnt)
                        dup2(fd[0][1],STDOUT_FILENO);
                    }
                    else if(t == cnt){
                        //关闭所有写端
                        for(int j = 0;j < cnt ; j++) close(fd[j][1]);
                        //关闭非cnt-1外所有读端
                        for(int j = 0;j < cnt - 1 ; j++) close(fd[j][0]);
                        if(0 != cnt)
                        dup2(fd[cnt-1][0],STDIN_FILENO);
                    }
                    else{
                        //关闭除t外所有写端
                        for(int j = 0;j < cnt ; j++)
                            if(j != t)
                                close(fd[j][1]);
                        //关闭除t-1外所有读端
                        for(int j = 0;j < cnt ; j++)
                            if(j != t-1)
                                close(fd[j][0]);
                        dup2(fd[t][1],STDOUT_FILENO);
                        dup2(fd[t-1][0],STDIN_FILENO);
                    }
                }
            }
            for(int i = 0;i <= cnt;i++){//i为当前第i个进程，
                if(t == 0 && i == 0){
                    while(l<len && arr[l]!='|' && arr[l]!='<'&& arr[l]!='>') l++;
                    r = l+1;
                    if(arr[l] == '|' || l==len){//若是正常命令，正常解析
                        l = r =0;
                    }
                    else{
                        while(arr[r]==' '||arr[r]=='<'||arr[r]=='>'||arr[r]=='\0') r++;
                        char pa3[3000];//拿路径
                        if(arr[r]!='/'){
                            char *nowpath = NULL;
                            nowpath = getcwd(NULL, 0);
                            sprintf(pa3,"%s/%s",nowpath,arr+r);
                        }
                        else strcpy(pa3,arr+r);
                        int fp;
                        if(arr[l]!=arr[l+1] && arr[l]=='>')//截断
                            fp = open(pa3,O_CREAT|O_RDWR|O_APPEND|O_TRUNC,0643);
                        else//追加
                            fp = open(pa3,O_CREAT|O_RDWR|O_APPEND,0643);
                        if(fp == -1) sys_err("open");
                        if(arr[l] == '<')  dup2(fp,STDIN_FILENO);
                        else dup2(fp,STDOUT_FILENO);
                        int hh = -1;
                        for(int e = 0; e < l; e++)
                        if((e == 0 || arr[e-1]=='\0')&&arr[e]!='\0') expp[++hh] = e;
                        if(toexeclp(hh) == -1){
                            exit(0);
                        }
                    }
                    
                }
                else if(t == cnt && i == cnt){
                    int tmp = 0;
                    for(tmp = 0 ;tmp < t;){
                        if(arr[l++] == '|') tmp++;
                    }
                    int ll = l;
                    while(ll<len && arr[ll]!='|' && arr[ll]!='<'&& arr[ll]!='>') ll++;//ll指向第一个重定向符
                    r=ll+1;
                    if(ll==len){//若是正常命令，正常解析
                        l = r =0;
                    }
                    else{
                        while(arr[r]==' '||arr[r]=='<'||arr[r]=='>'||arr[r]=='\0') r++;//r指向文件名的首位置
                        //printf("arr[r]:%c    arr[r+1]:%c\n",arr[r],arr[r+1]);
                        char pa3[3000];//拿路径
                        if(arr[r]!='/'){
                            char *nowpath = NULL;
                            nowpath = getcwd(NULL, 0);
                            sprintf(pa3,"%s/%s",nowpath,arr+r);
                        }
                        else strcpy(pa3,arr+r);
                        int fp;
                        if(arr[ll]!=arr[ll+1] && arr[ll]=='>')//截断
                            fp = open(pa3,O_CREAT|O_RDWR|O_APPEND|O_TRUNC,0643);
                        else//追加
                            fp = open(pa3,O_CREAT|O_RDWR|O_APPEND,0643);
                        if(fp == -1) sys_err("open");
                        if(arr[ll] == '<')  dup2(fp,STDIN_FILENO);
                        else dup2(fp,STDOUT_FILENO);
                        int hh = -1;
                        while(arr[l]==' '||arr[l]=='\0') l++;
                        for(int e = l; e < ll; e++)
                        if((e == l || arr[e-1]=='\0')&&arr[e]!='\0') expp[++hh] = e;
                        if(toexeclp(hh) == -1){
                            exit(0);
                        }
                    }
                }
                if(i == t){//找到对应进程
                    //解析命令
                    int tmp = 0;
                    for(tmp = 0 ;tmp < t;){
                        if(arr[l++] == '|') tmp++;
                    }
                    r=l+1;
                    while(arr[r]!='|'){
                        r++;
                        if(r == len) break;
                    }
                    int hh=-1;
                    for(int e = l; e < r; e++)
                        if((e == l || arr[e-1]=='\0')&&arr[e]!='\0') expp[++hh] = e;
                    if(toexeclp(hh) == -1){
                        exit(0);
                    }
                }
            }
        }
        free(path);
    }
    return 0;
}
