#include <ros/ros.h>
#include <ros/package.h>
#include <std_msgs/String.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include <alice_foot_step_generator/FootStepCommand.h>
#include <diagnostic_msgs/KeyValue.h>
using namespace std;

class Log_test
{
public:
  Log_test();
  void Write_Log(void);
  int init_hour, init_min, init_sec;
  clock_t start_time;
  string init_log_path;
  ofstream out;

  alice_foot_step_generator::FootStepCommand walking_command;
  string current_status, prev_status;
  int command_diff, status_diff;
  string accept_or_ignore;
  float Log_Period;

private:
  void Make_Log(void);
  void WalkingCommandCallback(const alice_foot_step_generator::FootStepCommand::ConstPtr& msg);
  void CurrentStatusCallback(const std_msgs::String::ConstPtr& msg);
  void Input_Text(void);
  ros::NodeHandle nh_;

  ros::Subscriber walking_command_sub;
  ros::Subscriber curr_status_sub;
};

Log_test::Log_test()
{
  //Default_Setting//
  Input_Text();
  Make_Log();
  walking_command.command = "";
  walking_command.step_num = 0;
  walking_command.step_length = 0;
  walking_command.side_step_length = 0;
  walking_command.step_angle_rad = 0;
  walking_command.step_time = 0;
  walking_command_sub = nh_.subscribe<alice_foot_step_generator::FootStepCommand>
  ("/heroehs/alice_foot_step_generator/walking_command",10,&Log_test::WalkingCommandCallback, this);

  current_status = "";
  curr_status_sub = nh_.subscribe<std_msgs::String>("/heroehs/log_moving_status", 10, &Log_test::CurrentStatusCallback, this);
  start_time = clock();

  command_diff = 0;
  status_diff = 0;
  //////////////////

  ROS_INFO("Data logging_start");
  //ROS_INFO("%f", (float)CLOCKS_PER_SEC);
  ROS_INFO("Log_Period : %f",Log_Period);
}

void Log_test::WalkingCommandCallback(const alice_foot_step_generator::FootStepCommand::ConstPtr& msg)
{
	if(walking_command.command == msg->command)command_diff = 1;
	else command_diff = 0;
	walking_command.command = msg->command;
	walking_command.step_num = msg->step_num;
	walking_command.step_length = msg->step_length;
	walking_command.side_step_length = msg->side_step_length;
	walking_command.step_angle_rad = msg->step_angle_rad;
	walking_command.step_time = msg->step_time;
	prev_status = current_status;
}

void Log_test::CurrentStatusCallback(const std_msgs::String::ConstPtr& msg)
{
	current_status = msg->data;
}

void Log_test::Input_Text(void)
{
  int i = 0;
  size_t found;
  string init_pose_path;
  ifstream inFile;
  init_pose_path = ros::package::getPath("alice_log") + "/log_input.txt";
  inFile.open(init_pose_path.c_str());
  for(string line; std::getline(inFile,line);)
  {
      found=line.find("=");

      switch(i)
      {
      case 0: Log_Period = atof(line.substr(found+2).c_str()); break;
      }
      i +=1;
  }
  inFile.close();
}

void Log_test::Make_Log(void)
{
    time_t curr_time;
    struct tm *curr_tm;
    int year, month, day;
    curr_time = time(NULL);
    curr_tm = localtime(&curr_time);
    year = curr_tm->tm_year + 1900;
    month = curr_tm->tm_mon + 1;
    day = curr_tm->tm_mday;
    init_hour = curr_tm->tm_hour;
    init_min = curr_tm->tm_min;
    init_sec = curr_tm->tm_sec;
    char Logname[256];
    sprintf(Logname,"%d-%d-%d-%d-%d-%d",year,month,day,init_hour,init_min,init_sec);
    init_log_path = ros::package::getPath("alice_log") + "/log/" + Logname + ".txt";
    out.open(init_log_path.c_str());
    out<<"command|";
    out<<"status|";
    out<<"accept/ignore|";
    out<<"step_time|";
    out<<"step_num|";
    out<<"step_length|";
    out<<"side_step_length|";
    out<<"step_angle_rad|";
    out<<"logtime|"<<'\n';
}

void Log_test::Write_Log(void)
{
    clock_t curr_t;
    curr_t = clock();
    float result_time;
    result_time = (float)(curr_t-start_time)/(CLOCKS_PER_SEC);
    time_t curr_time;
    struct tm *curr_tm;
    int year, month, day, hour, min, sec;
    curr_time = time(NULL);
    curr_tm = localtime(&curr_time);
    year = curr_tm->tm_year + 1900;
    month = curr_tm->tm_mon + 1;
    day = curr_tm->tm_mday;
    hour = curr_tm->tm_hour - init_hour;
    min = curr_tm->tm_min - init_min;
    sec = curr_tm->tm_sec - init_sec;
    if(sec < 0) 
    {
      sec = sec+60;
      min = min - 1;
    }
    if(min < 0)
    {
      min = min+60;
      hour = hour - 1;
    }
    if(hour < 0)
    {
      hour = hour+24;
    }
    if(walking_command.command == current_status)accept_or_ignore = "accept";
    else accept_or_ignore = "ignore";
    char Logname[256];
    sprintf(Logname,"%d:%d:%d",hour,min,sec);
    out<<walking_command.command<<"|";
    out<<current_status<<"|";
    out<<accept_or_ignore<<"|";
    out<<walking_command.step_time<<"|";
    out<<walking_command.step_num<<"|";
    out<<walking_command.step_length<<"|";
    out<<walking_command.side_step_length<<"|";
    out<<walking_command.step_angle_rad<<"|";
    out<<Logname<<"|"<<'\n';
}


int main(int argc, char** argv)
{
  ros::init(argc, argv, "alice_log");
  Log_test log_test;
  float count;
  count = 0;
  while(ros::ok())
  {
	if(count > 1000*log_test.Log_Period)
	{
		if(log_test.command_diff == 1 && log_test.status_diff == 1);
		else log_test.Write_Log();
		//ROS_INFO("%d and %d", log_test.command_diff, log_test.status_diff);
		count = 0;
	}
	else count += 1;
    usleep(1000);
    ros::spinOnce();
  }
  log_test.out.close();
}


