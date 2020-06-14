#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <time.h>

#define BUF_SIZE 100

void error_handling(char *message);
void z_handler(int sig);
int who_win(int a, int b);

int main(int argc, char *argv[])
{
    int fd1[2], fd2[2];
    char buffer[BUF_SIZE];
    char intro[] = "Input (가위: 0, 바위: 1, 보: 2) :  ";
    char win[] = "You Win!!\n";
    char lose[] = "You lose!!\n";
    char drawn[] = "You are drawn!!\n";

    char operation[10];
    FILE *fp;
    char buf[BUF_SIZE];

    int serv_sock, clnt_sock, game_rand;
    struct sockaddr_in serv_addr, clnt_addr;

    pid_t pid;
    struct sigaction act;
    int str_len, state, addr_size;

    int game_init = 1;             // init 체크
	int game_money = 100;          // 게임 머니
	char load_file[10];            // 파일 로드
	int size;                      // 파일 크기
    int bet_option = 10;           // 배팅 금액

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    if (pipe(fd1) < 0 || pipe(fd2) < 0)
        error_handling("Pipe() error!!");

    act.sa_handler = z_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    state = sigaction(SIGCHLD, &act, 0);

    if (state != 0)
        error_handling("sigaction() error");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while(1)
    {
        addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &addr_size);

        if (clnt_sock == -1)
            continue;

        pid = fork();

        if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        else if (pid > 0)
        {
            /* Parent Process
             * 가위 바위 보 게임 처리
             */

            int result;
            puts("Connection Created..");
            close(clnt_sock);

            while((str_len = read(fd1[0], &buffer[1], BUF_SIZE-1)) > 0)
            {
                srand(time(NULL));
                game_rand = rand() % 3;
                sprintf(&buffer[0], "%d", game_rand);

                /* Game */
                if (game_init)
                {
                    fp = fopen("history/history.txt", "r");
                    fseek(fp, 0, SEEK_END);
                    size = ftell(fp);
                    if (size)
                    {
                        fseek(fp, -4, SEEK_END);
                        fread(load_file, 4, 1, fp);
                        fclose(fp);
                        game_money = atoi(load_file);
                        game_init = 0;
                    }
                }

                /* 저장된 배팅 금액 가져옴 */
                fp = fopen("option.txt", "r");
                fseek(fp, -4, SEEK_END);
                fread(load_file, 4, 1, fp);
                fclose(fp);
                bet_option = atoi(load_file);
                printf("bet: %d\n", bet_option);

                fp = fopen("history/history.txt", "a");
                result = who_win(atoi(&buffer[0]), atoi(&buffer[1]));

                printf("Server: %d\n", game_rand);

                if (result == 0)
                {
                    write(1, drawn, sizeof(drawn));
                    write(fd2[1], drawn, sizeof(drawn));
                    fprintf(fp, "%5s %d %d\n", "drawn", atoi(&buffer[1]), game_money);
                }
                else if (result == 1)
                {
                    write(1, win, sizeof(win));
                    write(fd2[1], lose, sizeof(lose));
                    fprintf(fp, "%5s %d %d\n", "lose", atoi(&buffer[1]), game_money-bet_option);
                    game_money -= 10;
                }
                else
                {
                    write(1, lose, sizeof(lose));
                    write(fd2[1], win, sizeof(win));
                    fprintf(fp, "%5s %d %d\n", "win", atoi(&buffer[1]), game_money+bet_option);
                    game_money += 10;
                }
                fclose(fp);
            }
            printf("Terminate the Parent Process\n");
        }
        else
        {
            /* Child Process
             * 클라이언트와 데이터 통신
             */

            close(serv_sock);

            while (1)
            {
                read(clnt_sock, operation, BUF_SIZE);

                if (atoi(operation) == 4)
                    break;
                else if (atoi(operation) == 1)
                {
                    /* Game */
                    write(clnt_sock, intro, sizeof(intro));
                    read(clnt_sock, buffer, BUF_SIZE);
                    write(fd1[1], buffer, 1);
                    str_len = read(fd2[0], buffer, BUF_SIZE);
                    write(clnt_sock, buffer, str_len);
                }
            }

            puts("Disconnected..");
            close(clnt_sock);
            exit(0);
        }
    }
    return 0;
}

int who_win(int a, int b)
{
    int result;

    if (a == b) result = 0;
    else if (a % 3 == (b+1) % 3) result = 1;
    else result = -1;

    return result;
}

void z_handler(int sig)
{
    pid_t pid;
    int status;

    pid = waitpid(-1, &status, WNOHANG);
    printf("removed proc id: %d \n", pid);
    printf("Returned data : %d \n", WEXITSTATUS(status));
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
