#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 100
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	char intro[BUF_SIZE];
	int str_len;

	struct sockaddr_in serv_addr;

	FILE *fp;
	char operation[10];

	char win[] = "You Win!!\n";
    char lose[] = "You lose!!\n";
	int game_over = 0; // 0: game 1: over
	int game_init = 1;
	int game_money = 100;
	char load_file[10];
	int size;
	int bet_option = 10;

	// 통계 변수
    int stat_win = 0, stat_drawn = 0, stat_lose = 0;
	char log_stat[10];
    int log_input, log_money;

	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");

	printf("===========================================\n");
	printf("* WELCOME ROCK SCISSOR PAPER!! *\n\n");
	printf("- 저장된 게임은 자동으로 불러와 실행됩니다.\n");
	printf("- Good Luck!!\n");
	printf("===========================================\n");
	while(1)
	{
		/* operation */
		if (!game_over)
		{
			/* menu select */
			printf("- 1: Game\n");
			printf("- 2: Statistics\n");
			printf("- 3: Option\n");
			printf("- 4: Quit\n");
			printf("===========================================\n");

			str_len = read(0, operation, BUF_SIZE);
			printf("===========================================\n");

			// 배팅 금액 설정
			if (atoi(operation) == 3)
			{
				printf("배팅 금액을 설정하실 수 있습니다: \n");
				scanf("%d", &bet_option);
				printf("===========================================\n");
				fp = fopen("option.txt", "w");
				fprintf(fp, "bet: %d\n", bet_option);
				fclose(fp);
				continue;
			}

			write(sock, operation, str_len);
		}
		else
		{
			// 게임 오버 시 게임 종료
			operation[0] = 0;
			strcat(operation, "4");
			write(sock, operation, str_len);
		}

		if (atoi(operation) == 4)
		{
			/* Quit */
			printf("Good Bye!!\n");
			printf("===========================================\n");
			break;
		}

		switch (atoi(operation)) {
			case 1:
				/* Game */
				if (game_init)
				{
					// 게임 init
					// 기록이 존재한다면 불러옴
					fp = fopen("history/history.txt", "r");
					fseek(fp, 0, SEEK_END);
					size = ftell(fp);
					if (size)
					{
						fseek(fp, -4, SEEK_END);
						fread(load_file, 4, 1, fp);
						fclose(fp);
						game_money = atoi(load_file);
						printf("게임을 불러왔습니다.\nGame Money: %d\n", game_money);
						printf("===========================================\n");
						game_init = 0;
					}
					else
					{
						printf("새로운 게임을 시작합니다.\n");
						printf("===========================================\n");
					}
				}

				str_len = read(sock, intro, BUF_SIZE - 1); // 게임 메시지 표시

				while (1) {
					// 게임 입력 유효성 검사
					fputs(intro, stdout);
					fflush(stdout);

					message[0] = 0;
					str_len = read(0, message, BUF_SIZE);

					if (atoi(message) == 0 || atoi(message) == 1 || atoi(message) == 2)
						break;

					printf("ERROR: 잘못된 입력입니다.\n");
				}

				write(sock, message, str_len);

				str_len = read(sock, message, BUF_SIZE - 1);
				printf("===========================================\n");
				fputs(message, stdout);

				if (!strcmp(message, win))
					game_money += bet_option; // 승리 시
				else if (!strcmp(message, lose))
					game_money -= bet_option; // 패배 시

				if (game_money <= 0)
				{
					// 게임 머니가 0보다 작거나 같으면 파일에 retry 저장 후 종료
					printf("Game Over!!\n");
					fp = fopen("history/history.txt", "a");
					fprintf(fp, "%5s %d %d\n", "retry", 0, 100);
					fclose(fp);
					game_over = 1;
				}
				else
				{
					printf("Game Money: %d\n", game_money);
				}

				printf("===========================================\n");
				fflush(stdout);

				break;
			case 2:
				/* Statistics */
				fp = fopen("history/history.txt", "r"); // 서버에 저장된 history 파일을 이용해 승률 표시

				while (0 < fscanf(fp, "%5s %d %d", log_stat, &log_input, &log_money))
				{
					if (!strcmp(log_stat, "win"))
						stat_win += 1;
					else if (!strcmp(log_stat, "lose"))
						stat_lose += 1;
					else if (!strcmp(log_stat, "drawn"))
						stat_drawn += 1;
				}

				sprintf(message, "    승: %f %% (%d / %d)\n    패: %f %% (%d / %d)\n무승부: %f %% (%d / %d)\n  시도: %d\n",
					(float)stat_win/(float)(stat_win+stat_lose+stat_drawn)*100, (int)stat_win, (int)(stat_win+stat_lose+stat_drawn),
					(float)stat_lose/(float)(float)(stat_win+stat_lose+stat_drawn)*100, (int)stat_lose, (int)(stat_win+stat_lose+stat_drawn),
					(float)stat_drawn/(float)(stat_win+stat_lose+stat_drawn)*100, (int)stat_drawn, (int)(stat_win+stat_lose+stat_drawn),
					(int)(stat_win+stat_lose+stat_drawn));

				stat_win = 0;
				stat_lose = 0;
				stat_drawn = 0;

				fclose(fp);

				fputs(message, stdout);
				printf("===========================================\n");

				fflush(stdout);
				break;
			default:
			printf("ERROR: 잘못된 입력입니다.\n");
				break;
		}
		operation[0] = 0;
		message[0] = 0;
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
