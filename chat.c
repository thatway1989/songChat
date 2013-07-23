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

#define TIME  10  //10���� 

#define RESULT_SUCCESS  1
#define RESULT_FAILED  -1

#define DEFAULT_PORT  1105    //socket�����˿ڣ���������˿ڽ���ͨ��

#define LISTEN_SELEEP_TIME  2  //�������õȴ���ô��¼���һ�Σ���λΪ��

#define MAX_CONNECT_NUM  10    //֧�ֵ������������
#define CONNECT_SELEEP_TIME  2  //�������õȴ���ô�������һ�Σ���λΪ��

#define LOG printf   //�������ִ�ӡ�͵����ô�ӡ

/*Ϊ����ϵͳ��ֲ���������һ�㶼��ֱ����int��������s32��ʾ�������ռ32λ������Ժ�Ӳ�������ˣ�int��ʾ64λ�ˣ�short��ʾ32λ
��ʱ�Ϳ��԰Ѵ˾��Ϊ#define s32 short  Ȼ����������ã�����ֻ�ø���һ���ط����ɣ���Ҳ���ú궨��ĳ���*/
#define s32   int
#define s8    char
#define u32   unsigned int

/*������ȫ�ֱ���*/
fd_set read_fd;   //��fd��
fd_set write_fd;  //дfd��

s32 sock_listen;  //����socket
s32 sock_accept;  /*����socket���������ڴ������ݵ�socket����Ϊ�õ�TCPЭ�飬����˫ͨ����
                   ��:sock_listen�ǿ���ͨ����sock_accept������ͨ��*/
s32 sock_connect;  //����socket

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

    /*�ڳ����У�ֻҪ��A.B.C.D���Ҷ����ڵ���0�� ���ң�С�ڵ���255����*/
    if (a < 0 || a > 255 || b < 0 || b > 255
        || c < 0 || c > 255 || d < 0 || d > 255)
    {
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}
/*******************************************************************************
 Func Name: kbhit                                                  
   Purpose: �˺������ڼ���Ƿ��м�������                                 
    Input: ��       
    Output: ��
    Return: ����м������뷵��1��û�м������뷵��0
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
  if( (ch != EOF)&&(ch != 10) ) //�����ϴ�����س���Ӱ�죬10Ϊ�س���SCII��ֵ
  {
 ungetc(ch, stdin); 
 return 1;  
  }  
  return 0;  
}  

/*******************************************************************************
 Func Name: my_scanf                                                  
   Purpose: �˺����ṩ�˸�time��û������ͷ��أ���������루1����ʱ�������һ�Σ���ͣ�µȴ���������Ϣ��str                                
    Input: time�ȴ�ʱ�䣬str���ڷ�������ṹ��ָ��   
    Output: ��
    Return: ��������뷵��1��û�����뷵��0
     Notes: ��������������������ʱ�ã��ܿ��ٿ�һ���߳�                                
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
  /*��buf����*/
  memset(buf_write, 0, sizeof(buf_write));
  memset(buf_read, 0, sizeof(buf_read));  
  /*������socket��ʹ��ǰ����������0*/
  FD_ZERO(&read_fd);
  /*ÿ��selectǰҪ��socket����λ*/
  FD_SET(sock_chart,&read_fd);
  /*дsocket�����Ƿ���������λ*/
  if(!write_flag)
  {
   /*����ʱ������һ�˵�IP���룬�ÿ�*/
   num = strlen(ip)-strlen("my inpute");
   while(num--)
    printf(">");
   printf("my ");
   printf("inpute:");
   write_flag = 1;
  }

  ret = my_scanf(TIME,buf_write);
  if(ret)  //�����д��socket
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
  /*����socket�Ķ�д����*/
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
   sleep(CONNECT_SELEEP_TIME); //˯һ���������
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
 /*������socket��ʹ��ǰ����������0*/
 FD_ZERO(&read_fd);
 /*�Ѽ���socket���õ�read_fd����,��ʾÿ�ζ�����*/
    FD_SET(sock_listen,&read_fd);
    
 /*���û����Ҫ�����socket�ͼ���*/
 time_out.tv_sec = LISTEN_SELEEP_TIME;
 time_out.tv_usec= 0;
 if(select(sock_listen+1,&read_fd,NULL,NULL,&time_out)<0)
  return RESULT_FAILED; 
 
 /*�������������,�򴴽��ڶ���socket���жԻ�*/
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

    /*���һ��socket������,���ڼ���*/
    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_listen<0)
    {
  printf("listen_socket build failed!\n");
  return RESULT_FAILED;
    }
    
    /*��ʼ���ṹ�� sin*/
 memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET; 
    sin.sin_port = DEFAULT_PORT;
    sin.sin_addr.s_addr = INADDR_ANY;  /*��ip��ַ����Ϊ����ip*/

 /*���а󶨣���Ϊ���socket����һЩ����*/
    if (bind (sock_listen, (struct sockaddr*) &sin, sizeof(struct sockaddr_in)) < 0)
    {
  printf("bind listen_socket failed!\n");
  return RESULT_FAILED;
    }

 /*���м������ȴ�����������*/
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
 s8  ip_str[50];     //������ʱ����ȡ�Է���IP
 s32 info_flag=0;
 /*��������socket*/
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
   /*˵��ʹ�÷���*/
   printf("You can only use cmd as flows:\n1.connect \n2.exit\n");
   printf("\nPlease input your cmd:");
   info_flag = 1;
  }
  
  memset(str[0],0,sizeof(str[0]));
  my_scanf(TIME,str[0]);  //�ȴ�����

  /*�����ǽ�������*/
  if(!strcmp(str[0],"connect"))
  { 
   printf("pleale inpute the ip:");
   scanf("%s",str[1]);
   ret = isValidIpAddr(str[1]);
   if(RESULT_FAILED == ret)
   {
    printf("the IP you input is not avalied-->eg. 192.168.0.5\n");
    info_flag = 0;
    
    continue;    //�������ּ�������
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
    /*���ڿ��Խ��жԻ���*/
    socket_chart_func(sock_connect,str[1]);
    info_flag = 0;
   }
  }
  else if(!strcmp(str[0],"exit"))
  {
   printf("\n>>>>>thank you for using my program,and welcome to contact me!\n>>>>>myQQ:906470132\n");
   return 0; //�˳�����
  }
  else 
  { 
   if(strlen(str[0]) > 1)
   {
    printf("input error!!!\n");
    info_flag = 0;
   }
  }

  /*��������Ϊ����������ˢ�£��ȴ�����,���ں�����Ϊˢ����Ҫʱ�䣬���㼰ʱ��Ӧ������������*/
  ret = listen_socket_refresh_func(ip_str);
  if(RESULT_SUCCESS == ret)
  { 
   /*���ڿ��Խ��жԻ���*/
   socket_chart_func(sock_accept,ip_str);
   info_flag = 0;
  }
 }

 return 0;
}