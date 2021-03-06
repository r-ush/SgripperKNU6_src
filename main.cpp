/***************************************************************************
 *
 *  Copyright (C) 2009-2010  Moehwald GmbH B. Benner
 *                     2011  IgH Andreas Stewering-Bone
 *                     2012  Florian Pose <fp@igh-essen.com>
 *
 *  This file is part of the IgH EtherCAT master
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT master. If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *  The license mentioned above concerns the source code only. Using the
 *  EtherCAT technology and brand is only permitted in compliance with the
 *  industrial property and similar rights of Beckhoff Automation GmbH.
 * 
 * 	100당 0.425A [BL42 suction gripper 용]
 * 
 *  24 count per revolution
 * 	최대 3600 count
 * 
 * 	long double Kp[] = {7.2, 7.2, };
	long double Ki[] = {0.5, 0.5, };
	long double Kd[] = {0.6, 0.6, };
 *
 *****************************************************************************/
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <cctype>

#include <time.h>
#include <stdlib.h>
#include <termios.h>

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <istream>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <math.h>

//Xenomai
#include <alchemy/task.h>
#include <alchemy/sem.h>
#include <alchemy/mutex.h>
#include <alchemy/timer.h>
#include <trank/rtdk.h>

//Eigen
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Eigen>
#include <eigen3/Eigen/Dense>


// ROS
#include <ros/ros.h>
#include "std_msgs/Int16.h"
#include "std_msgs/Int32.h"

//*****************Ethercat***************//
// EtherCAT Mater
#include "Elmo_EtherCAT.h"


int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;

//*****************Ethercat***************//

#define ms(x) (x*1000000)

#define PI 3.141592
#define TARGET_TIME 2000000000 //0.2s = 2*10^8 ns
#define ACC_TORQUE_PARAM 0.0384
#define SEC_TO_NANOSEC 1000000000
#define TORQUE_MODE 10
#define POSITION_MODE 05

/*****************************************************************************
 * Global variable
 ****************************************************************************/
using namespace Eigen;
RT_TASK my_task;
RT_TASK ecat_ch;

RT_TASK print_task;
static int run = 1;

// Ethercat Master Object
EthercatMaster ethercatMaster;

const unsigned int cycle_ns = 1000000; /* 1 ms */

// EtherCAT Data (in pulse)
int32_t 	ZeroPos[NUM_ELMO_DRIVER ] = {0,};
uint16_t	StatusWord[NUM_ELMO_DRIVER ] = {0,};
int32_t 	ActualPos[NUM_ELMO_DRIVER ] = {0,};
int32_t 	ActualVel[NUM_ELMO_DRIVER ] = {0,};
int16_t 	ActualTor[NUM_ELMO_DRIVER ] = {0,};
uint32_t	DataIn[NUM_ELMO_DRIVER ] = {0,};
uint32_t	DCLinkCircuitVoltage[NUM_ELMO_DRIVER ] = {0,};
int32_t		DigitalInputs[NUM_ELMO_DRIVER ] = {0,};
int16_t		AnalogInput1[NUM_ELMO_DRIVER ] = {0,};
int32_t		AuxiliaryPositionActualValue[NUM_ELMO_DRIVER ] = {0,};
int16_t		CurrentActualValue[NUM_ELMO_DRIVER ] = {0,};
			// fake_real_position[0] = ActualPos[0] - initializing_constant[0]; 



int32_t 	TargetPos[NUM_ELMO_DRIVER ] = {0,};
int32_t 	TargetVel[NUM_ELMO_DRIVER ] = {0,};
int16_t 	TargetTor[NUM_ELMO_DRIVER ] = {0,};
int16_t 	TargetTor4I[NUM_ELMO_DRIVER ] = {0,};  // TARGET TORQUE FOR INITIALIZING
uint32_t 	DataOut[NUM_ELMO_DRIVER ] = {0,};
uint8_t 	ModeOfOperation[NUM_ELMO_DRIVER ] = {0,};
uint16_t	MaxTorque[NUM_ELMO_DRIVER ] = {0,};
uint16_t	ControlWord[NUM_ELMO_DRIVER ] = {0,};
int32_t		DigitalOutputs[NUM_ELMO_DRIVER ] = {0,};

// uint8_t     OutputByte0000[NUM_HMS_ANYBUS] = {1,}; //CAN DATA 1
// uint8_t     OutputByte0001[NUM_HMS_ANYBUS] = {3,}; //CAN DATA 2
// uint8_t     OutputByte0002[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 3
// uint8_t     OutputByte0003[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 4
// uint8_t     OutputByte0004[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 5
// uint8_t     OutputByte0005[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 6
// uint8_t     OutputByte0006[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 7
// uint8_t     OutputByte0007[NUM_HMS_ANYBUS] = {0,}; //CAN DATA 8


uint8_t     InputByte0000[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0001[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0002[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0003[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0004[NUM_HMS_ANYBUS] = {0,}; // Data 1
uint8_t     InputByte0005[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0006[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0007[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0008[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0009[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0010[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0011[NUM_HMS_ANYBUS] = {0,}; 
uint8_t     InputByte0012[NUM_HMS_ANYBUS] = {0,}; // Data 2
uint8_t     InputByte0013[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0014[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0015[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0016[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0017[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0018[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0019[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0020[NUM_HMS_ANYBUS] = {0,}; // Data 3
uint8_t     InputByte0021[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0022[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0023[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0024[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0025[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0026[NUM_HMS_ANYBUS] = {0,};
uint8_t     InputByte0027[NUM_HMS_ANYBUS] = {0,};

//initializing

int initializing_constant[] = {0,0,};
int fake_real_position[] = {0,0,};

//Current Control
int16_t target_Current[NUM_ELMO_DRIVER ] = {0,0,};
int16_t target_Current4I[NUM_ELMO_DRIVER ] = {0,0,};

// double max_torque = 800;
float max_Position = 3100;

int16_t initializing_motor = 0;
int32_t selected_actuator_num_current = 3;
int16_t init_stop_motor = 0;


float error[] = {0,0,}; // error값이란 설정값과 출력값의 편차를 말합니다.
float errorPrevious[] = {0,0,}; //전에 error값을 기억해둡니다.

float error4I[] = {0,0,}; // error값이란 설정값과 출력값의 편차를 말합니다.
float errorPrevious4I[] = {0,0,}; //전에 error값을 기억해둡니다.

/*
for initializing constant
*/

int target_torque[NUM_ELMO_DRIVER] = {0,0,};	
// int digi_in[0] = {0};
// int digi_out[0] = {0};

int target_torque4I[NUM_ELMO_DRIVER] = {0,0,};	




/*================Jogging Parameter===============*/

double temp[NUM_ELMO_DRIVER ] = {0,};

// Function Prototypes

void my_task_proc(void* arg);

void print_run(void *arg);
void signal_handler(int sig);

void current_control(int actuator_index);

void initializing_function();

//void pid_control_trapezoid(int32_t selected_actuator_num,  const RTIME* now);

/*****************************************************************************
 * ROS Node Class
 ****************************************************************************/
class RosNodeClass
{
	public:
		RosNodeClass(){		
			sub_initializing = nh.subscribe("initializing", 100, &RosNodeClass::initializingCallback, this);
			sub_init_stop = nh.subscribe("init_stop", 100, &RosNodeClass::initstopCallback, this);
			sub_current_select_motor = nh.subscribe("motor_num_current", 100, &RosNodeClass::selectMotorCurrentCallback, this);
			sub_target_Current = nh.subscribe("target_Current", 100, &RosNodeClass::currentControlCallback, this);
		}
		
		void initializingCallback(const std_msgs::Int16& msg){
			std::cout << "initializing mode selected\n";   
			initializing_motor = msg.data;
		}
		void initstopCallback(const std_msgs::Int16& msg){
			std::cout << "initailizing will be stop";

			init_stop_motor = msg.data;
			// funcinitstop(init_stop_motor);
		}

		void selectMotorCurrentCallback(const std_msgs::Int32& msg){
			// if (msg.data != 7){
			std::cout << "Motor: " << msg.data << " selected\n";
			selected_actuator_num_current = msg.data;
			// }

			// else if (msg.data == 7){
			// 	std::cout << "initializing mode selected\n";   
			// 	initializing_motor = msg.data;
			// 	initializing_function(initializing_motor);
			// }
		}
		void currentControlCallback(const std_msgs::Int16& msg){ 
			std::cout << "Current: " << msg.data << " selected\n";
			target_Current[selected_actuator_num_current] = msg.data;
		}
		

	private: 
		ros::NodeHandle nh;

		ros::Subscriber sub_initializing;
		ros::Subscriber sub_init_stop;
		ros::Subscriber sub_current_select_motor;
		ros::Subscriber sub_target_Current;
};

// int funcinitstop(int kk){

// 	if (kk == 1){
// 		std::cout << "initailizing will be stop";
// 		TargetTor[selected_actuator_num_current] = 0;
// 		init_stop_motor = 0;
// 	}
// }

/*****************************************************************************
 * Realtime task
 ****************************************************************************/
void my_task_proc(void* arg)
{	

//Xenomai
	unsigned int cycle_counter = 0;
	RTIME start_time;

	RTIME now;
	RTIME previous;

	rt_task_set_periodic(NULL, TM_NOW, (int)ms(1)); // ns

	RosNodeClass RosNodepub;
	
	
//Xenomai Loop Start
	start_time = rt_timer_read();
	int i = 0;
	while (run) 
	{
		rt_task_wait_period(NULL);

		cycle_counter++;

		// receive EtherCAT frames
		if (!run)
		{
			break;
		}
		previous = now;
		now = rt_timer_read(); 
		
	//Ethercat process 
		
		wkc = ethercatMaster.processTxDomain();	
		// ethercatMaster.readBuffer(DIGITAL_INPUTS, digi_in);
		// ethercatMaster.readBuffer(DIGITAL_OUTPUTS, digi_out);

		ethercatMaster.readBuffer(STATUS_WORD, StatusWord);
		ethercatMaster.readBuffer(POSITION_ACTUAL_VALUE, ActualPos);
		ethercatMaster.readBuffer(VELOCITY_ACTUAL_VALUE, ActualVel);
		ethercatMaster.readBuffer(TORQUE_ACTUAL_VALUE, ActualTor);
		ethercatMaster.readBuffer(DC_LINK_CIRCUIT_VOLTAGE, DCLinkCircuitVoltage);
		ethercatMaster.readBuffer(DIGITAL_INPUTS, DigitalInputs);
		ethercatMaster.readBuffer(ANALOG_INPUT_1, AnalogInput1);
		ethercatMaster.readBuffer(AUXILIARY_POSITION_ACTUAL_VALUE, AuxiliaryPositionActualValue);
		ethercatMaster.readBuffer(CURRENT_ACTUAL_VALUE, CurrentActualValue);	
		//ethercatMaster.readBuffer(INPUT_BYTE_0000, InputByte0000);
		//ethercatMaster.readBuffer(INPUT_BYTE_0001, InputByte0001);
		//ethercatMaster.readBuffer(INPUT_BYTE_0002, InputByte0002);
		//ethercatMaster.readBuffer(INPUT_BYTE_0003, InputByte0003);
		//ethercatMaster.readBuffer(INPUT_BYTE_0004, InputByte0004);
		//ethercatMaster.readBuffer(INPUT_BYTE_0005, InputByte0005);
		// ethercatMaster.readBuffer(INPUT_BYTE_0006, InputByte0006);
		// ethercatMaster.readBuffer(INPUT_BYTE_0007, InputByte0007);
		// ethercatMaster.readBuffer(INPUT_BYTE_0008, InputByte0008);
		// ethercatMaster.readBuffer(INPUT_BYTE_0009, InputByte0009);
		// ethercatMaster.readBuffer(INPUT_BYTE_0010, InputByte0010);
		// ethercatMaster.readBuffer(INPUT_BYTE_0011, InputByte0011);
		// ethercatMaster.readBuffer(INPUT_BYTE_0012, InputByte0012);
		// ethercatMaster.readBuffer(INPUT_BYTE_0013, InputByte0013);
		// ethercatMaster.readBuffer(INPUT_BYTE_0014, InputByte0014);
		// ethercatMaster.readBuffer(INPUT_BYTE_0015, InputByte0015);
		// ethercatMaster.readBuffer(INPUT_BYTE_0016, InputByte0016);
		// ethercatMaster.readBuffer(INPUT_BYTE_0017, InputByte0017);
		// ethercatMaster.readBuffer(INPUT_BYTE_0018, InputByte0018);
		// ethercatMaster.readBuffer(INPUT_BYTE_0019, InputByte0019);
		// ethercatMaster.readBuffer(INPUT_BYTE_0020, InputByte0020);
		// ethercatMaster.readBuffer(INPUT_BYTE_0021, InputByte0021);
		


		if(wkc >= expectedWKC) 
		{
			fake_real_position[0] = ActualPos[0] - initializing_constant[0]; 
			fake_real_position[1] = ActualPos[1] - initializing_constant[1]; 

			if (initializing_motor == 7){
				current_control(7);

				// initializing_function();
			}
			else if(initializing_motor != 7){
				current_control(selected_actuator_num_current);
			}
			//current_control(0);
		}
		needlf = TRUE;

		// ethercatMaster.writeBuffer(TARGET_POSITION, TargetPos);
		ethercatMaster.writeBuffer(TARGET_TORQUE, TargetTor);   //(const int EntryID, void* data)

		ethercatMaster.processRxDomain();

		ros::spinOnce();

	}
}

void print_run(void *arg)
{
	RTIME now, previous=0;
	unsigned long itime=0, step;
	long stick=0;
	int count=0;
	unsigned int NumSlaves=0, masterState=0, slaveState=0;

	rt_task_set_periodic(NULL, TM_NOW, (int)ms(100));

	
	while (1)
	{
		if (++count==10)
		{
			++stick;
			count=0;
		}
		if (1)
		{
			now = rt_timer_read();  
			step=(unsigned long)(now - previous) / 1000000;
			itime+=step;
			previous=now;

			for(int i = 0 ; i < NUM_ELMO_DRIVER ; i++)
			{
				rt_printf("\n\e[32;1m\tPID value: %d Slave %d / targetPos: %d Tor: %d Pos: %d  Vel: %d  \e[0m", ActualTor[i], i, target_Current[i], TargetTor[i], ActualPos[i], ActualVel[i]);  //AuxiliaryPositionActualValue[i]
				rt_printf("\n\e[32;1m\tcurrent: %d error: %d errorprevious %d \e[0m", CurrentActualValue[i], error[i], errorPrevious[i]);
				rt_printf("\n\e[32;1m\t AnalogInput1 %d initializing_constantI %d / ActualPosI: %d,  fake_real_positionI: %d" ,AnalogInput1[i] , initializing_constant[i], ActualPos[i], fake_real_position[0]);

				rt_printf("\n");

				// rt_printf("\n\e[32;1m\t ID: %d Error: %d ActualPos: %d, target_torque %Lf avg %LF \e[0m", 0, error4I[0], ActualPos[0], target_torque4I[0], avg4i);
				// rt_printf("\n\e[32;1m\t ID: %d avg: %LF \e[0m", 0, avg4i); 
				// rt_printf("\n\e[32;1m\t ID: %d initializing_constant: %d \e[0m", 0, initializing_constant); 

			}

			rt_printf("\n");

		}

		rt_task_wait_period(NULL); //wait for next cycle
	}
}

/****************************************************************************
 * Signal handler
 ***************************************************************************/

void signal_handler(int sig)
{
	rt_task_delete(&my_task);
	rt_task_delete(&ecat_ch);

	rt_task_delete(&print_task);
    run = 0;

}
/****************************************************************************
 * initializing

 ***************************************************************************/

/****************************************************************************
 * Current Control

 ***************************************************************************/

void current_control(int actuator_index) {
	// long double target_position[] = {0,0,};
	// long double Kp[] = {0.1, 7.2, };
	// long double Ki[] = {0.002, 0.5, };
	// long double Kd[] = {0.009, 0.6, };
	float Kp[] = {7.2, 7.2, };     // 53	51
	float Ki[] = {0.7, 0.5, };	// 101	107
	float Kd[] = {0.6, 0.6, };	// 1.7	1.8
	float PControl[] = {0,0, };
	float IControl[] = {0,0, };
	float DControl[] = {0,0, }; // 비례, 적분, 미분 제어항의 결과 값입니다.

	// target_torque[actuator_index] = target_Current[actuator_index];

	if (actuator_index == 2){
		if(target_Current[0] > (int16_t)max_Position)
		{
			target_Current[0] = (int16_t)max_Position;
		}
		else if(target_Current[1] > (int16_t)max_Position)
		{
			target_Current[1] = (int16_t)max_Position;
		}
		else if(target_Current[0]< 0)
		{
			target_Current[0] = 0;
		}
		else if(target_Current[1]< 0)
		{
			target_Current[1] = 0;
		}

		if (target_Current[0] >= 20){
			target_Current[0] = target_Current[2] - 20;
		}
		else if (target_Current[0] < 20){
			target_Current[0] = target_Current[2];
		}
		target_Current[1] = target_Current[2];

		error[0] =  target_Current[0] - fake_real_position[0];
		PControl[0] = Kp[0] * error[0]; //비례항 입니다.
		IControl[0] += Ki[0] * error[0] * 0.001; // 적분항 입니다.
		DControl[0] = Kd[0] * (error[0] - errorPrevious[0]) / 0.001; // 미분항입니다.

		target_torque[0] = int(PControl[0] + IControl[0] + DControl[0]); // P, I, D 결과값을 더해줍니다.
		errorPrevious[0] = error[0]; //이전 에러값를 기억합니다.
		// rt_printf("\n\e[32;1m\t ID: %d Error: %d ActualPos: %d, target_torque %d \e[0m", 0, error[0], ActualPos[0], target_torque[0]);
		// rt_printf("\n\e[32;1m\t ID: %d PControl %d / IControl: %d,  DControl: %d" , 1, PControl[0], IControl[0], DControl[0]);

		TargetTor[0] = target_torque[0];

		error[1] =  target_Current[1] - fake_real_position[1];
		PControl[1] = Kp[1] * error[1]; //비례항 입니다.
		IControl[1] += Ki[1] * error[1] * 0.001; // 적분항 입니다.
		DControl[1] = Kd[1] * (error[1] - errorPrevious[1]) / 0.001; // 미분항입니다.

		target_torque[1] = int(PControl[1] + IControl[1] + DControl[1]); // P, I, D 결과값을 더해줍니다.
		errorPrevious[1] = error[1]; //이전 에러값를 기억합니다.
		// rt_printf("\n\e[32;1m\t ID: %d Error: %d ActualPos: %d, target_torque %d \e[0m", 0, error[1], ActualPos[1], target_torque[1]);
		// rt_printf("\n\e[32;1m\t ID: %d PControl %d / IControl: %d,  DControl: %d" , 1, PControl[1], IControl[1], DControl[1]);

		TargetTor[1] = target_torque[1];

	}

	if(actuator_index == 0){

		if(target_Current[actuator_index] > (int16_t)max_Position)
		{
			target_Current[actuator_index] = (int16_t)max_Position;
		}
		else if(target_Current[actuator_index]< 0)
		{
			target_Current[actuator_index] = 0;
		}
		// else if(ActualPos[actuator_index] < 0){
		// 	TargetTor[actuator_index] = 0;
		// }

		error[actuator_index] =  target_Current[actuator_index] - fake_real_position[actuator_index];
		PControl[actuator_index] = Kp[actuator_index] * error[actuator_index]; //비례항 입니다.
		IControl[actuator_index] += Ki[actuator_index] * error[actuator_index] * 0.001; // 적분항 입니다.
		DControl[actuator_index] = Kd[actuator_index] * (error[actuator_index] - errorPrevious[actuator_index]) / 0.001; // 미분항입니다.

		target_torque[actuator_index] = PControl[actuator_index] + IControl[actuator_index] + DControl[actuator_index]; // P, I, D 결과값을 더해줍니다.
  		errorPrevious[actuator_index] = error[actuator_index]; //이전 에러값를 기억합니다.
		// rt_printf("\n\e[32;1m\t ActualPos: %d, PControl %f / IControl: %f,  DControl: %f  target_torque %f \e[0m",  ActualPos[actuator_index], PControl, IControl, DControl,  target_torque);
		// rt_printf("\n\e[32;1m\t error: %d, errorPre %d / ActualPos: %d \e[0m",  error[actuator_index], errorPrevious[actuator_index], ActualPos[actuator_index]);
		TargetTor[actuator_index] = target_torque[actuator_index];
		
		// ethercatMaster.writeBuffer(TARGET_TORQUE, TargetTor);   //(const int EntryID, void* data)
	}
	if(actuator_index == 1){
		TargetTor[0] = 0;
	// long double Kp[] = {0.1, 7.2, }; 속도
	// long double Ki[] = {0.002, 0.5, };
	// long double Kd[] = {0.009, 0.6, };
		if(target_Current[actuator_index] > (int16_t)max_Position)
		{
			target_Current[actuator_index] = (int16_t)max_Position;
		}
		else if(target_Current[actuator_index] < 0)
		{
			target_Current[actuator_index] = 0;
		}
		// else if(ActualPos[1] < 0){
		// 	TargetTor[1] = 0;
		// }

		error[actuator_index] =  target_Current[actuator_index] - fake_real_position[actuator_index];
		PControl[actuator_index] = Kp[actuator_index] * error[actuator_index]; //비례항 입니다.
		IControl[actuator_index] += Ki[actuator_index] * error[actuator_index] * 0.001; // 적분항 입니다.
		DControl[actuator_index] = Kd[actuator_index] * (error[actuator_index] - errorPrevious[actuator_index]) / 0.001; // 미분항입니다.

		target_torque[actuator_index] = PControl[actuator_index] + IControl[actuator_index] + DControl[actuator_index]; // P, I, D 결과값을 더해줍니다.
  		errorPrevious[actuator_index] = error[actuator_index]; //이전 에러값를 기억합니다.
		// rt_printf("\n\e[32;1m\t ActualPos: %d, PControl %f / IControl: %f,  DControl: %f  target_torque %f \e[0m",  ActualPos[actuator_index], PControl, IControl, DControl,  target_torque);
		// rt_printf("\n\e[32;1m\t error: %d, errorPre %f / IControl: %f,  DControl: %f  target_torque %f \e[0m",  ActualPos[actuator_index], PControl, IControl, DControl,  target_torque);
		TargetTor[actuator_index] = target_torque[actuator_index];

		// ethercatMaster.writeBuffer(TARGET_TORQUE, TargetTor);   //(const int EntryID, void* data)
	}
	if(actuator_index == 7){

		float Kp4I[] = {0.1, 0.1, };
		float Ki4I[] = {0.002, 0.002, };
		float Kd4I[] = {0.015, 0.015, };

		float PControl4I[] = {0,0, };
		float IControl4I[] = {0,0, };
		float DControl4I[] = {0,0, }; // 비례, 적분, 미분 제어항의 결과 값입니다.

		if (AnalogInput1[1]< 150){
			target_Current4I[1] = -1500;  //-1930
		}
		else if(AnalogInput1[1]> 150){
			target_Current4I[1] = 0;
		}


		if (AnalogInput1[0]< 150){
			target_Current4I[0] = -1495;  //-1930
		}
		else if (AnalogInput1[0] > 150){
			target_Current4I[0] = 0;  //-1930
		}

		// if(AnalogInput1[0]< 150 && AnalogInput1[1]< 150){
		error4I[0] =  target_Current4I[0] - ActualVel[0];
		PControl4I[0] = Kp4I[0] * error4I[0]; //비례항 입니다.
		IControl4I[0] += Ki4I[0] * error4I[0] * 0.001; // 적분항 입니다.
		DControl4I[0] = Kd4I[0] * (error4I[0] - errorPrevious4I[0]) / 0.001; // 미분항입니다.

		target_torque4I[0] = int(PControl4I[0] + IControl4I[0] + DControl4I[0]); // P, I, D 결과값을 더해줍니다.
		errorPrevious4I[0] = error4I[0]; //이전 에러값를 기억합니다.
		TargetTor[0] = target_torque4I[0];

		error4I[1] =  target_Current4I[1] - ActualVel[1];
		PControl4I[1] = Kp4I[1] * error4I[1]; //비례항 입니다.
		IControl4I[1] += Ki4I[1] * error4I[1] * 0.001; // 적분항 입니다.
		DControl4I[1] = Kd4I[1] * (error4I[1] - errorPrevious4I[0]) / 0.001; // 미분항입니다.

		target_torque4I[1] = int(PControl4I[1] + IControl4I[1] + DControl4I[1]); // P, I, D 결과값을 더해줍니다.
		errorPrevious4I[1] = error4I[1]; //이전 에러값를 기억합니다.
		TargetTor[1] = target_torque4I[1];
		// }
		// else if(AnalogInput1[0]> 150 && AnalogInput1[1] < 150){
		// 	TargetTor[0] = 0;
		// 	target_Current[0] = 0;
		// 	target_Current4I[0] = 0;
		// 	error4I[1] =  target_Current4I[1] - ActualVel[1];
		// 	PControl4I[1] = Kp4I[1] * error4I[1]; //비례항 입니다.
		// 	IControl4I[1] += Ki4I[1] * error4I[1] * 0.001; // 적분항 입니다.
		// 	DControl4I[1] = Kd4I[1] * (error4I[1] - errorPrevious4I[0]) / 0.001; // 미분항입니다.
		// 	target_torque4I[1] = int(PControl4I[1] + IControl4I[1] + DControl4I[1]); // P, I, D 결과값을 더해줍니다.
		// 	errorPrevious4I[1] = error4I[1]; //이전 에러값를 기억합니다.
		// 	TargetTor[1] = target_torque4I[1];
		// 	initializing_constant[0] = ActualPos[0];
		// 	fake_real_position[0] = ActualPos[0] - initializing_constant[0]; 
		// 	// rt_printf("\n\e[32;1m\t initializing_constantI %d / ActualTorI: %d,  fake_real_positionI: %d" , initializing_constant, ActualTor[0], fake_real_position);
		// }
		// else if(AnalogInput1[0]< 150 && AnalogInput1[1] > 150){
		// 	TargetTor[1] = 0;
		// 	target_Current[1] = 0;
		// 	target_Current4I[1] = 0;
		// 	error4I[0] =  target_Current4I[0] - ActualVel[0];
		// 	PControl4I[0] = Kp4I[0] * error4I[0]; //비례항 입니다.
		// 	IControl4I[0] += Ki4I[0] * error4I[0] * 0.001; // 적분항 입니다.
		// 	DControl4I[0] = Kd4I[0] * (error4I[0] - errorPrevious4I[0]) / 0.001; // 미분항입니다.
		// 	target_torque4I[0] = int(PControl4I[0] + IControl4I[0] + DControl4I[0]); // P, I, D 결과값을 더해줍니다.
		// 	errorPrevious4I[0] = error4I[0]; //이전 에러값를 기억합니다.
		// 	TargetTor[0] = target_torque4I[0];
		// 	initializing_constant[1] = ActualPos[1];
		// 	fake_real_position[1] = ActualPos[1] - initializing_constant[1]; 
		// 	// rt_printf("\n\e[32;1m\t initializing_constantI %d / ActualTorI: %d,  fake_real_positionI: %d" , initializing_constant, ActualTor[0], fake_real_position);
		// }
		
		if(AnalogInput1[0]> 150 && AnalogInput1[1]> 150){
			TargetTor[0] = 0;
			target_Current[0] = 0;
			target_Current4I[0] = 0;

			initializing_constant[0] = ActualPos[0];
			fake_real_position[0] = ActualPos[0] - initializing_constant[0];

			TargetTor[1] = 0;
			target_Current[1] = 0;
			target_Current4I[1] = 0;

			initializing_constant[1] = ActualPos[1];
			fake_real_position[1] = ActualPos[1] - initializing_constant[1]; 

			initializing_motor = 0;		
		}
		// rt_printf("\n\e[32;1m\t ID: %d PControl %Lf / IControl: %Lf,  DControl: %Lf" , 0, PControl4I[0], IControl4I[0], DControl4I[0]);
		// ethercatMaster.writeBuffer(TARGET_TORQUE, TargetTor);   //(const int EntryID, void* data)
	}
}

void ecatcheck( void *ptr )
{
    int slave;

    while(1)
    {
        if( inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            if (needlf)
            {
               needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                    printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                    ec_slave[slave].state = EC_STATE_OPERATIONAL;
                    ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > 0)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (!ec_slave[slave].state)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(!ec_slave[slave].state)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[currentgroup].docheckstate)
               printf(".");
        }
        usleep(250);
    }
}

int main(int argc, char **argv)
{

	ros::init(argc, argv, "listener");
	RosNodeClass RosNode;

	int iret1;
    printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");


	rt_task_create(&ecat_ch, "ecat_check_task", 0, 99, 0);
	rt_task_start(&ecat_ch, &ecatcheck, NULL);


	int ret;

    /* Perform auto-init of rt_print buffers if the task doesn't do so */
    rt_print_auto_init(1);

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    mlockall(MCL_CURRENT | MCL_FUTURE);

	inOP = ethercatMaster.init(TORQUE_MODE, "rteth0");
	
	if(inOP == FALSE)
	{
		printf("System Initialization Failed\n");
        return 0;
	}

    ret = rt_task_create(&my_task, "my_task", 0, 98, 0);
	rt_task_create(&print_task, "printing", 0, 80, 0);
	
    printf("Starting my_task...\n");
    ret = rt_task_start(&my_task, &my_task_proc, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to start task: %s\n", strerror(-ret));
        return -1;
    }
	rt_task_start(&print_task, &print_run, NULL);

	while (run) {
		sched_yield();
		
	}
			
    printf("Deleting realtime task...\n");

    rt_task_delete(&my_task);
	rt_task_delete(&print_task);
	rt_task_delete(&ecat_ch);
	run = 0;
    printf("End of Program\n");

    return 0;
}
