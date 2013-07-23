#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <string.h>
#include <time.h>

#include <termios.h>  
#include <fcntl.h> 

#define TIME  10  //10毫秒 

#define RESULT_SUCCESS  1
#define RESULT_FAILED  -1

#define DEFAULT_PORT  1105    //socket监听端口，即用这个端口进行通信

#define LISTEN_SELEEP_TIME  2  //用于设置等待多久从新监听一次，单位为秒

#define MAX_CONNECT_NUM  10    //支持的最大重连次数
#define CONNECT_SELEEP_TIME  2  //用于设置等待多久从新连接一次，单位为秒

#define LOG printf   //用于区分打印和调试用打印

/*为方便系统移植，大型软件一般都不直接用int。例如用s32表示这个变量占32位，如果以后硬件进步了，int表示64位了，short表示32位
这时就可以把此句改为#define s32 short  然后程序照样用，而且只用改这一个地方即可，这也是用宏定义的初衷*/
#define s32   int
#define s8    char
#define u32   unsigned int

/*以下是全局变量*/
fd_set read_fd;   //读fd集
fd_set write_fd;  //写fd集

s32 sock_listen;  //监听socket
s32 sock_accept;  /*监听socket建立的用于传送数据的socket。因为用的TCP协议，必须双通道，
                   即:sock_listen是控制通道，sock_accept是数据通道*/
s32 sock_connect;  //连接socket

int isValidIpAddr(char *szIpAddr)
{
    int a, b, c, d;
    
    if (NULL == szIpAddr)
    {
        return RESULT_FAILED;
    }

    if (strspn(szIpAddr, "0123456789.") != strlen(szIpAddr))
    {
        return RESULT_FAILED;
    }
 
    if (sscanf(szIpAddr, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    {
        return RESULT_FAILED;
    }

    /*在程序中，只要是A.B.C.D，且都大于等于0， 并且，小于等于255就行*/
    if (a < 0 || a > 255 || b < 0 || b > 255
        || c < 0 || c > 255 || d < 0 || d > 255)
    {
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}
/*******************************************************************************
 Func Name: kbhit                                                  
   Purpose: 此函数用于检测是否有键盘输入                                 
    Input: 无       
    Output: 无
    Return: 如果有键盘输入返回1，没有键盘输入返回0
     Notes:                                 
*******************************************************************************/
int kbhit(void)  
{  
  struct termios oldt, newt;  
  int ch;  
  int oldf;  
  tcgetattr(STDIN_FILENO, &oldt);  
  newt = oldt; 
  newt.c_lflag &= ~(ICANON | ECHO);  
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); 
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);  
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); 
  ch = getchar();  
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
  fcntl(STDIN_FILENO, F_SETFL, oldf);  
  if( (ch != EOF)&&(ch != 10) ) //避免上次输入回车的影响，10为回车的SCII码值
  {
 ungetc(ch, stdin); 
 return 1;  
  }  
  return 0;  
}  

/*******************************************************************************
 Func Name: my_scanf                                                  
   Purpose: 此函数提供了隔time秒没有输入就返回，如果有输入（1毫秒时间间隔检测一次）会停下等待输入完信息给str                                
    Input: time等待时间，str用于返回输入结构的指针   
    Output: 无
    Return: 如果有输入返回1，没有输入返回0
     Notes: 这个函数可以做聊天程序时用，避开再开一个线程                                
*******************************************************************************/
int my_scanf(int time,char *str)
{
 int i,flag=0;
 
 for(i=0;i<time;i++)
 {
  if(kbhit())
  {
   scanf("%s",str);
   flag = 1;
   break;
  }
  
  usleep(1000);
 }

 return flag;
}


s32 socket_chart_func(s32 sock_chart,char *ip)
{
 s32 ret=0,num=0;
 s8  buf_write[50];
 s8  buf_read[50];
 s32 write_num=0,read_num=0;
 s32 write_flag=0;
 struct timeval time_out;
 
 printf("\n!now,%s is connected ,pleas chart :\n",ip);
 
 while(1)
 {
  /*对buf清零*/
  memset(buf_write, 0, sizeof(buf_write));
  memset(buf_read, 0, sizeof(buf_read));  
  /*这两个socket集使用前，必须先清0*/
  FD_ZERO(&read_fd);
  /*每次select前要把socket置上位*/
  FD_SET(sock_chart,&read_fd);
  /*写socket根据是否有输入置位*/
  if(!write_flag)
  {
   /*聊天时与另外一端的IP对齐，好看*/
   num = strlen(ip)-strlen("my inpute");
   while(num--)
    printf(">");
   printf("my ");
   printf("inpute:");
   write_flag = 1;
  }

  ret = my_scanf(TIME,buf_write);
  if(ret)  //如果有写入socket
  { 
   if(!strcmp(buf_write,"exit"))
   {
    printf("You have break the connect with %s\n\n",ip);
    close(sock_chart);
    return 0;
   }
   write_num = write(sock_chart,buf_write,sizeof(buf_write));
   if(write_num<=0)
    printf("\nwrite error!!!\n");
   write_flag = 0;
  }
  time_out.tv_sec = 0;
  time_out.tv_usec= 1;
  /*处理socket的读写操作*/
  if(select(sock_chart+1,&read_fd,NULL,NULL,&time_out) > 0)
  {
   if(FD_ISSET(sock_chart,&read_fd))
   {
    read_num = read(sock_chart,buf_read,sizeof(buf_read));
    if(read_num<0)
     printf("\nread error!!!\n");
    else if(0==read_num)
    {
     printf("\n----->%s has closed the chart!\n\n",ip);
     close(sock_chart);
     return 0;
    }
    else
    { 
     printf("\n%s:%s\n",ip,buf_read);
     FD_CLR(sock_chart,&read_fd);
     write_flag = 0;
    }
   }
  }
 }

 return 0;
}

s32 connect_server(char *ip)
{
 s32 i;
 struct sockaddr_in addr;

 sock_connect = socket(AF_INET,SOCK_STREAM,0);
 if(sock_connect < 0)
 {
  printf("connect_socket build failed!\n");
  return RESULT_FAILED;
 }
 
 memset(&addr, 0, sizeof(struct sockaddr_in));
 addr.sin_family = AF_INET;
 addr.sin_port = DEFAULT_PORT;
 inet_aton(ip,&addr.sin_addr);

 printf(">>>>>connect %s begin!\n",ip);
 for(i=0;i<MAX_CONNECT_NUM;i++)
 {
  if(connect(sock_connect,(struct sockaddr*)&addr,sizeof(addr)) >= 0)
   return RESULT_SUCCESS;
  else
   sleep(CONNECT_SELEEP_TIME); //睡一会继续连接
  if(i != MAX_CONNECT_NUM)
   printf("sleep %ds and connect again!\n",CONNECT_SELEEP_TIME);
 }
 
 return RESULT_FAILED; 
}

u32 listen_socket_refresh_func(s8 *ip)
{
    struct sockaddr_in addr;
    s32 len,i;
    struct timeval time_out;


 memset(&addr, 0, sizeof(struct sockaddr_in));
 /*这两个socket集使用前，必须先清0*/
 FD_ZERO(&read_fd);
 /*把监听socket，置到read_fd集中,表示每次都监听*/
    FD_SET(sock_listen,&read_fd);
    
 /*如果没有需要处理的socket就继续*/
 time_out.tv_sec = LISTEN_SELEEP_TIME;
 time_out.tv_usec= 0;
 if(select(sock_listen+1,&read_fd,NULL,NULL,&time_out)<0)
  return RESULT_FAILED; 
 
 /*如果有人连接我,则创建第二个socket进行对话*/
 if(FD_ISSET(sock_listen,&read_fd))
 {
  len = sizeof (addr);
  sock_accept = accept (sock_listen, (struct sockaddr*)&addr, &len);
  strcpy(ip,(void *)inet_ntoa(addr.sin_addr));
  return RESULT_SUCCESS;
 }

 return RESULT_FAILED;
}

s32 listen_socket_init()
{
 struct sockaddr_in sin;

    /*获得一个socket描述字,用于监听*/
    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_listen<0)
    {
  printf("listen_socket build failed!\n");
  return RESULT_FAILED;
    }
    
    /*初始化结构体 sin*/
 memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET; 
    sin.sin_port = DEFAULT_PORT;
    sin.sin_addr.s_addr = INADDR_ANY;  /*将ip地址设置为任意ip*/

 /*进行绑定，即为这个socket设置一些参数*/
    if (bind (sock_listen, (struct sockaddr*) &sin, sizeof(struct sockaddr_in)) < 0)
    {
  printf("bind listen_socket failed!\n");
  return RESULT_FAILED;
    }

 /*进行监听，等待其他人连接*/
    if(listen (sock_listen, 5) < 0)
    {
  printf(" listen socket failed!\n");
  return RESULT_FAILED;
    }
    
    return RESULT_SUCCESS;
}

s32 main()
{
 s32 ret=0;
 s8  str[2][50];
 s8  ip_str[50];     //有连接时，提取对方的IP
 s32 info_flag=0;
 /*建立监听socket*/
 ret = listen_socket_init();
 if(RESULT_FAILED == ret)
 {
  printf("listen_socket_init failed\n");
  return 0;
 }

 while(1)
 {
  if(!info_flag)
  {
   /*说明使用方法*/
   printf("You can only use cmd as flows:\n1.connect \n2.exit\n");
   printf("\nPlease input your cmd:");
   info_flag = 1;
  }
  
  memset(str[0],0,sizeof(str[0]));
  my_scanf(TIME,str[0]);  //等待输入

  /*下面是进行连接*/
  if(!strcmp(str[0],"connect"))
  { 
   printf("pleale inpute the ip:");
   scanf("%s",str[1]);
   ret = isValidIpAddr(str[1]);
   if(RESULT_FAILED == ret)
   {
    printf("the IP you input is not avalied-->eg. 192.168.0.5\n");
    info_flag = 0;
    
    continue;    //结束本轮继续输入
   }
   ret = connect_server(str[1]);
   if(RESULT_FAILED == ret)
   {
    printf("connect_server:%s failed!\n",str[1]);
    info_flag = 0;
    continue;
   }
   else
   {
    /*现在可以进行对话了*/
    socket_chart_func(sock_connect,str[1]);
    info_flag = 0;
   }
  }
  else if(!strcmp(str[0],"exit"))
  {
   printf("\n>>>>>thank you for using my program,and welcome to contact me!\n>>>>>myQQ:906470132\n");
   return 0; //退出程序
  }
  else 
  { 
   if(strlen(str[0]) > 1)
   {
    printf("input error!!!\n");
    info_flag = 0;
   }
  }

  /*下面是作为服务器进行刷新，等待连接,放在后面因为刷新需要时间，方便及时响应上面两个命令*/
  ret = listen_socket_refresh_func(ip_str);
  if(RESULT_SUCCESS == ret)
  { 
   /*现在可以进行对话了*/
   socket_chart_func(sock_accept,ip_str);
   info_flag = 0;
  }
 }

 return 0;
}