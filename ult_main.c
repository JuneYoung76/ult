#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>
#include <sys/time.h>
#include <math.h>

#include "led.h"

#define TRIG 4
#define ECHO 5
#define ALARM 1

pthread_mutex_t lock_ult;
float ult_speed,ult_dis;
float alarm_speed = 1;
float max_distance = 5;
float min_distance = 0.5;

void *ult(void *arg)
{
	struct timeval t1,t2;
	long time_start, time_stop;
	long time_old = 0;
	long time_new = 0;
	float dis_old = 0;
	float dis_new = 0;
	float dis_temp = 0;
	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT);
	while(1) {
		digitalWrite(TRIG, LOW);
		digitalWrite(TRIG, HIGH);
		delayMicroseconds(15);
		digitalWrite(TRIG, LOW);
		while(1) {
			if (digitalRead(ECHO))
				break;
		}
		gettimeofday(&t1, NULL);
		while(1) {
			if (!digitalRead(ECHO))
				break;
		}
		gettimeofday(&t2, NULL);
		time_start = t1.tv_sec * 1000000 + t1.tv_usec;
		time_stop = t2.tv_sec * 1000000 + t2.tv_usec;
		dis_temp = (float)(time_stop - time_start) /1000000 * 340 / 2;
		
		if((dis_temp < max_distance)&&(dis_temp > min_distance)) {
                        dis_old = dis_new;
                        time_old = time_new;
                        dis_new = dis_temp;
                        time_new = (time_stop - time_start)/2 + time_start;
                        
                        pthread_mutex_lock(&lock_ult);
                        ult_dis = dis_new;
                        ult_speed = (fabs)(dis_new - dis_old) / ((float)(time_new - time_old) / 1000000);
                        printf("dis:%f m, speed:%f m/s\n", ult_dis, ult_speed);
                        pthread_mutex_unlock(&lock_ult);
                }
		delay(200);
	}
}

void *led_disp(void *arg)
{
	int speed_disp;
	int dot_mask = pow(10,DOTMASK);
	led_gpio_init();
	while(1) {
		pthread_mutex_lock(&lock_ult);
		speed_disp = (int)(ult_speed*dot_mask);
		pthread_mutex_unlock(&lock_ult);
		led_display(speed_disp);		
	}
}

int main()
{
	float speed,distance;
	bool phflag=0;
	bool firstflag = 1;
	char buffer[100]={0};
	int num_photos=0;
	FILE *fp = NULL;
	pthread_t ult_data,ult_disp;
	
	wiringPiSetup();
	pinMode(ALARM, OUTPUT);
	pthread_create(&ult_data,NULL,ult,NULL);
	pthread_create(&ult_disp,NULL,led_disp,NULL);
	
	while(1)
	{
		pthread_mutex_lock(&lock_ult);
		speed=ult_speed;
		distance=ult_dis;
		pthread_mutex_unlock(&lock_ult);
		
		fp=fopen("/tmp/ult.conf", "r");
		if(fp) {
			fscanf(fp, "%f, %f", &max_distance, &speed_alarm);
			fclose(fp);
		}
		
		if(speed > alarm_speed) {
			if(firstflag) {
				firstflag = 0;
			} else {
                        	digitalWrite(ALARM, LOW);
                        	if(!phflag) {
                                	sprintf(buffer, "raspistill -t 200 -o /public/photos/photo-%d.jpeg -w 640 -h 480", num_photos++);
					system(buffer);
                                	phflag = 1;
                        	}
			}
                } else {
                        digitalWrite(ALARM, HIGH);
                        phflag = 0;
			firstflag = 1;
                }
		
		fp=fopen("/tmp/ult.log", "w");
		if(fp) {
			fprintf(fp, "%d, %d, %d", num_photos, distance, speed);
			fclose(fp);
		}
		
		delay(500);
	}
	return 0;
}
