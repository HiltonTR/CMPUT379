#include "switch.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <regex>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace std;

int switchID;
// int pswj;
// int pswk;
vector<Session> switch_sessions;
vector<ft> forwardingTable;

PacketInfo switch_stats;
Mode switch_mode = MODE_DISCONNECTED;

map<string, PktType> name_pkt_map = {
    { "HELLO", HELLO },
    { "HELLO_ACK", HELLO_ACK },
    { "ASK", ASK },
    { "ADD", ADD },
    { "RELAY", RELAYIN },
    { "ADMIT", ADMIT }
};

/*
 * 0 -> controller
 * 1 -> pswj
 * 2 -> pswk
 * 3 -> ip_range
*/
void masterSwitch(int sid, int pswj, int pswk, string traffic_file, IPs ip_range) {
    switchID = sid;
    switch_sessions = init_session(switchID, pswj, pswk);

    pollfd pfd[switch_sessions.size() + 1];
    forwardingTable = init_forwardingTable(ip_range);
    switch_stats = init_switch_stats();
    vector<string> traffic_file_cache = cache_traffic_file(traffic_file);

    // non-blocking I/O polling from STDIN
    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    for (int i = 0; i < switch_sessions.size(); i++) {
        pfd[i+1].fd = switch_sessions[i].inFd;
        pfd[i+1].events = POLLIN;
    }

    send_open_to_controller(switchID, pswj, pswk);

    int rule_cnt = 0;
    while (true) {
        // fflush(stdout);
        // IO multiplexing
        poll(pfd, switch_sessions.size() + 1, 1);

        // read traffic file
        if (rule_cnt < traffic_file_cache.size() && switch_mode == MODE_CONNECTED) {
        TrafficRule rule = parse_traffic_rule(traffic_file_cache[rule_cnt]);
        if (rule.switchID == switchID && ip_range.low <= rule.dest_ip && rule.dest_ip <= ip_range.high) {
            // found match in flow table, admit packet
            forwardingTable[0].pkt_count += 1;
            switch_stats.received[ADMIT] += 1;
        } else if (rule.switchID == switchID) {
            switch_stats.received[ADMIT] += 1;
            int rule_index = find_matching_rule(rule.dest_ip);
            if (rule_index > 0) {
            // find matched rule send relay message
            int session_idx = find_matching_session(forwardingTable[rule_index].action_value);
            stringstream ss;
            ss << "RELAY " << switchID;
            forwardingTable[rule_index].pkt_count += 1;
            switch_stats.transmitted[RELAYIN] += 1;
            send_message(switch_sessions[session_idx].outFd, RELAYIN, ss.str());
            } else {
            // send ASK to controller
            stringstream ss;
            ss << "ASK " << switchID << " " << rule.dest_ip;
            switch_stats.transmitted[ASK] += 1;
            switch_mode = MODE_WAITINGADD;
            send_message(switch_sessions[0].outFd, HELLO, ss.str());
            }
        }
        rule_cnt += 1;
        }

        // STDIN
        if (pfd[0].revents & POLLIN) {
            string cmd;
            cin >> cmd;
            if (cmd == "info") {
                switch_list_handler();
            } else if (cmd == "exit") {
                switch_list_handler();
                exit(0);
            } else {
                printf("Not a command. Use info or exit.\n");
            } 
        }

        // FIFO
        for (int i = 0; i < switch_sessions.size(); i++) {
        if (pfd[i+1].revents & POLLIN) {
            char buffer[MAXBUF];
            read(switch_sessions[i].inFd, buffer, MAXBUF);
            string s = string(buffer);
            stringstream ss(s);
            vector<string> temp;
            string buf;

            while (getline(ss, buf, ' ')) {
                temp.push_back(buf);
            }
            switch_incoming_message_handler(temp);
        }
        }
    }
}

void switch_incoming_message_handler(vector<string> message) {
    if (message.size() > 1) {
        PktType type = name_pkt_map[message[0]];
        switch_stats.received[type] += 1;

        switch (type) {
        case ADD:
            cout << "Received: (src= master, dest= sw" << message[1] << ") [ADD]" << endl;
            if (message.size() == 3) {
            // not found
            add_drop_rule(stoi(message[2]));
            } else if (message.size() == 6) {
            // update flow table and relay packet
            int forwarding_port = stoi(message[2]) == switchID ? 2 : 1;
            int forwardingTable_idx = add_forward_rule(forwarding_port, { stoi(message[4]), stoi(message[5]) });
            int session_idx = find_matching_session(forwarding_port);
            stringstream ss;
            ss << "RELAY " << switchID;
            if (session_idx > 0) send_message(switch_sessions[session_idx].outFd, RELAYIN, ss.str());
            switch_stats.transmitted[RELAYIN] += 1;
            forwardingTable[forwardingTable_idx].pkt_count += 1;
            }
            switch_mode = MODE_CONNECTED;
            break;
        case RELAYIN:
            cout << "Received: (src= sw" << message[1] << ") [RELAY]" << endl;
            forwardingTable[0].pkt_count += 1;
            break;
        case HELLO_ACK:
            cout << "Received: (src= master, dest= sw" << message[1] << ") [HELLO_ACK]" << endl;
            switch_mode = MODE_CONNECTED;
            break;
        }
    }
}

int add_forward_rule(int port, IPs dest) {
    forwardingTable.push_back({
        { 0, MAXIP },
        { dest.low, dest.high },
        FORWARD,
        port,
        0
    });
    return forwardingTable.size() - 1;
}

void add_drop_rule(int ip) {
    forwardingTable.push_back({
        { 0, MAXIP },
        { ip, ip },
        DROP,
        0,
        1
    });
}

int find_matching_rule(int dest_ip) {
    int i = 0;
    int result_index = -1;
    for (auto entry : forwardingTable) {
        if (entry.dest_range.low <= dest_ip && dest_ip <= entry.dest_range.high) {
        result_index = i;
        break;
        }
        i += 1;
    }
    return result_index;
}

int find_matching_session(int port) {
    int cnt = 0;
    int result = -1;
    for (auto s : switch_sessions) {
        if (s.port == port) {
        result = cnt;
        break;
        }
        cnt += 1;
    }
    return result;
}

void send_open_to_controller(int sid, int pswj, int pswk) {
    stringstream ss;
    ss << "HELLO" << " " << sid << " " << pswj << " " << pswk << " " <<
    forwardingTable[0].dest_range.low << " " << forwardingTable[0].dest_range.high;
    send_message(switch_sessions[0].outFd, HELLO, ss.str());
    switch_stats.transmitted[HELLO] += 1;
    }

    vector<ft> init_forwardingTable(IPs ip_range) {
    vector<ft> forwardingTable = {{
        { 0, MAXIP },
        { ip_range.low, ip_range.high },
        FORWARD,
        3,
        0
    }};

    return forwardingTable;
}
//https://www.ibm.com/docs/en/aix/7.2?topic=m-mkfifo-command for flags
vector<Session> init_session(int switchID, int left, int right) {
    string inFifo = get_fifo(0, switchID);
    string outFifo = get_fifo(switchID, 0);
    mkfifo(inFifo.c_str(), 0666);
    if (errno) perror("Error: Could not create a FIFO connection.\n");
    errno = 0;

    mkfifo(outFifo.c_str(), 0666);
    if (errno) perror("Error: Could not create a FIFO connection.\n");
    errno = 0;

    int inFd = open(inFifo.c_str(), O_RDONLY | O_NONBLOCK);
    if (errno) perror("Error: Could not open FIFO.\n");
    errno = 0;

    // Returns lowest unused file descriptor on success
    int outFd = open(outFifo.c_str(), O_RDWR | O_NONBLOCK);
    if (errno) perror("Error: Could not open FIFO.\n");
    errno = 0; 
    vector<Session> sessions = {
        {
        inFifo, outFifo, inFd, outFd, 0
        }
    };

    if (left > 0) {
        sessions.push_back({
        get_fifo(left, switchID), // in
        get_fifo(switchID, left), // out
        open(get_fifo(left, switchID).c_str(), O_RDWR | O_NONBLOCK),
        open(get_fifo(switchID, left).c_str(), O_RDWR | O_NONBLOCK),
        1
        });
    }

    if (right > 0) {
        sessions.push_back({
        get_fifo(right, switchID), // in
        get_fifo(switchID, right), // out
        open(get_fifo(right, switchID).c_str(), O_RDWR | O_NONBLOCK),
        open(get_fifo(switchID, right).c_str(), O_RDWR | O_NONBLOCK),
        2
        });
    }

    return sessions;
}

void switch_list_handler() {
    print_forwardingTable();
    print_switch_stats();
}

void print_forwardingTable() {
    cout << "Forwarding table:" << endl;
    int i = 0;
    for (auto entry : forwardingTable) {
        string action;
        if (entry.actionType == FORWARD){
            action = "FORWARD";
        } else if (entry.actionType == DROP){
            action = "DROP";
        }
        printf("[%i] (srcIp= %i-%i, destIp= %i-%i, ", i,  entry.src_range.low ,
            entry.src_range.high, entry.dest_range.low, entry.dest_range.high);
        printf("action= %s:%i, pktCount= %i)\n", action.c_str(),
            entry.action_value, entry.pkt_count);

        i++;
    }
}

void print_switch_stats() {
    cout << "Packet Stats:" << endl;
    string received = "\tReceived: ";
    string transmitted = "\tTransmitted: ";

    int counter = 0;
    for (auto stat : switch_stats.received) {
        stringstream ss;
        if (stat.first == ADMIT) ss << "ADMIT:" << stat.second;
        else if (stat.first == HELLO_ACK) ss << "HELLO_ACK:" << stat.second;
        else if (stat.first == ADD) ss << "ADDRULE:" << stat.second;
        else if (stat.first == RELAYIN) ss << "RELAYIN:" << stat.second;

        if (counter != switch_stats.received.size() - 1) ss << ", ";

        received += ss.str();
        counter += 1;
    }

    counter = 0;
    for (auto stat : switch_stats.transmitted) {
        stringstream ss;
        if (stat.first == HELLO) ss << "HELLO:" << stat.second;
        else if (stat.first == ASK) ss << "ASK:" << stat.second;
        else if (stat.first == RELAYIN) ss << "RELAYOUT:" << stat.second;

        if (counter != switch_stats.received.size() - 1) ss << ", ";

        transmitted += ss.str();
        counter += 1;
    }

    cout << received << endl;
    cout << transmitted << endl;
}

PacketInfo init_switch_stats() {
    PacketInfo stats = {
        { {ADMIT, 0}, {HELLO_ACK, 0}, {ADD, 0}, {RELAYIN, 0} },
        { {HELLO, 0}, {ASK, 0}, {RELAYIN, 0} }
    };
    return stats;
}

vector<string> cache_traffic_file(string traffic_file) {
    vector<string> lines;
    ifstream file(traffic_file);
    string buffer;
    while (getline(file, buffer)) {
        lines.push_back(buffer);
    }
    return lines;
}

TrafficRule parse_traffic_rule(string line) {
    TrafficRule rule;
    rule.switchID = -1;
    if (line.find("#") == string::npos && !line.empty()) {
        regex ws_re("\\s+");
        vector<string> placeholder {
        sregex_token_iterator(line.begin(), line.end(), ws_re, -1), {}
        };
        //cout << "parse_traffic_here" << endl;  
        //cout << placeholder[0].substr(3) << endl;
        //cout << placeholder[1] << endl;
        //cout << placeholder[2] << endl;
        rule.switchID = stoi(placeholder[0].substr(3));
        rule.src_ip = stoi(placeholder[1]);
        rule.dest_ip = stoi(placeholder[2]);
    }
    return rule;
}
