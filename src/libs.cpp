#include "libs.h"

namespace inout{
    void say(variable * s, int sp){
        LOG("say");
        s[sp].say();
    }

    void ask(variable * s, int sp){
        LOG("ask");
    }
}

libs::libs(){
    LOG_DEBUG("Libs: Loading std lib");

    liblist["+"] = {bytecodes::ADD};
    liblist["-"] = {bytecodes::SUB};
    liblist["*"] = {bytecodes::MULT};
    liblist["/"] = {bytecodes::DIV};

    liblist["say"] = {bytecodes::V_CALL, 0};
    virtuals.push_back(inout::say);

    liblist["ask"] = {bytecodes::V_CALL, 1};
    virtuals.push_back(inout::ask);
}

map <string, vector <int> > libs::get(){
    return liblist;
}

void libs::call(int n, variable * s, int sp){
    virtuals[n](s,sp);
}
