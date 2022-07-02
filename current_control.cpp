#include <iostream>
#include <cctype>

#include <termios.h>
#include <unistd.h>

#include <ros/ros.h>
#include "std_msgs/Int16.h"
#include "std_msgs/Int32.h"

#include "Elmo_EtherCAT.h"

int main(int argc, char **argv)
{
    ros::init(argc, argv, "current_control"); // Initializes Node Name

    ros::NodeHandle nh;

    ros::Publisher motor_num_current_pub = nh.advertise<std_msgs::Int32>("motor_num_current", 100);  // = nh 라는 instance에서 <std_msgs> 데이터 구조로 motor_num_current 라는 토픽 명으로 발행하겠다.
    ros::Publisher target_Current_pub = nh.advertise<std_msgs::Int16>("target_Current", 100);
    ros::Publisher initializing_pub = nh.advertise<std_msgs::Int16>("initializing", 100);
    ros::Publisher init_stop_pub = nh.advertise<std_msgs::Int16>("init_stop", 100);

    ros::Rate loop_rate(10);
    
    std_msgs::Int16 initializing_msg;
    std_msgs::Int16 init_stop_msg;
    std_msgs::Int32 motor_num_current_msg;
    std_msgs::Int16 target_Current_msg;

    int16_t input_initializing_motor;
    int16_t input_init_stop;
    int32_t input_motor_num_current;
    int16_t input_target_Current;

    while (ros::ok())
    {
        std::cout << "to initialize : 7 / to operating mode: 1" << std::endl;
        std::cin >> input_initializing_motor;
        if(!std::cin)
        {
            std::cout << "Wrong Input, try again." << std::endl;
            std::cin.clear();
            std::cin.ignore(INT_MAX, '\n');
            continue;
        }
        else if(input_initializing_motor == -1)
        {
            // motor_num_current_msg.data = (int32_t)NUM_ELMO_DRIVER;
            // motor_num_current_pub.publish(motor_num_current_msg);
            // target_Current_msg.data = (int16_t)0;
            // target_Current_pub.publish(target_Current_msg);

            printf("Quit Program...\n");
            break;
        }
        else if(input_initializing_motor == 1)
        {
            printf("go to operating mode...\n");
        }
        else if(input_initializing_motor == 7)
        {
            printf("initalizing mode\n");
            initializing_msg.data = input_initializing_motor;
            initializing_pub.publish(initializing_msg);
            continue;
            // std::cout << "enter [1] to stop" << std::endl;
            // std::cin >> input_init_stop;

            // if(!std::cin){
            //     std::cout << "Wrong Input, try again." << std::endl;
            //     std::cin.clear();
            //     std::cin.ignore(INT_MAX, '\n');
            //     continue;
            // }
            // else if(input_init_stop == 1){
            //     init_stop_msg.data = (int16_t)1;
            //     std::cout << "initailizing will be stop" << std::endl;
            //     init_stop_pub.publish(init_stop_msg);
            //     continue;
            // }
            // else if(input_init_stop == -1)
            // {
            // motor_num_current_msg.data = (int32_t)NUM_ELMO_DRIVER;
            // motor_num_current_pub.publish(motor_num_current_msg);
            // target_Current_msg.data = (int16_t)0;
            // target_Current_pub.publish(target_Current_msg);

            // printf("Quit Program...\n");
            // break;
            // }
        }
        
        std::cout << "Select Motor(Motornum: (num), All:" << NUM_ELMO_DRIVER << ", quit: -1) (to initialize : 7)" << std::endl;
        std::cin >> input_motor_num_current;
        if(!std::cin)
        {
            std::cout << "Wrong Input, try again." << std::endl;
            std::cin.clear();
            std::cin.ignore(INT_MAX, '\n');
            continue;
        }
        else if(input_motor_num_current == -1)
        {
            motor_num_current_msg.data = (int32_t)NUM_ELMO_DRIVER;
            motor_num_current_pub.publish(motor_num_current_msg);
            target_Current_msg.data = (int16_t)0;
            target_Current_pub.publish(target_Current_msg);

            printf("Quit Program...\n");
            break;
        }
        else if(input_motor_num_current >-1 && input_motor_num_current <= NUM_ELMO_DRIVER )
        {
            printf("Published Motor Number\n");
	        motor_num_current_msg.data = input_motor_num_current;
            motor_num_current_pub.publish(motor_num_current_msg);
        }
        else
        {
            printf("Wrong Input, try again.");
            continue;
        }

        std::cout << "Input target Current(-2700<I<2700)" << std::endl;
        std::cin >> input_target_Current;
        if(!std::cin)
        {
            std::cout << "Wrong Input, try again." << std::endl;
            std::cin.clear();
            std::cin.ignore(INT_MAX, '\n');
            continue;
        }
        else if(input_target_Current == -1)
        {
            motor_num_current_msg.data = (int32_t)NUM_ELMO_DRIVER;
            motor_num_current_pub.publish(motor_num_current_msg);
            target_Current_msg.data = (int16_t)0;
            target_Current_pub.publish(target_Current_msg);
            printf("Quit Program...\n");
            break;
        }
        else
        {
            printf("Published Target Current\n");
            target_Current_msg.data = input_target_Current;
            target_Current_pub.publish(target_Current_msg);
        }
        ros::spinOnce();
    }

    return 0;
}
