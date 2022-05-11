// Implementation of the Socket class.

#include "ClpeSocket.h"
#include "ClpeGlobalDef.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

ClpeSocket::ClpeSocket() : m_sock_master(-1), m_sock_slave(-1)
{
	memset(&m_addr, 0, sizeof(m_addr));
}

ClpeSocket::~ClpeSocket()
{
	if(is_valid_master())
	{
		::close (m_sock_master);
	}

	if(is_valid_slave())
	{
		::close (m_sock_slave);
	}
}

bool ClpeSocket::create(int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	int sock_id;
	sock_id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(mcu_id == MCU_ID_MASTER)			
	{
		m_sock_master = sock_id;
		
		if(!is_valid_master())
		{
			return false;
		}
	}
	else
	{
		m_sock_slave = sock_id;
		if(!is_valid_slave())
		{
			return false;
		}
	}
	

	// TIME_WAIT - argh
	int on = 1;
	if(setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) == -1)
	{
		return false;
	}

	// nagle off
	int nagleOff = 1;
	if(setsockopt(sock_id, IPPROTO_TCP, TCP_NODELAY, (const char*)&nagleOff, sizeof(nagleOff)) == -1)
	{
		return false;
	}

	struct timeval timeout = {20, 0};  /* timeout 20 second */

#ifdef SOCKET_CLIENT
	if(setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
	{
		return false;
	}

	if(setsockopt(sock_id, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1)
	{
		return false;
	}
#endif

#ifdef SOCKET_SERVER
	if(setsockopt(sock_id, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1)
	{
		return false;
	}
#endif

	

	return true;
}


bool ClpeSocket::bind(const char *host, const int port, int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	int sock_id;
	
	if(mcu_id == MCU_ID_MASTER)
	{
		if(!is_valid_master())
		{
			return false;
		}

		sock_id = m_sock_master;
		
	}
	else
	{
		if(!is_valid_slave())
		{
			return false;
		}

		sock_id = m_sock_slave;
	}
	
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(host);
	m_addr.sin_port = htons(port);

	int bind_return = ::bind( sock_id, (struct sockaddr *)&m_addr, sizeof(m_addr));

	if (bind_return == -1)
	{
		return false;
	}

	return true;
}


bool ClpeSocket::listen(int mcu_id) const // master (mcu_id = 0), slave (mcu_id = 1)
{
	int sock_id;

	if(mcu_id == MCU_ID_MASTER)
	{
		if (!is_valid_master())
		{
			return false;
		}
		sock_id = m_sock_master;
	}
	else
	{
		if (!is_valid_slave())
		{
			return false;
		}	
		sock_id = m_sock_slave;
	}
	
	int listen_return = ::listen(sock_id, MAXCONNECTIONS);

	if(listen_return == -1)
	{
		return false;
	}

	return true;
}


bool ClpeSocket::accept(ClpeSocket *new_socket, int mcu_id) const // master (mcu_id = 0), slave (mcu_id = 1)
{
	int addr_length = sizeof(m_addr);

	if(mcu_id == MCU_ID_MASTER)
	{
		new_socket->m_sock_master = ::accept(m_sock_master, (sockaddr *)&m_addr, (socklen_t *)&addr_length);
		if(new_socket->m_sock_master<= 0)
		{
			return false;
		}

	}
	else
	{
		new_socket->m_sock_slave = ::accept(m_sock_slave, (sockaddr *)&m_addr, (socklen_t *)&addr_length);
		if(new_socket->m_sock_master<= 0)
		{
			return false;
		}
	}

	return true;
}


bool ClpeSocket::send(unsigned char *s, int mcu_id) const // master (mcu_id = 0), slave (mcu_id = 1)
{
#ifdef SOCKET_CLIENT
	int data = SOCKET_CMD_TX_PACKET_SIZE_MAX;
#endif
#ifdef SOCKET_SERVER
	int data = SOCKET_CMD_RX_PACKET_SIZE_MAX;
#endif

	int sock_id;

	if(mcu_id == MCU_ID_MASTER)
	{
		sock_id = m_sock_master;
	}
	else
	{
		sock_id = m_sock_slave;
	}
	int status = ::send(sock_id, s, data, MSG_NOSIGNAL);
	if(status == -1)
	{
		return false;
	}
	else
	{
		return true;
	}
} 


bool ClpeSocket::recv(unsigned char *s, int mcu_id) const // master (mcu_id = 0), slave (mcu_id = 1)
{
#ifdef SOCKET_CLIENT
	int data = SOCKET_CMD_RX_PACKET_SIZE_MAX;
#endif
#ifdef SOCKET_SERVER
	int data = SOCKET_CMD_TX_PACKET_SIZE_MAX;
#endif
	unsigned char *buf = (unsigned char*) malloc(data);

	int sock_id;

	if(mcu_id == MCU_ID_MASTER)
	{
		sock_id = m_sock_master;
	}
	else
	{
		sock_id = m_sock_slave;
	}
	
	int status = ::recv(sock_id, buf, data, 0);

	if(status == -1)
	{
		return false;
	}
	else if(status == 0)
	{
		return false;
	}
	else
	{
		for(int i = 0; i < status; i++) 
		{
			s[i] = buf[i];
		}

		free(buf);

		return true;
	}
}


bool ClpeSocket::connect(const string host, const int port, int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	int sock_id;


	if(mcu_id == MCU_ID_MASTER)
	{
		if(!is_valid_master())
		{
			return false;
		}	
		sock_id = m_sock_master;
	}
	else
	{
		if(!is_valid_slave())
		{
			return false;
		}	
		sock_id = m_sock_slave;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);

	int status = inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);

	if(errno == EAFNOSUPPORT)
	{
		return false;
	}

	status = ::connect(sock_id, (sockaddr *)&m_addr, sizeof(m_addr));

	if(status == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ClpeSocket::set_non_blocking(const bool b, int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	int opts;
	int sock_id;

	if(mcu_id == MCU_ID_MASTER) 
	{
		sock_id = m_sock_master;
	}
	else
	{
		sock_id = m_sock_slave;
	}

	opts = fcntl(sock_id, F_GETFL);

	if(opts < 0)
	{
		return;
	}

	if(b)
		opts = (opts | O_NONBLOCK);
	else
		opts = (opts & ~O_NONBLOCK);

	fcntl(sock_id, F_SETFL,opts);
}

void ClpeSocket::close(int mcu_id) // master (mcu_id = 0), slave (mcu_id = 1)
{
	if(mcu_id == MCU_ID_MASTER)
	{
		if(is_valid_master())
		{
			::close (m_sock_master);
		}
	}
	else
	{
		if(is_valid_slave())
		{
			::close (m_sock_slave);
		}
	}
}
