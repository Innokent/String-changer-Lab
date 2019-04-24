#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

int pipefirst[2];
int pipesecond[2];
int pipemodone[2];
int pipemodtwo[2];
int mode = 0;

void switch_mode() {
    if (!mode) {
        mode = 1;
        puts("  Переключено в режим изменения состояния");
    } else {
        mode = 0;
        puts("  Переключение в режим считывания строк");
    }
}

char Up(char Symbol) {
	if(((int)Symbol >= -63) && ((int)Symbol <= -40)) {
		Symbol = (char*)((int*)Symbol + 8);
	}
	return Symbol;
}

char Low(char Symbol) {
	if(((int)Symbol >= -31) && ((int)Symbol <= -6)) {
		Symbol = (char*)((int*)Symbol - 8);
	}
	return Symbol;
}

char Change_Register(char Symbol) {
	if(((int*)Symbol >= -63) && ((int*)Symbol <= -40)) {
		Symbol = (char*)((int*)Symbol + 8);
	}
	else if(((int*)Symbol >= -31) && ((int*)Symbol <= -6)) {
		Symbol = (char*)((int*)Symbol - 8);
	}
	return Symbol;
}

void first_proc() {
    char name[80];
    char buf;
    name [0]='\0';
    int len = 0;
    int mod1 = 1;
    int KOI8 = 0;
    fcntl(pipefirst[0], F_SETFL, O_NONBLOCK);
    fcntl(pipemodone[0], F_SETFL, O_NONBLOCK);
    while (1) {	
        if (read(pipefirst[0], &buf, 1) > 0) {
            if (buf == '\a') {
                close (pipefirst[0]);
                exit(1);
            }

            name[0]=buf;
            int i = 1;

            for (; read(pipefirst[0], &buf, 1) > 0; i++) {
                name[i]=buf;
            }
            name[i]='\0';
            switch(mod1) {
                case 1:
                    puts("#1 трансляция строки в неизменном виде:");
                    break;
                case 2:
                    puts("#2 инвертирование строки - первый символ становится последним и т.д.:");
                    for (int i = 0; i < strlen(name) / 2; i++) {
                        char interm = name[i];
                        name[i] = name[strlen(name) - i - 1];
                        name[strlen(name) - i - 1] = interm;
                    }
                    break;
                case 3:
                    puts("#3 обмен соседних символов - нечетный становится на место четного и наоборот:");
                    for (int i = 0; i < strlen(name) - 2; i+=2) {
                        char interm = name[i];
                        name[i] = name[i + 1];
                        name[i + 1] = interm;
                    }
                    break;
                case 4:
                    puts("#4 перевод в КОИ-8 - установление в 1 старшего (8-ого) бита каждого символа:");
                    len = strlen(name);
                    for (int i = 0; i < len; i++) {
                        name[i] = name[i] | (1 << 7);
                    }
                    KOI8 = 1;
                    break;
                default:
                    puts("Wrong argument #2");
            }
            puts(name);
            if(KOI8) {
                name[strlen(name) + 1] = '\0';
                name[strlen(name)] = '1';
            } else {
                name[strlen(name) + 1] = '\0';
                name[strlen(name)] = '0';
              }

            write(pipesecond[1], name, strlen(name));
            KOI8 = 0;
        }
        if (read(pipemodone[0], &buf, 1) > 0) {
            if (buf == '\a') {
                close (pipefirst[0]);
                close (pipemodone[0]);
                exit(1);
            }
            if ((atoi(&buf) > 0) && (atoi(&buf) < 5)) {
                mod1 = atoi(&buf);
            }
        }
    }
}


void second_proc() {
    fcntl(pipemodtwo[0], F_SETFL, O_NONBLOCK);
    fcntl(pipesecond[0], F_SETFL, O_NONBLOCK);
    char name[80];
    char buf;
    int mod2 = 1;
    int KOI8 = 0;
    while (1) {
        if (read(pipesecond[0], &buf, 1) > 0) {
            if (buf == '\a') {
                close (pipesecond[0]);
                close (pipemodtwo[0]);
                exit(2);
            }
            name[0]=buf;
            int i = 1;

            for (; read(pipesecond[0], &buf, 1) > 0; i++) {
                name[i]=buf;
            }
            KOI8 = atoi(&name[i-1]);
            printf("%d %d\n", KOI8, i - 1);
            name[i-1]='\0';

            switch(mod2) {
                case 1:
                    puts("#1 трансляция строки в неизменном виде:");
                    printf("%s\n",name);
                    break;
                case 2:
                    puts("#2 перевод всех символов строки в \"верхний регистр\":");
                    if(KOI8) {
                        for (int i = 0; name[i] != '\0'; i++) {
			name[i] = Up(name[i]);
                        printf("%c", name[i]);
                        }
                    } else {
		        for (int i = 0; name[i] != '\0'; i++) {
                            printf("%c", toupper(name[i]));
                        }
                    }
                    printf("\n");
                    break;
                case 3:
                    puts("#3 перевод всех символов строки в \"нижний регистр\":");
                    if(KOI8) {
                        for (int i = 0; name[i] != '\0'; i++) {
			name[i] = Low(name[i]);
                        printf("%c", name[i]);
                        }
                    } else {
		        for (int i = 0; name[i] != '\0'; i++) {
                            printf("%c", tolower(name[i]));
                        }
                    }
                    printf("\n");
                    break;
                case 4:
                    puts("#4 смена \"регистра\" всех символов строки:");
                    if(KOI8) {
                        for (int i = 0; name[i] != '\0'; i++) {
			name[i] = Change_Register(name[i]);
                        printf("%c", name[i]);
                        }
                    } else {
		        for (int i = 0; name[i] != '\0'; i++) {
                            if(islower(name[i])) {
                                printf("%c", toupper(name[i]));
                            } else {
                                if(isupper(name[i])) {
                                    printf("%c", tolower(name[i]));
                                } else {
                                    printf("%c", name[i]);
                                  }
                              }
                        }
                      }
                    printf("\n");
                    break;
                default:
                    puts("Wrong argument #2");
            }

        } else {
            for(int i = 0; i < strlen(name); i++) {
                name[i] = '\0';
            } 
          }
        if (read(pipemodtwo[0], &buf, 1) > 0) {
            if (buf == '\a') {
                close (pipefirst[0]);
                close (pipemodtwo[0]);
                exit(1);
            }
            if ((atoi(&buf) > 0) && (atoi(&buf) < 5)) {
                mod2 = atoi(&buf);
            }
        }
    }
}

void ending() {
    int status = -1;
    write(pipefirst[1], "\a", 1);
    write(pipesecond[1], "\a", 1);
    wait(&status);
    wait(&status);
    close (pipefirst[0]);
    close (pipesecond[0]);
    close (pipemodone[0]);
    close (pipemodtwo[0]);
    exit(0);
}

int main() {
    signal(SIGINT, SIG_IGN);
    puts("Функции первого процесса:");
    puts("#1 трансляция строки в неизменном виде");
    puts("#2 инвертирование строки - первый символ становится последним и т.д.");
    puts("#3 обмен соседних символов - нечетный становится на место четного и наоборот");
    puts("#4 перевод в КОИ-8 - установление в 1 старшего (8-ого) бита каждого символа");
    puts("Функции второго процесса:");
    puts("#1 трансляция строки в неизменном виде");
    puts("#2 перевод всех символов строки в \"верхний регистр\"");
    puts("#3 перевод всех символов строки в \"нижний регистр\"");
    puts("#4 смена \"регистра\" всех символов строки");
    pipe(pipefirst);
    pipe(pipesecond);
    pipe(pipemodone);
    pipe(pipemodtwo);

    int pid = 0;

    pid = fork();
    if (pid == 0) {
        first_proc();
    }

    pid = fork();
    if (pid == 0) {
        second_proc();
    }
    signal(SIGINT,switch_mode);

    char title[100];

    while(1) {
        if ((fgets(title, 100, stdin)) == NULL) {
            ending();
        }
        if (!mode) {
            write(pipefirst[1], title, strlen(title));
        } else {
            write(pipemodone[1], title, strlen(title));
            fgets(title, 100, stdin);
            write(pipemodtwo[1], title, strlen(title));
            switch_mode();
        }
        
    }
}
