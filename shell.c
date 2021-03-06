#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include <termios.h>
#include "shell_cd.h"
#include "shell_pwd.h"
#include "shell_echo.h"
#include "shell_ls.h"
#include "shell_pinfo.h"
#include "foreground.h"
#include "background.h"
#include "nightswatch.h"
int arr[100],ind = 0;
char name[100][100];
int builtin_fn(char **tokenlist,int index,char home[],char temp[]);
int exec_fn(char *tokenlist[],int index);
int tokenize(char **tokenlist,char *token,char buf[],char del[]);
int call_fn(char *tokenlist[],int index,char home[],char temp[],char *builtins[]);
void child_terminate();
int print_prompt(char home[]);

int main(){
    char *inp=NULL,*token,**tokenlist,home[1024],multiple;
    char *buf2,*buf,**tokenlist2,*temp;
    char del[] = ";\n";
    char del2[] = " \t\r\n;";
    int index,flag,i,index2,j,ret;
    char *builtins[] = {
        "cd","pwd","ls","pinfo","echo","nightswatch"
    };
    getcwd(home,sizeof(home));
    ssize_t bufsize = 0;
    while(1){
        ret = 0;
        flag = 0;
        index = 0;
        print_prompt(home);
        getline(&inp, &bufsize, stdin);
        buf = (char *)malloc(sizeof(char)*1024);
        strcpy(buf,inp);
        if(strcmp(buf,"\n")==0)
            continue;
        tokenlist = (char **)malloc(sizeof(char *)*1024);
        index = tokenize(tokenlist,token,buf,del);
        for(i=0;i<index;i++){
            temp = (char *)malloc(sizeof(char)*1024);
            buf2 = (char *)malloc(sizeof(char)*1024);
            strcpy(buf2,tokenlist[i]);
            strcpy(temp,buf2);
            tokenlist2 = (char **)malloc(sizeof(char *)*1024);
            index2 = tokenize(tokenlist2,token,buf2,del2);
            ret = call_fn(tokenlist2,index2,home,temp,builtins);
            if(ret == -1)
                break;
            free(buf2);
            free(tokenlist2);
            free(temp);
        }
        signal(SIGCHLD, child_terminate);
        // printf("%s",name[0]);                        
        //printf("%s",tokenlist[0]);                        
        if(ret==-1)
            break;
        free(buf);
    }
    return 0;
}

int tokenize(char *tokenlist[],char *token,char buf[],char del[]){
    int index = 0;
    token = strtok(buf,del);
    while(token!=NULL){
        tokenlist[index] = token;
        token = strtok(NULL,del);
        index++;
    }
    tokenlist[index]=NULL;
    return index;
}

int call_fn(char *tokenlist[],int index,char home[],char temp[],char *builtins[]){
    int flag = 0,j;
    if(tokenlist[index-1][0]=='&'){
        flag = 1;
        exec_fn(tokenlist,index);
    }
    if(flag==0){
        if(strcmp(tokenlist[0],"exit")==0){
            return -1;
        }
        for(j=0;j<6;j++){
            if(strcmp(tokenlist[0],builtins[j])==0){
                builtin_fn(tokenlist,index,home,temp);
                break;
            }
        }
        if(j==6)
            exec_fn(tokenlist,index);
    }
    return 0;
}

int exec_fn(char *tokenlist[],int index){
    int pid = 0;
    if(tokenlist[index-1][0] == '&'){
        tokenlist[index-1] = NULL;
        strcpy(name[ind],tokenlist[0]);
        pid = bg(tokenlist);
        arr[ind] = pid;
        ind++;
    }
    else{
        fg(tokenlist);
    }
    return 0;
}

int builtin_fn(char **tokenlist,int index,char home[],char temp[]){
    int i,j,length,flag = 0;
    
    if(strcmp(tokenlist[0],"cd")==0){
        if(index>2)
            printf("cd : excess arguments given");
        else{
            if(tokenlist[1]!=NULL)
                cd(tokenlist[1],home);
            else{
                tokenlist[1]="\0";
                cd(tokenlist[1],home);
            }
        }
    }
    else if(strcmp(tokenlist[0],"pwd")==0){
        if(index > 1)
            printf("pwd : excess arguments given");
        else
            pwd();
    }
    else if(strcmp(tokenlist[0],"ls")==0)
            ls(tokenlist,index);

    else if(strcmp(tokenlist[0],"echo")==0){
        for(i=1;i<index;i++){
            length = strlen(tokenlist[i]);
            for(j=0;j<length;j++)
                if(tokenlist[i][j] =='\"' || tokenlist[i][j]=='\''){
                    flag = 1;
                    break;
                }
        }
        if(flag!=1)
            echo(tokenlist,index);
        else
            echo_quote(temp);
    }
    else if(strcmp(tokenlist[0],"pinfo")==0)
        pinfo(tokenlist);
    else if(strcmp(tokenlist[0],"nightswatch")==0)
        nightswatch(tokenlist,index);
    return 0;
}

void child_terminate()
{
        union wait wstat;
        pid_t   pid;
        char *pname;
        while (1) {
            pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL );
            if (pid == 0)
                return;
            else if (pid == -1)
                return;
            else
            {
                int i;
                for(i=0;i<ind;i++)
                {
                    if(arr[i]==pid){
                        pname = name[i];
                    }
                } 
                fprintf (stderr,"%s with pid: %d terminated %s\n",pname,pid,(wstat.w_retcode==0)?"normally":"abnormally");
            }
        }
}

int print_prompt(char home[]){
    int index;
    char host[1024],cwd[1024];
    gethostname(host,1024);
    getcwd(cwd,1024);
    if(strstr(cwd,home)){
        index = strlen(home);
        cwd[index-1] = '~';
        printf("\033[1;32m<%s@%s\033[0m:\033[1;36m%s>\033[0m$ ",getenv("USER"),host,&cwd[index-1]);
    }
    else
        printf("\033[1;32m<%s@%s\033[0m:\033[1;36m%s>\033[0m$ ",getenv("USER"),host,cwd);
    return 0;
}
