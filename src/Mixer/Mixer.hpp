/*
 * Copyright (c) 2019 Doku Enterprise
 * Author: Friedrich Doku
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *   This program is distributed in the hope that it will be useful
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once
#define MAX_BOOTSTRAP_NODES 20


#include <iostream>
#include <fstream>
#include <vector>

//Core Libs
#include <fstream>
#include <opendht.h>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <random>
#include <mutex>
#include <csignal>
#include <memory>
#include <fstream>
#include <thread>
#include <sodium.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <cstdlib>
#include "shuffle.hpp"
#include <zephyr/utils.hpp>
#include <unistd.h>

#include <future>
#include <ctime>

#include <stdio.h>
#include <map>
#include <sys/types.h>//socket
#include <sys/socket.h>//socket
#include <string.h>//memset
#include <stdlib.h>//sizeof
#include <netinet/in.h>//INADDR_ANY

#include "mixerclient.hpp"
#include "mixerserver.hpp"
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <jsonrpccpp/server/connectors/httpserver.h>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/portable_binary.hpp>


#define KEY_LENGTH 2048
#define PORT 8080
#define MAXSZ 4096


class Mixer
{
private:
    dht::DhtRunner node;
    std::vector<std::string> myip;
    std::vector<std::string> mixers;
    std::vector<std::string> mailboxes;
    //std::vector<std::string> messages;
    std::map<std::string, std::string> whos;
    std::string mixer_ip;
    unsigned char public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char private_key[crypto_box_SECRETKEYBYTES];
    char id[20];
    int deadline;
    int readymixers = 0;
    bool is_the_first = false;
    
public:
    Mixer(std::string mixerip, std::vector<std::string> mixers, std::vector<std::string> mailboxes, std::string configpath);
    //void ListenForMessages();
    void StartRoundAsMixer();
    ~Mixer();
};

void GiveMeDataForPublic(std::string pub, std::string ip);
void senddata(std::string ip, std::string msg);
void StartServerInBackground();
void GetPrimaryIp(char* buffer, size_t buflen);
int ListenForMessages();;

// Returns hostname for the local computer 
void checkHostName(int hostname);

// Returns host information corresponding to host name 
void checkHostEntry(struct hostent * hostentry);

// Converts space-delimited IPv4 addresses 
// to dotted-decimal format 
void checkIPbuffer(char *IPbuffer);

