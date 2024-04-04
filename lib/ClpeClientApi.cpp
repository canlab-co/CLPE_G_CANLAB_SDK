/**********************************************************************
  This is the ClpeClientApi Class Function of CanLab CLPE Client API. 
**********************************************************************/

#include "ClpeClientApi.h"
#include "ClpeSDKVersion.h"
#include "ClpeGlobalDef.h"
#include <string.h>

#define MAX_CAMERA_CNT_PER_BOARD	(4)

/********************************************************
	  * ClpeClientApi Class Constructor
*********************************************************/
ClpeClientApi::ClpeClientApi()
{

}

/********************************************************
	  * ClpeClientApi Class Disconstructor
*********************************************************/
ClpeClientApi::~ClpeClientApi()
{
	ClpeSocket::close(MCU_ID_MASTER);
	if(m_isAttachedSlave == 1)
	{
		ClpeSocket::close(MCU_ID_SLAVE);
	}
}

/********************************************************
		  * Clpe_Send
		  - Send data to Xavier Server
*********************************************************/
bool ClpeClientApi::Clpe_Send(unsigned char *s, int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	if(ClpeSocket::send(s, mcu_id))
	{
		return true;	
	}
	else
	{
		return false;
	}
}

/********************************************************
		  * Clpe_Recv
		  - Receive data from Xavier Server
*********************************************************/
bool ClpeClientApi::Clpe_Recv(unsigned char *s, int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	if(ClpeSocket::recv(s, mcu_id))
	{
		return true;
	}
	else
	{
		return false;
	}
}

/********************************************************
  * Clpe_CheckConnect
  - Check network connection between PC and Xavier 
*********************************************************/
int ClpeClientApi::Clpe_CheckConnect(string password, int settingValue)
{
	int ret = 0;
	int cnt = 0;
	int errEthUp = 0;
	int errEthSet = 0;
	int errPing = 0;
	string ethDev;
	string ethDevOther;
	string pcIpAddress;
	string xavierNxIpAddress;
	string cmdEthDown;
	string cmdEthDownOther;
	string cmdEthUp;
	string cmdEthSet;
	string cmdPing;

	if(settingValue == 0)
	{
		ethDev = "eth0";
		ethDevOther = "eth1";
		pcIpAddress = " 192.168.7.8";
		xavierNxIpAddress = "192.168.7.7";	
	}
	else if(settingValue == 1)
	{
		ethDev = "eth1";
		ethDevOther = "eth0";
		pcIpAddress = " 192.168.7.8";
		xavierNxIpAddress = "192.168.7.7";	
	}
	else if(settingValue == 2)
	{
		ethDev = "eth1";
		pcIpAddress = " 192.168.8.8";
		xavierNxIpAddress = "192.168.8.7";	
	}
	else
	{
		ethDev = "eth0";
		pcIpAddress = " 192.168.8.8";
		xavierNxIpAddress = "192.168.8.7";	
	}

	cmdEthDown = "echo '" + password + "' | sudo -kS nmcli dev disconnect " + ethDev + " > /dev/null 2>&1";	
	cmdEthDownOther = "echo '" + password + "' | sudo -kS nmcli dev disconnect " + ethDevOther + " > /dev/null 2>&1";	
	cmdEthUp = "echo '" + password + "' | sudo -kS ifconfig " + ethDev + " up > /dev/null 2>&1";
	cmdEthSet = "echo '" + password + "' | sudo -kS ifconfig " + ethDev + pcIpAddress + " > /dev/null 2>&1";
	cmdPing = "ping -c 1 " + xavierNxIpAddress + " -W 1 > /dev/null 2>&1";

	/** ethernet device disconnect **/
	if(settingValue == 0 || settingValue == 1)
	{
		ret = system(cmdEthDown.c_str());
		ret = system(cmdEthDownOther.c_str());
	}
	else
	{
		ret = system(cmdEthDown.c_str());
	}
	
	while(1)
	{
		/** ethernet device connect **/
		ret = system(cmdEthUp.c_str());		

		if(ret == 0)
		{
			errEthUp = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		
		if(cnt > 3)
		{
			errEthUp = 1;
			cnt = 0;
			break;
		}

		usleep(500000);
	}

	if(errEthUp)
	{
		return ERROR_CONNECT_NETWORK;
	}

	while(1)
	{
		/** ethernet device set **/
		ret = system(cmdEthSet.c_str());		

		if(ret == 0)
		{
			errEthSet = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		
		if(cnt > 3)
		{
			errEthSet = 1;
			cnt = 0;
			break;
		}
		
		usleep(500000);
	}

	if(errEthSet)
	{
		return ERROR_CONNECT_ADDRESS;
	}

	while(1)
	{
		/** ping **/
		ret = system(cmdPing.c_str());		

		if(ret == 0)
		{
			errPing = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		
		if(cnt > 3)
		{
			errPing = 1;
			cnt = 0;
			break;
		}

		sleep(1);
	}

	if(errPing)
	{
		return ERROR_CONNECT_PING;
	}

	usleep(500000);
	
	return SUCCESSED;
}

/********************************************************
  * Clpe_Initial
  - Initial network connection between PC and Xavier 
*********************************************************/
int ClpeClientApi::Clpe_Connection(string password, int isAttachedSlave)
{
	int ret = 0;
	int cnt = 0;
	int errConnect = 0;
	int masterCheckedPosition = 0;
	//int isAttachedSlave = 0;

	string cmdModule = "echo '" + password + "' | sudo -kS modprobe tegra_vnet > /dev/null 2>&1";

	string cmdNet1 = "echo '" + password + "' | sudo -kS sysctl -w net.ipv4.ip_forward=1 > /dev/null 2>&1";
	string cmdNet2 = "echo '" + password + "' | sudo -kS sysctl -w net.core.rmem_default=90000000 > /dev/null 2>&1";
	string cmdNet3 = "echo '" + password + "' | sudo -kS sysctl -w net.core.rmem_max=90000000 > /dev/null 2>&1";
	string cmdNet4 = "echo '" + password + "' | sudo -kS sysctl -w net.ipv4.udp_rmem_min=10000000 > /dev/null 2>&1";
	string cmdNet5 = "echo '" + password + "' | sudo -kS sysctl -w net.ipv4.udp_mem='90000000 90000000 90000000' > /dev/null 2>&1";

	//isAttachedSlave = 0; // only support 1 clpe.
	m_isAttachedSlave = isAttachedSlave;
	/** tegra_vnet module probe **/
	ret = system(cmdModule.c_str());
	
	if(ret != 0)
	{
		return ERROR_CONNECT_DRIVER;
	}
	
	usleep(500000);

	/* Check the connection for master */
	masterCheckedPosition = 0;
	ret = Clpe_CheckConnect(password, 0);
	if(ret != SUCCESSED)
	{
		masterCheckedPosition = 1;
		ret = Clpe_CheckConnect(password, 1);
		if(ret != SUCCESSED)
		{
			return ret;
		}
	}

	if(isAttachedSlave == 1)
	{
		/* Check the connection for slave */
		if(masterCheckedPosition == 0)
		{
			ret = Clpe_CheckConnect(password, 2);
			if(ret != SUCCESSED)
			{
				return ret;	
			}
		}
		else
		{
			ret = Clpe_CheckConnect(password, 3);
			if(ret != SUCCESSED)
			{
				return ret;
			}
		}
	}

	if(!ClpeSocket::create(MCU_ID_MASTER))
	{
		return ERROR_CONNECT_CREATE;
	}

	if(isAttachedSlave == 1)
	{
		if(!ClpeSocket::create(MCU_ID_SLAVE))
		{
			return ERROR_CONNECT_CREATE;
		}
	}

	while(1)
	{
		/** socket connect **/
		if(!ClpeSocket::connect(HOST_MASTER, PORT, MCU_ID_MASTER))
		{
			cnt++;
		}
		else
		{
			errConnect = 0;
			cnt = 0;
			break;
		}
		
		if(cnt > 3)
		{
			errConnect = 1;
			cnt = 0;
			break;
		}

		sleep(1);
	}

	if(errConnect)
	{
		return ERROR_CONNECT_CONNECT;
	}

	if(isAttachedSlave == 1)
	{
		while(1)
		{
			/** socket connect **/
			if(!ClpeSocket::connect(HOST_SLAVE, PORT, MCU_ID_SLAVE))
			{
				cnt++;
			}
			else
			{
				errConnect = 0;
				cnt = 0;
				break;
			}
			
			if(cnt > 3)
			{
				errConnect = 1;
				cnt = 0;
				break;
			}

			sleep(1);
		}

		if(errConnect)
		{
			return ERROR_CONNECT_CONNECT;
		}
	}

	/** network memory setting **/
	system(cmdNet1.c_str());
	system(cmdNet2.c_str());
	system(cmdNet3.c_str());
	system(cmdNet4.c_str());
	system(cmdNet5.c_str());

	return SUCCESSED;
}

int ClpeClientApi::Clpe_Connection(int isAttachedSlave)
{
	int ret = 0;
	int cnt = 0;
	int errConnect = 0;
	int masterCheckedPosition = 0;

	m_isAttachedSlave = isAttachedSlave;

	cnt = 0;
	int errPing = 0;

	string cmdPing;
	string xavierNxIpAddress;

	xavierNxIpAddress = "192.168.7.7";
	cmdPing = "ping -c 1 " + xavierNxIpAddress + " -W 1 > /dev/null 2>&1";

	while (1)
	{
		/** ping **/
		ret = system(cmdPing.c_str());
		if (ret == 0)
		{
			errPing = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		if (cnt > 3)
		{
			errPing = 1;
			cnt = 0;
			break;
		}
		sleep(1);
	}
	if (errPing)
	{
		return ERROR_CONNECT_PING;
	}

	xavierNxIpAddress = "192.168.8.7";
	cmdPing = "ping -c 1 " + xavierNxIpAddress + " -W 1 > /dev/null 2>&1";

	while (1)
	{
		/** ping **/
		ret = system(cmdPing.c_str());
		if (ret == 0)
		{
			errPing = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		if (cnt > 3)
		{
			errPing = 1;
			cnt = 0;
			break;
		}
		sleep(1);
	}
	if (errPing)
	{
		return ERROR_CONNECT_PING;
	}

	if (!ClpeSocket::create(MCU_ID_MASTER))
	{
		return ERROR_CONNECT_CREATE;
	}
	if (isAttachedSlave == 1)
	{
		if (!ClpeSocket::create(MCU_ID_SLAVE))
		{
			return ERROR_CONNECT_CREATE;
		}
	}

	cnt = 0;
	while (1)
	{
		/** socket connect **/
		if (!ClpeSocket::connect(HOST_MASTER, PORT, MCU_ID_MASTER))
		{
			cnt++;
		}
		else
		{
			errConnect = 0;
			cnt = 0;
			break;
		}
		if (cnt > 3)
		{
			errConnect = 1;
			cnt = 0;
			break;
		}
		sleep(1);
	}
	if (errConnect)
	{
		return ERROR_CONNECT_CONNECT;
	}
	if (isAttachedSlave == 1)
	{
		while (1)
		{
			/** socket connect **/
			if (!ClpeSocket::connect(HOST_SLAVE, PORT, MCU_ID_SLAVE))
			{
				cnt++;
			}
			else
			{
				errConnect = 0;
				cnt = 0;
				break;
			}
			if (cnt > 3)
			{
				errConnect = 1;
				cnt = 0;
				break;
			}
			sleep(1);
		}
		if (errConnect)
		{
			return ERROR_CONNECT_CONNECT;
		}
	}
	return SUCCESSED;
}

/********************************************************
* Clpe_CheckChrony
- Check the chrony time sync between PC and Xavier
*********************************************************/
int ClpeClientApi::Clpe_CheckTimeSyncStatus()
{
	int ret = 0;
	int cnt = 0;
	int errChrony = 0;

	while(1)
	{
		ret = Clpe_TimeSync();		
		if(ret == 0)
		{
			errChrony = 0;
			cnt = 0;
			break;
		}
		else
		{
			cnt++;
		}
		
		if(cnt > 4)
		{
			errChrony = 1;
			cnt = 0;
			break;
		}

		sleep(1);
	}

	if(errChrony)
	{
		return ERROR_CHECK_CHRONY;
	}

	return SUCCESSED;	
}

/********************************************************
* Clpe_ReqResyncTime
- Check the pci device connection between PC and Xavier
*********************************************************/
int ClpeClientApi::Clpe_ReqResyncTime()
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;
	int sendCmdCnt = 0;

	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_REQ_RESYNC_TIME;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				if(socketRx[4] == 1)
				{
					returnVal = SUCCESSED;
				}
				else
				{
					returnVal = ERROR_RESYNC_TIME;
					break;
				}
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
			if(socketRx[3] == 4)
			{
				returnVal = ERROR_GETSET_CHECKSUM;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
		
	}
	
	free(socketTx);
	free(socketRx);

	return returnVal;	
}

/********************************************************
* Clpe_CheckPci
- Check the pci device connection between PC and Xavier
*********************************************************/
int ClpeClientApi::Clpe_CheckPci()
{
	int ret = 0;

	ret = system("lspci -v | grep 'Subsystem' | grep 'NVIDIA Corporation Device 0000' > /dev/null 2>&1");

	if(ret != 0)
	{
		return ERROR_CHECK_CONNECT;
	}

	return SUCCESSED;
}

/********************************************************
		* Clpe_CheckEthernet
		- Check the ethernet device is up.
*********************************************************/
int ClpeClientApi::Clpe_CheckNetwork()
{
	int ret = 0;

	ret = system("ifconfig | grep eth0 | grep 'mtu 64512' > /dev/null 2>&1");

	if(ret != 0)
	{
		return ERROR_CHECK_CONNECT;
	}

	if(m_isAttachedSlave == 1)
	{
		ret = system("ifconfig | grep eth1 | grep 'mtu 64512' > /dev/null 2>&1");
		if(ret != 0)
		{
			return ERROR_CHECK_CONNECT;
		}
	}
	return SUCCESSED;
}

/********************************************************
	* Clpe_CheckPing
	- Check the ping to Xavier network address
*********************************************************/
int ClpeClientApi::Clpe_CheckPing()
{
	int ret = 0;

	ret = system("ping -c 1 192.168.7.7 -W 1 > /dev/null 2>&1");

	if(ret != 0)
	{
		return ERROR_CHECK_CONNECT;
	}

	if(m_isAttachedSlave == 1)
	{
		ret = system("ping -c 1 192.168.8.7 -W 1 > /dev/null 2>&1");
		if(ret != 0)
		{
			return ERROR_CHECK_CONNECT;
		}
	}
	
	return SUCCESSED;
}

/********************************************************
		  * Clpe_TimeSync
		  - Get time sync status between PC and Xavier
*********************************************************/
int ClpeClientApi::Clpe_TimeSync()
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;
	int sendCmdCnt = 0;

	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_SET_TIME_SYNC;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				if(socketRx[4] == 1)
				{
					returnVal = SUCCESSED;
				}
				else
				{
					returnVal = ERROR_GETSET_TIMESYNC;
					break;
				}
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
			if(socketRx[3] == 4)
			{
				returnVal = ERROR_GETSET_CHECKSUM;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
		
	}
	
	free(socketTx);
	free(socketRx);

	return returnVal;	
}

/********************************************************
		  * Clpe_StartCam
		  - Start the cam you want to start
*********************************************************/
int ClpeClientApi::Clpe_StartCam(char use_cam_0, char use_cam_1, char use_cam_2, char use_cam_3, int mcu_id)
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;

	socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
	socketTx[1] = CMD_ID_REQ_START_STREAM;

	socketTx[2] = 0x00;
	socketTx[2] = (socketTx[2] << 1) | use_cam_3;
	socketTx[2] = (socketTx[2] << 1) | 	use_cam_2;
	socketTx[2] = (socketTx[2] << 1) | 	use_cam_1;
	socketTx[2] = (socketTx[2] << 1) | 	use_cam_0;

	checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
	socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
	socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

	ret = Clpe_Send(socketTx, mcu_id);
	if(!ret)
	{
		free(socketTx);
		free(socketRx);
		return ERROR_GETSET_COMMUNICATION;
	}
	ret = Clpe_Recv(socketRx, mcu_id);
	if(!ret)
	{
		free(socketTx);
		free(socketRx);
		return ERROR_GETSET_COMMUNICATION;
	}

	for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
	{
		DataSum += socketRx[3+i];
	}

	checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

	checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

	if(checkSumRx == checkSumRxData)
	{
		if(socketRx[3] == 0)
		{
			if(socketRx[4] == 1)
			{
				returnVal = SUCCESSED;
			}
			else
			{
				returnVal = ERROR_GETSET_START;
			}
		}
		if(socketRx[3] == 1)
		{
			returnVal = ERROR_GETSET_COMMAND;
		}
		if(socketRx[3] == 3)
		{
			returnVal = ERROR_GETSET_COMMUNICATION;
		}
		if(socketRx[3] == 4)
		{
			returnVal = ERROR_GETSET_CHECKSUM;
		}
	}
	else
	{
		returnVal = ERROR_GETSET_CHECKSUM;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;	
}

/********************************************************
		  * Clpe_StopCam
		  - Stop the all of cam
*********************************************************/
int ClpeClientApi::Clpe_StopCam()
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;

	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{

		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_REQ_STOP_STREAM;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				if(socketRx[4] == 1)
				{
					returnVal = SUCCESSED;
				}
				else
				{
					returnVal = ERROR_GETSET_STOP;
					break;
				}
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
			if(socketRx[3] == 4)
			{
				returnVal = ERROR_GETSET_CHECKSUM;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;	
}

/********************************************************
			  * Clpe_GetMicomVersion
			  - Get Version of Micom
*********************************************************/
int ClpeClientApi::Clpe_GetMicomVersion(unsigned char* version_master, unsigned char* version_slave)
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;
	unsigned char *version;
	
	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;
	
	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		if(cpu_id == MCU_ID_MASTER)
		{
			version = version_master;
		}
		else
		{
			version = version_slave;
		}	
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_GET_MICOM_FIRM_VERSION;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				for(int i = 0; i < 6; i++)
				{ 
					version[i] = socketRx[4+i];
				}
				returnVal = SUCCESSED;
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;
}

/********************************************************
			  * Clpe_GetXavierVersion
			  - Get Version of Xavier
*********************************************************/
int ClpeClientApi::Clpe_GetXavierVersion(unsigned char* version_master, unsigned char* version_slave)
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;
	unsigned char *version;

	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;
	
	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		if(cpu_id == MCU_ID_MASTER)
		{
			version = version_master;
		}
		else
		{
			version = version_slave;
		}
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_GET_XAVIER_VERSION;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				for(int i = 0; i < (SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL - 2); i++)
				{ 
					version[i] = socketRx[4+i];
				}
				returnVal = SUCCESSED;
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;
}

/********************************************************
			  * Clpe_GetSDKVersion
			  - Get Version of SDK
*********************************************************/
int ClpeClientApi::Clpe_GetSDKVersion(unsigned char* version)
{
	int returnVal = SUCCESSED;

	memcpy(version, sdkVersion, 6);

	return returnVal;
}

/********************************************************
			  * Clpe_GetCamStatus
			  - Get Lock Status of Camera
*********************************************************/
int ClpeClientApi::Clpe_GetCamStatus(int* status)
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);
	//unsigned char socketRx[13] = {0, };

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;
	
	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_GET_SER_LOCK_STAUS;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				for(int i = 0; i < MAX_CAMERA_CNT_PER_BOARD; i++)
				{ 
					status[i + (cpu_id * MAX_CAMERA_CNT_PER_BOARD)] = (int)socketRx[4+i];
				}
				returnVal = SUCCESSED;
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}
	free(socketTx);
	free(socketRx);

	return returnVal;
}

/********************************************************
		  * Clpe_SetXavierPowerOff
		  - Set Power Off the Xavier Server
*********************************************************/
int ClpeClientApi::Clpe_SetXavierPowerOff()
{
	unsigned char *socketTx = (unsigned char*) malloc(6);
	unsigned char *socketRx = (unsigned char*) malloc(13);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;
	
	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_REQ_POWER_OFF;
		socketTx[2] = 0x00;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				returnVal = SUCCESSED;
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;
}

/********************************************************
	  * Clpe_StartStream
	  - Start stream cam you want to stream
*********************************************************/
int ClpeClientApi::Clpe_StartStream(T_CB_APP cb_app, int use_cam_0, int use_cam_1, int use_cam_2, int use_cam_3,
	int use_cam_4, int use_cam_5, int use_cam_6, int use_cam_7, int display_on)
{
	int ret = 0;
	int returnVal = 0;

	int sendCmdCnt = 0;

	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		if(cpu_id == MCU_ID_MASTER)
		{
			ret = Clpe_StartCam((char)use_cam_0, (char)use_cam_1, (char)use_cam_2, (char)use_cam_3, cpu_id);
		}
		else
		{
			ret = Clpe_StartCam((char)use_cam_4, (char)use_cam_5, (char)use_cam_6, (char)use_cam_7, cpu_id);
		}
	}

	sleep(2);

	if(ret == 0)
	{
		returnVal = clpe_startStream(cb_app, (char)use_cam_0, (char)use_cam_1, (char)use_cam_2, (char)use_cam_3,
			(char)use_cam_4, (char)use_cam_5, (char)use_cam_6, (char)use_cam_7, display_on);
	}
	else
	{
		returnVal = ERROR_START_STREAM;
	}

	return returnVal;
}

/********************************************************
	  * Clpe_StopStream
	  - Stop stream all the cam
*********************************************************/
int ClpeClientApi::Clpe_StopStream()
{
	int ret = 0;
	int returnVal = 0;

	ret = Clpe_StopCam();

	if(ret == 0)
	{
		returnVal = clpe_stopStream();
	}
	else
	{
		returnVal = ERROR_STOP_STREAM;
	}

	return returnVal;
}

/********************************************************
	  * Clpe_GetFrameAllCam
	  - Get frame from all of streaming cam
*********************************************************/
int ClpeClientApi::Clpe_GetFrameAllCam(int *p_camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp)
{
	int returnVal = 0;

	returnVal = clpe_getFrameAnyCam(p_camera_id, p_buffer, p_size, pt_camera_timeStamp);

	return returnVal;
}

/********************************************************
	  * Clpe_GetFrameOneCam
	  - Get frame from one of streaming cam you choose
*********************************************************/
int ClpeClientApi::Clpe_GetFrameOneCam(int camera_id, unsigned char **p_buffer, unsigned int *p_size, struct timeval *pt_camera_timeStamp)
{
	int returnVal = 0;

	returnVal = clpe_getFrameWithCamId(camera_id, p_buffer, p_size, pt_camera_timeStamp);

	return returnVal;
}

/********************************************************
	  * Clpe_SelectFormat
	  - Select Stream Format
*********************************************************/
int ClpeClientApi::Clpe_SelectFormat(int format)
{
	unsigned char *socketTx = (unsigned char*) malloc(SOCKET_CMD_TX_PACKET_SIZE_MAX);
	unsigned char *socketRx = (unsigned char*) malloc(SOCKET_CMD_RX_PACKET_SIZE_MAX);

	unsigned int checkSumTx = 0;
	unsigned int checkSumRx = 0;
	unsigned int checkSumRxData = 0;
	unsigned int DataSum = 0;

	bool ret = false;

	int returnVal = 0;

	int sendCmdCnt = 0;
	
	if(m_isAttachedSlave == 1)
	{
		sendCmdCnt = 2;
	}
	else
	{
		sendCmdCnt = 1;
	}

	for(int cpu_id = 0; cpu_id < sendCmdCnt; cpu_id++)
	{
		socketTx[0] = SOCKET_PACKET_ID_PC_TO_XAVIER;
		socketTx[1] = CMD_ID_GET_XAVIER_STREAM_FORMAT;
		if(format == 0) socketTx[2] = 0x00;
		else socketTx[2] = 0x01;
		checkSumTx = socketTx[0] + socketTx[1] + socketTx[2];
		socketTx[3] = (unsigned char)((checkSumTx & 0x0000FF00) >> 8);
		socketTx[4] = (unsigned char)(checkSumTx & 0x000000FF);

		ret = Clpe_Send(socketTx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		ret = Clpe_Recv(socketRx, cpu_id);
		if(!ret)
		{
			free(socketTx);
			free(socketRx);
			return ERROR_GETSET_COMMUNICATION;
		}
		
		clpe_setFormat(format);

		for(int i = 0; i < SOCKET_CMD_RX_PAYLOAD_SIZE_NORMAL; i++)
		{
			DataSum += socketRx[3+i];
		}

		checkSumRx = socketRx[0] + socketRx[1] + socketRx[2] + DataSum;

		checkSumRxData = (unsigned int)(socketRx[11] << 8 | socketRx[12]);

		if(checkSumRx == checkSumRxData)
		{
			if(socketRx[3] == 0)
			{
				returnVal = SUCCESSED;
			}
			if(socketRx[3] == 1)
			{
				returnVal = ERROR_GETSET_COMMAND;
				break;
			}
			if(socketRx[3] == 3)
			{
				returnVal = ERROR_GETSET_COMMUNICATION;
				break;
			}
		}
		else
		{
			returnVal = ERROR_GETSET_CHECKSUM;
			break;
		}
		DataSum = 0;
	}

	free(socketTx);
	free(socketRx);

	return returnVal;		

}


