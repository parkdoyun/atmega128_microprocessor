
// 컴퓨터공학과 12161569 박도윤
// 마이크로프로세서응용 프로젝트

// INT0 : 게임 화면 전환, 스테이지 초기화, 모든 스테이지 클리어 후 처음 화면으로 돌아오기
// INT1 : 게임 시작
// PIND2 : 우로 이동
// PIND3 : 점프

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <time.h>
#include "_main.h"
#include "_glcd.h"
#include "_buzzer.h"
#include "bmp_resource.h"

//#define F_CPU 14745600UL 

int player_x = 10; // 플레이어의 x 좌표
int player_y = 35; // 플레이어의 y 좌표
int object_y = 47; // 장애물(굼바), 버섯의 y 좌표
int score = 0; // 플레이어의 점수
int play_time = 0; // 플레이 시간, 1 = 0.5sec
int start_count; // 1이면 게임 시작, 게임 오버 시 0, 모든 게임 클리어 시 2
int screen_x = 0; // 카메라의 x 좌표
int heart_num = 3; // 하트 개수, 0이 되면 게임 오버
int star_count = 0; // 모은 별의 개수
int stage_number = 1; // 현재 스테이지
int s_y; // 별의 y 좌표

#define STAGE_DISTANCE	1000 // 스테이지의 길이는 3개 다 1000
#define STAGE_ARR_SIZE	1127 // 1000 + 127(화면 가로 크기)

int stage_rnd_arr[STAGE_ARR_SIZE]; // 장애물(굼바), 버섯 위치 넣는 배열

#define LCD_WINST	PORTA
#define LCD_CTRL	PORTG
#define LCD_EN	0
#define LCD_RW	1
#define LCD_RS	2
#define LCD_WDATA	PORTA

void mario_change(); // 장애물(굼바) 밟았을 때 이펙트 함수, 플레이어 캐릭터가 해골로 변함
void game_over(int); // 게임 오버 시 화면 함수
void game_clear(int); // 게임 클리어 시 화면 함수

void draw_element(int stage_arr[]); // 장애물(굼바), 버섯 그리는 함수
void draw_star(int star_arr[]); // 별을 그리는 함수
void goomba_check(int stage_arr[]); // 장애물(굼바) 걸렸는지 확인, 버섯 점수 부여 함수
int draw_heart(); // 하트 그리는 함수, 0 : 하트 없음 (게임 오버), 나머지 1
void draw_player(); // 플레이어 캐릭터 (마리오, 요시, 부끄부끄) 그리는 함수


SIGNAL(INT0_vect) // 스테이지 초기화 & 모든 스테이지 클리어 후 처음 화면으로 돌아오기 인터럽트
{
	// 초기 화면
	_delay_ms(5000);
	lcd_clear(); 
	ScreenBuffer_clear();
	
	// 모든 변수 초기화
	heart_num = 3;
	star_count = 0;
	score = 0;
	play_time = 0;
	player_x = 10;
	player_y = 35;
	screen_x = 0;
	if(start_count == 2) // 모든 스테이지 클리어 시 처음 화면으로 돌아오기
	{
		glcd_draw_bitmap(super_mario_64_hor, 3, 30, 47, 64);
		lcd_string(7, 4, "<PRESS KEY 0>");
		
		// 모든 변수 초기화
		object_y = 50;
		stage_number = 1;
		start_count = 0;
		_delay_ms(500);
	}
	else // 스테이지 초기화
	{
		srand(100 * stage_number); // 랜덤 시드 스테이지마다 다르게 설정. 스테이지마다 랜덤하게 장애물, 버섯 생성
		for(int i = 0; i < STAGE_DISTANCE; i++)
		{
			stage_rnd_arr[i] = 0;
			if(i >= 150 && i % 50 == 0) // 50마다 장애물/버섯 설정
			{
				int num = rand() % 10; // 0 ~ 9
				stage_rnd_arr[i] = num; // 1,2 : 장애물 / 3,4,5 : 버섯 (100 ~ 500 점수 랜덤 부여)
			}
		}
		
		_delay_ms(1000);
		
		// 스테이지 표시 화면
		S_S1(); // 효과음
		glcd_draw_bitmap(marioface3232, 7, 50, 32, 32); // 마리오 얼굴
		lcd_string(6, 7, "STAGE ");
		lcd_xy(6, 13);
		GLCD_1DigitDecimal(stage_number, 1); // 스테이지 번호
		_delay_ms(15000);
		
		lcd_clear();
		_delay_ms(1000);
		
		draw_player();
		glcd_draw_bitmap(heart33208, 0, 50, 8, 32); // 처음엔 하트 3개 (고정)
		lcd_string(0, 0, "SCORE"); // 점수 자리, 게임 시작하면 숫자로 변환
		lcd_string(1, 0, "DIST"); // 이동거리 자리, 게임 시작하면 숫자로 변환
		lcd_string(0, 57, "TIME"); // 시간(sec) 자리, 게임 시작하면 숫자로 변환
		lcd_string(3, 4, "<PRESS KEY 1>");
		
		_delay_ms(50);
		start_count = 1;
	}
	
}

SIGNAL(INT1_vect) // 게임 시작 인터럽트
{
	if(start_count == 1) // 게임 시작하지 않았을 땐 인터럽트 발생되지 않도록
	{
		S_S4(); // 효과음
		ScreenBuffer_clear();
		lcd_clear();
		_delay_ms(50);
		
		draw_player(); // 플레이어 캐릭터 표현
		
		glcd_draw_bitmap(heart33208, 0, 50, 8, 32); // 처음엔 하트 3개 (고정)
		lcd_xy(0, 0);
		GLCD_4DigitDecimal(score); // 점수 표시
		lcd_xy(1, 0);
		GLCD_4DigitDecimal(screen_x); // 이동거리 표시
		lcd_xy(0, 57);
		GLCD_4DigitDecimal(play_time); // 플레이 시간 표시
		glcd_draw_bitmap(cloud2417, 20, 70, 17, 24); // 구름 (배경)
		_delay_ms(50);
				
		//timer 시작
		Init_timer0();
	}
		
}

unsigned char cnt;

void Init_timer0(void) // 타이머 초기화 함수
{
	TCCR0 = 0x07;
	TCNT0 = 0x00;
	TIMSK = 0x01;
}
ISR(TIMER0_OVF_vect)
{
	cnt++;
	if(cnt == 28) // 0.5 sec
	{
		play_time++;
		lcd_xy(0, 57);
		GLCD_4DigitDecimal(play_time);		
		cnt = 0;
		if(play_time >= 9999) play_time = 0;
	}
}
void interrupt_init() // 인터럽트 초기화 함수
{
	EICRA = 0x0A; // falling edge trigger (INT0, INT1)
	EIMSK = 0x03; // INT0, INT1, INT6
	SREG |= 0x80;
}

void Port_init(void) // 포트 초기화 함수
{
	PORTA = 0x00; DDRA = 0xFF;
	PORTB = 0xFF; DDRB = 0xFF;
	PORTC = 0x00; DDRC = 0xF0;
	PORTD = 0x00; DDRD = 0x00; // D 포트 : 인터럽트, 이동 입력
	PORTE = 0x00; DDRE = 0xFF;
	PORTF = 0x00; DDRF = 0xFF;
}

void init_devices(void) // 초기화 함수
{
	cli(); // clear device
	Port_init();
	lcd_init();
	S_Start();
	sei(); // interrupt enable
}

void del_heart(void) // 하트 제거 함수
{
	heart_num--;	
}

int main(void){

	interrupt_init(); // 인터럽트 초기화
	DDRD = 0x00; // 입력 버튼으로 D 포트 사용
	
	init_devices(); // 초기화
	
	
	// 초기 화면
	glcd_draw_bitmap(super_mario_64_hor, 3, 30, 47, 64); // 슈퍼 마리오 로고
	lcd_string(7, 4, "<PRESS KEY 0>"); // 0번 키 누를 때 (인터럽트) 게임 화면으로 이동
	start_count = 0;
	
	PIND = 0x00; // 입력 버튼으로 D 포트 사용
	while(start_count != 1) {}; // 게임 시작 인터럽트 발생될 때까지 대기
		
	srand(300); // 시드 설정
	
	// 각 스테이지마다 별 3개씩 부여
	int star[9]; // star[0]~star[2] : stage 1, star[3]~star[5] : stage 2, star[6]~star[8] : stage 3
	
	for(int i = 0; i < 3; i++) // 랜덤하게 별 위치 설정
	{
		star[i * 3] = rand() % STAGE_DISTANCE; // 별 위치는 0~999 중 하나
		star[i * 3 + 1] = rand() % STAGE_DISTANCE;
		while(star[i * 3] == star[i * 3 + 1]) star[i*3+1] = rand() % STAGE_DISTANCE; // 스테이지에 있는 별 3개의 위치 겹치지 않도록 설정
		star[i * 3 + 2] = rand() % STAGE_DISTANCE;
		while((star[i * 3] == star[i * 3 + 2]) || (star[i * 3 + 2] == star[i * 3 + 1])) star[i*3+2] = rand() % STAGE_DISTANCE;
	}
	
	s_y = 15; // 별의 y 좌표
	
	while(1){		
		
			while((PIND & 0x04) != 0x04) // PIND 2번 누르면 우로 이동
			{								
				ScreenBuffer_clear();
				int clear_cnt = 0;
				
				if(player_x >= STAGE_DISTANCE) // 스테이지 완주
				{
					glcd_draw_bitmap(flower2424, player_y-21, player_x-screen_x+50, 24, 24); // 완주 시 파이프 위에 꽃이 핌
					_delay_ms(30000);
					
					// 게임 클리어 화면 이동
					game_clear(stage_number);
					clear_cnt = 1;
					break;
					
				}
				if(clear_cnt == 1) break;
				lcd_clear();
				screen_x += 5; // 한번 이동 시 5씩 이동
				player_x += 5;
				
				glcd_draw_bitmap(cloud2417, 20, 70, 17, 24); // 구름(배경)
				
				// 장애물(굼바), 버섯 표시
				draw_element(stage_rnd_arr);				
				
				// 별 표시
				draw_star(star);				
				
				// 장애물 확인, 버섯 점수 부여
				goomba_check(stage_rnd_arr);				
								
				draw_player(); // 플레이어 표시
								
				lcd_xy(0, 0);
				GLCD_4DigitDecimal(score); // 점수
				lcd_xy(1, 0);
				GLCD_4DigitDecimal(screen_x); // 이동 거리
				
				if(draw_heart() == 0) // 하트 0개, 게임 오버
				{
					game_over(stage_number); // 게임 오버 화면 이동
					_delay_ms(30000);
				}				
			}
			
			while((PIND & 0x08) != 0x08) // PIND 3번 누르면 점프
			{
				ScreenBuffer_clear();
				// 1. 점프 당시 (장애물 & 버섯에 부딪치지 않음, 별 획득 가능)
				lcd_clear();
				_delay_ms(1000);
				screen_x += 10; // 점프 시에는 10씩 이동 (총 20 이동)
				player_x += 10;
				player_y -= 10; // 25
				
				glcd_draw_bitmap(cloud2417, 20, 70, 17, 24); // 구름(배경)
				
				// 장애물 버섯 표시
				draw_element(stage_rnd_arr);				
				
				// 별 표시
				draw_star(star);
				
				// 별 먹었나 확인 (판정 범위 -5 ~ +5)
				int star_idx = (stage_number - 1) * 3;
								
				if((star[star_idx] >= player_x - 5 && star[star_idx] <= player_x + 5) ||
				(star[star_idx + 1] >= player_x - 5 && star[star_idx + 1] <= player_x + 5 ) ||
				(star[star_idx + 2] >= player_x - 5 && star[star_idx + 2] <= player_x + 5)) star_count++; // 별 먹었으면 카운트 +1
						
				draw_player(); // 플레이어 표시
				
				lcd_xy(0, 0);
				GLCD_4DigitDecimal(score); // 점수
				lcd_xy(1, 0);
				GLCD_4DigitDecimal(screen_x); // 이동 거리
				
				if(draw_heart() == 0) // 하트 0개, 게임 오버
				{
					game_over(stage_number); // 게임 오버 화면 이동
					_delay_ms(30000);
				}
				
				int clear_cnt = 0;
				if(player_x >= STAGE_DISTANCE) // 스테이지 완주
				{
					glcd_draw_bitmap(flower2424, player_y-21, player_x-screen_x+50, 24, 24); // 완주 시 파이프 위에 꽃이 핌
					_delay_ms(30000);
					// 게임 클리어 화면 이동
					game_clear(stage_number);
					clear_cnt == 1;				
					
				}
				if(clear_cnt == 1) break;
				
				// 2. 점프 뛰고 난 후
				ScreenBuffer_clear();
				lcd_clear();
				_delay_ms(1000);
				screen_x += 10;
				player_x += 10; // 10 이동 (총 20 이동)
				player_y += 10; // 35
				
				glcd_draw_bitmap(cloud2417, 20, 70, 17, 24); // 구름(배경)
				
				// 장애물, 버섯 표시
				draw_element(stage_rnd_arr);
				
				// 별 표시
				draw_star(star);
				
				// 장애물 확인, 버섯 점수 부여
				goomba_check(stage_rnd_arr);
				
				draw_player(); // 플레이어 표시
								
				lcd_xy(0, 0);
				GLCD_4DigitDecimal(score); // 점수
				lcd_xy(1, 0);
				GLCD_4DigitDecimal(screen_x); // 이동 거리
				
				if(draw_heart() == 0) // 게임 오버
				{
					game_over(stage_number);
					_delay_ms(30000);
				}
				
				
				clear_cnt = 0;
				if(player_x >= STAGE_DISTANCE) // 스테이지 완주
				{
					glcd_draw_bitmap(flower2424, player_y-21, player_x-screen_x+50, 24, 24); // 완주 시 파이프 위에 꽃이 핌
					_delay_ms(30000);
					// 게임 클리어 화면 이동
					game_clear(stage_number);
					clear_cnt == 1;
					break;
					
				}
				if(clear_cnt == 1) break;
				
			}						
				
	}
	
	
}


void mario_change() // 장애물 밟았을 때 이펙트 효과 함수
{
	glcd_draw_bitmap(skeleton3232, player_y, player_x-screen_x, 32, 32);
	_delay_ms(1000);
	glcd_draw_bitmap(mario3232, player_y, player_x-screen_x, 32, 32);
	_delay_ms(1000);
	glcd_draw_bitmap(skeleton3232, player_y, player_x-screen_x, 32, 32);
	_delay_ms(1000);
	glcd_draw_bitmap(mario3232, player_y, player_x-screen_x, 32, 32);
	
}

void game_over(int stage_n) // 게임 오버 화면
{
	TCCR0 = 0x00; // 타이머 동작 중지
	
	_delay_ms(1000);
	lcd_clear();
	_delay_ms(1000);
	
	glcd_draw_bitmap(mariogameover2424, 0, 50, 24, 24);
	lcd_string(3, 0, "----<GAME OVER>-----");
	
	lcd_string(4, 3, "STAGE ");
	lcd_xy(4, 16);
	GLCD_1DigitDecimal(stage_n, 1); // 현재 스테이지 정보
	
	lcd_string(5, 3, "SCORE ");
	lcd_xy(5, 13);
	GLCD_4DigitDecimal(score); // 점수
	
	lcd_string(6, 3, "TIME ");
	lcd_xy(6, 13);
	GLCD_4DigitDecimal(play_time); // 플레이 시간
	
	lcd_string(7, 1, "*[RESTART] KEY 0*"); // 0 버튼 누르면 해당 스테이지 재시작 인터럽트 발생
	
	start_count = 0;
}

void game_clear(int stage_n) // 게임 클리어 화면
{
	TCCR0 = 0x00; // 타이머 동작 중지
	
	_delay_ms(1000);
	lcd_clear();
	_delay_ms(1000);
	
	glcd_draw_bitmap(peach4040, 5, 0, 40, 40); // 피치 공주
	lcd_string(2, 4, "----------------");
	
	lcd_string(1, 4, "<STAGE ");
	lcd_string(1, 12, " CLEAR!>");
	
	lcd_xy(1, 11);
	GLCD_1DigitDecimal(stage_n, 1); // 클리어한 스테이지 정보
	
	lcd_string(3, 6, "SCORE ");
	lcd_xy(3, 15);
	GLCD_4DigitDecimal(score); // 점수
	
	lcd_string(4, 6, "TIME ");
	lcd_xy(4, 15);
	GLCD_4DigitDecimal(play_time); // 플레이 시간
	
	lcd_string(5, 6, "STAR ");
	lcd_xy(5, 18);
	GLCD_1DigitDecimal(star_count, 1); // 획득한 별 개수
	
	
	// 마지막 스테이지(3)였다면 게임 종료
	if(stage_number == 3)
	{
		lcd_string(7, 3, "*[EXIT] KEY 0*");
		start_count = 2; // 2이면 종료
		return;
	}
	
	lcd_string(7, 1, "*[CONTINUE] KEY 0*"); // 마지막 스테이지가 아니라면 다음 스테이지로 이동
	
	_delay_ms(500);
	stage_number++; // 다음 스테이지
	start_count = 0; 
	
}

void draw_element(int stage_arr[]) // 장애물(굼바), 버섯 표시 함수
{
	for(int i = screen_x; i < screen_x + 127; i++)
	{
		if(stage_arr[i] == 0) {}
		else if(stage_arr[i] == 1 || stage_arr[i] == 2) // 1, 2 굼바
		{
			glcd_draw_bitmap(goomba1616, object_y, i-screen_x, 16, 16);
			
		}
		else if(stage_arr[i] == 3 || stage_arr[i] == 4 || stage_arr[i] == 5) // 3, 4, 5 버섯
		{
			glcd_draw_bitmap(mushroom1616, object_y, i-screen_x, 16, 16);
		}
		
		if(i == STAGE_DISTANCE) // 스테이지의 끝에 다다를 시에
		{
			glcd_draw_bitmap(pipe2424, player_y + 5, i-screen_x+50, 24, 24); // 파이프 표시
		}
		
	}
}

void draw_star(int star_arr[]) // 별 표시 함수
{
	int s1 = star_arr[(stage_number-1) * 3];
	int s2 = star_arr[(stage_number-1) * 3 + 1];
	int s3 = star_arr[(stage_number-1) * 3 + 2];
	
	if(s1 >= screen_x && s1 <= (screen_x+127) ) glcd_draw_bitmap(star1616, s_y, s1-screen_x, 16, 16); // 별 표시
	if(s2 >= screen_x && s2 <= (screen_x + 127)) glcd_draw_bitmap(star1616, s_y, s2-screen_x, 16, 16);
	if(s3 >= screen_x && s3 <= (screen_x + 127)) glcd_draw_bitmap(star1616, s_y, s3-screen_x, 16, 16);
	
}

void goomba_check(int stage_arr[]) // 장애물(굼바) 걸렸는지 확인, 버섯 점수 부여 함수
{
	int check = stage_arr[player_x];
	if(check == 1 || check == 2) // 장애물(굼바) 밟았을 때
	{
		del_heart(); // 하트 한개 제거
		mario_change(); // 플레이어 이펙트
	}
	else if(check == 3) score += 100; // 점수 100 ~ 500 까지 랜덤하게 부여
	else if(check == 4) score += 200;
	else if(check == 5) score += 500;
}

int draw_heart() // 하트 표시 함수
{
	if(heart_num == 0) return 0; // 하트 없을 때에 0 반환, 게임 오버
	
	if(heart_num == 3) glcd_draw_bitmap(heart33208, 0, 50, 8, 32); // 3개
	else if(heart_num == 2) glcd_draw_bitmap(heart21608, 0, 55, 8, 16); // 2개
	else  glcd_draw_bitmap(heart188, 0, 60, 8, 8); // 1개
	
	return 1; // 남은 하트 있을 때에 1 반환
}

void draw_player() // 플레이어 캐릭터 표시 함수
{
	if(stage_number == 1) glcd_draw_bitmap(mario3232, player_y, player_x-screen_x, 32, 32); // 1 스테이지 : 마리오 
	else if(stage_number == 2) glcd_draw_bitmap(yoshi3232, player_y, player_x-screen_x, 32, 32); // 2 스테이지 : 요시
	else if(stage_number == 3) glcd_draw_bitmap(boo3232, player_y, player_x-screen_x, 32, 32); // 3 스테이지 : 부끄부끄
}