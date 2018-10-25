/******************************************************************************
 *
 * \file RTSPReceiverTest.cpp
 * \brief Class methods for RTSPReceiver
 * \author Yunfang Che
 *
 * Copyright Aqueti 2017.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 *****************************************************************************/
#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <aqt_api.hpp>

#include "RTSPReceiver.h"

using namespace aqt;
using namespace atl;

RTSPReceiver *g_receiver; 
std::thread g_thread;

void ProcessRun()
{
	g_receiver->m_running = true;
	g_receiver->mainLoop();
}
int main()
{
	//aqt_Status status;

	g_receiver = new RTSPReceiver("rtsp://192.168.1.148/live");//,status);
	g_thread = std::thread(ProcessRun);
	g_thread.join();
	delete g_receiver;
	return 0;
}

