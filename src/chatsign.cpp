//
// Created by 王诛魔 on 2019-05-27.
//

#include <string>
#include <iostream>
#include <ctime>
#include <sstream>

using namespace std;


const char *key = "MZiya6yfio4GHcey";

string create_chat_sign(const char *chatKey){
    stringstream strstream;
    //get local time
    time_t tm = time(NULL);
    string time_str;
    strstream << tm;
    strstream >> time_str;
    string key_value = key;
    key_value.append(chatKey);
    key_value.append(time_str);
    //append
    cout << key_value << endl;
}